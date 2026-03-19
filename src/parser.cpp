#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace one {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}

const Token& Parser::peek() const { return tokens_[pos_]; }
const Token& Parser::advance() { return tokens_[pos_++]; }
bool Parser::atEnd() const { return tokens_[pos_].kind == TOK_EOF; }

Program* Parser::parse() {
    Program* prog = new Program();
    while (!atEnd()) {
        const Token& tok = advance();
        if (tok.kind == TOK_STRING) {
            prog->stmts.push_back(new StringOutputNode(tok.value, tok.line));
        } else if (tok.kind == TOK_NUMBER) {
            bool isFloat = tok.value.find('.') != std::string::npos;
            prog->stmts.push_back(new NumberOutputNode(tok.value, isFloat, tok.line));
        }
    }
    return prog;
}

} // namespace one
