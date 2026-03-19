#include "lexer.h"
#include <cctype>
#include <stdexcept>

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

// スペース・タブのみスキップ（改行はスキップしない）
void Lexer::skipSpaces() {
    while (!atEnd() && (peek() == ' ' || peek() == '\t'))
        advance();
}

// "..." 形式（\nエスケープ等対応）
Token Lexer::readQuotedString() {
    int startLine = line_;
    advance(); // 開き "
    std::string val;
    while (!atEnd() && peek() != '"') {
        if (peek() == '\\') {
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
    if (!atEnd()) advance(); // 閉じ "
    return Token(TOK_STRING, val, startLine);
}

// 数値リテラル
Token Lexer::readNumber() {
    int startLine = line_;
    std::string val;
    if (!atEnd() && peek() == '-') val += advance();
    while (!atEnd() && std::isdigit((unsigned char)peek()))
        val += advance();
    if (!atEnd() && peek() == '.') {
        val += advance();
        while (!atEnd() && std::isdigit((unsigned char)peek()))
            val += advance();
    }
    return Token(TOK_NUMBER, val, startLine);
}

// 識別子
Token Lexer::readIdent() {
    int startLine = line_;
    std::string val;
    while (!atEnd() && (std::isalnum((unsigned char)peek()) || peek() == '_'))
        val += advance();
    return Token(TOK_IDENT, val, startLine);
}

// クォートなし・行末まで（スペース込みの文字列）
Token Lexer::readUnquotedLine() {
    int startLine = line_;
    std::string val;
    while (!atEnd() && peek() != '\n' && peek() != '\r')
        val += advance();
    // 末尾の空白を除去
    size_t end = val.find_last_not_of(" \t");
    if (end != std::string::npos)
        val = val.substr(0, end + 1);
    return Token(TOK_STRING, val, startLine);
}

Token Lexer::nextToken() {
    skipSpaces();
    if (atEnd()) return Token(TOK_EOF, "", line_);

    char c = peek();

    // 改行
    if (c == '\r' || c == '\n') {
        int l = line_;
        if (c == '\r') advance();
        if (!atEnd() && peek() == '\n') advance();
        return Token(TOK_NEWLINE, "", l);
    }

    // クォート付き文字列
    if (c == '"') return readQuotedString();

    // 代入演算子
    if (c == '=') { advance(); return Token(TOK_ASSIGN, "=", line_); }

    // 負の数値
    if (c == '-' && pos_ + 1 < src_.size() &&
        std::isdigit((unsigned char)src_[pos_ + 1]))
        return readNumber();

    // 正の数値
    if (std::isdigit((unsigned char)c)) return readNumber();

    // 識別子か判定:
    // 英字/アンダースコアで始まり、後ろが = か 行末なら識別子、それ以外はクォートなし文字列
    if (std::isalpha((unsigned char)c) || c == '_') {
        size_t saved = pos_;
        Token ident = readIdent();
        // 識別子の後ろ（スペース除く）を確認
        size_t tmpPos = pos_;
        while (tmpPos < src_.size() &&
               (src_[tmpPos] == ' ' || src_[tmpPos] == '\t'))
            tmpPos++;
        bool nextIsAssignOrEnd = tmpPos >= src_.size() ||
                                  src_[tmpPos] == '='  ||
                                  src_[tmpPos] == '\n' ||
                                  src_[tmpPos] == '\r';
        if (nextIsAssignOrEnd) return ident;
        // そうでなければ行全体をクォートなし文字列として読み直す
        pos_ = saved;
        return readUnquotedLine();
    }

    // それ以外はクォートなし文字列（行末まで）
    return readUnquotedLine();
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    // 先頭の空行をスキップ
    while (!atEnd() && (peek() == '\r' || peek() == '\n'))
        advance();

    while (true) {
        Token tok = nextToken();
        if (tok.kind == TOK_EOF) break;
        // 連続する改行は1つにまとめる
        if (tok.kind == TOK_NEWLINE) {
            if (!tokens.empty() && tokens.back().kind == TOK_NEWLINE)
                continue;
        }
        tokens.push_back(tok);
    }
    tokens.push_back(Token(TOK_EOF, "", line_));
    return tokens;
}

} // namespace one
