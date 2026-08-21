// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

// "%code top" blocks.
#line 31 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"

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
        if (!stmts || stmts->empty())
        {
            delete stmts;
            return nullptr;
        }
        if (stmts->size() == 1)
        {
            auto* stmt = (*stmts)[0];
            delete stmts;
            return stmt;
        }
        return new BlockStmt(stmts, loc.begin.line, loc.begin.column);
    }

#line 75 "yacc.tab.c"




#include "yacc.tab.h"




#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 4 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
namespace  FE  {
#line 175 "yacc.tab.c"

  /// Build a parser object.
   YaccParser :: YaccParser  (FE::Scanner& scanner_yyarg, FE::Parser& parser_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      parser (parser_yyarg)
  {}

   YaccParser ::~ YaccParser  ()
  {}

   YaccParser ::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
   YaccParser ::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

   YaccParser ::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
   YaccParser ::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
   YaccParser ::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

   YaccParser ::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

   YaccParser ::symbol_kind_type
   YaccParser ::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

   YaccParser ::stack_symbol_type::stack_symbol_type ()
  {}

   YaccParser ::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_ASSIGN_EXPR: // ASSIGN_EXPR
      case symbol_kind::S_EXPR: // EXPR
      case symbol_kind::S_NOCOMMA_EXPR: // NOCOMMA_EXPR
      case symbol_kind::S_LOGICAL_OR_EXPR: // LOGICAL_OR_EXPR
      case symbol_kind::S_CONDITIONAL_EXPR: // CONDITIONAL_EXPR
      case symbol_kind::S_LOGICAL_AND_EXPR: // LOGICAL_AND_EXPR
      case symbol_kind::S_EQUALITY_EXPR: // EQUALITY_EXPR
      case symbol_kind::S_RELATIONAL_EXPR: // RELATIONAL_EXPR
      case symbol_kind::S_SHIFT_EXPR: // SHIFT_EXPR
      case symbol_kind::S_BIT_OR_EXPR: // BIT_OR_EXPR
      case symbol_kind::S_BIT_XOR_EXPR: // BIT_XOR_EXPR
      case symbol_kind::S_BIT_AND_EXPR: // BIT_AND_EXPR
      case symbol_kind::S_ADDSUB_EXPR: // ADDSUB_EXPR
      case symbol_kind::S_MULDIV_EXPR: // MULDIV_EXPR
      case symbol_kind::S_UNARY_EXPR: // UNARY_EXPR
      case symbol_kind::S_POSTFIX_EXPR: // POSTFIX_EXPR
      case symbol_kind::S_BASIC_EXPR: // BASIC_EXPR
      case symbol_kind::S_FUNC_CALL_EXPR: // FUNC_CALL_EXPR
      case symbol_kind::S_ARRAY_DIMENSION_EXPR: // ARRAY_DIMENSION_EXPR
      case symbol_kind::S_LEFT_VAL_EXPR: // LEFT_VAL_EXPR
      case symbol_kind::S_LITERAL_EXPR: // LITERAL_EXPR
        value.YY_MOVE_OR_COPY< FE::AST::ExprNode* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INITIALIZER: // INITIALIZER
        value.YY_MOVE_OR_COPY< FE::AST::InitDecl* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_UNARY_OP: // UNARY_OP
        value.YY_MOVE_OR_COPY< FE::AST::Operator > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PARAM_DECLARATOR: // PARAM_DECLARATOR
        value.YY_MOVE_OR_COPY< FE::AST::ParamDeclarator* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PROGRAM: // PROGRAM
        value.YY_MOVE_OR_COPY< FE::AST::Root* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STMT: // STMT
      case symbol_kind::S_CONTINUE_STMT: // CONTINUE_STMT
      case symbol_kind::S_EXPR_STMT: // EXPR_STMT
      case symbol_kind::S_VAR_DECL_STMT: // VAR_DECL_STMT
      case symbol_kind::S_BLOCK_STMT: // BLOCK_STMT
      case symbol_kind::S_FUNC_BODY: // FUNC_BODY
      case symbol_kind::S_FUNC_DECL_STMT: // FUNC_DECL_STMT
      case symbol_kind::S_FOR_STMT: // FOR_STMT
      case symbol_kind::S_IF_STMT: // IF_STMT
      case symbol_kind::S_BREAK_STMT: // BREAK_STMT
      case symbol_kind::S_RETURN_STMT: // RETURN_STMT
      case symbol_kind::S_WHILE_STMT: // WHILE_STMT
        value.YY_MOVE_OR_COPY< FE::AST::StmtNode* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_TYPE: // TYPE
        value.YY_MOVE_OR_COPY< FE::AST::Type* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATION: // VAR_DECLARATION
        value.YY_MOVE_OR_COPY< FE::AST::VarDeclaration* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATOR: // VAR_DECLARATOR
        value.YY_MOVE_OR_COPY< FE::AST::VarDeclarator* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DOUBLE_CONST: // DOUBLE_CONST
        value.YY_MOVE_OR_COPY< double > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FLOAT_CONST: // FLOAT_CONST
        value.YY_MOVE_OR_COPY< float > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INT_CONST: // INT_CONST
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_LL_CONST: // LL_CONST
        value.YY_MOVE_OR_COPY< long long > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STR_CONST: // STR_CONST
      case symbol_kind::S_ERR_TOKEN: // ERR_TOKEN
      case symbol_kind::S_IDENT: // IDENT
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_EXPR_LIST: // EXPR_LIST
      case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST: // ARRAY_DIMENSION_EXPR_LIST
        value.YY_MOVE_OR_COPY< std::vector<FE::AST::ExprNode*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INITIALIZER_LIST: // INITIALIZER_LIST
        value.YY_MOVE_OR_COPY< std::vector<FE::AST::InitDecl*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PARAM_DECLARATOR_LIST: // PARAM_DECLARATOR_LIST
        value.YY_MOVE_OR_COPY< std::vector<FE::AST::ParamDeclarator*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STMT_LIST: // STMT_LIST
        value.YY_MOVE_OR_COPY< std::vector<FE::AST::StmtNode*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATOR_LIST: // VAR_DECLARATOR_LIST
        value.YY_MOVE_OR_COPY< std::vector<FE::AST::VarDeclarator*>* > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

   YaccParser ::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_ASSIGN_EXPR: // ASSIGN_EXPR
      case symbol_kind::S_EXPR: // EXPR
      case symbol_kind::S_NOCOMMA_EXPR: // NOCOMMA_EXPR
      case symbol_kind::S_LOGICAL_OR_EXPR: // LOGICAL_OR_EXPR
      case symbol_kind::S_CONDITIONAL_EXPR: // CONDITIONAL_EXPR
      case symbol_kind::S_LOGICAL_AND_EXPR: // LOGICAL_AND_EXPR
      case symbol_kind::S_EQUALITY_EXPR: // EQUALITY_EXPR
      case symbol_kind::S_RELATIONAL_EXPR: // RELATIONAL_EXPR
      case symbol_kind::S_SHIFT_EXPR: // SHIFT_EXPR
      case symbol_kind::S_BIT_OR_EXPR: // BIT_OR_EXPR
      case symbol_kind::S_BIT_XOR_EXPR: // BIT_XOR_EXPR
      case symbol_kind::S_BIT_AND_EXPR: // BIT_AND_EXPR
      case symbol_kind::S_ADDSUB_EXPR: // ADDSUB_EXPR
      case symbol_kind::S_MULDIV_EXPR: // MULDIV_EXPR
      case symbol_kind::S_UNARY_EXPR: // UNARY_EXPR
      case symbol_kind::S_POSTFIX_EXPR: // POSTFIX_EXPR
      case symbol_kind::S_BASIC_EXPR: // BASIC_EXPR
      case symbol_kind::S_FUNC_CALL_EXPR: // FUNC_CALL_EXPR
      case symbol_kind::S_ARRAY_DIMENSION_EXPR: // ARRAY_DIMENSION_EXPR
      case symbol_kind::S_LEFT_VAL_EXPR: // LEFT_VAL_EXPR
      case symbol_kind::S_LITERAL_EXPR: // LITERAL_EXPR
        value.move< FE::AST::ExprNode* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INITIALIZER: // INITIALIZER
        value.move< FE::AST::InitDecl* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_UNARY_OP: // UNARY_OP
        value.move< FE::AST::Operator > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PARAM_DECLARATOR: // PARAM_DECLARATOR
        value.move< FE::AST::ParamDeclarator* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PROGRAM: // PROGRAM
        value.move< FE::AST::Root* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STMT: // STMT
      case symbol_kind::S_CONTINUE_STMT: // CONTINUE_STMT
      case symbol_kind::S_EXPR_STMT: // EXPR_STMT
      case symbol_kind::S_VAR_DECL_STMT: // VAR_DECL_STMT
      case symbol_kind::S_BLOCK_STMT: // BLOCK_STMT
      case symbol_kind::S_FUNC_BODY: // FUNC_BODY
      case symbol_kind::S_FUNC_DECL_STMT: // FUNC_DECL_STMT
      case symbol_kind::S_FOR_STMT: // FOR_STMT
      case symbol_kind::S_IF_STMT: // IF_STMT
      case symbol_kind::S_BREAK_STMT: // BREAK_STMT
      case symbol_kind::S_RETURN_STMT: // RETURN_STMT
      case symbol_kind::S_WHILE_STMT: // WHILE_STMT
        value.move< FE::AST::StmtNode* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_TYPE: // TYPE
        value.move< FE::AST::Type* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATION: // VAR_DECLARATION
        value.move< FE::AST::VarDeclaration* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATOR: // VAR_DECLARATOR
        value.move< FE::AST::VarDeclarator* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DOUBLE_CONST: // DOUBLE_CONST
        value.move< double > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FLOAT_CONST: // FLOAT_CONST
        value.move< float > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INT_CONST: // INT_CONST
        value.move< int > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_LL_CONST: // LL_CONST
        value.move< long long > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STR_CONST: // STR_CONST
      case symbol_kind::S_ERR_TOKEN: // ERR_TOKEN
      case symbol_kind::S_IDENT: // IDENT
        value.move< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_EXPR_LIST: // EXPR_LIST
      case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST: // ARRAY_DIMENSION_EXPR_LIST
        value.move< std::vector<FE::AST::ExprNode*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_INITIALIZER_LIST: // INITIALIZER_LIST
        value.move< std::vector<FE::AST::InitDecl*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_PARAM_DECLARATOR_LIST: // PARAM_DECLARATOR_LIST
        value.move< std::vector<FE::AST::ParamDeclarator*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_STMT_LIST: // STMT_LIST
        value.move< std::vector<FE::AST::StmtNode*>* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_VAR_DECLARATOR_LIST: // VAR_DECLARATOR_LIST
        value.move< std::vector<FE::AST::VarDeclarator*>* > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
   YaccParser ::stack_symbol_type&
   YaccParser ::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_ASSIGN_EXPR: // ASSIGN_EXPR
      case symbol_kind::S_EXPR: // EXPR
      case symbol_kind::S_NOCOMMA_EXPR: // NOCOMMA_EXPR
      case symbol_kind::S_LOGICAL_OR_EXPR: // LOGICAL_OR_EXPR
      case symbol_kind::S_CONDITIONAL_EXPR: // CONDITIONAL_EXPR
      case symbol_kind::S_LOGICAL_AND_EXPR: // LOGICAL_AND_EXPR
      case symbol_kind::S_EQUALITY_EXPR: // EQUALITY_EXPR
      case symbol_kind::S_RELATIONAL_EXPR: // RELATIONAL_EXPR
      case symbol_kind::S_SHIFT_EXPR: // SHIFT_EXPR
      case symbol_kind::S_BIT_OR_EXPR: // BIT_OR_EXPR
      case symbol_kind::S_BIT_XOR_EXPR: // BIT_XOR_EXPR
      case symbol_kind::S_BIT_AND_EXPR: // BIT_AND_EXPR
      case symbol_kind::S_ADDSUB_EXPR: // ADDSUB_EXPR
      case symbol_kind::S_MULDIV_EXPR: // MULDIV_EXPR
      case symbol_kind::S_UNARY_EXPR: // UNARY_EXPR
      case symbol_kind::S_POSTFIX_EXPR: // POSTFIX_EXPR
      case symbol_kind::S_BASIC_EXPR: // BASIC_EXPR
      case symbol_kind::S_FUNC_CALL_EXPR: // FUNC_CALL_EXPR
      case symbol_kind::S_ARRAY_DIMENSION_EXPR: // ARRAY_DIMENSION_EXPR
      case symbol_kind::S_LEFT_VAL_EXPR: // LEFT_VAL_EXPR
      case symbol_kind::S_LITERAL_EXPR: // LITERAL_EXPR
        value.copy< FE::AST::ExprNode* > (that.value);
        break;

      case symbol_kind::S_INITIALIZER: // INITIALIZER
        value.copy< FE::AST::InitDecl* > (that.value);
        break;

      case symbol_kind::S_UNARY_OP: // UNARY_OP
        value.copy< FE::AST::Operator > (that.value);
        break;

      case symbol_kind::S_PARAM_DECLARATOR: // PARAM_DECLARATOR
        value.copy< FE::AST::ParamDeclarator* > (that.value);
        break;

      case symbol_kind::S_PROGRAM: // PROGRAM
        value.copy< FE::AST::Root* > (that.value);
        break;

      case symbol_kind::S_STMT: // STMT
      case symbol_kind::S_CONTINUE_STMT: // CONTINUE_STMT
      case symbol_kind::S_EXPR_STMT: // EXPR_STMT
      case symbol_kind::S_VAR_DECL_STMT: // VAR_DECL_STMT
      case symbol_kind::S_BLOCK_STMT: // BLOCK_STMT
      case symbol_kind::S_FUNC_BODY: // FUNC_BODY
      case symbol_kind::S_FUNC_DECL_STMT: // FUNC_DECL_STMT
      case symbol_kind::S_FOR_STMT: // FOR_STMT
      case symbol_kind::S_IF_STMT: // IF_STMT
      case symbol_kind::S_BREAK_STMT: // BREAK_STMT
      case symbol_kind::S_RETURN_STMT: // RETURN_STMT
      case symbol_kind::S_WHILE_STMT: // WHILE_STMT
        value.copy< FE::AST::StmtNode* > (that.value);
        break;

      case symbol_kind::S_TYPE: // TYPE
        value.copy< FE::AST::Type* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATION: // VAR_DECLARATION
        value.copy< FE::AST::VarDeclaration* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATOR: // VAR_DECLARATOR
        value.copy< FE::AST::VarDeclarator* > (that.value);
        break;

      case symbol_kind::S_DOUBLE_CONST: // DOUBLE_CONST
        value.copy< double > (that.value);
        break;

      case symbol_kind::S_FLOAT_CONST: // FLOAT_CONST
        value.copy< float > (that.value);
        break;

      case symbol_kind::S_INT_CONST: // INT_CONST
        value.copy< int > (that.value);
        break;

      case symbol_kind::S_LL_CONST: // LL_CONST
        value.copy< long long > (that.value);
        break;

      case symbol_kind::S_STR_CONST: // STR_CONST
      case symbol_kind::S_ERR_TOKEN: // ERR_TOKEN
      case symbol_kind::S_IDENT: // IDENT
        value.copy< std::string > (that.value);
        break;

      case symbol_kind::S_EXPR_LIST: // EXPR_LIST
      case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST: // ARRAY_DIMENSION_EXPR_LIST
        value.copy< std::vector<FE::AST::ExprNode*>* > (that.value);
        break;

      case symbol_kind::S_INITIALIZER_LIST: // INITIALIZER_LIST
        value.copy< std::vector<FE::AST::InitDecl*>* > (that.value);
        break;

      case symbol_kind::S_PARAM_DECLARATOR_LIST: // PARAM_DECLARATOR_LIST
        value.copy< std::vector<FE::AST::ParamDeclarator*>* > (that.value);
        break;

      case symbol_kind::S_STMT_LIST: // STMT_LIST
        value.copy< std::vector<FE::AST::StmtNode*>* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATOR_LIST: // VAR_DECLARATOR_LIST
        value.copy< std::vector<FE::AST::VarDeclarator*>* > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

   YaccParser ::stack_symbol_type&
   YaccParser ::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_ASSIGN_EXPR: // ASSIGN_EXPR
      case symbol_kind::S_EXPR: // EXPR
      case symbol_kind::S_NOCOMMA_EXPR: // NOCOMMA_EXPR
      case symbol_kind::S_LOGICAL_OR_EXPR: // LOGICAL_OR_EXPR
      case symbol_kind::S_CONDITIONAL_EXPR: // CONDITIONAL_EXPR
      case symbol_kind::S_LOGICAL_AND_EXPR: // LOGICAL_AND_EXPR
      case symbol_kind::S_EQUALITY_EXPR: // EQUALITY_EXPR
      case symbol_kind::S_RELATIONAL_EXPR: // RELATIONAL_EXPR
      case symbol_kind::S_SHIFT_EXPR: // SHIFT_EXPR
      case symbol_kind::S_BIT_OR_EXPR: // BIT_OR_EXPR
      case symbol_kind::S_BIT_XOR_EXPR: // BIT_XOR_EXPR
      case symbol_kind::S_BIT_AND_EXPR: // BIT_AND_EXPR
      case symbol_kind::S_ADDSUB_EXPR: // ADDSUB_EXPR
      case symbol_kind::S_MULDIV_EXPR: // MULDIV_EXPR
      case symbol_kind::S_UNARY_EXPR: // UNARY_EXPR
      case symbol_kind::S_POSTFIX_EXPR: // POSTFIX_EXPR
      case symbol_kind::S_BASIC_EXPR: // BASIC_EXPR
      case symbol_kind::S_FUNC_CALL_EXPR: // FUNC_CALL_EXPR
      case symbol_kind::S_ARRAY_DIMENSION_EXPR: // ARRAY_DIMENSION_EXPR
      case symbol_kind::S_LEFT_VAL_EXPR: // LEFT_VAL_EXPR
      case symbol_kind::S_LITERAL_EXPR: // LITERAL_EXPR
        value.move< FE::AST::ExprNode* > (that.value);
        break;

      case symbol_kind::S_INITIALIZER: // INITIALIZER
        value.move< FE::AST::InitDecl* > (that.value);
        break;

      case symbol_kind::S_UNARY_OP: // UNARY_OP
        value.move< FE::AST::Operator > (that.value);
        break;

      case symbol_kind::S_PARAM_DECLARATOR: // PARAM_DECLARATOR
        value.move< FE::AST::ParamDeclarator* > (that.value);
        break;

      case symbol_kind::S_PROGRAM: // PROGRAM
        value.move< FE::AST::Root* > (that.value);
        break;

      case symbol_kind::S_STMT: // STMT
      case symbol_kind::S_CONTINUE_STMT: // CONTINUE_STMT
      case symbol_kind::S_EXPR_STMT: // EXPR_STMT
      case symbol_kind::S_VAR_DECL_STMT: // VAR_DECL_STMT
      case symbol_kind::S_BLOCK_STMT: // BLOCK_STMT
      case symbol_kind::S_FUNC_BODY: // FUNC_BODY
      case symbol_kind::S_FUNC_DECL_STMT: // FUNC_DECL_STMT
      case symbol_kind::S_FOR_STMT: // FOR_STMT
      case symbol_kind::S_IF_STMT: // IF_STMT
      case symbol_kind::S_BREAK_STMT: // BREAK_STMT
      case symbol_kind::S_RETURN_STMT: // RETURN_STMT
      case symbol_kind::S_WHILE_STMT: // WHILE_STMT
        value.move< FE::AST::StmtNode* > (that.value);
        break;

      case symbol_kind::S_TYPE: // TYPE
        value.move< FE::AST::Type* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATION: // VAR_DECLARATION
        value.move< FE::AST::VarDeclaration* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATOR: // VAR_DECLARATOR
        value.move< FE::AST::VarDeclarator* > (that.value);
        break;

      case symbol_kind::S_DOUBLE_CONST: // DOUBLE_CONST
        value.move< double > (that.value);
        break;

      case symbol_kind::S_FLOAT_CONST: // FLOAT_CONST
        value.move< float > (that.value);
        break;

      case symbol_kind::S_INT_CONST: // INT_CONST
        value.move< int > (that.value);
        break;

      case symbol_kind::S_LL_CONST: // LL_CONST
        value.move< long long > (that.value);
        break;

      case symbol_kind::S_STR_CONST: // STR_CONST
      case symbol_kind::S_ERR_TOKEN: // ERR_TOKEN
      case symbol_kind::S_IDENT: // IDENT
        value.move< std::string > (that.value);
        break;

      case symbol_kind::S_EXPR_LIST: // EXPR_LIST
      case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST: // ARRAY_DIMENSION_EXPR_LIST
        value.move< std::vector<FE::AST::ExprNode*>* > (that.value);
        break;

      case symbol_kind::S_INITIALIZER_LIST: // INITIALIZER_LIST
        value.move< std::vector<FE::AST::InitDecl*>* > (that.value);
        break;

      case symbol_kind::S_PARAM_DECLARATOR_LIST: // PARAM_DECLARATOR_LIST
        value.move< std::vector<FE::AST::ParamDeclarator*>* > (that.value);
        break;

      case symbol_kind::S_STMT_LIST: // STMT_LIST
        value.move< std::vector<FE::AST::StmtNode*>* > (that.value);
        break;

      case symbol_kind::S_VAR_DECLARATOR_LIST: // VAR_DECLARATOR_LIST
        value.move< std::vector<FE::AST::VarDeclarator*>* > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
   YaccParser ::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
   YaccParser ::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
   YaccParser ::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
   YaccParser ::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
   YaccParser ::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
   YaccParser ::debug_stream () const
  {
    return *yycdebug_;
  }

  void
   YaccParser ::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


   YaccParser ::debug_level_type
   YaccParser ::debug_level () const
  {
    return yydebug_;
  }

  void
   YaccParser ::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

   YaccParser ::state_type
   YaccParser ::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
   YaccParser ::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
   YaccParser ::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
   YaccParser ::operator() ()
  {
    return parse ();
  }

  int
   YaccParser ::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (scanner, parser));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_ASSIGN_EXPR: // ASSIGN_EXPR
      case symbol_kind::S_EXPR: // EXPR
      case symbol_kind::S_NOCOMMA_EXPR: // NOCOMMA_EXPR
      case symbol_kind::S_LOGICAL_OR_EXPR: // LOGICAL_OR_EXPR
      case symbol_kind::S_CONDITIONAL_EXPR: // CONDITIONAL_EXPR
      case symbol_kind::S_LOGICAL_AND_EXPR: // LOGICAL_AND_EXPR
      case symbol_kind::S_EQUALITY_EXPR: // EQUALITY_EXPR
      case symbol_kind::S_RELATIONAL_EXPR: // RELATIONAL_EXPR
      case symbol_kind::S_SHIFT_EXPR: // SHIFT_EXPR
      case symbol_kind::S_BIT_OR_EXPR: // BIT_OR_EXPR
      case symbol_kind::S_BIT_XOR_EXPR: // BIT_XOR_EXPR
      case symbol_kind::S_BIT_AND_EXPR: // BIT_AND_EXPR
      case symbol_kind::S_ADDSUB_EXPR: // ADDSUB_EXPR
      case symbol_kind::S_MULDIV_EXPR: // MULDIV_EXPR
      case symbol_kind::S_UNARY_EXPR: // UNARY_EXPR
      case symbol_kind::S_POSTFIX_EXPR: // POSTFIX_EXPR
      case symbol_kind::S_BASIC_EXPR: // BASIC_EXPR
      case symbol_kind::S_FUNC_CALL_EXPR: // FUNC_CALL_EXPR
      case symbol_kind::S_ARRAY_DIMENSION_EXPR: // ARRAY_DIMENSION_EXPR
      case symbol_kind::S_LEFT_VAL_EXPR: // LEFT_VAL_EXPR
      case symbol_kind::S_LITERAL_EXPR: // LITERAL_EXPR
        yylhs.value.emplace< FE::AST::ExprNode* > ();
        break;

      case symbol_kind::S_INITIALIZER: // INITIALIZER
        yylhs.value.emplace< FE::AST::InitDecl* > ();
        break;

      case symbol_kind::S_UNARY_OP: // UNARY_OP
        yylhs.value.emplace< FE::AST::Operator > ();
        break;

      case symbol_kind::S_PARAM_DECLARATOR: // PARAM_DECLARATOR
        yylhs.value.emplace< FE::AST::ParamDeclarator* > ();
        break;

      case symbol_kind::S_PROGRAM: // PROGRAM
        yylhs.value.emplace< FE::AST::Root* > ();
        break;

      case symbol_kind::S_STMT: // STMT
      case symbol_kind::S_CONTINUE_STMT: // CONTINUE_STMT
      case symbol_kind::S_EXPR_STMT: // EXPR_STMT
      case symbol_kind::S_VAR_DECL_STMT: // VAR_DECL_STMT
      case symbol_kind::S_BLOCK_STMT: // BLOCK_STMT
      case symbol_kind::S_FUNC_BODY: // FUNC_BODY
      case symbol_kind::S_FUNC_DECL_STMT: // FUNC_DECL_STMT
      case symbol_kind::S_FOR_STMT: // FOR_STMT
      case symbol_kind::S_IF_STMT: // IF_STMT
      case symbol_kind::S_BREAK_STMT: // BREAK_STMT
      case symbol_kind::S_RETURN_STMT: // RETURN_STMT
      case symbol_kind::S_WHILE_STMT: // WHILE_STMT
        yylhs.value.emplace< FE::AST::StmtNode* > ();
        break;

      case symbol_kind::S_TYPE: // TYPE
        yylhs.value.emplace< FE::AST::Type* > ();
        break;

      case symbol_kind::S_VAR_DECLARATION: // VAR_DECLARATION
        yylhs.value.emplace< FE::AST::VarDeclaration* > ();
        break;

      case symbol_kind::S_VAR_DECLARATOR: // VAR_DECLARATOR
        yylhs.value.emplace< FE::AST::VarDeclarator* > ();
        break;

      case symbol_kind::S_DOUBLE_CONST: // DOUBLE_CONST
        yylhs.value.emplace< double > ();
        break;

      case symbol_kind::S_FLOAT_CONST: // FLOAT_CONST
        yylhs.value.emplace< float > ();
        break;

      case symbol_kind::S_INT_CONST: // INT_CONST
        yylhs.value.emplace< int > ();
        break;

      case symbol_kind::S_LL_CONST: // LL_CONST
        yylhs.value.emplace< long long > ();
        break;

      case symbol_kind::S_STR_CONST: // STR_CONST
      case symbol_kind::S_ERR_TOKEN: // ERR_TOKEN
      case symbol_kind::S_IDENT: // IDENT
        yylhs.value.emplace< std::string > ();
        break;

      case symbol_kind::S_EXPR_LIST: // EXPR_LIST
      case symbol_kind::S_ARRAY_DIMENSION_EXPR_LIST: // ARRAY_DIMENSION_EXPR_LIST
        yylhs.value.emplace< std::vector<FE::AST::ExprNode*>* > ();
        break;

      case symbol_kind::S_INITIALIZER_LIST: // INITIALIZER_LIST
        yylhs.value.emplace< std::vector<FE::AST::InitDecl*>* > ();
        break;

      case symbol_kind::S_PARAM_DECLARATOR_LIST: // PARAM_DECLARATOR_LIST
        yylhs.value.emplace< std::vector<FE::AST::ParamDeclarator*>* > ();
        break;

      case symbol_kind::S_STMT_LIST: // STMT_LIST
        yylhs.value.emplace< std::vector<FE::AST::StmtNode*>* > ();
        break;

      case symbol_kind::S_VAR_DECLARATOR_LIST: // VAR_DECLARATOR_LIST
        yylhs.value.emplace< std::vector<FE::AST::VarDeclarator*>* > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // PROGRAM: STMT_LIST
#line 171 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
              {
        yylhs.value.as < FE::AST::Root* > () = new Root(yystack_[0].value.as < std::vector<FE::AST::StmtNode*>* > ());
        parser.ast = yylhs.value.as < FE::AST::Root* > ();
    }
#line 1123 "yacc.tab.c"
    break;

  case 3: // PROGRAM: PROGRAM END
#line 175 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        YYACCEPT;
    }
#line 1131 "yacc.tab.c"
    break;

  case 4: // STMT_LIST: STMT
#line 181 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
         {
        yylhs.value.as < std::vector<FE::AST::StmtNode*>* > () = new std::vector<StmtNode*>();
        if (yystack_[0].value.as < FE::AST::StmtNode* > ()) yylhs.value.as < std::vector<FE::AST::StmtNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::StmtNode* > ());
    }
#line 1140 "yacc.tab.c"
    break;

  case 5: // STMT_LIST: STMT_LIST STMT
#line 185 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                     {
        yylhs.value.as < std::vector<FE::AST::StmtNode*>* > () = yystack_[1].value.as < std::vector<FE::AST::StmtNode*>* > ();
        if (yystack_[0].value.as < FE::AST::StmtNode* > ()) yylhs.value.as < std::vector<FE::AST::StmtNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::StmtNode* > ());
    }
#line 1149 "yacc.tab.c"
    break;

  case 6: // STMT: EXPR_STMT
#line 192 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
              {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1157 "yacc.tab.c"
    break;

  case 7: // STMT: VAR_DECL_STMT
#line 195 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1165 "yacc.tab.c"
    break;

  case 8: // STMT: FUNC_DECL_STMT
#line 198 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                     {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1173 "yacc.tab.c"
    break;

  case 9: // STMT: FOR_STMT
#line 201 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
               {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1181 "yacc.tab.c"
    break;

  case 10: // STMT: IF_STMT
#line 204 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
              {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1189 "yacc.tab.c"
    break;

  case 11: // STMT: CONTINUE_STMT
#line 207 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1197 "yacc.tab.c"
    break;

  case 12: // STMT: BREAK_STMT
#line 211 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1205 "yacc.tab.c"
    break;

  case 13: // STMT: WHILE_STMT
#line 214 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1213 "yacc.tab.c"
    break;

  case 14: // STMT: RETURN_STMT
#line 217 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1221 "yacc.tab.c"
    break;

  case 15: // STMT: BLOCK_STMT
#line 220 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::StmtNode* > () = yystack_[0].value.as < FE::AST::StmtNode* > ();
    }
#line 1229 "yacc.tab.c"
    break;

  case 16: // STMT: SEMICOLON
#line 224 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                {
        yylhs.value.as < FE::AST::StmtNode* > () = nullptr;
    }
#line 1237 "yacc.tab.c"
    break;

  case 17: // CONTINUE_STMT: CONTINUE SEMICOLON
#line 231 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                       {
        yylhs.value.as < FE::AST::StmtNode* > () = new ContinueStmt(yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1245 "yacc.tab.c"
    break;

  case 18: // EXPR_STMT: EXPR SEMICOLON
#line 237 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                   {
        yylhs.value.as < FE::AST::StmtNode* > () = new ExprStmt(yystack_[1].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1253 "yacc.tab.c"
    break;

  case 19: // VAR_DECLARATION: TYPE VAR_DECLARATOR_LIST
#line 243 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                             {
        yylhs.value.as < FE::AST::VarDeclaration* > () = new VarDeclaration(yystack_[1].value.as < FE::AST::Type* > (), yystack_[0].value.as < std::vector<FE::AST::VarDeclarator*>* > (), false, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1261 "yacc.tab.c"
    break;

  case 20: // VAR_DECLARATION: CONST TYPE VAR_DECLARATOR_LIST
#line 246 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                     {
        yylhs.value.as < FE::AST::VarDeclaration* > () = new VarDeclaration(yystack_[1].value.as < FE::AST::Type* > (), yystack_[0].value.as < std::vector<FE::AST::VarDeclarator*>* > (), true, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
    }
#line 1269 "yacc.tab.c"
    break;

  case 21: // VAR_DECL_STMT: VAR_DECLARATION SEMICOLON
#line 253 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                              {
        yylhs.value.as < FE::AST::StmtNode* > () = new VarDeclStmt(yystack_[1].value.as < FE::AST::VarDeclaration* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1277 "yacc.tab.c"
    break;

  case 22: // BLOCK_STMT: LBRACE RBRACE
#line 259 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::StmtNode* > () = nullptr;
    }
#line 1285 "yacc.tab.c"
    break;

  case 23: // BLOCK_STMT: LBRACE STMT_LIST RBRACE
#line 262 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                              {
        yylhs.value.as < FE::AST::StmtNode* > () = makeBlockStmt(yystack_[1].value.as < std::vector<FE::AST::StmtNode*>* > (), yystack_[2].location);
    }
#line 1293 "yacc.tab.c"
    break;

  case 24: // FUNC_BODY: LBRACE RBRACE
#line 268 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::StmtNode* > () = nullptr;
    }
#line 1301 "yacc.tab.c"
    break;

  case 25: // FUNC_BODY: LBRACE STMT_LIST RBRACE
#line 271 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                              {
        yylhs.value.as < FE::AST::StmtNode* > () = makeBlockStmt(yystack_[1].value.as < std::vector<FE::AST::StmtNode*>* > (), yystack_[2].location);
    }
#line 1309 "yacc.tab.c"
    break;

  case 26: // FUNC_DECL_STMT: TYPE IDENT LPAREN PARAM_DECLARATOR_LIST RPAREN FUNC_BODY
#line 277 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                             {
        Entry* entry = Entry::getEntry(yystack_[4].value.as < std::string > ());
        yylhs.value.as < FE::AST::StmtNode* > () = new FuncDeclStmt(yystack_[5].value.as < FE::AST::Type* > (), entry, yystack_[2].value.as < std::vector<FE::AST::ParamDeclarator*>* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), yystack_[5].location.begin.line, yystack_[5].location.begin.column);
    }
#line 1318 "yacc.tab.c"
    break;

  case 27: // FOR_STMT: FOR LPAREN VAR_DECLARATION SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT
#line 284 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                                         {
        VarDeclStmt* initStmt = new VarDeclStmt(yystack_[6].value.as < FE::AST::VarDeclaration* > (), yystack_[6].location.begin.line, yystack_[6].location.begin.column);
        yylhs.value.as < FE::AST::StmtNode* > () = new ForStmt(initStmt, yystack_[4].value.as < FE::AST::ExprNode* > (), yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), yystack_[8].location.begin.line, yystack_[8].location.begin.column);
    }
#line 1327 "yacc.tab.c"
    break;

  case 28: // FOR_STMT: FOR LPAREN EXPR SEMICOLON EXPR SEMICOLON EXPR RPAREN STMT
#line 288 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                                {
        StmtNode* initStmt = new ExprStmt(yystack_[6].value.as < FE::AST::ExprNode* > (), yystack_[6].value.as < FE::AST::ExprNode* > ()->line_num, yystack_[6].value.as < FE::AST::ExprNode* > ()->col_num);
        yylhs.value.as < FE::AST::StmtNode* > () = new ForStmt(initStmt, yystack_[4].value.as < FE::AST::ExprNode* > (), yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), yystack_[8].location.begin.line, yystack_[8].location.begin.column);
    }
#line 1336 "yacc.tab.c"
    break;

  case 29: // IF_STMT: IF LPAREN EXPR RPAREN STMT
#line 296 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        yylhs.value.as < FE::AST::StmtNode* > () = new IfStmt(yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), nullptr, yystack_[4].location.begin.line, yystack_[4].location.begin.column);
    }
#line 1344 "yacc.tab.c"
    break;

  case 30: // IF_STMT: IF LPAREN EXPR RPAREN STMT ELSE STMT
#line 299 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                           {
        yylhs.value.as < FE::AST::StmtNode* > () = new IfStmt(yystack_[4].value.as < FE::AST::ExprNode* > (), yystack_[2].value.as < FE::AST::StmtNode* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), yystack_[6].location.begin.line, yystack_[6].location.begin.column);
    }
#line 1352 "yacc.tab.c"
    break;

  case 31: // BREAK_STMT: BREAK SEMICOLON
#line 305 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::StmtNode* > () = new BreakStmt(yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1360 "yacc.tab.c"
    break;

  case 32: // RETURN_STMT: RETURN SEMICOLON
#line 311 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                     {
        yylhs.value.as < FE::AST::StmtNode* > () = new ReturnStmt(nullptr, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1368 "yacc.tab.c"
    break;

  case 33: // RETURN_STMT: RETURN EXPR SEMICOLON
#line 314 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                            {
        yylhs.value.as < FE::AST::StmtNode* > () = new ReturnStmt(yystack_[1].value.as < FE::AST::ExprNode* > (), yystack_[2].location.begin.line, yystack_[2].location.begin.column);
    }
#line 1376 "yacc.tab.c"
    break;

  case 34: // WHILE_STMT: WHILE LPAREN EXPR RPAREN STMT
#line 320 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                  {
        yylhs.value.as < FE::AST::StmtNode* > () = new WhileStmt(yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::StmtNode* > (), yystack_[4].location.begin.line, yystack_[4].location.begin.column);
    }
#line 1384 "yacc.tab.c"
    break;

  case 35: // PARAM_DECLARATOR: TYPE IDENT
#line 331 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
               {
        Entry* entry = Entry::getEntry(yystack_[0].value.as < std::string > ());
        yylhs.value.as < FE::AST::ParamDeclarator* > () = new ParamDeclarator(yystack_[1].value.as < FE::AST::Type* > (), entry, nullptr, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1393 "yacc.tab.c"
    break;

  case 36: // PARAM_DECLARATOR: TYPE IDENT LBRACKET RBRACKET
#line 335 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                   {
        std::vector<ExprNode*>* dim = new std::vector<ExprNode*>();
        dim->emplace_back(new LiteralExpr(-1, yystack_[1].location.begin.line, yystack_[1].location.begin.column));
        Entry* entry = Entry::getEntry(yystack_[2].value.as < std::string > ());
        yylhs.value.as < FE::AST::ParamDeclarator* > () = new ParamDeclarator(yystack_[3].value.as < FE::AST::Type* > (), entry, dim, yystack_[3].location.begin.line, yystack_[3].location.begin.column);
    }
#line 1404 "yacc.tab.c"
    break;

  case 37: // PARAM_DECLARATOR: TYPE IDENT ARRAY_DIMENSION_EXPR_LIST
#line 342 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                           {  // 带维度的参数
        Entry* entry = Entry::getEntry(yystack_[1].value.as < std::string > ());
        yylhs.value.as < FE::AST::ParamDeclarator* > () = new ParamDeclarator(yystack_[2].value.as < FE::AST::Type* > (), entry, yystack_[0].value.as < std::vector<FE::AST::ExprNode*>* > (), yystack_[2].location.begin.line, yystack_[2].location.begin.column);
    }
#line 1413 "yacc.tab.c"
    break;

  case 38: // PARAM_DECLARATOR: TYPE IDENT LBRACKET RBRACKET ARRAY_DIMENSION_EXPR_LIST
#line 346 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                             {  // 混合参数
        auto* dim = yystack_[0].value.as < std::vector<FE::AST::ExprNode*>* > ();
        if (!dim) dim = new std::vector<ExprNode*>();
        dim->insert(dim->begin(), new LiteralExpr(-1, yystack_[2].location.begin.line, yystack_[2].location.begin.column));
        Entry* entry = Entry::getEntry(yystack_[3].value.as < std::string > ());
        yylhs.value.as < FE::AST::ParamDeclarator* > () = new ParamDeclarator(yystack_[4].value.as < FE::AST::Type* > (), entry, dim, yystack_[4].location.begin.line, yystack_[4].location.begin.column);
    }
#line 1425 "yacc.tab.c"
    break;

  case 39: // PARAM_DECLARATOR_LIST: %empty
#line 356 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                {
        yylhs.value.as < std::vector<FE::AST::ParamDeclarator*>* > () = new std::vector<ParamDeclarator*>();
    }
#line 1433 "yacc.tab.c"
    break;

  case 40: // PARAM_DECLARATOR_LIST: PARAM_DECLARATOR
#line 360 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                       {
        yylhs.value.as < std::vector<FE::AST::ParamDeclarator*>* > () = new std::vector<ParamDeclarator*>();
        yylhs.value.as < std::vector<FE::AST::ParamDeclarator*>* > ()->push_back(yystack_[0].value.as < FE::AST::ParamDeclarator* > ());
    }
#line 1442 "yacc.tab.c"
    break;

  case 41: // PARAM_DECLARATOR_LIST: PARAM_DECLARATOR_LIST COMMA PARAM_DECLARATOR
#line 364 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                   {
        yylhs.value.as < std::vector<FE::AST::ParamDeclarator*>* > () = yystack_[2].value.as < std::vector<FE::AST::ParamDeclarator*>* > ();
        if (yystack_[0].value.as < FE::AST::ParamDeclarator* > ()) yylhs.value.as < std::vector<FE::AST::ParamDeclarator*>* > ()->push_back(yystack_[0].value.as < FE::AST::ParamDeclarator* > ());
    }
#line 1451 "yacc.tab.c"
    break;

  case 42: // VAR_DECLARATOR: LEFT_VAL_EXPR
#line 372 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::VarDeclarator* > () = new VarDeclarator(yystack_[0].value.as < FE::AST::ExprNode* > (), nullptr, yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 1459 "yacc.tab.c"
    break;

  case 43: // VAR_DECLARATOR: LEFT_VAL_EXPR ASSIGN INITIALIZER
#line 375 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                       {
        yylhs.value.as < FE::AST::VarDeclarator* > () = new VarDeclarator(yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::InitDecl* > (), yystack_[2].location.begin.line, yystack_[2].location.begin.column);
    }
#line 1467 "yacc.tab.c"
    break;

  case 44: // VAR_DECLARATOR_LIST: VAR_DECLARATOR
#line 381 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                   {
        yylhs.value.as < std::vector<FE::AST::VarDeclarator*>* > () = new std::vector<VarDeclarator*>();
        yylhs.value.as < std::vector<FE::AST::VarDeclarator*>* > ()->push_back(yystack_[0].value.as < FE::AST::VarDeclarator* > ());
    }
#line 1476 "yacc.tab.c"
    break;

  case 45: // VAR_DECLARATOR_LIST: VAR_DECLARATOR_LIST COMMA VAR_DECLARATOR
#line 385 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                               {
        yylhs.value.as < std::vector<FE::AST::VarDeclarator*>* > () = yystack_[2].value.as < std::vector<FE::AST::VarDeclarator*>* > ();
        yylhs.value.as < std::vector<FE::AST::VarDeclarator*>* > ()->push_back(yystack_[0].value.as < FE::AST::VarDeclarator* > ());
    }
#line 1485 "yacc.tab.c"
    break;

  case 46: // INITIALIZER: NOCOMMA_EXPR
#line 394 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::InitDecl* > () = new Initializer(yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 1493 "yacc.tab.c"
    break;

  case 47: // INITIALIZER: LBRACE RBRACE
#line 397 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        auto* list = new std::vector<InitDecl*>();
        yylhs.value.as < FE::AST::InitDecl* > () = new InitializerList(list, yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1502 "yacc.tab.c"
    break;

  case 48: // INITIALIZER: LBRACE INITIALIZER_LIST RBRACE
#line 401 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                     {
        yylhs.value.as < FE::AST::InitDecl* > () = new InitializerList(yystack_[1].value.as < std::vector<FE::AST::InitDecl*>* > (), yystack_[2].location.begin.line, yystack_[2].location.begin.column);
    }
#line 1510 "yacc.tab.c"
    break;

  case 49: // INITIALIZER_LIST: INITIALIZER
#line 407 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                {
        yylhs.value.as < std::vector<FE::AST::InitDecl*>* > () = new std::vector<InitDecl*>();
        yylhs.value.as < std::vector<FE::AST::InitDecl*>* > ()->push_back(yystack_[0].value.as < FE::AST::InitDecl* > ());
    }
#line 1519 "yacc.tab.c"
    break;

  case 50: // INITIALIZER_LIST: INITIALIZER_LIST COMMA INITIALIZER
#line 411 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                         {
        yylhs.value.as < std::vector<FE::AST::InitDecl*>* > () = yystack_[2].value.as < std::vector<FE::AST::InitDecl*>* > ();
        yylhs.value.as < std::vector<FE::AST::InitDecl*>* > ()->push_back(yystack_[0].value.as < FE::AST::InitDecl* > ());
    }
#line 1528 "yacc.tab.c"
    break;

  case 51: // ASSIGN_EXPR: CONDITIONAL_EXPR
#line 419 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                     {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1536 "yacc.tab.c"
    break;

  case 52: // ASSIGN_EXPR: LEFT_VAL_EXPR ASSIGN ASSIGN_EXPR
#line 422 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                       {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1544 "yacc.tab.c"
    break;

  case 53: // ASSIGN_EXPR: LEFT_VAL_EXPR PLUSEQ ASSIGN_EXPR
#line 425 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                       {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::ADD_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1552 "yacc.tab.c"
    break;

  case 54: // ASSIGN_EXPR: LEFT_VAL_EXPR MINUSEQ ASSIGN_EXPR
#line 428 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                        {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::SUB_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1560 "yacc.tab.c"
    break;

  case 55: // ASSIGN_EXPR: LEFT_VAL_EXPR MULEQ ASSIGN_EXPR
#line 431 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::MUL_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1568 "yacc.tab.c"
    break;

  case 56: // ASSIGN_EXPR: LEFT_VAL_EXPR DIVEQ ASSIGN_EXPR
#line 434 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::DIV_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1576 "yacc.tab.c"
    break;

  case 57: // ASSIGN_EXPR: LEFT_VAL_EXPR MODEQ ASSIGN_EXPR
#line 437 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::MOD_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1584 "yacc.tab.c"
    break;

  case 58: // ASSIGN_EXPR: LEFT_VAL_EXPR BITOREQ ASSIGN_EXPR
#line 440 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                        {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITOR_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1592 "yacc.tab.c"
    break;

  case 59: // ASSIGN_EXPR: LEFT_VAL_EXPR BITANDEQ ASSIGN_EXPR
#line 443 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                         {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITAND_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1600 "yacc.tab.c"
    break;

  case 60: // ASSIGN_EXPR: LEFT_VAL_EXPR BITXOREQ ASSIGN_EXPR
#line 446 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                         {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITXOR_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1608 "yacc.tab.c"
    break;

  case 61: // ASSIGN_EXPR: LEFT_VAL_EXPR LSHIFTEQ ASSIGN_EXPR
#line 449 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                         {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::LSHIFT_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1616 "yacc.tab.c"
    break;

  case 62: // ASSIGN_EXPR: LEFT_VAL_EXPR RSHIFTEQ ASSIGN_EXPR
#line 452 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                         {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::RSHIFT_ASSIGN, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1624 "yacc.tab.c"
    break;

  case 63: // EXPR_LIST: NOCOMMA_EXPR
#line 458 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > () = new std::vector<ExprNode*>();
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
    }
#line 1633 "yacc.tab.c"
    break;

  case 64: // EXPR_LIST: EXPR_LIST COMMA NOCOMMA_EXPR
#line 462 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                   {
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > () = yystack_[2].value.as < std::vector<FE::AST::ExprNode*>* > ();
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
    }
#line 1642 "yacc.tab.c"
    break;

  case 65: // EXPR: NOCOMMA_EXPR
#line 469 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1650 "yacc.tab.c"
    break;

  case 66: // EXPR: EXPR COMMA NOCOMMA_EXPR
#line 472 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                              {
        if (yystack_[2].value.as < FE::AST::ExprNode* > ()->isCommaExpr()) {
            CommaExpr* ce = static_cast<CommaExpr*>(yystack_[2].value.as < FE::AST::ExprNode* > ());
            ce->exprs->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
            yylhs.value.as < FE::AST::ExprNode* > () = ce;
        } else {
            auto vec = new std::vector<ExprNode*>();
            vec->push_back(yystack_[2].value.as < FE::AST::ExprNode* > ());
            vec->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
            yylhs.value.as < FE::AST::ExprNode* > () = new CommaExpr(vec, yystack_[2].value.as < FE::AST::ExprNode* > ()->line_num, yystack_[2].value.as < FE::AST::ExprNode* > ()->col_num);
        }
    }
#line 1667 "yacc.tab.c"
    break;

  case 67: // NOCOMMA_EXPR: ASSIGN_EXPR
#line 487 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1675 "yacc.tab.c"
    break;

  case 68: // LOGICAL_OR_EXPR: LOGICAL_OR_EXPR OR LOGICAL_AND_EXPR
#line 495 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                        {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::OR, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1683 "yacc.tab.c"
    break;

  case 69: // LOGICAL_OR_EXPR: LOGICAL_AND_EXPR
#line 498 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                       {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1691 "yacc.tab.c"
    break;

  case 70: // CONDITIONAL_EXPR: LOGICAL_OR_EXPR
#line 504 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1699 "yacc.tab.c"
    break;

  case 71: // CONDITIONAL_EXPR: LOGICAL_OR_EXPR QUESTION EXPR COLON CONDITIONAL_EXPR
#line 507 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                           {
        yylhs.value.as < FE::AST::ExprNode* > () = new ConditionalExpr(yystack_[4].value.as < FE::AST::ExprNode* > (), yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[3].location.begin.line, yystack_[3].location.begin.column);
    }
#line 1707 "yacc.tab.c"
    break;

  case 72: // LOGICAL_AND_EXPR: LOGICAL_AND_EXPR AND BIT_OR_EXPR
#line 515 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                     {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::AND, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1715 "yacc.tab.c"
    break;

  case 73: // LOGICAL_AND_EXPR: BIT_OR_EXPR
#line 518 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1723 "yacc.tab.c"
    break;

  case 74: // EQUALITY_EXPR: EQUALITY_EXPR EQ RELATIONAL_EXPR
#line 526 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                     {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::EQ, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1731 "yacc.tab.c"
    break;

  case 75: // EQUALITY_EXPR: EQUALITY_EXPR NE RELATIONAL_EXPR
#line 529 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                       {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::NEQ, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1739 "yacc.tab.c"
    break;

  case 76: // EQUALITY_EXPR: RELATIONAL_EXPR
#line 532 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                      {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1747 "yacc.tab.c"
    break;

  case 77: // RELATIONAL_EXPR: RELATIONAL_EXPR LT SHIFT_EXPR
#line 540 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                  {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::LT, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1755 "yacc.tab.c"
    break;

  case 78: // RELATIONAL_EXPR: RELATIONAL_EXPR LE SHIFT_EXPR
#line 543 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::LE, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1763 "yacc.tab.c"
    break;

  case 79: // RELATIONAL_EXPR: RELATIONAL_EXPR GT SHIFT_EXPR
#line 546 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::GT, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1771 "yacc.tab.c"
    break;

  case 80: // RELATIONAL_EXPR: RELATIONAL_EXPR GE SHIFT_EXPR
#line 549 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::GE, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1779 "yacc.tab.c"
    break;

  case 81: // RELATIONAL_EXPR: SHIFT_EXPR
#line 552 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1787 "yacc.tab.c"
    break;

  case 82: // SHIFT_EXPR: SHIFT_EXPR LSHIFT ADDSUB_EXPR
#line 558 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                  {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::LSHIFT, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1795 "yacc.tab.c"
    break;

  case 83: // SHIFT_EXPR: SHIFT_EXPR RSHIFT ADDSUB_EXPR
#line 561 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::RSHIFT, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1803 "yacc.tab.c"
    break;

  case 84: // SHIFT_EXPR: ADDSUB_EXPR
#line 564 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1811 "yacc.tab.c"
    break;

  case 85: // BIT_OR_EXPR: BIT_OR_EXPR BITOR BIT_XOR_EXPR
#line 570 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                   {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITOR, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1819 "yacc.tab.c"
    break;

  case 86: // BIT_OR_EXPR: BIT_XOR_EXPR
#line 573 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                   {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1827 "yacc.tab.c"
    break;

  case 87: // BIT_XOR_EXPR: BIT_XOR_EXPR BITXOR BIT_AND_EXPR
#line 579 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                     {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITXOR, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1835 "yacc.tab.c"
    break;

  case 88: // BIT_XOR_EXPR: BIT_AND_EXPR
#line 582 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                   {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1843 "yacc.tab.c"
    break;

  case 89: // BIT_AND_EXPR: BIT_AND_EXPR BITAND EQUALITY_EXPR
#line 588 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::BITAND, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1851 "yacc.tab.c"
    break;

  case 90: // BIT_AND_EXPR: EQUALITY_EXPR
#line 591 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1859 "yacc.tab.c"
    break;

  case 91: // ADDSUB_EXPR: ADDSUB_EXPR PLUS MULDIV_EXPR
#line 599 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                 {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::ADD, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1867 "yacc.tab.c"
    break;

  case 92: // ADDSUB_EXPR: ADDSUB_EXPR MINUS MULDIV_EXPR
#line 602 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::SUB, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1875 "yacc.tab.c"
    break;

  case 93: // ADDSUB_EXPR: MULDIV_EXPR
#line 605 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1883 "yacc.tab.c"
    break;

  case 94: // MULDIV_EXPR: MULDIV_EXPR STAR UNARY_EXPR
#line 613 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::MUL, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1891 "yacc.tab.c"
    break;

  case 95: // MULDIV_EXPR: MULDIV_EXPR SLASH UNARY_EXPR
#line 616 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                   {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::DIV, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1899 "yacc.tab.c"
    break;

  case 96: // MULDIV_EXPR: MULDIV_EXPR MOD UNARY_EXPR
#line 619 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                 {
        yylhs.value.as < FE::AST::ExprNode* > () = new BinaryExpr(Operator::MOD, yystack_[2].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1907 "yacc.tab.c"
    break;

  case 97: // MULDIV_EXPR: UNARY_EXPR
#line 622 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1915 "yacc.tab.c"
    break;

  case 98: // UNARY_EXPR: POSTFIX_EXPR
#line 628 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1923 "yacc.tab.c"
    break;

  case 99: // UNARY_EXPR: UNARY_OP UNARY_EXPR
#line 631 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                          {
        yylhs.value.as < FE::AST::ExprNode* > () = new UnaryExpr(yystack_[1].value.as < FE::AST::Operator > (), yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[0].value.as < FE::AST::ExprNode* > ()->line_num, yystack_[0].value.as < FE::AST::ExprNode* > ()->col_num);
    }
#line 1931 "yacc.tab.c"
    break;

  case 100: // UNARY_EXPR: PLUSPLUS LEFT_VAL_EXPR
#line 634 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                             {
        yylhs.value.as < FE::AST::ExprNode* > () = new UnaryExpr(Operator::PRE_INC, yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1939 "yacc.tab.c"
    break;

  case 101: // UNARY_EXPR: MINUSMINUS LEFT_VAL_EXPR
#line 637 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                               {
        yylhs.value.as < FE::AST::ExprNode* > () = new UnaryExpr(Operator::PRE_DEC, yystack_[0].value.as < FE::AST::ExprNode* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 1947 "yacc.tab.c"
    break;

  case 102: // POSTFIX_EXPR: BASIC_EXPR
#line 643 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
               {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1955 "yacc.tab.c"
    break;

  case 103: // POSTFIX_EXPR: POSTFIX_EXPR PLUSPLUS
#line 646 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                            {
        yylhs.value.as < FE::AST::ExprNode* > () = new UnaryExpr(Operator::POST_INC, yystack_[1].value.as < FE::AST::ExprNode* > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 1963 "yacc.tab.c"
    break;

  case 104: // POSTFIX_EXPR: POSTFIX_EXPR MINUSMINUS
#line 649 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                              {
        yylhs.value.as < FE::AST::ExprNode* > () = new UnaryExpr(Operator::POST_DEC, yystack_[1].value.as < FE::AST::ExprNode* > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 1971 "yacc.tab.c"
    break;

  case 105: // BASIC_EXPR: LITERAL_EXPR
#line 655 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                 {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1979 "yacc.tab.c"
    break;

  case 106: // BASIC_EXPR: LEFT_VAL_EXPR
#line 658 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                    {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 1987 "yacc.tab.c"
    break;

  case 107: // BASIC_EXPR: LPAREN EXPR RPAREN
#line 661 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                         {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[1].value.as < FE::AST::ExprNode* > ();
    }
#line 1995 "yacc.tab.c"
    break;

  case 108: // BASIC_EXPR: FUNC_CALL_EXPR
#line 664 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                     {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[0].value.as < FE::AST::ExprNode* > ();
    }
#line 2003 "yacc.tab.c"
    break;

  case 109: // FUNC_CALL_EXPR: IDENT LPAREN RPAREN
#line 670 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                        {
        std::string funcName = yystack_[2].value.as < std::string > ();
        if (funcName != "starttime" && funcName != "stoptime")
        {
            Entry* entry = Entry::getEntry(funcName);
            yylhs.value.as < FE::AST::ExprNode* > () = new CallExpr(entry, nullptr, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
        }
        else
        {    
            funcName = "_sysy_" + funcName;
            std::vector<ExprNode*>* args = new std::vector<ExprNode*>();
            args->emplace_back(new LiteralExpr(static_cast<int>(yystack_[2].location.begin.line), yystack_[2].location.begin.line, yystack_[2].location.begin.column));
            yylhs.value.as < FE::AST::ExprNode* > () = new CallExpr(Entry::getEntry(funcName), args, yystack_[2].location.begin.line, yystack_[2].location.begin.column);
        }
    }
#line 2023 "yacc.tab.c"
    break;

  case 110: // FUNC_CALL_EXPR: IDENT LPAREN EXPR_LIST RPAREN
#line 685 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                    {
        Entry* entry = Entry::getEntry(yystack_[3].value.as < std::string > ());
        yylhs.value.as < FE::AST::ExprNode* > () = new CallExpr(entry, yystack_[1].value.as < std::vector<FE::AST::ExprNode*>* > (), yystack_[3].location.begin.line, yystack_[3].location.begin.column);
    }
#line 2032 "yacc.tab.c"
    break;

  case 111: // ARRAY_DIMENSION_EXPR: LBRACKET NOCOMMA_EXPR RBRACKET
#line 692 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                   {
        yylhs.value.as < FE::AST::ExprNode* > () = yystack_[1].value.as < FE::AST::ExprNode* > ();
    }
#line 2040 "yacc.tab.c"
    break;

  case 112: // ARRAY_DIMENSION_EXPR_LIST: ARRAY_DIMENSION_EXPR
#line 700 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                         {
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > () = new std::vector<ExprNode*>();
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
    }
#line 2049 "yacc.tab.c"
    break;

  case 113: // ARRAY_DIMENSION_EXPR_LIST: ARRAY_DIMENSION_EXPR_LIST ARRAY_DIMENSION_EXPR
#line 704 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                                     {
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > () = yystack_[1].value.as < std::vector<FE::AST::ExprNode*>* > ();
        yylhs.value.as < std::vector<FE::AST::ExprNode*>* > ()->push_back(yystack_[0].value.as < FE::AST::ExprNode* > ());
    }
#line 2058 "yacc.tab.c"
    break;

  case 114: // LEFT_VAL_EXPR: IDENT
#line 711 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
          {
        Entry* entry = Entry::getEntry(yystack_[0].value.as < std::string > ());
        yylhs.value.as < FE::AST::ExprNode* > () = new LeftValExpr(entry, nullptr, yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 2067 "yacc.tab.c"
    break;

  case 115: // LEFT_VAL_EXPR: IDENT ARRAY_DIMENSION_EXPR_LIST
#line 715 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                                      {
        Entry* entry = Entry::getEntry(yystack_[1].value.as < std::string > ());
        yylhs.value.as < FE::AST::ExprNode* > () = new LeftValExpr(entry, yystack_[0].value.as < std::vector<FE::AST::ExprNode*>* > (), yystack_[1].location.begin.line, yystack_[1].location.begin.column);
    }
#line 2076 "yacc.tab.c"
    break;

  case 116: // LITERAL_EXPR: INT_CONST
#line 722 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
              {
        yylhs.value.as < FE::AST::ExprNode* > () = new LiteralExpr(yystack_[0].value.as < int > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 2084 "yacc.tab.c"
    break;

  case 117: // LITERAL_EXPR: LL_CONST
#line 726 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
               {
        yylhs.value.as < FE::AST::ExprNode* > () = new LiteralExpr(yystack_[0].value.as < long long > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 2092 "yacc.tab.c"
    break;

  case 118: // LITERAL_EXPR: FLOAT_CONST
#line 729 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                  {
        yylhs.value.as < FE::AST::ExprNode* > () = new LiteralExpr(yystack_[0].value.as < float > (), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 2100 "yacc.tab.c"
    break;

  case 119: // LITERAL_EXPR: DOUBLE_CONST
#line 732 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
                   {
        yylhs.value.as < FE::AST::ExprNode* > () = new LiteralExpr(static_cast<float>(yystack_[0].value.as < double > ()), yystack_[0].location.begin.line, yystack_[0].location.begin.column);
    }
#line 2108 "yacc.tab.c"
    break;

  case 120: // TYPE: INT
#line 739 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
        {
        yylhs.value.as < FE::AST::Type* > () = intType;
    }
#line 2116 "yacc.tab.c"
    break;

  case 121: // TYPE: FLOAT
#line 742 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
            {
        yylhs.value.as < FE::AST::Type* > () = floatType;
    }
#line 2124 "yacc.tab.c"
    break;

  case 122: // TYPE: VOID
#line 745 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
           {
        yylhs.value.as < FE::AST::Type* > () = voidType;
    }
#line 2132 "yacc.tab.c"
    break;

  case 123: // TYPE: DOUBLE
#line 748 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
             {
        yylhs.value.as < FE::AST::Type* > () = floatType;
    }
#line 2140 "yacc.tab.c"
    break;

  case 124: // UNARY_OP: PLUS
#line 755 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
         {
        yylhs.value.as < FE::AST::Operator > () = Operator::ADD;
    }
#line 2148 "yacc.tab.c"
    break;

  case 125: // UNARY_OP: MINUS
#line 758 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
            {
        yylhs.value.as < FE::AST::Operator > () = Operator::SUB;
    }
#line 2156 "yacc.tab.c"
    break;

  case 126: // UNARY_OP: NOT
#line 761 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
          {
        yylhs.value.as < FE::AST::Operator > () = Operator::NOT;
    }
#line 2164 "yacc.tab.c"
    break;

  case 127: // UNARY_OP: BITNOT
#line 764 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
             {
        yylhs.value.as < FE::AST::Operator > () = Operator::BITNOT;
    }
#line 2172 "yacc.tab.c"
    break;


#line 2176 "yacc.tab.c"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
   YaccParser ::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
   YaccParser ::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

  std::string
   YaccParser ::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  //  YaccParser ::context.
   YaccParser ::context::context (const  YaccParser & yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
   YaccParser ::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    const int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        const int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        const int yychecklim = yylast_ - yyn + 1;
        const int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }






  int
   YaccParser ::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
   YaccParser ::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const short  YaccParser ::yypact_ninf_ = -185;

  const signed char  YaccParser ::yytable_ninf_ = -1;

  const short
   YaccParser ::yypact_[] =
  {
     355,  -185,  -185,  -185,  -185,    -2,   -14,     4,     7,    36,
      39,    38,     3,  -185,   477,   191,  -185,  -185,  -185,  -185,
    -185,  -185,    43,    43,  -185,  -185,     8,   355,  -185,  -185,
    -185,    78,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,
    -185,    -9,  -185,   -47,  -185,    56,    18,   -17,   -20,    64,
      74,    87,    30,    12,  -185,    35,  -185,  -185,   501,  -185,
     143,   477,   400,   477,  -185,   128,   477,   100,   477,  -185,
    -185,  -185,    85,    43,    -4,  -185,   232,   128,  -185,  -185,
    -185,  -185,  -185,  -185,  -185,   477,   477,   477,   477,   477,
     477,   477,   477,   477,   477,   477,   477,   477,   477,   477,
     477,   477,   477,   477,   477,  -185,  -185,   477,   477,   477,
     477,   477,   477,   477,   477,   477,   477,   477,     1,  -185,
     139,   129,  -185,  -185,  -185,    76,  -185,   142,  -185,    94,
     149,   122,    43,    95,  -185,   139,  -185,  -185,  -185,    56,
      -5,    64,   -17,   -17,   -20,   -20,   -20,   -20,    30,    30,
      74,    87,    18,    12,    12,  -185,  -185,  -185,  -185,  -185,
    -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,     3,
      43,   438,   477,  -185,  -185,   355,   477,   477,   355,   477,
    -185,   104,   164,  -185,   393,  -185,  -185,  -185,   165,   124,
     134,  -185,  -185,     3,   147,   151,  -185,  -185,    -6,   355,
     477,   477,  -185,   273,  -185,   465,   128,   438,  -185,  -185,
     113,   118,  -185,   314,   128,  -185,   355,   355,  -185,   128,
    -185,  -185
  };

  const signed char
   YaccParser ::yydefact_[] =
  {
       0,   116,   117,   118,   119,   114,     0,     0,     0,     0,
       0,     0,     0,    16,     0,     0,   120,   121,   122,   123,
     124,   125,     0,     0,   126,   127,     0,     2,     4,    11,
       6,     0,     7,    15,     8,     9,    10,    12,    14,    13,
      67,     0,    65,    70,    51,    69,    90,    76,    81,    73,
      86,    88,    84,    93,    97,    98,   102,   108,   106,   105,
       0,     0,     0,     0,   112,   115,     0,     0,     0,    17,
      31,    32,     0,     0,     0,    22,     0,   114,   100,   101,
       1,     3,     5,    21,    18,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   103,   104,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   114,    44,
      19,    42,    99,   106,   109,     0,    63,     0,   113,     0,
       0,     0,     0,     0,    33,    20,   107,    23,    66,    68,
       0,    72,    74,    75,    77,    78,    79,    80,    82,    83,
      85,    87,    89,    91,    92,    94,    95,    96,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    39,
       0,     0,     0,   110,   111,     0,     0,     0,     0,     0,
      40,     0,     0,    45,     0,    43,    46,    64,    29,     0,
       0,    34,    71,     0,     0,    35,    47,    49,     0,     0,
       0,     0,    41,     0,    26,     0,    37,     0,    48,    30,
       0,     0,    24,     0,    36,    50,     0,     0,    25,    38,
      27,    28
  };

  const short
   YaccParser ::yypgoto_[] =
  {
    -185,  -185,   -10,   -25,  -185,  -185,   111,  -185,  -185,  -185,
    -185,  -185,  -185,  -185,  -185,  -185,   -13,  -185,     9,   108,
    -175,  -185,   446,  -185,   -11,   -56,  -185,     5,    96,    84,
      33,    19,    98,    90,   101,    63,    67,   -35,  -185,  -185,
    -185,   -64,  -184,    -7,  -185,    -8,  -185
  };

  const unsigned char
   YaccParser ::yydefgoto_[] =
  {
       0,    26,    27,    28,    29,    30,    31,    32,    33,   204,
      34,    35,    36,    37,    38,    39,   180,   181,   119,   120,
     185,   198,    40,   125,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    64,    65,    58,    59,    60,    61
  };

  const unsigned char
   YaccParser ::yytable_[] =
  {
      72,   128,    82,    74,    73,    76,   126,   127,    80,   197,
      66,   206,    86,    84,    85,    78,    79,   207,    85,    85,
      87,   136,    62,   208,    63,   169,   122,    63,    67,   138,
     219,    68,   215,    16,    17,    18,    19,    91,    92,    93,
      94,     1,     2,     3,     4,    95,    96,     5,   102,   103,
     104,    82,    77,   121,   123,   129,   131,   133,    69,   132,
      71,    70,    14,   179,   100,   101,   121,   155,   156,   157,
      89,    90,    20,    21,   105,   106,   140,    22,    23,   123,
      81,   123,   123,   123,   123,   123,   123,   123,   123,   123,
     123,   123,   123,   123,   123,   123,   123,   123,    24,   172,
      83,   173,    25,     1,     2,     3,     4,   134,    85,     5,
     144,   145,   146,   147,    88,   186,   187,    85,    85,   175,
     178,    12,   142,   143,    14,   121,    97,   193,   186,   194,
      16,    17,    18,    19,    20,    21,    85,    98,   216,    22,
      23,    85,   128,   217,   177,    85,   200,    85,    99,   127,
     188,   186,   118,   191,    63,   128,   201,    85,   148,   149,
      24,   182,   170,   121,    25,   189,   190,   153,   154,   174,
     171,   176,   123,   195,   209,   203,   199,   205,   130,   183,
     202,   135,   139,   152,   192,   182,   141,   150,    82,   210,
     211,   220,   221,   213,     1,     2,     3,     4,     0,   151,
       5,     6,     0,     7,     8,     9,    10,     0,     0,     0,
       0,    11,    12,    13,     0,    14,     0,     0,     0,    15,
      75,    16,    17,    18,    19,    20,    21,     0,     0,     0,
      22,    23,     0,     0,     0,     1,     2,     3,     4,     0,
       0,     5,     6,     0,     7,     8,     9,    10,     0,     0,
       0,    24,    11,    12,    13,    25,    14,     0,     0,     0,
      15,   137,    16,    17,    18,    19,    20,    21,     0,     0,
       0,    22,    23,     0,     0,     0,     1,     2,     3,     4,
       0,     0,     5,     6,     0,     7,     8,     9,    10,     0,
       0,     0,    24,    11,    12,    13,    25,    14,     0,     0,
       0,    15,   212,    16,    17,    18,    19,    20,    21,     0,
       0,     0,    22,    23,     0,     0,     0,     1,     2,     3,
       4,     0,     0,     5,     6,     0,     7,     8,     9,    10,
       0,     0,     0,    24,    11,    12,    13,    25,    14,     0,
       0,     0,    15,   218,    16,    17,    18,    19,    20,    21,
       0,     0,     0,    22,    23,     0,     0,     0,     1,     2,
       3,     4,     0,     0,     5,     6,     0,     7,     8,     9,
      10,     0,     0,     0,    24,    11,    12,    13,    25,    14,
       0,     0,     0,    15,     0,    16,    17,    18,    19,    20,
      21,     0,     0,     0,    22,    23,     1,     2,     3,     4,
       0,     0,     5,     1,     2,     3,     4,     0,     0,     5,
       0,     0,     0,     0,     0,    24,     0,    14,     0,    25,
       0,   184,   196,     0,    14,   124,     0,    20,    21,     0,
       0,     0,    22,    23,    20,    21,     0,     0,     0,    22,
      23,     1,     2,     3,     4,     0,     0,     5,     0,     0,
       0,     0,     0,    24,     0,     0,     0,    25,     0,     0,
      24,     0,    14,     0,    25,     0,   184,     0,     1,     2,
       3,     4,    20,    21,     5,     0,     0,    22,    23,     0,
       1,     2,     3,     4,     0,     0,     5,     0,     0,    14,
       0,     0,   214,     0,     0,     0,     0,     0,    24,    20,
      21,    14,    25,     0,    22,    23,     0,     0,     0,     0,
       0,    20,    21,     0,     0,     0,    22,    23,     0,     0,
       0,     0,     0,     0,     0,    24,     0,     0,     0,    25,
       0,     0,     0,     0,     0,     0,     0,    24,     0,     0,
       0,    25,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168
  };

  const short
   YaccParser ::yycheck_[] =
  {
      11,    65,    27,    14,    12,    15,    62,    63,     0,   184,
      24,   195,    59,    22,    23,    22,    23,    23,    23,    23,
      67,    25,    24,    29,    26,    24,    61,    26,    24,    85,
     214,    24,   207,    30,    31,    32,    33,    54,    55,    56,
      57,     3,     4,     5,     6,    65,    66,     9,    36,    37,
      38,    76,     9,    60,    61,    66,    67,    68,    22,    67,
      22,    22,    24,    68,    34,    35,    73,   102,   103,   104,
      52,    53,    34,    35,    39,    40,    87,    39,    40,    86,
      72,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,   103,   104,    60,    23,
      22,    25,    64,     3,     4,     5,     6,    22,    23,     9,
      91,    92,    93,    94,    58,   171,   172,    23,    23,    25,
      25,    21,    89,    90,    24,   132,    62,    23,   184,    25,
      30,    31,    32,    33,    34,    35,    23,    63,    25,    39,
      40,    23,   206,    25,    22,    23,    22,    23,    61,   205,
     175,   207,     9,   178,    26,   219,    22,    23,    95,    96,
      60,   169,    23,   170,    64,   176,   177,   100,   101,    27,
      41,    22,   179,     9,   199,    28,    11,    26,    67,   170,
     193,    73,    86,    99,   179,   193,    88,    97,   213,   200,
     201,   216,   217,   203,     3,     4,     5,     6,    -1,    98,
       9,    10,    -1,    12,    13,    14,    15,    -1,    -1,    -1,
      -1,    20,    21,    22,    -1,    24,    -1,    -1,    -1,    28,
      29,    30,    31,    32,    33,    34,    35,    -1,    -1,    -1,
      39,    40,    -1,    -1,    -1,     3,     4,     5,     6,    -1,
      -1,     9,    10,    -1,    12,    13,    14,    15,    -1,    -1,
      -1,    60,    20,    21,    22,    64,    24,    -1,    -1,    -1,
      28,    29,    30,    31,    32,    33,    34,    35,    -1,    -1,
      -1,    39,    40,    -1,    -1,    -1,     3,     4,     5,     6,
      -1,    -1,     9,    10,    -1,    12,    13,    14,    15,    -1,
      -1,    -1,    60,    20,    21,    22,    64,    24,    -1,    -1,
      -1,    28,    29,    30,    31,    32,    33,    34,    35,    -1,
      -1,    -1,    39,    40,    -1,    -1,    -1,     3,     4,     5,
       6,    -1,    -1,     9,    10,    -1,    12,    13,    14,    15,
      -1,    -1,    -1,    60,    20,    21,    22,    64,    24,    -1,
      -1,    -1,    28,    29,    30,    31,    32,    33,    34,    35,
      -1,    -1,    -1,    39,    40,    -1,    -1,    -1,     3,     4,
       5,     6,    -1,    -1,     9,    10,    -1,    12,    13,    14,
      15,    -1,    -1,    -1,    60,    20,    21,    22,    64,    24,
      -1,    -1,    -1,    28,    -1,    30,    31,    32,    33,    34,
      35,    -1,    -1,    -1,    39,    40,     3,     4,     5,     6,
      -1,    -1,     9,     3,     4,     5,     6,    -1,    -1,     9,
      -1,    -1,    -1,    -1,    -1,    60,    -1,    24,    -1,    64,
      -1,    28,    29,    -1,    24,    25,    -1,    34,    35,    -1,
      -1,    -1,    39,    40,    34,    35,    -1,    -1,    -1,    39,
      40,     3,     4,     5,     6,    -1,    -1,     9,    -1,    -1,
      -1,    -1,    -1,    60,    -1,    -1,    -1,    64,    -1,    -1,
      60,    -1,    24,    -1,    64,    -1,    28,    -1,     3,     4,
       5,     6,    34,    35,     9,    -1,    -1,    39,    40,    -1,
       3,     4,     5,     6,    -1,    -1,     9,    -1,    -1,    24,
      -1,    -1,    27,    -1,    -1,    -1,    -1,    -1,    60,    34,
      35,    24,    64,    -1,    39,    40,    -1,    -1,    -1,    -1,
      -1,    34,    35,    -1,    -1,    -1,    39,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,    -1,    64,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,    -1,    -1,
      -1,    64,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,   107,   108,   109,   110,   111,   112,   113,
     114,   115,   116,   117
  };

  const signed char
   YaccParser ::yystos_[] =
  {
       0,     3,     4,     5,     6,     9,    10,    12,    13,    14,
      15,    20,    21,    22,    24,    28,    30,    31,    32,    33,
      34,    35,    39,    40,    60,    64,    75,    76,    77,    78,
      79,    80,    81,    82,    84,    85,    86,    87,    88,    89,
      96,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   117,   118,
     119,   120,    24,    26,   115,   116,    24,    24,    24,    22,
      22,    22,    98,   119,    98,    29,    76,     9,   117,   117,
       0,    72,    77,    22,    22,    23,    59,    67,    58,    52,
      53,    54,    55,    56,    57,    65,    66,    62,    63,    61,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,     9,    92,
      93,   117,   111,   117,    25,    97,    99,    99,   115,    98,
      80,    98,   119,    98,    22,    93,    25,    29,    99,   102,
      98,   106,   104,   104,   105,   105,   105,   105,   109,   109,
     107,   108,   103,   110,   110,   111,   111,   111,    96,    96,
      96,    96,    96,    96,    96,    96,    96,    96,    96,    24,
      23,    41,    23,    25,    27,    25,    22,    22,    25,    68,
      90,    91,   119,    92,    28,    94,    99,    99,    77,    98,
      98,    77,   101,    23,    25,     9,    29,    94,    95,    11,
      22,    22,    90,    28,    83,    26,   116,    23,    29,    77,
      98,    98,    29,    76,    27,    94,    25,    25,    29,   116,
      77,    77
  };

  const signed char
   YaccParser ::yyr1_[] =
  {
       0,    74,    75,    75,    76,    76,    77,    77,    77,    77,
      77,    77,    77,    77,    77,    77,    77,    78,    79,    80,
      80,    81,    82,    82,    83,    83,    84,    85,    85,    86,
      86,    87,    88,    88,    89,    90,    90,    90,    90,    91,
      91,    91,    92,    92,    93,    93,    94,    94,    94,    95,
      95,    96,    96,    96,    96,    96,    96,    96,    96,    96,
      96,    96,    96,    97,    97,    98,    98,    99,   100,   100,
     101,   101,   102,   102,   103,   103,   103,   104,   104,   104,
     104,   104,   105,   105,   105,   106,   106,   107,   107,   108,
     108,   109,   109,   109,   110,   110,   110,   110,   111,   111,
     111,   111,   112,   112,   112,   113,   113,   113,   113,   114,
     114,   115,   116,   116,   117,   117,   118,   118,   118,   118,
     119,   119,   119,   119,   120,   120,   120,   120
  };

  const signed char
   YaccParser ::yyr2_[] =
  {
       0,     2,     1,     2,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     2,     2,
       3,     2,     2,     3,     2,     3,     6,     9,     9,     5,
       7,     2,     2,     3,     5,     2,     4,     3,     5,     0,
       1,     3,     1,     3,     1,     3,     1,     2,     3,     1,
       3,     1,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     1,     3,     1,     3,     1,     3,     1,
       1,     5,     3,     1,     3,     3,     1,     3,     3,     3,
       3,     1,     3,     3,     1,     3,     1,     3,     1,     3,
       1,     3,     3,     1,     3,     3,     3,     1,     1,     2,
       2,     2,     1,     2,     2,     1,     1,     3,     1,     3,
       4,     3,     1,     2,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1
  };


#if YYDEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const  YaccParser ::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "INT_CONST",
  "LL_CONST", "FLOAT_CONST", "DOUBLE_CONST", "STR_CONST", "ERR_TOKEN",
  "IDENT", "IF", "ELSE", "FOR", "WHILE", "CONTINUE", "BREAK", "SWITCH",
  "CASE", "GOTO", "DO", "RETURN", "CONST", "SEMICOLON", "COMMA", "LPAREN",
  "RPAREN", "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "INT", "FLOAT",
  "VOID", "DOUBLE", "PLUS", "MINUS", "STAR", "SLASH", "MOD", "PLUSPLUS",
  "MINUSMINUS", "ASSIGN", "PLUSEQ", "MINUSEQ", "MULEQ", "DIVEQ", "MODEQ",
  "BITOREQ", "BITANDEQ", "BITXOREQ", "LSHIFTEQ", "RSHIFTEQ", "EQ", "NE",
  "LT", "LE", "GT", "GE", "AND", "OR", "NOT", "BITAND", "BITOR", "BITXOR",
  "BITNOT", "LSHIFT", "RSHIFT", "QUESTION", "COLON", "ARROW", "DOT",
  "ELLIPSIS", "END", "THEN", "$accept", "PROGRAM", "STMT_LIST", "STMT",
  "CONTINUE_STMT", "EXPR_STMT", "VAR_DECLARATION", "VAR_DECL_STMT",
  "BLOCK_STMT", "FUNC_BODY", "FUNC_DECL_STMT", "FOR_STMT", "IF_STMT",
  "BREAK_STMT", "RETURN_STMT", "WHILE_STMT", "PARAM_DECLARATOR",
  "PARAM_DECLARATOR_LIST", "VAR_DECLARATOR", "VAR_DECLARATOR_LIST",
  "INITIALIZER", "INITIALIZER_LIST", "ASSIGN_EXPR", "EXPR_LIST", "EXPR",
  "NOCOMMA_EXPR", "LOGICAL_OR_EXPR", "CONDITIONAL_EXPR",
  "LOGICAL_AND_EXPR", "EQUALITY_EXPR", "RELATIONAL_EXPR", "SHIFT_EXPR",
  "BIT_OR_EXPR", "BIT_XOR_EXPR", "BIT_AND_EXPR", "ADDSUB_EXPR",
  "MULDIV_EXPR", "UNARY_EXPR", "POSTFIX_EXPR", "BASIC_EXPR",
  "FUNC_CALL_EXPR", "ARRAY_DIMENSION_EXPR", "ARRAY_DIMENSION_EXPR_LIST",
  "LEFT_VAL_EXPR", "LITERAL_EXPR", "TYPE", "UNARY_OP", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
   YaccParser ::yyrline_[] =
  {
       0,   171,   171,   175,   181,   185,   192,   195,   198,   201,
     204,   207,   211,   214,   217,   220,   224,   231,   237,   243,
     246,   253,   259,   262,   268,   271,   277,   284,   288,   296,
     299,   305,   311,   314,   320,   331,   335,   342,   346,   356,
     360,   364,   372,   375,   381,   385,   394,   397,   401,   407,
     411,   419,   422,   425,   428,   431,   434,   437,   440,   443,
     446,   449,   452,   458,   462,   469,   472,   487,   495,   498,
     504,   507,   515,   518,   526,   529,   532,   540,   543,   546,
     549,   552,   558,   561,   564,   570,   573,   579,   582,   588,
     591,   599,   602,   605,   613,   616,   619,   622,   628,   631,
     634,   637,   643,   646,   649,   655,   658,   661,   664,   670,
     685,   692,   700,   704,   711,   715,   722,   726,   729,   732,
     739,   742,   745,   748,   755,   758,   761,   764
  };

  void
   YaccParser ::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
   YaccParser ::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 4 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"
} //  FE 
#line 2878 "yacc.tab.c"

#line 769 "/home/nuo/project/nkucompiler/frontend/parser/yacc.y"


void FE::YaccParser::error(const FE::location& location, const std::string& message)
{
    std::cerr << "msg: " << message << ", error happened at: " << location << std::endl;
}
