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
    Expr* parseExpr();
    Expr* parseTerm();
    Expr* parsePrimary();
    Node* parseOneStmt();
    CondNode* parseCondStatement(int line);
    Node* parseCondBody(int line);
};

} // namespace one

#endif
