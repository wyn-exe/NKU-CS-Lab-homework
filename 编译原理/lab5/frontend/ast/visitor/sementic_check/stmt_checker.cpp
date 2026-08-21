#include <frontend/ast/visitor/sementic_check/ast_checker.h>
#include <debug.h>

namespace FE::AST
{
    bool ASTChecker::visit(ExprStmt& node)
    {
        // 示例实现：表达式语句的语义检查
        // 空表达式直接通过，否则访问内部表达式
        if (!node.expr) return true;
        return apply(*this, *node.expr);
    }

    bool ASTChecker::visit(FuncDeclStmt& node)
    {
        // TODO(Lab3-1): 实现函数声明的语义检查
        // 检查作用域，记录函数信息，处理形参和函数体，检查返回语句
        bool res = true;

        auto it = funcDecls.find(node.entry);
        if (it != funcDecls.end())
        {
            addError(node, "Redefinition of function " + node.entry->getName());
            res = false;
        }
        else
        {
            funcDecls[node.entry] = &node;
        }

        if (node.entry->getName() == "main")
        {
            mainExists = true;
            if (node.retType != intType) addError(node, "main must return int");
            if (node.params && !node.params->empty()) addError(node, "main should not have parameters");
        }

        symTable.enterScope();
        curFuncRetType = node.retType;
        funcHasReturn  = false;
        loopDepth      = 0;

        if (node.params)
        {
            for (auto* p : *node.params)
            {
                if (!p) continue;
                res &= apply(*this, *p);
            }
        }

        if (node.body) res &= apply(*this, *node.body);

        symTable.exitScope();
        curFuncRetType = voidType;
        return res;
    }

    bool ASTChecker::visit(VarDeclStmt& node)
    {
        // TODO(Lab3-1): 实现变量声明语句的语义检查
        // 空声明直接通过，否则委托给变量声明处理
        if (!node.decl) return true;
        return apply(*this, *node.decl);
    }

    bool ASTChecker::visit(BlockStmt& node)
    {
        // TODO(Lab3-1): 实现块语句的语义检查
        // 进入新作用域，逐条访问语句，退出作用域
        bool res = true;
        symTable.enterScope();
        if (node.stmts)
        {
            for (auto* stmt : *node.stmts)
            {
                if (!stmt) continue;
                res &= apply(*this, *stmt);
            }
        }
        symTable.exitScope();
        return res;
    }

    bool ASTChecker::visit(ReturnStmt& node)
    {
        // TODO(Lab3-1): 实现返回语句的语义检查
        // 设置返回标记，检查作用域，检查返回值类型匹配
        bool res = true;
        funcHasReturn = true;

        if (isVoidType(curFuncRetType))
        {
            if (node.retExpr)
            {
                addError(node, "Void function should not return a value");
                res = false;
            }
            return res;
        }

        if (!node.retExpr)
        {
            addError(node, "Non-void function must return a value");
            return false;
        }

        res &= apply(*this, *node.retExpr);
        if (!canConvertTo(node.retExpr->attr.val.value.type, curFuncRetType))
        {
            addError(node, "Return type mismatch");
            res = false;
        }
        return res;
    }

    bool ASTChecker::visit(WhileStmt& node)
    {
        // TODO(Lab3-1): 实现while循环的语义检查
        // 检查作用域，访问条件表达式，管理循环深度，访问循环体
        bool res = true;
        if (node.cond)
        {
            res &= apply(*this, *node.cond);
            if (isVoidType(node.cond->attr.val.value.type) ||
                node.cond->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER)
            {
                addError(*node.cond, "While condition must be scalar");
                res = false;
            }
        }

        ++loopDepth;
        if (node.body) res &= apply(*this, *node.body);
        --loopDepth;
        return res;
    }

    bool ASTChecker::visit(IfStmt& node)
    {
        // TODO(Lab3-1): 实现if语句的语义检查
        // 检查作用域，访问条件表达式，分别访问then和else分支
        bool res = true;
        if (node.cond)
        {
            res &= apply(*this, *node.cond);
            if (isVoidType(node.cond->attr.val.value.type) ||
                node.cond->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER)
            {
                addError(*node.cond, "If condition must be scalar");
                res = false;
            }
        }

        if (node.thenStmt) res &= apply(*this, *node.thenStmt);
        if (node.elseStmt) res &= apply(*this, *node.elseStmt);
        return res;
    }

    bool ASTChecker::visit(BreakStmt& node)
    {
        // TODO(Lab3-1): 实现break语句的语义检查
        // 检查是否在循环内使用
        if (loopDepth == 0)
        {
            addError(node, "break used outside of loop");
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ContinueStmt& node)
    {
        // TODO(Lab3-1): 实现continue语句的语义检查
        // 检查是否在循环内使用
        if (loopDepth == 0)
        {
            addError(node, "continue used outside of loop");
            return false;
        }
        return true;
    }

    bool ASTChecker::visit(ForStmt& node)
    {
        // TODO(Lab3-1): 实现for循环的语义检查
        // 检查作用域，访问初始化、条件、步进表达式，管理循环深度
        bool res = true;
        symTable.enterScope();

        if (node.init) res &= apply(*this, *node.init);

        if (node.cond)
        {
            res &= apply(*this, *node.cond);
            if (isVoidType(node.cond->attr.val.value.type) ||
                node.cond->attr.val.value.type->getTypeGroup() == TypeGroup::POINTER)
            {
                addError(*node.cond, "For condition must be scalar");
                res = false;
            }
        }

        if (node.step) res &= apply(*this, *node.step);

        ++loopDepth;
        if (node.body) res &= apply(*this, *node.body);
        --loopDepth;

        symTable.exitScope();
        return res;
    }
}  // namespace FE::AST
