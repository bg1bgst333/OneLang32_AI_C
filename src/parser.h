#ifndef ONE_PARSER_H
#define ONE_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <vector>

namespace one {

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    Program* parse();

private:
    std::vector<Token> tokens_;
    size_t pos_;

    const Token& peek() const;
    const Token& peekAt(size_t offset) const;
    const Token& advance();
    bool atEnd() const;
    void skipNewlines();
    void skipSpacesInTokens();
    bool isExprOp(TokenKind k) const;
    bool isCompOp(TokenKind k) const;
    bool lineHasArrow() const;
    bool lineHasLoop() const;
    int  lineSemicolonCount() const;
    bool isCompoundAssignOp(TokenKind k) const;
    bool lineHasFuncDef() const;
    bool lineHasFilename() const;
    bool lineHasAppendOp() const;
    Node* parseFileWrite(int line);
    Node* parseFileAppend(int line);
    Expr* parseLogicExpr();   // & |
    Expr* parseCompareExpr(); // 比較演算子
    Expr* parseExpr();        // + -
    Expr* parseTerm();        // * /
    Expr* parsePrimary();     // 数値・変数・括弧・!
    Expr* parseCondExpr();    // 条件全体（論理式）
    Expr* parseFuncCallExpr(const std::string& name); // 関数呼び出し式
    Node* parseOneStmt();
    Node* parseCondOrLoop(int line);
    Node* parseForLoop(int line);
    Node* parseLoopClause(int line); // init/incr用: 代入 or 複合代入
    Node* parseCondBody(int line);
    Node* parseBlock(int line);
    Node* parseFuncDef(int line);
    Node* parseFuncCallStmt(const std::string& name, int line);
    Expr* parseListLiteral();
    Expr* parsePostfix(Expr* base, const std::string& baseName);
};

} // namespace one

#endif
