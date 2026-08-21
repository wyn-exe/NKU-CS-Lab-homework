#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>
#include <string>

namespace FE::AST
{
    bool ASTChecker::visit(LeftValExpr& node)
    {
        // TODO(Lab3-1): 实现左值表达式的语义检查
        // 检查变量是否存在，处理数组下标访问，进行类型检查和常量折叠
        bool res = true;
        node.isLval = true;

        VarAttr* attr = symTable.getSymbol(node.entry);
        if (!attr)
        {
            addError(node, "Use of undeclared identifier " + node.entry->getName());
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            return false;
        }

        std::vector<int> idxVals;
        size_t           idxCnt = node.indices ? node.indices->size() : 0;
        if (idxCnt > attr->arrayDims.size())
        {
            addError(node, "Too many indices for array " + node.entry->getName());
            res = false;
        }

        if (node.indices)
        {
            for (auto* idx : *node.indices)
            {
                if (!idx) continue;
                res &= apply(*this, *idx);
                if (!isIntegerType(idx->attr.val.value.type))
                {
                    addError(*idx, "Array index must be integer");
                    res = false;
                }
                if (idx->attr.val.isConstexpr) idxVals.push_back(idx->attr.val.getInt());
                else idxVals.push_back(0);
            }
        }

        Type* baseType      = attr->type;
        size_t remainDimNum = (attr->arrayDims.size() > idxCnt) ? attr->arrayDims.size() - idxCnt : 0;
        if (!attr->arrayDims.empty() && remainDimNum > 0)
        {
            node.attr.val.value.type = TypeFactory::getPtrType(baseType);
            node.isLval              = false;
        }
        else
        {
            node.attr.val.value.type = baseType;
            node.isLval              = true;
        }

        node.attr.val.isConstexpr = false;
        if (attr->isConstDecl && !attr->initList.empty())
        {
            if (attr->arrayDims.empty())
            {
                node.attr.val.isConstexpr = true;
                node.attr.val.value       = attr->initList[0];
            }
            else if (idxCnt == attr->arrayDims.size())
            {
                bool canCalc = true;
                for (size_t i = 0; i < idxCnt; ++i)
                {
                    if (attr->arrayDims[i] <= 0 || !node.indices || !(*node.indices)[i]->attr.val.isConstexpr)
                    {
                        canCalc = false;
                        break;
                    }
                }
                if (canCalc)
                {
                    size_t offset = 0;
                    size_t stride = 1;
                    for (int d : attr->arrayDims) stride *= static_cast<size_t>(d);
                    for (size_t i = 0; i < idxCnt; ++i)
                    {
                        stride /= static_cast<size_t>(attr->arrayDims[i]);
                        offset += static_cast<size_t>(idxVals[i]) * stride;
                    }
                    if (offset < attr->initList.size())
                    {
                        node.attr.val.isConstexpr = true;
                        node.attr.val.value       = attr->initList[offset];
                    }
                }
            }
        }

        return res;
    }

    bool ASTChecker::visit(LiteralExpr& node)
    {
        // 示例实现：字面量表达式的语义检查
        // 字面量总是编译期常量，直接设置属性
        node.attr.val.isConstexpr = true;
        node.attr.val.value       = node.literal;
        return true;
    }

    bool ASTChecker::visit(UnaryExpr& node)
    {
        // TODO(Lab3-1): 实现一元表达式的语义检查
        // 访问子表达式，检查操作数类型，调用类型推断函数
        bool res = apply(*this, *node.expr);
        Type* t  = node.expr->attr.val.value.type;
        if (!t || isVoidType(t) || t->getTypeGroup() == TypeGroup::POINTER)
        {
            addError(node, "Invalid operand type for unary operator");
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            return false;
        }

        bool      hasError = false;
        ExprValue v        = typeInfer(node.expr->attr.val, node.op, node, hasError);
        node.attr.val      = v;
        return res && !hasError;
    }

    bool ASTChecker::visit(BinaryExpr& node)
    {
        // TODO(Lab3-1): 实现二元表达式的语义检查
        // 访问左右子表达式，检查操作数类型，调用类型推断
        if (node.op == Operator::ASSIGN)
        {
            bool res = true;
            res &= apply(*this, *node.lhs);
            res &= apply(*this, *node.rhs);

            auto* lval = dynamic_cast<LeftValExpr*>(node.lhs);
            if (!lval || !lval->isLval)
            {
                addError(node, "Left operand of assignment is not assignable");
                return false;
            }

            VarAttr* lhsAttr = symTable.getSymbol(lval->entry);
            if (lhsAttr && lhsAttr->isConstDecl)
            {
                addError(node, "Cannot assign to const variable " + lval->entry->getName());
                res = false;
            }
            if (lhsAttr && !lhsAttr->arrayDims.empty() && (!lval->indices || lval->indices->size() < lhsAttr->arrayDims.size()))
            {
                addError(node, "Cannot assign to array " + lval->entry->getName());
                res = false;
            }

            if (!canConvertTo(node.rhs->attr.val.value.type, node.lhs->attr.val.value.type))
            {
                addError(node, "Type mismatch in assignment");
                res = false;
            }

            node.attr.val.value.type  = node.lhs->attr.val.value.type;
            node.attr.val.isConstexpr = false;
            return res;
        }

        if (node.op == Operator::AND || node.op == Operator::OR)
        {
            bool res = true;
            res &= apply(*this, *node.lhs);
            res &= apply(*this, *node.rhs);

            Type* lt = node.lhs->attr.val.value.type;
            Type* rt = node.rhs->attr.val.value.type;
            if ((lt && lt->getTypeGroup() == TypeGroup::POINTER) || (rt && rt->getTypeGroup() == TypeGroup::POINTER) ||
                isVoidType(lt) || isVoidType(rt))
            {
                addError(node, "Logical operation requires scalar operands");
                return false;
            }

            node.attr.val.value.type  = boolType;
            node.attr.val.isConstexpr = node.lhs->attr.val.isConstexpr && node.rhs->attr.val.isConstexpr;
            if (node.attr.val.isConstexpr)
            {
                bool lv = node.lhs->attr.val.getBool();
                bool rv = node.rhs->attr.val.getBool();
                node.attr.val.value = VarValue(node.op == Operator::AND ? (lv && rv) : (lv || rv));
            }
            return res;
        }

        bool res = true;
        res &= apply(*this, *node.lhs);
        res &= apply(*this, *node.rhs);

        Type* lt = node.lhs->attr.val.value.type;
        Type* rt = node.rhs->attr.val.value.type;
        if (!lt || !rt || lt->getTypeGroup() == TypeGroup::POINTER || rt->getTypeGroup() == TypeGroup::POINTER ||
            isVoidType(lt) || isVoidType(rt))
        {
            addError(node, "Invalid operand type for binary operator");
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            return false;
        }

        bool      hasError = false;
        ExprValue v        = typeInfer(node.lhs->attr.val, node.rhs->attr.val, node.op, node, hasError);
        node.attr.val      = v;
        return res && !hasError;
    }

    bool ASTChecker::visit(CallExpr& node)
    {
        // TODO(Lab3-1): 实现函数调用表达式的语义检查
        // 检查函数是否存在，访问实参列表，检查参数数量和类型匹配
        bool res = true;
        auto it  = funcDecls.find(node.func);
        if (it == funcDecls.end())
        {
            addError(node, "Call to undefined function " + node.func->getName());
            node.attr.val.value.type  = voidType;
            node.attr.val.isConstexpr = false;
            return false;
        }

        FuncDeclStmt* decl = it->second;
        size_t        argCnt = node.args ? node.args->size() : 0;
        size_t        paramCnt = decl->params ? decl->params->size() : 0;
        if (argCnt != paramCnt)
        {
            addError(node, "Function argument number mismatch for " + node.func->getName());
            res = false;
        }

        if (node.args)
        {
            for (auto* arg : *node.args)
            {
                if (!arg) continue;
                res &= apply(*this, *arg);
            }
        }

        size_t checkCnt = std::min(argCnt, paramCnt);
        for (size_t i = 0; i < checkCnt; ++i)
        {
            auto* arg = (*(node.args))[i];
            auto* pd  = (*(decl->params))[i];
            Type* expectType =
                (pd->dims && !pd->dims->empty()) ? TypeFactory::getPtrType(pd->type) : pd->type;

            if (!canConvertTo(arg->attr.val.value.type, expectType))
            {
                addError(*arg, "Argument type mismatch at position " + std::to_string(i));
                res = false;
            }
            if (pd->dims && !pd->dims->empty() && expectType->getTypeGroup() == TypeGroup::POINTER &&
                arg->attr.val.value.type->getTypeGroup() != TypeGroup::POINTER)
            {
                addError(*arg, "Array argument expected at position " + std::to_string(i));
                res = false;
            }
        }

        node.attr.val.value.type  = decl->retType ? decl->retType : voidType;
        node.attr.val.isConstexpr = false;
        return res;
    }

    bool ASTChecker::visit(CommaExpr& node)
    {
        // TODO(Lab3-1): 实现逗号表达式的语义检查
        // 依序访问各子表达式，结果为最后一个表达式的属性
        bool res = true;
        if (!node.exprs || node.exprs->empty()) return true;

        for (auto* expr : *node.exprs)
        {
            if (!expr) continue;
            res &= apply(*this, *expr);
            node.attr = expr->attr;
        }
        return res;
    }
}  // namespace FE::AST
