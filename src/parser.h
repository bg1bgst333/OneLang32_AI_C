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
    const Token& advance();
    bool atEnd() const;
};

} // namespace one

#endif
