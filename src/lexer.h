#ifndef ONE_LEXER_H
#define ONE_LEXER_H

#include <string>
#include <vector>

namespace one {

enum TokenKind {
    TOK_STRING,   // 文字列リテラル（クォートあり/なし）
    TOK_NUMBER,   // 数値リテラル (整数・浮動小数)
    TOK_IDENT,    // 識別子 (変数名など)
    TOK_ASSIGN,   // =
    TOK_COLON,    // :
    TOK_NEWLINE,  // 改行 (文のセパレータ)
    TOK_EOF
};

struct Token {
    TokenKind kind;
    std::string value;
    int line;
    Token(TokenKind k, const std::string& v, int l)
        : kind(k), value(v), line(l) {}
};

class Lexer {
public:
    Lexer(const std::string& src);
    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t pos_;
    int line_;

    bool atEnd() const;
    char peek() const;
    char advance();
    void skipSpaces();
    Token readQuotedString();
    Token readNumber();
    Token readIdent();
    Token readUnquotedLine();
    Token nextToken();
};

} // namespace one

#endif
