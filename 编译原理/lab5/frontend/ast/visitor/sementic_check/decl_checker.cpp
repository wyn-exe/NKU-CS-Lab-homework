#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>
#include <numeric>
#include <functional>
#include <iostream>

namespace FE::AST
{
    namespace
    {
        VarValue makeZero(Type* t)
        {
            if (!t) return VarValue(0);
            switch (t->getBaseType())
            {
                case Type_t::BOOL: return VarValue(false);
                case Type_t::INT: return VarValue(0);
                case Type_t::LL: return VarValue(0LL);
                case Type_t::FLOAT: return VarValue(0.0f);
                default: return VarValue(0);
            }
        }

        size_t totalSize(const std::vector<int>& dims)
        {
            if (dims.empty()) return 1;
            return static_cast<size_t>(
                std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<int>()));
        }
    }  // namespace

    bool ASTChecker::visit(Initializer& node)
    {
        // 示例实现：单个初始化器的语义检查
        // 1) 访问初始化值表达式
        // 2) 将子表达式的属性拷贝到当前节点
        ASSERT(node.init_val && "Null initializer value");
        bool res  = apply(*this, *node.init_val);
        node.attr = node.init_val->attr;
        return res;
    }

    bool ASTChecker::visit(InitializerList& node)
    {
        // 示例实现：初始化器列表的语义检查
        // 遍历列表中的每个初始化器并逐个访问
        if (!node.init_list) return true;
        bool res = true;
        for (auto* init : *(node.init_list))
        {
            if (!init) continue;
            res &= apply(*this, *init);
        }
        return res;
    }

    bool ASTChecker::visit(VarDeclarator& node)
    {
        // TODO(Lab3-1): 实现变量声明器的语义检查
        // 访问左值表达式，同步属性，处理初始化器（如果有）
        bool res = true;
        if (!curDeclType)
        {
            addError(node, "Internal error: declaration type missing");
            return false;
        }

        auto* lvalNode = dynamic_cast<LeftValExpr*>(node.lval);
        Entry* entry   = lvalNode ? lvalNode->entry : nullptr;
        if (!lvalNode || !entry)
        {
            addError(node, "Invalid declarator");
            return false;
        }

        VarAttr* prev = symTable.getSymbol(entry);
        if (prev && prev->scopeLevel == symTable.getScopeDepth())
        {
            addError(node, "Redefinition of variable " + entry->getName());
            res = false;
        }

        VarAttr attr(curDeclType, curDeclConst, symTable.getScopeDepth());

        if (lvalNode && lvalNode->indices)
        {
            for (auto* dimExpr : *lvalNode->indices)
            {
                if (!dimExpr) continue;
                res &= apply(*this, *dimExpr);

                bool dimConst = dimExpr->attr.val.isConstexpr && isIntegerType(dimExpr->attr.val.value.type);
                int  dimVal   = dimConst ? dimExpr->attr.val.getInt() : 0;
                if (!dimConst)
                {
                    addError(*dimExpr, "Array dimension must be a constant integer");
                    res = false;
                }
                else if (dimVal <= 0)
                {
                    addError(*dimExpr, "Array dimension must be positive");
                    res = false;
                }
                attr.arrayDims.push_back(dimVal);
            }
        }

        auto zeroVal = makeZero(curDeclType);

        if (node.init)
        {
            res &= apply(*this, *node.init);
            if (attr.arrayDims.empty())
            {
                if (!node.init->singleInit)
                {
                    addError(*node.init, "Scalar variable cannot use initializer list");
                    res = false;
                }
                if (!canConvertTo(node.init->attr.val.value.type, curDeclType))
                {
                    addError(*node.init, "Type mismatch in initializer of " + entry->getName());
                    res = false;
                }

                VarValue cval;
                if (collectConstValue(node.init->attr.val, curDeclType, cval))
                {
                    attr.initList.push_back(cval);
                }
                else if (symTable.isGlobalScope() || curDeclConst)
                {
                    addError(*node.init, "Initializer must be constant");
                    res = false;
                }
            }
            else
            {
                attr.initList.clear();

                // Helper to calculate sub-array size (product of dims[idx...])
                auto getSubSize = [&](size_t start_idx) -> size_t {
                    size_t s = 1;
                    for (size_t i = start_idx; i < attr.arrayDims.size(); ++i) s *= attr.arrayDims[i];
                    return s;
                };

                                // Recursive serialization with padding
                                std::function<void(InitDecl*, size_t)> serialize = [&](InitDecl* init, size_t level) {
                                    if (!init) return;

                                    size_t expected   = getSubSize(level);
                                    size_t start_size = attr.initList.size();

                                    if (init->singleInit)
                                    {
                                        // Scalar initializer: consumes one slot (and pads if it was treated as the whole object, 
                                        // but here we usually call it for Scalar level, so expected=1).
                                        // If called for Array level with singleInit, it means "init whole array with this scalar" (if valid)
                                        // or "put this scalar at start".
                                        // In our logic below, we handle scalars in lists explicitly.
                                        // This block handles the case where the top-level init is a single scalar.

                                        VarValue tmp;
                                        if (!collectConstValue(init->attr.val, curDeclType, tmp))
                                        {
                                            if (symTable.isGlobalScope() || curDeclConst)
                                            {
                                                addError(*init, "Array initializer must be constant");
                                                res = false;
                                            }
                                        }
                                        attr.initList.push_back(tmp);
                                    }
                                    else
                                    {
                                        // Initializer list
                                        auto* lst = static_cast<InitializerList*>(init)->init_list;
                                        if (lst)
                                        {
                                            for (auto* child : *lst)
                                            {
                                                if (!child) continue;
                                                if (attr.initList.size() - start_size >= expected)
                                                {
                                                    // Warn about excess elements
                                                    std::cerr << "Warning: Excess elements in array initializer at line " << init->line_num << std::endl;
                                                    break;
                                                }

                                                if (child->singleInit)
                                                {
                                                    // Scalar in a list: consumes the next scalar slot.
                                                    // We do not recurse because recursion would force padding to 'expected' of next level.
                                                    VarValue tmp;
                                                    if (!collectConstValue(child->attr.val, curDeclType, tmp))
                                                    {
                                                        if (symTable.isGlobalScope() || curDeclConst)
                                                        {
                                                            addError(*child, "Array initializer must be constant");
                                                            res = false;
                                                        }
                                                    }
                                                    attr.initList.push_back(tmp);
                                                }
                                                else
                                                {
                                                    // Nested List: Must align to a sub-object boundary.
                                                    // Find the tightest enclosing object type that starts at current position.
                                                    size_t current_pos = attr.initList.size();
                                                    size_t target_lvl = level + 1;
                                                    while (target_lvl <= attr.arrayDims.size())
                                                    {
                                                        if (current_pos % getSubSize(target_lvl) == 0) break;
                                                        target_lvl++;
                                                    }
                                                    serialize(child, target_lvl);
                                                }
                                            }
                                        }
                                        // Pad the rest of this dimension scope
                                        while (attr.initList.size() - start_size < expected)
                                        {
                                            attr.initList.push_back(zeroVal);
                                        }
                                    }
                                };

                                serialize(node.init, 0);            }
        }
        else
        {
            if (curDeclConst)
            {
                addError(node, "Const variable requires an initializer");
                res = false;
            }

            if (symTable.isGlobalScope())
            {
                size_t total = totalSize(attr.arrayDims);
                attr.initList.assign(total, zeroVal);
            }
        }

        symTable.addSymbol(entry, attr);
        if (symTable.isGlobalScope()) glbSymbols[entry] = attr;

        node.attr.val.value.type  = curDeclType;
        node.attr.val.isConstexpr = false;
        return res;
    }

    bool ASTChecker::visit(ParamDeclarator& node)
    {
        // TODO(Lab3-1): 实现函数形参的语义检查
        // 检查形参重定义，处理数组形参的类型退化，将形参加入符号表
        bool res = true;

        VarAttr* prev = symTable.getSymbol(node.entry);
        if (prev && prev->scopeLevel == symTable.getScopeDepth())
        {
            addError(node, "Duplicate parameter name " + node.entry->getName());
            res = false;
        }

        VarAttr attr(node.type, false, symTable.getScopeDepth());
        if (node.dims)
        {
            for (size_t i = 0; i < node.dims->size(); ++i)
            {
                auto* dimExpr = (*(node.dims))[i];
                if (!dimExpr) continue;
                res &= apply(*this, *dimExpr);
                bool dimConst = dimExpr->attr.val.isConstexpr && isIntegerType(dimExpr->attr.val.value.type);
                int  dimVal   = dimConst ? dimExpr->attr.val.getInt() : 0;

                if (!dimConst)
                {
                    addError(*dimExpr, "Parameter dimension must be constant");
                    res = false;
                }
                else if (dimVal <= 0 && i != 0)
                {
                    addError(*dimExpr, "Array dimension must be positive");
                    res = false;
                }

                attr.arrayDims.push_back(dimVal);
            }
        }

        symTable.addSymbol(node.entry, attr);
        node.attr.val.value.type  = node.type;
        node.attr.val.isConstexpr = false;
        return res;
    }

    bool ASTChecker::visit(VarDeclaration& node)
    {
        // TODO(Lab3-1): 实现变量声明的语义检查
        // 遍历声明列表，检查重定义，处理数组维度和初始化，将符号加入符号表
        bool res = true;
        if (isVoidType(node.type))
        {
            addError(node, "Variable cannot be declared as void");
            return false;
        }

        curDeclType  = node.type;
        curDeclConst = node.isConstDecl;

        if (node.decls)
        {
            for (auto* decl : *node.decls)
            {
                if (!decl) continue;
                res &= apply(*this, *decl);
            }
        }

        curDeclType  = nullptr;
        curDeclConst = false;
        return res;
    }
}  // namespace FE::AST