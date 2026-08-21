#include <middleend/visitor/codegen/ast_codegen.h>
#include <debug.h>
#include <numeric>
#include <functional>

namespace ME
{
    namespace
    {
        std::vector<int> evalDims(FE::AST::LeftValExpr* lval)
        {
            std::vector<int> dims;
            if (!lval || !lval->indices) return dims;
            for (auto* d : *lval->indices)
            {
                if (!d) continue;
                dims.push_back(d->attr.val.getInt());
            }
            return dims;
        }

        std::vector<FE::AST::InitDecl*> flattenInit(FE::AST::InitDecl* init)
        {
            std::vector<FE::AST::InitDecl*> res;
            if (!init) return res;
            if (init->singleInit)
            {
                res.push_back(init);
            }
            else
            {
                auto* lst = static_cast<FE::AST::InitializerList*>(init)->init_list;
                if (!lst) return res;
                for (auto* sub : *lst)
                {
                    auto subRes = flattenInit(sub);
                    res.insert(res.end(), subRes.begin(), subRes.end());
                }
            }
            return res;
        }

        std::vector<int> linearToCoords(int idx, const std::vector<int>& dims)
        {
            std::vector<int> coords;
            int              rem = idx;
            for (size_t i = 0; i < dims.size(); ++i)
            {
                int stride = 1;
                for (size_t j = i + 1; j < dims.size(); ++j) stride *= dims[j];
                int c = stride ? rem / stride : 0;
                rem   = stride ? rem % stride : 0;
                coords.push_back(c);
            }
            return coords;
        }
    }  // namespace

    void ASTCodeGen::visit(FE::AST::Initializer& node, Module* m)
    {
        (void)m;
        ERROR("Initializer should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::InitializerList& node, Module* m)
    {
        (void)m;
        ERROR("InitializerList should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::VarDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("VarDeclarator should not appear here, at line %d", node.line_num);
    }
    void ASTCodeGen::visit(FE::AST::ParamDeclarator& node, Module* m)
    {
        (void)m;
        ERROR("ParamDeclarator should not appear here, at line %d", node.line_num);
    }

    void ASTCodeGen::visit(FE::AST::VarDeclaration& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明 IR（alloca、数组零初始化、可选初始化表达式）
        if (!node.decls) return;
        for (auto* decl : *node.decls)
        {
            if (!decl || !decl->lval) continue;

            auto* lval = dynamic_cast<FE::AST::LeftValExpr*>(decl->lval);
            if (!lval) continue;

            std::vector<int> dims = evalDims(lval);
            DataType         dt   = convert(node.type);

            size_t allocaReg = getNewRegId();
            Instruction* allocInst = nullptr;
            if (dims.empty())
                allocInst = createAllocaInst(dt, allocaReg);
            else
                allocInst = createAllocaInst(dt, allocaReg, dims);
            ASSERT(entryBlock && "entry block should be set when allocating locals");
            entryBlock->insertFront(allocInst);

            name2reg.addSymbol(lval->entry, allocaReg);
            FE::AST::VarAttr attr(node.type, node.isConstDecl, 0);
            attr.arrayDims     = dims;
            reg2attr[allocaReg] = attr;

            Operand* basePtr = getRegOperand(allocaReg);

            if (!decl->init) continue;

            if (dims.empty())
            {
                FE::AST::ExprNode* expr = nullptr;
                if (decl->init->singleInit)
                    expr = static_cast<FE::AST::Initializer*>(decl->init)->init_val;
                else
                {
                    auto flat = flattenInit(decl->init);
                    if (!flat.empty() && flat[0]->singleInit)
                        expr = static_cast<FE::AST::Initializer*>(flat[0])->init_val;
                }
                if (expr)
                {
                    apply(*this, *expr, m);
                    size_t   valReg  = getMaxReg();
                    DataType srcType = convert(expr->attr.val.value.type);
                    if (srcType != dt)
                    {
                        auto conv = createTypeConvertInst(srcType, dt, valReg);
                        for (auto* inst : conv) insert(inst);
                        valReg = getMaxReg();
                    }
                    insert(createStoreInst(dt, valReg, basePtr));
                }
            }
            else
            {
                size_t   current_index = 0;
                Operand* zeroOp = (dt == DataType::F32) ? (Operand*)getImmeF32Operand(0.0f) : (Operand*)getImmeI32Operand(0);

                auto getSubSize = [&](size_t start_idx) -> size_t {
                    size_t s = 1;
                    for (size_t i = start_idx; i < dims.size(); ++i) s *= dims[i];
                    return s;
                };

                auto generateStore = [&](size_t idx, bool isZero, FE::AST::InitDecl* init) {
                    auto                  coords = linearToCoords(static_cast<int>(idx), dims);
                    std::vector<Operand*> idxOps;
                    idxOps.push_back(getImmeI32Operand(0));
                    for (int c : coords) idxOps.push_back(getImmeI32Operand(c));
                    size_t gepReg = getNewRegId();
                    insert(createGEP_I32Inst(dt, basePtr, dims, idxOps, gepReg));

                    if (isZero)
                    {
                        insert(createStoreInst(dt, zeroOp, getRegOperand(gepReg)));
                    }
                    else
                    {
                        apply(*this, *static_cast<FE::AST::Initializer*>(init)->init_val, m);
                        size_t   valReg  = getMaxReg();
                        DataType srcType = convert(init->attr.val.value.type);
                        if (srcType != dt)
                        {
                            auto conv = createTypeConvertInst(srcType, dt, valReg);
                            for (auto* inst : conv) insert(inst);
                            valReg = getMaxReg();
                        }
                        insert(createStoreInst(dt, valReg, getRegOperand(gepReg)));
                    }
                };

                std::function<void(FE::AST::InitDecl*, size_t)> serialize =
                    [&](FE::AST::InitDecl* init, size_t level) {
                        if (!init) return;
                        size_t expected  = getSubSize(level);
                        size_t stride    = (level + 1 < dims.size()) ? getSubSize(level + 1) : 1;
                        size_t start_idx = current_index;

                        if (init->singleInit)
                        {
                            generateStore(current_index, false, init);
                            current_index++;
                        }
                        else
                        {
                            auto* lst = static_cast<FE::AST::InitializerList*>(init)->init_list;
                            if (lst)
                            {
                                for (auto* child : *lst)
                                {
                                    if (!child) continue;
                                    if (!child->singleInit)
                                    {
                                        // Align
                                        size_t curr = current_index - start_idx;
                                        size_t rem  = curr % stride;
                                        if (rem != 0)
                                        {
                                            size_t pad = stride - rem;
                                            for (size_t k = 0; k < pad; ++k)
                                            {
                                                generateStore(current_index++, true, nullptr);
                                            }
                                        }
                                    }
                                    serialize(child, level + 1);
                                }
                            }
                            // Pad scope
                            while (current_index - start_idx < expected)
                            {
                                generateStore(current_index++, true, nullptr);
                            }
                        }
                    };

                serialize(decl->init, 0);
            }
        }
    }
}  // namespace ME
