#include <middleend/visitor/codegen/ast_codegen.h>
#include <algorithm>

namespace ME
{
    namespace
    {
        bool isTerminated(Block* b) { return b && !b->insts.empty() && b->insts.back()->isTerminator(); }

        std::vector<int> evalParamDims(FE::AST::ParamDeclarator* p)
        {
            std::vector<int> dims;
            if (!p || !p->dims) return dims;
            for (size_t i = 0; i < p->dims->size(); ++i)
            {
                auto* d = (*(p->dims))[i];
                if (!d) continue;
                int v = d->attr.val.getInt();
                if (i == 0 && v < 0) v = 1;
                dims.push_back(v);
            }
            return dims;
        }
    }  // namespace

    void ASTCodeGen::visit(FE::AST::ExprStmt& node, Module* m)
    {
        if (!node.expr) return;
        apply(*this, *node.expr, m);
    }

    void ASTCodeGen::visit(FE::AST::FuncDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成函数定义 IR（形参、入口/结束基本块、返回补丁）
        // 设置函数返回类型与参数寄存器，创建基本块骨架，并生成函数体
        reg2attr.clear();
        paramPtrTab.clear();
        lval2ptr.clear();
        while (name2reg.curScope && name2reg.curScope->parent)
        {
            auto* p = name2reg.curScope->parent;
            delete name2reg.curScope;
            name2reg.curScope = p;
        }
        if (!name2reg.curScope) name2reg.curScope = new RegTab::Scope(nullptr);
        name2reg.curScope->sym2Reg.clear();

        std::vector<std::pair<DataType, Operand*>> args;
        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p  = (*(node.params))[i];
                bool isPtr = p && p->dims && !p->dims->empty();
                DataType dt = isPtr ? DataType::PTR : convert(p->type);
                size_t   regId = args.size() + 1;
                args.push_back({dt, getRegOperand(regId)});
            }
        }

        auto* fd   = new FuncDefInst(convert(node.retType), node.entry->getName(), args);
        auto* func = new Function(fd);
        func->setMaxReg(args.size());
        curFunc = func;
        Block* entry = createBlock();
        entryBlock  = entry;
        enterBlock(entry);
        m->functions.push_back(func);

        name2reg.enterScope();
        if (node.params)
        {
            for (size_t i = 0; i < node.params->size(); ++i)
            {
                auto* p     = (*(node.params))[i];
                bool  isPtr = p && p->dims && !p->dims->empty();
                if (isPtr)
                {
                    size_t regId = args[i].second->getRegNum();
                    name2reg.addSymbol(p->entry, regId);
                    FE::AST::VarAttr attr(p->type, false, 1);
                    attr.arrayDims           = evalParamDims(p);
                    reg2attr[regId]          = attr;
                    paramPtrTab[i]           = true;
                }
                else
                {
                    size_t allocaReg = getNewRegId();
                    insert(createAllocaInst(convert(p->type), allocaReg));
                    insert(createStoreInst(convert(p->type), args[i].second, getRegOperand(allocaReg)));
                    name2reg.addSymbol(p->entry, allocaReg);
                    FE::AST::VarAttr attr(p->type, false, 1);
                    reg2attr[allocaReg] = attr;
                    paramPtrTab[i]      = false;
                }
            }
        }

        if (node.body) apply(*this, *node.body, m);

        if (!isTerminated(curBlock))
        {
            DataType rt = convert(node.retType);
            if (rt == DataType::VOID)
                insert(createRetInst());
            else if (rt == DataType::F32)
                insert(createRetInst(0.0f));
            else
                insert(createRetInst(0));
        }

        name2reg.exitScope();
        exitBlock();
        entryBlock = nullptr;
        exitFunc();
    }

    void ASTCodeGen::visit(FE::AST::VarDeclStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成变量声明语句 IR（局部变量分配、初始化）
        if (!node.decl) return;
        apply(*this, *node.decl, m);
    }

    void ASTCodeGen::visit(FE::AST::BlockStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成语句块 IR（作用域管理，顺序生成子语句）
        name2reg.enterScope();
        if (node.stmts)
        {
            for (auto* stmt : *node.stmts)
            {
                if (!stmt) continue;
                if (isTerminated(curBlock)) break;
                apply(*this, *stmt, m);
            }
        }
        name2reg.exitScope();
    }

    void ASTCodeGen::visit(FE::AST::ReturnStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 return 语句 IR（可选返回值与类型转换）
        DataType funcRet = curFunc->funcDef->retType;

        if (!node.retExpr)
        {
            if (funcRet == DataType::VOID)
                insert(createRetInst());
            else if (funcRet == DataType::F32)
                insert(createRetInst(0.0f));
            else
                insert(createRetInst(0));
            return;
        }

        apply(*this, *node.retExpr, m);
        size_t   reg     = getMaxReg();
        DataType retType = convert(node.retExpr->attr.val.value.type);
        if (funcRet == DataType::VOID)
        {
            insert(createRetInst());
            return;
        }
        if (retType != funcRet)
        {
            auto conv = createTypeConvertInst(retType, funcRet, reg);
            for (auto* inst : conv) insert(inst);
            reg = getMaxReg();
        }
        insert(createRetInst(funcRet, reg));
    }

    void ASTCodeGen::visit(FE::AST::WhileStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 while 循环 IR（条件块、循环体与结束块、循环标签）
        Block* condBlock = createBlock();
        Block* bodyBlock = createBlock();
        Block* endBlock  = createBlock();

        if (!isTerminated(curBlock)) insert(createBranchInst(condBlock->blockId));

        enterBlock(condBlock);
        if (node.cond)
        {
            apply(*this, *node.cond, m);
            size_t   condReg  = getMaxReg();
            DataType condType = convert(node.cond->attr.val.value.type);
            if (condType != DataType::I1)
            {
                auto conv = createTypeConvertInst(condType, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            insert(createBranchInst(condReg, bodyBlock->blockId, endBlock->blockId));
        }
        else
        {
            insert(createBranchInst(bodyBlock->blockId));
        }

        loopStartStack.push_back(condBlock->blockId);
        loopEndStack.push_back(endBlock->blockId);

        enterBlock(bodyBlock);
        if (node.body) apply(*this, *node.body, m);
        if (!isTerminated(curBlock)) insert(createBranchInst(condBlock->blockId));

        loopStartStack.pop_back();
        loopEndStack.pop_back();

        enterBlock(endBlock);
    }

    void ASTCodeGen::visit(FE::AST::IfStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 if/else IR（then/else/end 基本块与条件分支）
        Block* thenBlock = createBlock();
        Block* elseBlock = node.elseStmt ? createBlock() : nullptr;
        Block* endBlock  = createBlock();

        apply(*this, *node.cond, m);
        size_t   condReg  = getMaxReg();
        DataType condType = convert(node.cond->attr.val.value.type);
        if (condType != DataType::I1)
        {
            auto conv = createTypeConvertInst(condType, DataType::I1, condReg);
            for (auto* inst : conv) insert(inst);
            condReg = getMaxReg();
        }
        insert(createBranchInst(condReg, thenBlock->blockId, elseBlock ? elseBlock->blockId : endBlock->blockId));

        enterBlock(thenBlock);
        if (node.thenStmt) apply(*this, *node.thenStmt, m);
        if (!isTerminated(curBlock)) insert(createBranchInst(endBlock->blockId));

        if (elseBlock)
        {
            enterBlock(elseBlock);
            apply(*this, *node.elseStmt, m);
            if (!isTerminated(curBlock)) insert(createBranchInst(endBlock->blockId));
        }

        enterBlock(endBlock);
    }

    void ASTCodeGen::visit(FE::AST::BreakStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 break 的无条件跳转至循环结束块
        (void)node;
        insert(createBranchInst(loopEndStack.back()));
    }

    void ASTCodeGen::visit(FE::AST::ContinueStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 continue 的无条件跳转至循环步进/条件块
        (void)node;
        insert(createBranchInst(loopStartStack.back()));
    }

    void ASTCodeGen::visit(FE::AST::ForStmt& node, Module* m)
    {
        // TODO(Lab 3-2): 生成 for 循环 IR（init/cond/body/step 基本块与循环标签）
        Block* condBlock = createBlock();
        Block* bodyBlock = createBlock();
        Block* stepBlock = createBlock();
        Block* endBlock  = createBlock();

        name2reg.enterScope();
        if (node.init) apply(*this, *node.init, m);
        if (!isTerminated(curBlock)) insert(createBranchInst(condBlock->blockId));

        enterBlock(condBlock);
        if (node.cond)
        {
            apply(*this, *node.cond, m);
            size_t   condReg  = getMaxReg();
            DataType condType = convert(node.cond->attr.val.value.type);
            if (condType != DataType::I1)
            {
                auto conv = createTypeConvertInst(condType, DataType::I1, condReg);
                for (auto* inst : conv) insert(inst);
                condReg = getMaxReg();
            }
            insert(createBranchInst(condReg, bodyBlock->blockId, endBlock->blockId));
        }
        else
        {
            insert(createBranchInst(bodyBlock->blockId));
        }

        loopStartStack.push_back(stepBlock->blockId);
        loopEndStack.push_back(endBlock->blockId);

        enterBlock(bodyBlock);
        if (node.body) apply(*this, *node.body, m);
        if (!isTerminated(curBlock)) insert(createBranchInst(stepBlock->blockId));

        enterBlock(stepBlock);
        if (node.step) apply(*this, *node.step, m);
        if (!isTerminated(curBlock)) insert(createBranchInst(condBlock->blockId));

        loopStartStack.pop_back();
        loopEndStack.pop_back();

        enterBlock(endBlock);
        name2reg.exitScope();
    }
}  // namespace ME
