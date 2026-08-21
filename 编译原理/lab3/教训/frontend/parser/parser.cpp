#include <frontend/parser/parser.h>
#include <debug.h>

namespace FE
{
    using type = YaccParser::symbol_type;
    using kind = YaccParser::symbol_kind;

    void Parser::reportError(const location& loc, const std::string& message) { _parser.error(loc, message); }

    std::vector<Token> Parser::parseTokens_impl()
    {
        std::vector<Token> tokens;
        while (true)
        {
            type token = _scanner.nextToken();
            if (token.kind() == kind::S_END) break;

            Token result;
            result.token_name    = token.name();
            result.line_number   = token.location.begin.line;
            result.column_number = token.location.begin.column - 1;
            result.lexeme        = _scanner.YYText();

            switch (token.kind())
            {
                case kind::S_INT_CONST:
                    result.ival = token.value.as<int>();
                    result.type = Token::TokenType::T_INT;
                    break;
                case kind::S_LL_CONST:        /* 长整型常量 */
                    result.lval = token.value.as<long long>();
                    result.type = Token::TokenType::T_LL;
                    break;
                case kind::S_FLOAT_CONST:        /* 浮点常量 */
                    result.fval = token.value.as<float>();
                    result.type = Token::TokenType::T_FLOAT;
                    break;
                case kind::S_DOUBLE_CONST:        /* 双精度常量 */
                    result.dval = token.value.as<double>();
                    result.type = Token::TokenType::T_DOUBLE;
                    break;
                case kind::S_IDENT:        /* 标识符 */
                    result.sval = token.value.as<std::string>();
                    result.type = Token::TokenType::T_IDENTIFIER;
                    break;
                case kind::S_STR_CONST:        /* 字符串常量 */
                    result.sval = token.value.as<std::string>();
                    result.type = Token::TokenType::T_STRING;
                    break;
                case kind::S_ERR_TOKEN:        /* 错误token */
                    result.sval = token.value.as<std::string>();
                    result.type = Token::TokenType::T_NONE;
                    break;
                case kind::S_IF:        /* 关键字 */
                case kind::S_ELSE:
                case kind::S_FOR:
                case kind::S_WHILE:
                case kind::S_RETURN:
                case kind::S_CONST:
                case kind::S_CONTINUE:      
                case kind::S_BREAK:
                case kind::S_SWITCH:
                case kind::S_CASE:
                case kind::S_GOTO:
                case kind::S_DO:
                case kind::S_INT:        
                case kind::S_FLOAT:
                case kind::S_VOID:
                case kind::S_DOUBLE:
                    // 关键字token没有值，我们直接使用token名称
                    result.type = Token::TokenType::T_KEYWORD;
                    break;
                case kind::S_PLUS:        /* 运算符 */
                case kind::S_MINUS:
                case kind::S_STAR:
                case kind::S_SLASH:
                case kind::S_MOD:
                case kind::S_PLUSPLUS:
                case kind::S_MINUSMINUS:
                case kind::S_ASSIGN:
                case kind::S_PLUSEQ:
                case kind::S_MINUSEQ:
                case kind::S_MULEQ:
                case kind::S_DIVEQ:
                case kind::S_MODEQ:
                case kind::S_EQ:
                case kind::S_NE:
                case kind::S_LT:
                case kind::S_LE:
                case kind::S_GT:
                case kind::S_GE:
                case kind::S_AND:
                case kind::S_OR:
                case kind::S_NOT:
                case kind::S_BITAND:
                case kind::S_BITOR:
                case kind::S_BITXOR:
                case kind::S_BITNOT:
                case kind::S_LSHIFT:
                case kind::S_RSHIFT:
                case kind::S_QUESTION:
                case kind::S_COLON:
                case kind::S_ARROW:
                case kind::S_DOT:
                case kind::S_ELLIPSIS:
                    result.type = Token::TokenType::T_OPERATOR;
                    break;
                case kind::S_SEMICOLON:        /* 分界符 */
                case kind::S_COMMA:
                case kind::S_LPAREN:
                case kind::S_RPAREN:
                case kind::S_LBRACKET:
                case kind::S_RBRACKET:
                case kind::S_LBRACE:
                case kind::S_RBRACE:
                    result.type = Token::TokenType::T_DELIMITER;
                    break;
                default: result.type = Token::TokenType::T_NONE; break;
            }

            tokens.push_back(result);
        }

        return tokens;
    }

    AST::Root* Parser::parseAST_impl()
    {
        _parser.parse();
        return ast;
    }
}  // namespace FE
