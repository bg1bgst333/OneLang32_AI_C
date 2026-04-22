#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace one {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}

const Token& Parser::peek() const { return tokens_[pos_]; }

const Token& Parser::peekAt(size_t offset) const {
    size_t idx = pos_ + offset;
    if (idx >= tokens_.size()) return tokens_.back(); // EOF
    return tokens_[idx];
}

const Token& Parser::advance() { return tokens_[pos_++]; }
bool Parser::atEnd() const { return tokens_[pos_].kind == TOK_EOF; }

void Parser::skipNewlines() {
    while (!atEnd() && tokens_[pos_].kind == TOK_NEWLINE)
        pos_++;
}

bool Parser::isExprOp(TokenKind k) const {
    return k == TOK_PLUS || k == TOK_MINUS || k == TOK_STAR || k == TOK_SLASH;
}

// 加減算（低優先度）
Expr* Parser::parseExpr() {
    Expr* left = parseTerm();
    while (!atEnd() && (peek().kind == TOK_PLUS || peek().kind == TOK_MINUS)) {
        TokenKind k = peek().kind;
        advance();
        char op = (k == TOK_PLUS) ? '+' : '-';
        Expr* right = parseTerm();
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

// 乗除算（高優先度）
Expr* Parser::parseTerm() {
    Expr* left = parsePrimary();
    while (!atEnd() && (peek().kind == TOK_STAR || peek().kind == TOK_SLASH)) {
        TokenKind k = peek().kind;
        advance();
        char op = (k == TOK_STAR) ? '*' : '/';
        Expr* right = parsePrimary();
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

// 基本要素
Expr* Parser::parsePrimary() {
    if (peek().kind == TOK_NUMBER) {
        const Token& t = advance();
        bool isFloat = t.value.find('.') != std::string::npos;
        return new NumberExpr(t.value, isFloat);
    }
    if (peek().kind == TOK_IDENT) {
        std::string name = advance().value;
        return new VarExpr(name);
    }
    std::ostringstream ss;
    ss << "line " << peek().line << ": unexpected token '" << peek().value << "' in expression";
    throw std::runtime_error(ss.str());
}

Program* Parser::parse() {
    Program* prog = new Program();

    while (true) {
        skipNewlines();
        if (atEnd()) break;

        int startLine = peek().line;
        TokenKind firstKind = peek().kind;

        if (firstKind == TOK_ASSIGN) {
            // = 式 → 出力
            advance();
            Expr* e = parseExpr();
            prog->stmts.push_back(new ExprOutputNode(e, startLine));

        } else if (firstKind == TOK_STRING) {
            std::string text = peek().value;
            advance();
            prog->stmts.push_back(new StringOutputNode(text, startLine));

        } else if (firstKind == TOK_NUMBER) {
            // 数値または数値式 → 出力
            Expr* e = parseExpr();
            if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
            prog->stmts.push_back(new ExprOutputNode(e, startLine));

        } else if (firstKind == TOK_IDENT) {
            TokenKind nextKind = peekAt(1).kind;

            if (nextKind == TOK_COLON) {
                // x: → 標準入力
                std::string varName = peek().value;
                advance(); advance(); // ident, :
                prog->stmts.push_back(new InputNode(varName, startLine));

            } else if (nextKind == TOK_ASSIGN) {
                // x = ... → 代入
                std::string varName = peek().value;
                advance(); advance(); // ident, =
                if (!atEnd() && peek().kind == TOK_STRING) {
                    // 文字列代入は既存パス
                    std::string sval = peek().value;
                    advance();
                    prog->stmts.push_back(new AssignNode(varName, VAL_STRING, sval, startLine));
                } else {
                    Expr* e = parseExpr();
                    prog->stmts.push_back(new ExprAssignNode(varName, e, startLine));
                }

            } else {
                // 変数単独出力 or 式出力 (b+3, c+4=)
                Expr* e = parseExpr();
                if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
                prog->stmts.push_back(new ExprOutputNode(e, startLine));
            }
        }
        // 行末まで読み飛ばす（余分なトークンは無視）
        while (!atEnd() && peek().kind != TOK_NEWLINE)
            advance();
    }
    return prog;
}

void Parser::skipSpacesInTokens() {
    // トークン列にはスペースは含まれないので何もしない
}

} // namespace one
