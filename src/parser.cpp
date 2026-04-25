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

bool Parser::isCompOp(TokenKind k) const {
    return k == TOK_ASSIGN || k == TOK_NEQ ||
           k == TOK_GT || k == TOK_LT || k == TOK_GTE || k == TOK_LTE;
}

bool Parser::lineHasArrow() const {
    for (size_t i = pos_; i < tokens_.size(); i++) {
        if (tokens_[i].kind == TOK_NEWLINE || tokens_[i].kind == TOK_EOF)
            return false;
        if (tokens_[i].kind == TOK_ARROW)
            return true;
    }
    return false;
}

bool Parser::lineHasLoop() const {
    for (size_t i = pos_; i < tokens_.size(); i++) {
        if (tokens_[i].kind == TOK_NEWLINE || tokens_[i].kind == TOK_EOF)
            return false;
        if (tokens_[i].kind == TOK_LOOP)
            return true;
    }
    return false;
}

CondNode* Parser::parseCondStatement(int line) {
    Expr* left = parseExpr();

    if (!isCompOp(peek().kind)) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected comparison operator";
        throw std::runtime_error(ss.str());
    }
    CompOp op;
    switch (peek().kind) {
        case TOK_ASSIGN: op = CMP_EQ;  break;
        case TOK_NEQ:    op = CMP_NEQ; break;
        case TOK_GT:     op = CMP_GT;  break;
        case TOK_LT:     op = CMP_LT;  break;
        case TOK_GTE:    op = CMP_GTE; break;
        case TOK_LTE:    op = CMP_LTE; break;
        default:         op = CMP_EQ;  break;
    }
    advance(); // 比較演算子を消費

    Expr* right = parseExpr();

    if (peek().kind != TOK_ARROW) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected '->'";
        throw std::runtime_error(ss.str());
    }
    advance(); // -> を消費

    Node* body = parseCondBody(line);
    return new CondNode(left, op, right, body, line);
}

LoopNode* Parser::parseLoopStatement(int line) {
    Expr* left = parseExpr();

    if (!isCompOp(peek().kind)) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected comparison operator";
        throw std::runtime_error(ss.str());
    }
    CompOp op;
    switch (peek().kind) {
        case TOK_ASSIGN: op = CMP_EQ;  break;
        case TOK_NEQ:    op = CMP_NEQ; break;
        case TOK_GT:     op = CMP_GT;  break;
        case TOK_LT:     op = CMP_LT;  break;
        case TOK_GTE:    op = CMP_GTE; break;
        case TOK_LTE:    op = CMP_LTE; break;
        default:         op = CMP_EQ;  break;
    }
    advance(); // 比較演算子を消費

    Expr* right = parseExpr();

    if (peek().kind != TOK_LOOP) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected 'o'";
        throw std::runtime_error(ss.str());
    }
    advance(); // o を消費

    Node* body = parseCondBody(line);
    return new LoopNode(left, op, right, body, line);
}

Node* Parser::parseCondBody(int line) {
    // 折り返し形式: -> の後に改行があれば次行を本体とする
    if (!atEnd() && peek().kind == TOK_NEWLINE) {
        advance();
        skipNewlines();
    }
    // ブロック形式: { ... }
    if (!atEnd() && peek().kind == TOK_LBRACE) {
        return parseBlock(line);
    }
    Node* n = parseOneStmt();
    return n ? n : new StringOutputNode("", line);
}

Node* Parser::parseBlock(int line) {
    advance(); // { を消費
    BlockNode* block = new BlockNode(line);
    while (!atEnd() && peek().kind != TOK_RBRACE) {
        skipNewlines();
        if (atEnd() || peek().kind == TOK_RBRACE) break;
        Node* n = parseOneStmt();
        if (n) block->stmts.push_back(n);
        // 行末まで読み飛ばす（余分なトークンは無視）
        while (!atEnd() && peek().kind != TOK_NEWLINE && peek().kind != TOK_RBRACE)
            advance();
    }
    if (!atEnd() && peek().kind == TOK_RBRACE)
        advance(); // } を消費
    return block;
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

Node* Parser::parseOneStmt() {
    int startLine = peek().line;
    TokenKind firstKind = peek().kind;

    if (firstKind == TOK_ASSIGN) {
        // = 式 → 出力
        advance();
        Expr* e = parseExpr();
        return new ExprOutputNode(e, startLine);

    } else if (firstKind == TOK_STRING) {
        std::string text = peek().value;
        advance();
        return new StringOutputNode(text, startLine);

    } else if (firstKind == TOK_NUMBER) {
        // 数値または数値式 → 出力
        Expr* e = parseExpr();
        if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
        return new ExprOutputNode(e, startLine);

    } else if (firstKind == TOK_IDENT) {
        TokenKind nextKind = peekAt(1).kind;

        if (nextKind == TOK_COLON) {
            // x: → 標準入力
            std::string varName = peek().value;
            advance(); advance(); // ident, :
            return new InputNode(varName, startLine);

        } else if (nextKind == TOK_ASSIGN && lineHasLoop()) {
            // x = 式 o { } → ループ
            return parseLoopStatement(startLine);

        } else if (nextKind == TOK_ASSIGN && lineHasArrow()) {
            // x = 式 -> 実行 → 条件実行
            return parseCondStatement(startLine);

        } else if (nextKind == TOK_ASSIGN) {
            // x = ... → 代入
            std::string varName = peek().value;
            advance(); advance(); // ident, =
            if (!atEnd() && peek().kind == TOK_STRING) {
                std::string sval = peek().value;
                advance();
                return new AssignNode(varName, VAL_STRING, sval, startLine);
            } else {
                Expr* e = parseExpr();
                return new ExprAssignNode(varName, e, startLine);
            }

        } else if (isCompOp(nextKind) && nextKind != TOK_ASSIGN) {
            // x > 式 o { } / x > 式 -> 実行
            if (lineHasLoop()) return parseLoopStatement(startLine);
            return parseCondStatement(startLine);

        } else {
            // 変数単独出力 or 式出力
            Expr* e = parseExpr();
            if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
            return new ExprOutputNode(e, startLine);
        }
    }
    return NULL;
}

Program* Parser::parse() {
    Program* prog = new Program();
    while (true) {
        skipNewlines();
        if (atEnd()) break;
        Node* n = parseOneStmt();
        if (n) prog->stmts.push_back(n);
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
