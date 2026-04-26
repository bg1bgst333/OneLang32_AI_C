#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace one {

Lexer::Lexer(const std::string& src)
    : src_(src), pos_(0), line_(1), parenDepth_(0) {}

bool Lexer::atEnd() const { return pos_ >= src_.size(); }
char Lexer::peek() const { return src_[pos_]; }
char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') line_++;
    return c;
}

// スペース・タブのみスキップ（改行はスキップしない）
// # から行末まではコメントとして読み飛ばす
void Lexer::skipSpaces() {
    while (!atEnd() && (peek() == ' ' || peek() == '\t'))
        advance();
    if (!atEnd() && peek() == '#') {
        while (!atEnd() && peek() != '\n' && peek() != '\r')
            advance();
    }
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

    // セミコロン・カンマ
    if (c == ';') { advance(); return Token(TOK_SEMICOLON, ";", line_); }
    if (c == ',') { advance(); return Token(TOK_COMMA,     ",", line_); }

    // 代入演算子 / コロン / 四則演算子（複合代入を先にチェック）
    if (c == '=') { advance(); return Token(TOK_ASSIGN, "=", line_); }
    if (c == ':') { advance(); return Token(TOK_COLON,  ":", line_); }
    if (c == '+') {
        advance();
        if (!atEnd() && peek() == '=') { advance(); return Token(TOK_PLUS_ASSIGN,  "+=", line_); }
        return Token(TOK_PLUS, "+", line_);
    }
    if (c == '*') {
        advance();
        if (!atEnd() && peek() == '=') { advance(); return Token(TOK_STAR_ASSIGN,  "*=", line_); }
        return Token(TOK_STAR, "*", line_);
    }
    if (c == '/') {
        advance();
        if (!atEnd() && peek() == '=') { advance(); return Token(TOK_SLASH_ASSIGN, "/=", line_); }
        return Token(TOK_SLASH, "/", line_);
    }

    // -> / -= / 負の数値 / マイナス
    if (c == '-') {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '>') {
            advance(); advance();
            return Token(TOK_ARROW, "->", line_);
        }
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            advance(); advance();
            return Token(TOK_MINUS_ASSIGN, "-=", line_);
        }
        if (pos_ + 1 < src_.size() && std::isdigit((unsigned char)src_[pos_ + 1]))
            return readNumber();
        advance();
        return Token(TOK_MINUS, "-", line_);
    }

    // > >=
    if (c == '>') {
        advance();
        if (!atEnd() && peek() == '=') { advance(); return Token(TOK_GTE, ">=", line_); }
        return Token(TOK_GT, ">", line_);
    }

    // < <= <-
    if (c == '<') {
        advance();
        if (!atEnd() && peek() == '=') { advance(); return Token(TOK_LTE,    "<=", line_); }
        if (!atEnd() && peek() == '-') { advance(); return Token(TOK_LARROW,  "<-", line_); }
        return Token(TOK_LT, "<", line_);
    }

    // ブロック括弧・丸括弧
    if (c == '{') { advance(); return Token(TOK_LBRACE,  "{", line_); }
    if (c == '}') { advance(); return Token(TOK_RBRACE,  "}", line_); }
    if (c == '(') { advance(); parenDepth_++; return Token(TOK_LPAREN,  "(", line_); }
    if (c == ')') { advance(); if (parenDepth_ > 0) parenDepth_--; return Token(TOK_RPAREN,  ")", line_); }

    // 論理演算子
    if (c == '&') { advance(); return Token(TOK_AND, "&", line_); }
    if (c == '|') { advance(); return Token(TOK_OR,  "|", line_); }

    // != / ! (論理NOT)
    if (c == '!') {
        if (pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
            advance(); advance();
            return Token(TOK_NEQ, "!=", line_);
        }
        advance();
        return Token(TOK_NOT, "!", line_);
    }

    // 正の数値（後ろが演算子/空白/改行でない場合はクォートなし文字列として扱う）
    if (std::isdigit((unsigned char)c)) {
        size_t saved = pos_;
        Token numTok = readNumber();
        if (!atEnd()) {
            char nc2 = peek();
            bool afterOk = nc2 == ' ' || nc2 == '\t' || nc2 == '\n' || nc2 == '\r' ||
                           nc2 == '+' || nc2 == '-' || nc2 == '*' || nc2 == '/' ||
                           nc2 == '>' || nc2 == '<' || nc2 == '!' || nc2 == '=' ||
                           nc2 == ')' || nc2 == '&' || nc2 == '|' || nc2 == ';' ||
                           nc2 == '#' || (nc2 == ',' && parenDepth_ > 0);
            if (!afterOk) {
                pos_ = saved;
                return readUnquotedLine();
            }
        }
        return numTok;
    }

    // 識別子か判定:
    // 英字/アンダースコアで始まり、後ろが = か 行末なら識別子、それ以外はクォートなし文字列
    if (std::isalpha((unsigned char)c) || c == '_') {
        size_t saved = pos_;
        Token ident = readIdent();
        // キーワードチェック
        if (ident.value == "o") {
            return Token(TOK_LOOP, "o", ident.line);
        }
        // 識別子の後ろ（スペース除く）を確認
        size_t tmpPos = pos_;
        while (tmpPos < src_.size() &&
               (src_[tmpPos] == ' ' || src_[tmpPos] == '\t'))
            tmpPos++;
        char nc = (tmpPos < src_.size()) ? src_[tmpPos] : '\0';
        bool nextIsOk = tmpPos >= src_.size() ||
                        nc == '=' || nc == ':' ||
                        nc == '+' || nc == '-' || nc == '*' || nc == '/' ||
                        nc == '>' || nc == '<' || nc == '!' ||
                        nc == '(' || nc == ')' || nc == '&' || nc == '|' || nc == ';' ||
                        nc == '\n' || nc == '\r' || nc == '#' ||
                        (nc == ',' && parenDepth_ > 0); // カンマは括弧内のみ区切り
        if (nextIsOk) return ident;
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
