#include <middleend/visitor/codegen/ast_codegen.h>
#include <algorithm>

namespace ME
{
    namespace
    {
        bool isTerminated(Block* b) { return b && !b->insts.empty() && b->insts.back()->isTerminator(); }
    }  // namespace

    void ASTCodeGen::visit(FE::AST::LeftValExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成左值表达式的取址/取值 IR
        // 查找变量位置（全局或局部），处理数组下标/GEP，必要时发出load
        (void)m;
        Operand* ptrOp = nullptr;
        FE::AST::VarAttr attr;
        bool             hasAttr = false;

        size_t regId = name2reg.getReg(node.entry);
        if (regId != static_cast<size_t>(-1))
        {
            ptrOp = getRegOperand(regId);
            auto it = reg2attr.find(regId);
            if (it != reg2attr.end())
            {
                attr     = it->second;
                hasAttr  = true;
            }
        }

        if (!hasAttr)
        {
            auto git = glbSymbols.find(node.entry);
            if (git != glbSymbols.end())
            {
                ptrOp   = getGlobalOperand(node.entry->getName());
                attr    = git->second;
                hasAttr = true;
            }
        }

        if (!hasAttr)
        {
            ERROR("Unknown symbol in codegen: %s", node.entry->getName().c_str());
        }

        std::vector<int> dims;
        for (int d : attr.arrayDims)
        {
            if (d > 0) dims.push_back(d);
        }

        std::vector<Operand*> idxOps;
        if (!dims.empty() || (node.indices && !node.indices->empty()))
        {
            idxOps.push_back(getImmeI32Operand(0));
            if (node.indices)
            {
                for (auto* idx : *node.indices)
                {
                    apply(*this, *idx, m);
                    size_t   idxReg  = getMaxReg();
                    DataType idxType = convert(idx->attr.val.value.type);
                    if (idxType != DataType::I32)
                    {
                        auto conv = createTypeConvertInst(idxType, DataType::I32, idxReg);
                        for (auto* inst : conv) insert(inst);
                        idxReg = getMaxReg();
                    }
                    idxOps.push_back(getRegOperand(idxReg));
                }
            }

            size_t gepReg = getNewRegId();
            insert(createGEP_I32Inst(convert(attr.type), ptrOp, dims, idxOps, gepReg));
            ptrOp = getRegOperand(gepReg);
        }

        lval2ptr[&node] = ptrOp;

        // load value if fully indexed or scalar
        size_t resReg = getNewRegId();
        if (dims.empty() || (node.indices && node.indices->size() == dims.size()))
        {
            insert(createLoadInst(convert(attr.type), ptrOp, resReg));
        }
        else
        {
            // decay to pointer value
            std::vector<Operand*> zeroIdx = {getImmeI32Operand(0)};
            insert(createGEP_I32Inst(convert(attr.type), ptrOp, dims, zeroIdx, resReg));
        }
    }

    void ASTCodeGen::visit(FE::AST::LiteralExpr& node, Module* m)
    {
        (void)m;

        size_t reg = getNewRegId();
        switch (node.literal.type->getBaseType())
        {
            case FE::AST::Type_t::INT:
            case FE::AST::Type_t::LL:  // treat as I32
            {
                int             val  = node.literal.getInt();
                ArithmeticInst* inst = createArithmeticI32Inst_ImmeAll(Operator::ADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            case FE::AST::Type_t::FLOAT:
            {
                float           val  = node.literal.getFloat();
                ArithmeticInst* inst = createArithmeticF32Inst_ImmeAll(Operator::FADD, val, 0, reg);  // reg = val + 0
                insert(inst);
                break;
            }
            default: ERROR("Unsupported literal type");
        }
    }

    void ASTCodeGen::visit(FE::AST::UnaryExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成一元运算的 IR（访问操作数、必要的类型转换、发出运算指令）
        handleUnaryCalc(*node.expr, node.op, curBlock, m);
    }

    void ASTCodeGen::handleAssign(FE::AST::LeftValExpr& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成赋值语句的 IR（计算右值、类型转换、store 到左值地址）
        apply(*this, lhs, m);
        Operand* ptr = lval2ptr[&lhs];

        apply(*this, rhs, m);
        size_t   rhsReg  = getMaxReg();
        DataType dstType = convert(lhs.attr.val.value.type);
        DataType srcType = convert(rhs.attr.val.value.type);

        if (srcType != dstType)
        {
            auto insts = createTypeConvertInst(srcType, dstType, rhsReg);
            for (auto* inst : insts) insert(inst);
            rhsReg = getMaxReg();
        }

        insert(createStoreInst(dstType, rhsReg, ptr));
    }
    void ASTCodeGen::handleLogicalAnd(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成短路与的基本块与条件分支
        apply(*this, lhs, m);
        size_t   lhsReg  = getMaxReg();
        DataType lhsType = convert(lhs.attr.val.value.type);
        if (lhsType != DataType::I1)
        {
            auto conv = createTypeConvertInst(lhsType, DataType::I1, lhsReg);
            for (auto* inst : conv) insert(inst);
            lhsReg = getMaxReg();
        }

        size_t lhsLabel = curBlock->blockId;
        Block* rhsBlock = createBlock();
        size_t rhsLabel = rhsBlock->blockId;
        Block* endBlock = createBlock();
        size_t endLabel = endBlock->blockId;

        insert(createBranchInst(lhsReg, rhsLabel, endLabel));

        enterBlock(rhsBlock);
        apply(*this, rhs, m);
        size_t   rhsReg     = getMaxReg();
        size_t   rhsEndLabel = curBlock->blockId;
        DataType rhsType    = convert(rhs.attr.val.value.type);
        if (rhsType != DataType::I1)
        {
            auto conv = createTypeConvertInst(rhsType, DataType::I1, rhsReg);
            for (auto* inst : conv) insert(inst);
            rhsReg = getMaxReg();
        }
        if (!isTerminated(curBlock)) insert(createBranchInst(endLabel));

        enterBlock(endBlock);
        size_t resReg = getNewRegId();
        auto*  phi    = new PhiInst(DataType::I1, getRegOperand(resReg));
        phi->addIncoming(getImmeI32Operand(0), getLabelOperand(lhsLabel));
        phi->addIncoming(getRegOperand(rhsReg), getLabelOperand(rhsEndLabel));
        insert(phi);
    }
    void ASTCodeGen::handleLogicalOr(
        FE::AST::BinaryExpr& node, FE::AST::ExprNode& lhs, FE::AST::ExprNode& rhs, Module* m)
    {
        // TODO(Lab 3-2): 生成短路或的基本块与条件分支
        apply(*this, lhs, m);
        size_t   lhsReg  = getMaxReg();
        DataType lhsType = convert(lhs.attr.val.value.type);
        if (lhsType != DataType::I1)
        {
            auto conv = createTypeConvertInst(lhsType, DataType::I1, lhsReg);
            for (auto* inst : conv) insert(inst);
            lhsReg = getMaxReg();
        }

        size_t lhsLabel = curBlock->blockId;
        Block* rhsBlock = createBlock();
        size_t rhsLabel = rhsBlock->blockId;
        Block* endBlock = createBlock();
        size_t endLabel = endBlock->blockId;

        insert(createBranchInst(lhsReg, endLabel, rhsLabel));

        enterBlock(rhsBlock);
        apply(*this, rhs, m);
        size_t   rhsReg      = getMaxReg();
        size_t   rhsEndLabel = curBlock->blockId;
        DataType rhsType     = convert(rhs.attr.val.value.type);
        if (rhsType != DataType::I1)
        {
            auto conv = createTypeConvertInst(rhsType, DataType::I1, rhsReg);
            for (auto* inst : conv) insert(inst);
            rhsReg = getMaxReg();
        }
        if (!isTerminated(curBlock)) insert(createBranchInst(endLabel));

        enterBlock(endBlock);
        size_t resReg = getNewRegId();
        auto*  phi    = new PhiInst(DataType::I1, getRegOperand(resReg));
        phi->addIncoming(getImmeI32Operand(1), getLabelOperand(lhsLabel));
        phi->addIncoming(getRegOperand(rhsReg), getLabelOperand(rhsEndLabel));
        insert(phi);
    }
    void ASTCodeGen::visit(FE::AST::BinaryExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成二元表达式 IR（含赋值、逻辑与/或、算术/比较）
        if (node.op == FE::AST::Operator::ASSIGN)
        {
            auto* lval = dynamic_cast<FE::AST::LeftValExpr*>(node.lhs);
            ASSERT(lval && "Assignment lhs should be LeftVal");
            handleAssign(*lval, *node.rhs, m);
        }
        else if (node.op == FE::AST::Operator::AND)
        {
            handleLogicalAnd(node, *node.lhs, *node.rhs, m);
        }
        else if (node.op == FE::AST::Operator::OR)
        {
            handleLogicalOr(node, *node.lhs, *node.rhs, m);
        }
        else
        {
            handleBinaryCalc(*node.lhs, *node.rhs, node.op, curBlock, m);
        }
    }

    void ASTCodeGen::visit(FE::AST::CallExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数调用 IR（准备参数、可选返回寄存器、发出call）
        (void)m;
        CallInst::argList args;
        auto              it = funcDecls.find(node.func);
        ASSERT(it != funcDecls.end() && "Function should exist after semantic check");
        auto* fdecl = it->second;

        if (node.args)
        {
            for (size_t i = 0; i < node.args->size(); ++i)
            {
                auto* arg = (*(node.args))[i];
                if (!arg) continue;
                apply(*this, *arg, m);

                DataType argType = convert(arg->attr.val.value.type);
                DataType expect  = argType;
                if (fdecl->params && i < fdecl->params->size())
                {
                    auto* p = (*(fdecl->params))[i];
                    expect  = (p->dims && !p->dims->empty()) ? DataType::PTR : convert(p->type);
                }

                size_t reg = getMaxReg();
                if (expect != DataType::PTR && argType != expect)
                {
                    auto conv = createTypeConvertInst(argType, expect, reg);
                    for (auto* inst : conv) insert(inst);
                    reg = getMaxReg();
                }

                args.push_back({expect, getRegOperand(reg)});
            }
        }

        DataType retType = convert(fdecl->retType);
        if (retType == DataType::VOID)
        {
            insert(createCallInst(retType, node.func->getName(), args));
        }
        else
        {
            size_t resReg = getNewRegId();
            insert(createCallInst(retType, node.func->getName(), args, resReg));
        }
    }

    void ASTCodeGen::visit(FE::AST::CommaExpr& node, Module* m)
    {
        // TODO(Lab 3-2): 依序生成逗号表达式每个子表达式的 IR
        if (!node.exprs) return;
        for (auto* expr : *node.exprs)
        {
            if (!expr) continue;
            apply(*this, *expr, m);
        }
    }
}  // namespace ME
