#include "lexer.h"
#include <stdexcept>
#include <sstream>

namespace one {

Lexer::Lexer(const std::string& src)
    : src_(src), pos_(0), line_(1) {}

bool Lexer::atEnd() const { return pos_ >= src_.size(); }
char Lexer::peek() const { return src_[pos_]; }
char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') line_++;
    return c;
}

void Lexer::skipBlankLines() {
    while (!atEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            advance();
        else
            break;
    }
}

// "..." 形式: 複数行も可（\n リテラルを改行として扱う）
Token Lexer::readQuotedString() {
    int startLine = line_;
    advance(); // 開き "
    std::string val;
    while (!atEnd()) {
        char c = peek();
        if (c == '"') {
            advance(); // 閉じ "
            break;
        }
        if (c == '\\') {
            advance();
            if (atEnd()) break;
            char esc = advance();
            switch (esc) {
                case 'n':  val += '\n'; break;
                case 't':  val += '\t'; break;
                case '"':  val += '"';  break;
                case '\\': val += '\\'; break;
                default:   val += '\\'; val += esc; break;
            }
        } else {
            val += advance();
        }
    }
    return Token(TOK_STRING, val, startLine);
}

// クォートなし: 行末まで読む
Token Lexer::readUnquotedLine() {
    int startLine = line_;
    std::string val;
    while (!atEnd() && peek() != '\n' && peek() != '\r') {
        val += advance();
    }
    // 末尾の空白を除去
    size_t end = val.find_last_not_of(" \t");
    if (end != std::string::npos)
        val = val.substr(0, end + 1);
    return Token(TOK_STRING, val, startLine);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipBlankLines();
        if (atEnd()) break;

        char c = peek();
        if (c == '"') {
            tokens.push_back(readQuotedString());
        } else {
            tokens.push_back(readUnquotedLine());
        }
    }
    tokens.push_back(Token(TOK_EOF, "", line_));
    return tokens;
}

} // namespace one
