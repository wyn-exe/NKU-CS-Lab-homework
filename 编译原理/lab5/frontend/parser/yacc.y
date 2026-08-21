%skeleton "lalr1.cc"
%require "3.2"

%define api.namespace { FE }
%define api.parser.class { YaccParser }
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%defines

%code requires
{
    #include <memory>
    #include <string>
    #include <sstream>
    #include <frontend/ast/ast_defs.h>
    #include <frontend/ast/ast.h>
    #include <frontend/ast/stmt.h>
    #include <frontend/ast/expr.h>
    #include <frontend/ast/decl.h>
    #include <frontend/symbol/symbol_entry.h>

    namespace FE
    {
        class Parser;
        class Scanner;
    }
}

%code top
{
    #include <iostream>

    #include <frontend/parser/parser.h>
    #include <frontend/parser/location.hh>
    #include <frontend/parser/scanner.h>
    #include <frontend/parser/yacc.h>

    using namespace FE;
    using namespace FE::AST;

    static YaccParser::symbol_type yylex(Scanner& scanner, Parser &parser)
    {
        (void)parser;
        return scanner.nextToken(); 
    }

    extern size_t errCnt;
    // 创建复合语句节点 - 如果语句列表为空则返回nullptr，如果只有一个语句则返回该语句，否则创建BlockStmt
    static StmtNode* makeBlockStmt(std::vector<StmtNode*>* stmts, const location& loc)
    {
        if (!stmts) stmts = new std::vector<StmtNode*>();
        return new BlockStmt(stmts, loc.begin.line, loc.begin.column);
    }
}

%lex-param { FE::Scanner& scanner }
%lex-param { FE::Parser& parser }
%parse-param { FE::Scanner& scanner }
%parse-param { FE::Parser& parser }

%locations

%define parse.error verbose
%define api.token.prefix {TOKEN_}

// 从这开始定义你需要用到的 token
// 对于一些需要 "值" 的 token，可以在前面加上 <type> 来指定值的类型
// 例如，%token <int> INT_CONST 定义了一个名为 INT_CONST
%token <int> INT_CONST
%token <long long> LL_CONST        /* long long 常量 */
%token <float>     FLOAT_CONST     /* 单精度浮点常量 */
%token <double>    DOUBLE_CONST    /* 双精度常量 */
%token <std::string> STR_CONST ERR_TOKEN 

%token <std::string> IDENT 

%token IF ELSE FOR WHILE CONTINUE BREAK SWITCH CASE GOTO DO RETURN CONST
%token SEMICOLON COMMA LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token INT FLOAT VOID DOUBLE
%token PLUS MINUS STAR SLASH MOD        /* 算术运算符 */
%token ASSIGN PLUSEQ MINUSEQ MULEQ DIVEQ MODEQ        /* 复合赋值运算符 */
%token EQ NE LT LE GT GE                /* 关系运算符 */
%token AND OR NOT                        /* 逻辑运算符 */
%token BITAND BITOR BITXOR BITNOT        /* 位运算符 */
%token LSHIFT RSHIFT                    /* 移位运算符 */
%token QUESTION COLON ARROW                /* 其他运算符 */
%token DOT ELLIPSIS
%token END

%nterm <FE::AST::Operator> UNARY_OP
%nterm <FE::AST::Type*> TYPE
%nterm <FE::AST::InitDecl*> INITIALIZER
%nterm <std::vector<FE::AST::InitDecl*>*> INITIALIZER_LIST
%nterm <FE::AST::VarDeclarator*> VAR_DECLARATOR
%nterm <std::vector<FE::AST::VarDeclarator*>*> VAR_DECLARATOR_LIST
%nterm <FE::AST::VarDeclaration*> VAR_DECLARATION
%nterm <FE::AST::ParamDeclarator*> PARAM_DECLARATOR
%nterm <std::vector<FE::AST::ParamDeclarator*>*> PARAM_DECLARATOR_LIST

%nterm <FE::AST::ExprNode*> LITERAL_EXPR
%nterm <FE::AST::ExprNode*> BASIC_EXPR
%nterm <FE::AST::ExprNode*> FUNC_CALL_EXPR
%nterm <FE::AST::ExprNode*> UNARY_EXPR
%nterm <FE::AST::ExprNode*> MULDIV_EXPR
%nterm <FE::AST::ExprNode*> ADDSUB_EXPR
%nterm <FE::AST::ExprNode*> RELATIONAL_EXPR
%nterm <FE::AST::ExprNode*> EQUALITY_EXPR

%nterm <FE::AST::ExprNode*> BIT_AND_EXPR    // 位与表达式
%nterm <FE::AST::ExprNode*> BIT_OR_EXPR     // 位或表达式

%nterm <FE::AST::ExprNode*> LOGICAL_AND_EXPR
%nterm <FE::AST::ExprNode*> LOGICAL_OR_EXPR
%nterm <FE::AST::ExprNode*> ASSIGN_EXPR
%nterm <FE::AST::ExprNode*> NOCOMMA_EXPR
%nterm <FE::AST::ExprNode*> EXPR
%nterm <std::vector<FE::AST::ExprNode*>*> EXPR_LIST

%nterm <FE::AST::ExprNode*> ARRAY_DIMENSION_EXPR
%nterm <std::vector<FE::AST::ExprNode*>*> ARRAY_DIMENSION_EXPR_LIST
%nterm <FE::AST::ExprNode*> LEFT_VAL_EXPR

%nterm <FE::AST::StmtNode*> EXPR_STMT
%nterm <FE::AST::StmtNode*> VAR_DECL_STMT
%nterm <FE::AST::StmtNode*> BLOCK_STMT
%nterm <FE::AST::StmtNode*> FUNC_DECL_STMT
%nterm <FE::AST::StmtNode*> RETURN_STMT
%nterm <FE::AST::StmtNode*> WHILE_STMT
%nterm <FE::AST::StmtNode*> IF_STMT
%nterm <FE::AST::StmtNode*> BREAK_STMT
%nterm <FE::AST::StmtNode*> CONTINUE_STMT
%nterm <FE::AST::StmtNode*> FOR_STMT
%nterm <FE::AST::StmtNode*> FUNC_BODY
%nterm <FE::AST::StmtNode*> STMT

%nterm <std::vector<FE::AST::StmtNode*>*> STMT_LIST
%nterm <FE::AST::Root*> PROGRAM

%start PROGRAM

//THEN和ELSE用于处理if和else的移进-规约冲突
%precedence THEN
%precedence ELSE
// token 定义结束

%%

/*
语法分析：补全TODO(Lab2)处的文法规则及处理函数。
如果你不打算实现float、array这些进阶要求，可将对应部分删去。
*/

//语法树匹配从这里开始
PROGRAM:
    STMT_LIST {
        $$ = new Root($1);
        parser.ast = $$;
    }
    | PROGRAM END {
        YYACCEPT;
    }
    ;

STMT_LIST:
    STMT {
        $$ = new std::vector<StmtNode*>();
        if ($1) $$->push_back($1);
    }
    | STMT_LIST STMT {
        $$ = $1;
        if ($2) $$->push_back($2);
    }
    ;

STMT:
    EXPR_STMT {
        $$ = $1;
    }
    | VAR_DECL_STMT {
        $$ = $1;
    }
    | FUNC_DECL_STMT {
        $$ = $1;
    }
    | FOR_STMT {
        $$ = $1;
    }
    | IF_STMT {
        $$ = $1;
    }
    | CONTINUE_STMT {
        $$ = $1;
    }

    | BREAK_STMT {
        $$ = $1;
    }
    | WHILE_STMT {
        $$ = $1;
    }
    | RETURN_STMT {
        $$ = $1;
    }
    | BLOCK_STMT {
        $$ = $1;
    }

    | SEMICOLON {
        $$ = nullptr;
    }
    //TODO(Lab2)：考虑更多语句类型
    ;

CONTINUE_STMT:
    CONTINUE SEMICOLON {
        $$ = new ContinueStmt(@1.begin.line, @1.begin.column);
    }
    ;

EXPR_STMT:
    EXPR SEMICOLON {
        $$ = new ExprStmt($1, @1.begin.line, @1.begin.column);
    }
    ;

VAR_DECLARATION:
    TYPE VAR_DECLARATOR_LIST {
        $$ = new VarDeclaration($1, $2, false, @1.begin.line, @1.begin.column);
    }
    | CONST TYPE VAR_DECLARATOR_LIST {
        $$ = new VarDeclaration($2, $3, true, @1.begin.line, @1.begin.column);
    }
    ;

VAR_DECL_STMT:
    /* TODO(Lab2): Implement variable declaration statement rule */
    VAR_DECLARATION SEMICOLON {
        $$ = new VarDeclStmt($1, @1.begin.line, @1.begin.column);
    }
    ;

BLOCK_STMT:  // 复合语句
    LBRACE RBRACE {
        $$ = makeBlockStmt(new std::vector<StmtNode*>(), @1);
    }
    | LBRACE STMT_LIST RBRACE {
        $$ = makeBlockStmt($2, @1);
    }
    ;

FUNC_BODY:  // 函数体
    LBRACE RBRACE {
        $$ = makeBlockStmt(new std::vector<StmtNode*>(), @1);
    }
    | LBRACE STMT_LIST RBRACE {
        $$ = makeBlockStmt($2, @1);
    }
    ;

FUNC_DECL_STMT:
    TYPE IDENT LPAREN PARAM_DECLARATOR_LIST RPAREN FUNC_BODY {
        Entry* entry = Entry::getEntry($2);
        $$ = new FuncDeclStmt($1, entry, $4, $6, @1.begin.line, @1.begin.column);
    }
    ;

FOR_STMT:
    FOR LPAREN VAR_DECLARATION SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT {
        VarDeclStmt* initStmt = new VarDeclStmt($3, @3.begin.line, @3.begin.column);
        $$ = new ForStmt(initStmt, $5, $7, $9, @1.begin.line, @1.begin.column);
    }
    | FOR LPAREN EXPR SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT {
        StmtNode* initStmt = new ExprStmt($3, $3->line_num, $3->col_num);
        $$ = new ForStmt(initStmt, $5, $7, $9, @1.begin.line, @1.begin.column);
    }
    ;

IF_STMT:
    /* TODO(Lab2): Implement if statement rule */
IF LPAREN EXPR RPAREN STMT %prec THEN {
        $$ = new IfStmt($3, $5, nullptr, @1.begin.line, @1.begin.column);
    }
    | IF LPAREN EXPR RPAREN STMT ELSE STMT {
        $$ = new IfStmt($3, $5, $7, @1.begin.line, @1.begin.column);
    }
    ;

BREAK_STMT:  // break语句
    BREAK SEMICOLON {
        $$ = new BreakStmt(@1.begin.line, @1.begin.column);
    }
    ;

RETURN_STMT:  // 返回语句
    RETURN SEMICOLON {
        $$ = new ReturnStmt(nullptr, @1.begin.line, @1.begin.column);
    }
    | RETURN EXPR SEMICOLON {
        $$ = new ReturnStmt($2, @1.begin.line, @1.begin.column);
    }
    ;

WHILE_STMT:  // while循环语句
    WHILE LPAREN EXPR RPAREN STMT {
        $$ = new WhileStmt($3, $5, @1.begin.line, @1.begin.column);
    }
    ;



//TODO(Lab2)：按照你补充的语句类型，实现这些语句的处理


PARAM_DECLARATOR:
    TYPE IDENT {
        Entry* entry = Entry::getEntry($2);
        $$ = new ParamDeclarator($1, entry, nullptr, @1.begin.line, @1.begin.column);
    }
    | TYPE IDENT LBRACKET RBRACKET {
        std::vector<ExprNode*>* dim = new std::vector<ExprNode*>();
        dim->emplace_back(new LiteralExpr(-1, @3.begin.line, @3.begin.column));
        Entry* entry = Entry::getEntry($2);
        $$ = new ParamDeclarator($1, entry, dim, @1.begin.line, @1.begin.column);
    }
    //TODO(Lab2)：考虑函数形参更多情况
    | TYPE IDENT ARRAY_DIMENSION_EXPR_LIST {  // 带维度的参数
        Entry* entry = Entry::getEntry($2);
        $$ = new ParamDeclarator($1, entry, $3, @1.begin.line, @1.begin.column);
    }
    | TYPE IDENT LBRACKET RBRACKET ARRAY_DIMENSION_EXPR_LIST {  // 混合参数
        auto* dim = $5;
        if (!dim) dim = new std::vector<ExprNode*>();
        dim->insert(dim->begin(), new LiteralExpr(-1, @3.begin.line, @3.begin.column));
        Entry* entry = Entry::getEntry($2);
        $$ = new ParamDeclarator($1, entry, dim, @1.begin.line, @1.begin.column);
    }
    ;

PARAM_DECLARATOR_LIST:
    /* empty */ {
        $$ = new std::vector<ParamDeclarator*>();
    }
    //TODO(Lab2)：考虑函数形参列表的构成情况
    | PARAM_DECLARATOR {
        $$ = new std::vector<ParamDeclarator*>();
        $$->push_back($1);
    }
    | PARAM_DECLARATOR_LIST COMMA PARAM_DECLARATOR {
        $$ = $1;
        if ($3) $$->push_back($3);
    }
    ;

VAR_DECLARATOR:
    //TODO(Lab2)：完成变量声明符的处理
    LEFT_VAL_EXPR {
        $$ = new VarDeclarator($1, nullptr, @1.begin.line, @1.begin.column);
    }
    | LEFT_VAL_EXPR ASSIGN INITIALIZER {
        $$ = new VarDeclarator($1, $3, @1.begin.line, @1.begin.column);
    }
    ;

VAR_DECLARATOR_LIST:
    VAR_DECLARATOR {
        $$ = new std::vector<VarDeclarator*>();
        $$->push_back($1);
    }
    | VAR_DECLARATOR_LIST COMMA VAR_DECLARATOR {
        $$ = $1;
        $$->push_back($3);
    }
    ;

INITIALIZER:
    /* TODO(Lab2): Implement variable initializer rule */
    // 初始化表达式
    NOCOMMA_EXPR {
        $$ = new Initializer($1, @1.begin.line, @1.begin.column);
    }
    | LBRACE RBRACE {
        auto* list = new std::vector<InitDecl*>();
        $$ = new InitializerList(list, @1.begin.line, @1.begin.column);
    }
    | LBRACE INITIALIZER_LIST RBRACE {
        $$ = new InitializerList($2, @1.begin.line, @1.begin.column);
    }
    ;

INITIALIZER_LIST:
    INITIALIZER {
        $$ = new std::vector<InitDecl*>();
        $$->push_back($1);
    }
    | INITIALIZER_LIST COMMA INITIALIZER {
        $$ = $1;
        $$->push_back($3);
    }
    ;

ASSIGN_EXPR:
    // TODO(Lab2): 完成赋值表达式的处理
    LEFT_VAL_EXPR ASSIGN NOCOMMA_EXPR {
        $$ = new BinaryExpr(Operator::ASSIGN, $1, $3, @2.begin.line, @2.begin.column);
    }
    ;

EXPR_LIST:
    NOCOMMA_EXPR {
        $$ = new std::vector<ExprNode*>();
        $$->push_back($1);
    }
    | EXPR_LIST COMMA NOCOMMA_EXPR {
        $$ = $1;
        $$->push_back($3);
    }
    ;

EXPR:
    NOCOMMA_EXPR {
        $$ = $1;
    }
    | EXPR COMMA NOCOMMA_EXPR {
        if ($1->isCommaExpr()) {
            CommaExpr* ce = static_cast<CommaExpr*>($1);
            ce->exprs->push_back($3);
            $$ = ce;
        } else {
            auto vec = new std::vector<ExprNode*>();
            vec->push_back($1);
            vec->push_back($3);
            $$ = new CommaExpr(vec, $1->line_num, $1->col_num);
        }
    }
    ;

NOCOMMA_EXPR:
    LOGICAL_OR_EXPR {
        $$ = $1;
    }
    | ASSIGN_EXPR {
        $$ = $1;
    }
    ;

LOGICAL_OR_EXPR:
    /* TODO(Lab2): Implement logical OR expression rule */
    // 逻辑或表达式
    LOGICAL_OR_EXPR OR LOGICAL_AND_EXPR {
        $$ = new BinaryExpr(Operator::OR, $1, $3, @2.begin.line, @2.begin.column);
    }
    | LOGICAL_AND_EXPR {
        $$ = $1;
    }
    ;

LOGICAL_AND_EXPR:
    /* TODO(Lab2): Implement logical AND expression rule */
    // 逻辑与表达式
    LOGICAL_AND_EXPR AND BIT_OR_EXPR {
        $$ = new BinaryExpr(Operator::AND, $1, $3, @2.begin.line, @2.begin.column);
    }
    | BIT_OR_EXPR {
        $$ = $1;
    }
    ;

EQUALITY_EXPR:
    /* TODO(Lab2): Implement equality expression rule */
    // 相等性表达式
    EQUALITY_EXPR EQ RELATIONAL_EXPR {
        $$ = new BinaryExpr(Operator::EQ, $1, $3, @2.begin.line, @2.begin.column);
    }
    | EQUALITY_EXPR NE RELATIONAL_EXPR {
        $$ = new BinaryExpr(Operator::NEQ, $1, $3, @2.begin.line, @2.begin.column);
    }
    | RELATIONAL_EXPR {
        $$ = $1;
    }
    ;

RELATIONAL_EXPR:
    /* TODO(Lab2): Implement relational expression rule */
    // 关系表达式
    RELATIONAL_EXPR LT ADDSUB_EXPR {
        $$ = new BinaryExpr(Operator::LT, $1, $3, @2.begin.line, @2.begin.column);
    }
    | RELATIONAL_EXPR LE ADDSUB_EXPR {
        $$ = new BinaryExpr(Operator::LE, $1, $3, @2.begin.line, @2.begin.column);
    }
    | RELATIONAL_EXPR GT ADDSUB_EXPR {
        $$ = new BinaryExpr(Operator::GT, $1, $3, @2.begin.line, @2.begin.column);
    }
    | RELATIONAL_EXPR GE ADDSUB_EXPR {
        $$ = new BinaryExpr(Operator::GE, $1, $3, @2.begin.line, @2.begin.column);
    }
    | ADDSUB_EXPR {
        $$ = $1;
    }
    ;

BIT_OR_EXPR:  // 位或表达式
    BIT_OR_EXPR BITOR BIT_AND_EXPR {
        $$ = new BinaryExpr(Operator::BITOR, $1, $3, @2.begin.line, @2.begin.column);
    }
    | BIT_AND_EXPR {
        $$ = $1;
    }
    ;

BIT_AND_EXPR:  // 位与表达式
    BIT_AND_EXPR BITAND EQUALITY_EXPR {
        $$ = new BinaryExpr(Operator::BITAND, $1, $3, @2.begin.line, @2.begin.column);
    }
    | EQUALITY_EXPR {
        $$ = $1;
    }
    ;

ADDSUB_EXPR:
    /* TODO(Lab2): Implement addition and subtraction expression rule */
    // 加减表达式
    ADDSUB_EXPR PLUS MULDIV_EXPR {
        $$ = new BinaryExpr(Operator::ADD, $1, $3, @2.begin.line, @2.begin.column);
    }
    | ADDSUB_EXPR MINUS MULDIV_EXPR {
        $$ = new BinaryExpr(Operator::SUB, $1, $3, @2.begin.line, @2.begin.column);
    }
    | MULDIV_EXPR {
        $$ = $1;
    }
    ;

MULDIV_EXPR:
    /* TODO(Lab2): Implement multiplication and division expression rule */
    // 乘除表达式
    MULDIV_EXPR STAR UNARY_EXPR {
        $$ = new BinaryExpr(Operator::MUL, $1, $3, @2.begin.line, @2.begin.column);
    }
    | MULDIV_EXPR SLASH UNARY_EXPR {
        $$ = new BinaryExpr(Operator::DIV, $1, $3, @2.begin.line, @2.begin.column);
    }
    | MULDIV_EXPR MOD UNARY_EXPR {
        $$ = new BinaryExpr(Operator::MOD, $1, $3, @2.begin.line, @2.begin.column);
    }
    | UNARY_EXPR {
        $$ = $1;
    }
    ;

UNARY_EXPR:
    BASIC_EXPR {
        $$ = $1;
    }
    | UNARY_OP UNARY_EXPR {
        $$ = new UnaryExpr($1, $2, $2->line_num, $2->col_num);
    }
    ;

BASIC_EXPR:
    LITERAL_EXPR {
        $$ = $1;
    }
    | LEFT_VAL_EXPR {
        $$ = $1;
    }
    | LPAREN EXPR RPAREN {
        $$ = $2;
    }
    | FUNC_CALL_EXPR {
        $$ = $1;
    }
    ;

FUNC_CALL_EXPR:
    IDENT LPAREN RPAREN {
        std::string funcName = $1;
        if (funcName != "starttime" && funcName != "stoptime")
        {
            Entry* entry = Entry::getEntry(funcName);
            $$ = new CallExpr(entry, nullptr, @1.begin.line, @1.begin.column);
        }
        else
        {    
            funcName = "_sysy_" + funcName;
            std::vector<ExprNode*>* args = new std::vector<ExprNode*>();
            args->emplace_back(new LiteralExpr(static_cast<int>(@1.begin.line), @1.begin.line, @1.begin.column));
            $$ = new CallExpr(Entry::getEntry(funcName), args, @1.begin.line, @1.begin.column);
        }
    }
    | IDENT LPAREN EXPR_LIST RPAREN {
        Entry* entry = Entry::getEntry($1);
        $$ = new CallExpr(entry, $3, @1.begin.line, @1.begin.column);
    }
    ;

ARRAY_DIMENSION_EXPR:
    LBRACKET NOCOMMA_EXPR RBRACKET {
        $$ = $2;
    }
    ;

ARRAY_DIMENSION_EXPR_LIST:
    /* TODO(Lab2): Implement variable dimension rule */
    // 数组维度表达式列表
    ARRAY_DIMENSION_EXPR {
        $$ = new std::vector<ExprNode*>();
        $$->push_back($1);
    }
    | ARRAY_DIMENSION_EXPR_LIST ARRAY_DIMENSION_EXPR {
        $$ = $1;
        $$->push_back($2);
    }
    ;

LEFT_VAL_EXPR:
    IDENT {
        Entry* entry = Entry::getEntry($1);
        $$ = new LeftValExpr(entry, nullptr, @1.begin.line, @1.begin.column);
    }
    | IDENT ARRAY_DIMENSION_EXPR_LIST {
        Entry* entry = Entry::getEntry($1);
        $$ = new LeftValExpr(entry, $2, @1.begin.line, @1.begin.column);
    }
    ;

LITERAL_EXPR:
    INT_CONST {
        $$ = new LiteralExpr($1, @1.begin.line, @1.begin.column);
    }
    //TODO(Lab2): 处理更多字面量
    | LL_CONST {
        $$ = new LiteralExpr($1, @1.begin.line, @1.begin.column);
    }
    | FLOAT_CONST {
        $$ = new LiteralExpr($1, @1.begin.line, @1.begin.column);
    }
    | DOUBLE_CONST {
        $$ = new LiteralExpr(static_cast<float>($1), @1.begin.line, @1.begin.column);
    }
    ;

TYPE:
    // TODO(Lab2): 完成类型的处理
    INT {
        $$ = intType;
    }
    | FLOAT {
        $$ = floatType;
    }
    | VOID {
        $$ = voidType;
    }
    ;

UNARY_OP:
    // TODO(Lab2): 完成一元运算符的处理
    PLUS {
        $$ = Operator::ADD;
    }
    | MINUS {
        $$ = Operator::SUB;
    }
    | NOT {
        $$ = Operator::NOT;
    }
    ;

%%

void FE::YaccParser::error(const FE::location& location, const std::string& message)
{
    std::cerr << "msg: " << message << ", error happened at: " << location << std::endl;
}
