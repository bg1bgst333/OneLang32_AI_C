#ifndef ONE_LEXER_H
#define ONE_LEXER_H

#include <string>
#include <vector>

namespace one {

enum TokenKind {
    TOK_STRING, // 文字列リテラル（クォートあり/なし）
    TOK_EOF
};

struct Token {
    TokenKind kind;
    std::string value; // クォートを除いた文字列
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
    void skipBlankLines();
    Token readQuotedString();
    Token readUnquotedLine();
};

} // namespace one

#endif
