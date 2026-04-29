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

int Parser::lineSemicolonCount() const {
    int count = 0;
    for (size_t i = pos_; i < tokens_.size(); i++) {
        if (tokens_[i].kind == TOK_NEWLINE || tokens_[i].kind == TOK_EOF) break;
        if (tokens_[i].kind == TOK_LOOP) break; // o の手前まで
        if (tokens_[i].kind == TOK_SEMICOLON) count++;
    }
    return count;
}

bool Parser::isCompoundAssignOp(TokenKind k) const {
    return k == TOK_PLUS_ASSIGN || k == TOK_MINUS_ASSIGN ||
           k == TOK_STAR_ASSIGN || k == TOK_SLASH_ASSIGN;
}

bool Parser::lineHasFilename() const {
    for (size_t i = pos_; i < tokens_.size(); i++) {
        if (tokens_[i].kind == TOK_NEWLINE || tokens_[i].kind == TOK_EOF) break;
        if (tokens_[i].kind == TOK_FILENAME) return true;
    }
    return false;
}

// ファイル書き込み: expr > filename / filename < expr
Node* Parser::parseFileWrite(int line) {
    // Case: filename > expr または filename < expr
    if (peek().kind == TOK_FILENAME) {
        std::string filename = advance().value;
        if (!atEnd() && (peek().kind == TOK_GT || peek().kind == TOK_LT)) {
            advance(); // > or <
            Expr* val = parseExpr();
            return new FileWriteNode(filename, val, line);
        }
        return NULL;
    }
    // Case: expr > filename または expr < filename
    Expr* val = parseExpr();
    if (!atEnd() && (peek().kind == TOK_GT || peek().kind == TOK_LT)) {
        advance(); // > or <
        if (!atEnd() && peek().kind == TOK_FILENAME) {
            std::string filename = advance().value;
            return new FileWriteNode(filename, val, line);
        }
    }
    return new ExprOutputNode(val, line);
}

// IDENT ( params ) { の形か判定（関数定義）
bool Parser::lineHasFuncDef() const {
    size_t i = pos_;
    if (i >= tokens_.size() || tokens_[i].kind != TOK_IDENT) return false;
    i++;
    if (i >= tokens_.size() || tokens_[i].kind != TOK_LPAREN) return false;
    i++;
    int depth = 1;
    while (i < tokens_.size() && depth > 0) {
        if (tokens_[i].kind == TOK_NEWLINE || tokens_[i].kind == TOK_EOF) return false;
        if (tokens_[i].kind == TOK_LPAREN) depth++;
        if (tokens_[i].kind == TOK_RPAREN) depth--;
        i++;
    }
    // ) の後に { があれば関数定義
    while (i < tokens_.size() &&
           tokens_[i].kind != TOK_NEWLINE && tokens_[i].kind != TOK_EOF) {
        if (tokens_[i].kind == TOK_LBRACE) return true;
        i++;
    }
    return false;
}

// ── 式パーサー ────────────────────────────────────────────

// 基本要素: 数値・変数・(論理式)・!式
Expr* Parser::parsePrimary() {
    // 論理NOT: ! 式
    if (peek().kind == TOK_NOT) {
        advance();
        Expr* operand = parsePrimary();
        return new NotExpr(operand);
    }
    // 括弧: ( 論理式 )
    if (peek().kind == TOK_LPAREN) {
        advance(); // (
        Expr* e = parseLogicExpr();
        if (!atEnd() && peek().kind == TOK_RPAREN)
            advance(); // )
        else {
            std::ostringstream ss;
            ss << "line " << peek().line << ": expected ')'";
            throw std::runtime_error(ss.str());
        }
        return e;
    }
    // リストリテラル: [1, 2, 3]
    if (peek().kind == TOK_LBRACKET) {
        return parseListLiteral();
    }
    if (peek().kind == TOK_STRING) {
        std::string val = advance().value;
        return new StringExpr(val);
    }
    if (peek().kind == TOK_NUMBER) {
        const Token& t = advance();
        bool isFloat = t.value.find('.') != std::string::npos;
        return new NumberExpr(t.value, isFloat);
    }
    if (peek().kind == TOK_IDENT) {
        std::string name = advance().value;
        Expr* base = NULL;
        if (!atEnd() && peek().kind == TOK_LPAREN)
            base = parseFuncCallExpr(name);
        else
            base = new VarExpr(name);
        // ポストフィックス: a[i] / a.len / a.add(x)
        return parsePostfix(base, name);
    }
    std::ostringstream ss;
    ss << "line " << peek().line << ": unexpected token '" << peek().value << "' in expression";
    throw std::runtime_error(ss.str());
}

// 乗除算
Expr* Parser::parseTerm() {
    Expr* left = parsePrimary();
    while (!atEnd() && (peek().kind == TOK_STAR || peek().kind == TOK_SLASH)) {
        char op = (peek().kind == TOK_STAR) ? '*' : '/';
        advance();
        Expr* right = parsePrimary();
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

// 加減算
Expr* Parser::parseExpr() {
    Expr* left = parseTerm();
    while (!atEnd() && (peek().kind == TOK_PLUS || peek().kind == TOK_MINUS)) {
        char op = (peek().kind == TOK_PLUS) ? '+' : '-';
        advance();
        Expr* right = parseTerm();
        left = new BinaryExpr(op, left, right);
    }
    return left;
}

// 比較式: 式 比較演算子 式
Expr* Parser::parseCompareExpr() {
    Expr* left = parseExpr();
    if (!atEnd() && isCompOp(peek().kind)) {
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
        advance();
        Expr* right = parseExpr();
        return new CompareExpr(op, left, right);
    }
    return left;
}

// 論理式: 比較式 & 比較式 / 比較式 | 比較式
Expr* Parser::parseLogicExpr() {
    Expr* left = parseCompareExpr();
    while (!atEnd() && (peek().kind == TOK_AND || peek().kind == TOK_OR)) {
        char op = (peek().kind == TOK_AND) ? '&' : '|';
        advance();
        Expr* right = parseCompareExpr();
        left = new LogicExpr(op, left, right);
    }
    return left;
}

// 条件式全体 (論理式レベル)
Expr* Parser::parseCondExpr() {
    return parseLogicExpr();
}

// ── 文パーサー ────────────────────────────────────────────

Node* Parser::parseCondBody(int line) {
    // -> / o の後に改行があれば折り返し
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
        while (!atEnd() && peek().kind != TOK_NEWLINE && peek().kind != TOK_RBRACE)
            advance();
    }
    if (!atEnd() && peek().kind == TOK_RBRACE)
        advance(); // } を消費
    return block;
}

// init/incr用: 代入 (x=式) または複合代入 (x+=式)
Node* Parser::parseLoopClause(int line) {
    if (peek().kind != TOK_IDENT) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected variable in loop clause";
        throw std::runtime_error(ss.str());
    }
    std::string varName = advance().value;
    if (peek().kind == TOK_ASSIGN) {
        advance();
        Expr* e = parseExpr();
        return new ExprAssignNode(varName, e, line);
    } else if (isCompoundAssignOp(peek().kind)) {
        char op;
        switch (peek().kind) {
            case TOK_PLUS_ASSIGN:  op = '+'; break;
            case TOK_MINUS_ASSIGN: op = '-'; break;
            case TOK_STAR_ASSIGN:  op = '*'; break;
            default:               op = '/'; break;
        }
        advance();
        Expr* e = parseExpr();
        return new CompoundAssignNode(varName, op, e, line);
    }
    std::ostringstream ss;
    ss << "line " << peek().line << ": expected '=' or compound assign in loop clause";
    throw std::runtime_error(ss.str());
}

// for ループ: cond; incr o { } / init; cond; incr o { }
Node* Parser::parseForLoop(int line) {
    int semiCount = lineSemicolonCount();

    Node* init = NULL;
    Expr* cond = NULL;
    Node* incr = NULL;

    if (semiCount >= 2) {
        // init; cond; incr o { }
        init = parseLoopClause(line);
        advance(); // ; を消費
        if (peek().kind != TOK_SEMICOLON)
            cond = parseCondExpr();
        advance(); // ; を消費
        if (peek().kind != TOK_LOOP)
            incr = parseLoopClause(line);
    } else {
        // cond; incr o { }
        cond = parseCondExpr();
        advance(); // ; を消費
        if (peek().kind != TOK_LOOP)
            incr = parseLoopClause(line);
    }

    if (peek().kind != TOK_LOOP) {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected 'o'";
        throw std::runtime_error(ss.str());
    }
    advance(); // o を消費

    Node* body = parseCondBody(line);
    return new ForNode(init, cond, incr, body, line);
}

// 条件/ループ共通: 条件式を解析して -> か o で分岐
Node* Parser::parseCondOrLoop(int line) {
    Expr* cond = parseCondExpr();

    if (!atEnd() && peek().kind == TOK_ARROW) {
        advance(); // -> を消費
        Node* body = parseCondBody(line);

        // else チェック: } -> {...} (同行) または次行の行頭 ->
        Node* elseBody = NULL;
        if (!atEnd() && peek().kind == TOK_ARROW) {
            // ブロック後の同行 } ->
            advance();
            elseBody = parseCondBody(line);
        } else {
            // 次行の行頭 -> チェック（改行をまたぐ）
            size_t savedPos = pos_;
            while (!atEnd() && peek().kind == TOK_NEWLINE) advance();
            if (!atEnd() && peek().kind == TOK_ARROW) {
                advance();
                elseBody = parseCondBody(line);
            } else {
                pos_ = savedPos; // else なし → 巻き戻し
            }
        }

        return new CondNode(cond, body, elseBody, line);
    } else if (!atEnd() && peek().kind == TOK_LOOP) {
        advance(); // o を消費
        Node* body = parseCondBody(line);
        return new LoopNode(cond, body, line);
    } else {
        std::ostringstream ss;
        ss << "line " << peek().line << ": expected '->' or 'o'";
        throw std::runtime_error(ss.str());
    }
}

Node* Parser::parseOneStmt() {
    int startLine = peek().line;

    // ファイル書き込みチェック（最優先）
    if (lineHasFilename()) {
        return parseFileWrite(startLine);
    }

    TokenKind firstKind = peek().kind;

    if (firstKind == TOK_LARROW) {
        // <- 式 → return
        advance();
        if (atEnd() || peek().kind == TOK_NEWLINE) return new ReturnNode(NULL, startLine);
        Expr* e = parseExpr();
        return new ReturnNode(e, startLine);

    } else if (firstKind == TOK_ASSIGN) {
        // = 式 → 出力
        advance();
        Expr* e = parseExpr();
        return new ExprOutputNode(e, startLine);

    } else if (firstKind == TOK_STRING) {
        std::string text = peek().value;
        advance();
        return new StringOutputNode(text, startLine);

    } else if (firstKind == TOK_LPAREN || firstKind == TOK_NOT) {
        // ( や ! で始まる → 条件/ループ（; があれば for）
        if (lineHasLoop() && lineSemicolonCount() >= 1)
            return parseForLoop(startLine);
        return parseCondOrLoop(startLine);

    } else if (firstKind == TOK_NUMBER) {
        if (lineHasLoop() && lineSemicolonCount() >= 1)
            return parseForLoop(startLine);
        if (lineHasArrow() || lineHasLoop())
            return parseCondOrLoop(startLine);
        Expr* e = parseExpr();
        if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
        return new ExprOutputNode(e, startLine);

    } else if (firstKind == TOK_IDENT) {
        TokenKind nextKind = peekAt(1).kind;

        if (nextKind == TOK_LPAREN && lineHasFuncDef()) {
            // func(params) { body } → 関数定義
            return parseFuncDef(startLine);

        } else if (nextKind == TOK_LPAREN) {
            // func(args) → 関数呼び出し文
            std::string name = advance().value;
            return parseFuncCallStmt(name, startLine);

        } else if (nextKind == TOK_LBRACKET) {
            // a[i] = 式 → インデックス代入
            std::string name = advance().value;
            advance(); // [
            Expr* idx = parseExpr();
            if (!atEnd() && peek().kind == TOK_RBRACKET) advance(); // ]
            if (!atEnd() && peek().kind == TOK_ASSIGN) {
                advance(); // =
                Expr* val = parseExpr();
                return new IndexAssignNode(name, idx, val, startLine);
            }
            // 代入でなければ式出力
            Expr* e = new IndexExpr(name, idx);
            if (!atEnd() && peek().kind == TOK_ASSIGN) advance();
            return new ExprOutputNode(e, startLine);

        } else if (nextKind == TOK_DOT) {
            // a.method(args) → メソッド呼び出し文 / a.member → 出力
            std::string name = advance().value;
            advance(); // .
            std::string member = advance().value; // メンバー/メソッド名
            if (!atEnd() && peek().kind == TOK_LPAREN) {
                // a.method(args) 文
                advance(); // (
                MethodCallStmtNode* node = new MethodCallStmtNode(name, member, startLine);
                while (!atEnd() && peek().kind != TOK_RPAREN) {
                    if (peek().kind == TOK_NEWLINE || peek().kind == TOK_EOF) break;
                    node->args.push_back(parseExpr());
                    if (!atEnd() && peek().kind == TOK_COMMA) advance();
                }
                if (!atEnd() && peek().kind == TOK_RPAREN) advance(); // )
                return node;
            }
            // a.member → 出力
            return new ExprOutputNode(new MemberExpr(name, member), startLine);

        } else if (nextKind == TOK_COLON) {
            // x: → 標準入力
            std::string varName = peek().value;
            advance(); advance(); // ident, :
            return new InputNode(varName, startLine);

        } else if (isCompoundAssignOp(nextKind)) {
            // x += 式 など複合代入（単独文）
            return parseLoopClause(startLine);

        } else if (nextKind == TOK_ASSIGN && lineHasLoop() && lineSemicolonCount() >= 1) {
            // init; cond; incr o { } の init が x=式 の場合
            return parseForLoop(startLine);

        } else if (nextKind == TOK_ASSIGN && (lineHasArrow() || lineHasLoop())) {
            // x = 式 -> / x = 式 o → 条件/ループ
            return parseCondOrLoop(startLine);

        } else if (nextKind == TOK_ASSIGN) {
            // x = ... → 代入
            std::string varName = peek().value;
            advance(); advance(); // ident, =
            Expr* e = parseExpr();
            return new ExprAssignNode(varName, e, startLine);

        } else if (lineHasLoop() && lineSemicolonCount() >= 1) {
            return parseForLoop(startLine);

        } else if (lineHasArrow() || lineHasLoop()) {
            // x op 式 -> / x op 式 o (比較・算術を含む条件式)
            return parseCondOrLoop(startLine);

        } else {
            // 変数単独出力 or 式出力
            Expr* e = parseExpr();
            if (!atEnd() && peek().kind == TOK_ASSIGN) advance(); // 末尾 = を消費
            return new ExprOutputNode(e, startLine);
        }
    }
    return NULL;
}

// リストリテラル: [elem, elem, ...]
Expr* Parser::parseListLiteral() {
    advance(); // [ を消費
    ListExpr* list = new ListExpr();
    while (!atEnd() && peek().kind != TOK_RBRACKET) {
        if (peek().kind == TOK_NEWLINE || peek().kind == TOK_EOF) break;
        list->elems.push_back(parseExpr());
        if (!atEnd() && peek().kind == TOK_COMMA) advance();
    }
    if (!atEnd() && peek().kind == TOK_RBRACKET) advance(); // ] を消費
    return list;
}

// ポストフィックス: a[i] / a.len / a.add(x) / func()[i] など
Expr* Parser::parsePostfix(Expr* base, const std::string& baseName) {
    while (!atEnd()) {
        if (peek().kind == TOK_LBRACKET) {
            // a[i]
            advance(); // [
            Expr* idx = parseExpr();
            if (!atEnd() && peek().kind == TOK_RBRACKET) advance(); // ]
            delete base;
            base = new IndexExpr(baseName, idx);
        } else if (peek().kind == TOK_DOT) {
            // a.member / a.method(args)
            advance(); // .
            if (atEnd() || peek().kind != TOK_IDENT) break;
            std::string member = advance().value;
            if (!atEnd() && peek().kind == TOK_LPAREN) {
                // a.method(args)
                advance(); // (
                MethodCallExpr* mc = new MethodCallExpr(baseName, member);
                while (!atEnd() && peek().kind != TOK_RPAREN) {
                    if (peek().kind == TOK_NEWLINE || peek().kind == TOK_EOF) break;
                    mc->args.push_back(parseExpr());
                    if (!atEnd() && peek().kind == TOK_COMMA) advance();
                }
                if (!atEnd() && peek().kind == TOK_RPAREN) advance(); // )
                delete base;
                base = mc;
            } else {
                // a.member (プロパティ)
                delete base;
                base = new MemberExpr(baseName, member);
            }
        } else {
            break;
        }
    }
    return base;
}

// 関数呼び出し式: name( arg, arg, ... )
Expr* Parser::parseFuncCallExpr(const std::string& name) {
    advance(); // ( を消費
    FuncCallExpr* call = new FuncCallExpr(name);
    while (!atEnd() && peek().kind != TOK_RPAREN) {
        if (peek().kind == TOK_NEWLINE || peek().kind == TOK_EOF) break;
        call->args.push_back(parseExpr());
        if (!atEnd() && peek().kind == TOK_COMMA) advance(); // , をスキップ
    }
    if (!atEnd() && peek().kind == TOK_RPAREN) advance(); // ) を消費
    return call;
}

// 関数定義: name( params ) { body }
Node* Parser::parseFuncDef(int line) {
    std::string name = advance().value; // 関数名
    advance(); // (
    std::vector<std::string> params;
    while (!atEnd() && peek().kind != TOK_RPAREN) {
        if (peek().kind == TOK_IDENT) params.push_back(advance().value);
        if (!atEnd() && peek().kind == TOK_COMMA) advance();
    }
    if (!atEnd() && peek().kind == TOK_RPAREN) advance(); // )
    // { body }
    Node* body = parseCondBody(line);
    return new FuncDefNode(name, params, body, line);
}

// 関数呼び出し文: name( args )（戻り値不使用）
Node* Parser::parseFuncCallStmt(const std::string& name, int line) {
    advance(); // (
    FuncCallStmtNode* node = new FuncCallStmtNode(name, line);
    while (!atEnd() && peek().kind != TOK_RPAREN) {
        if (peek().kind == TOK_NEWLINE || peek().kind == TOK_EOF) break;
        node->args.push_back(parseExpr());
        if (!atEnd() && peek().kind == TOK_COMMA) advance();
    }
    if (!atEnd() && peek().kind == TOK_RPAREN) advance(); // )
    return node;
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
