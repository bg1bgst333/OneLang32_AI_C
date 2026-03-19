#include "parser.h"
#include <stdexcept>
#include <sstream>

namespace one {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}

const Token& Parser::peek() const { return tokens_[pos_]; }
const Token& Parser::advance() { return tokens_[pos_++]; }
bool Parser::atEnd() const { return tokens_[pos_].kind == TOK_EOF; }

void Parser::skipNewlines() {
    while (!atEnd() && tokens_[pos_].kind == TOK_NEWLINE)
        pos_++;
}

Program* Parser::parse() {
    Program* prog = new Program();

    while (true) {
        skipNewlines();
        if (atEnd()) break;

        const Token& tok = advance();

        if (tok.kind == TOK_STRING) {
            prog->stmts.push_back(new StringOutputNode(tok.value, tok.line));

        } else if (tok.kind == TOK_NUMBER) {
            bool isFloat = tok.value.find('.') != std::string::npos;
            prog->stmts.push_back(new NumberOutputNode(tok.value, isFloat, tok.line));

        } else if (tok.kind == TOK_IDENT) {
            // x = 値 の代入か、変数出力か
            if (!atEnd() && tokens_[pos_].kind == TOK_COLON) {
                advance(); // : を消費
                prog->stmts.push_back(new InputNode(tok.value, tok.line));
            } else if (!atEnd() && tokens_[pos_].kind == TOK_ASSIGN) {
                advance(); // = を消費
                skipSpacesInTokens();

                if (atEnd() || tokens_[pos_].kind == TOK_NEWLINE) {
                    std::ostringstream ss;
                    ss << "line " << tok.line << ": assignment missing value";
                    throw std::runtime_error(ss.str());
                }

                const Token& val = advance();
                ValKind vk;
                std::string raw;

                if (val.kind == TOK_NUMBER) {
                    vk = val.value.find('.') != std::string::npos ? VAL_FLOAT : VAL_INT;
                    raw = val.value;
                } else {
                    // TOK_STRING または TOK_IDENT
                    vk = VAL_STRING;
                    raw = val.value;
                }
                prog->stmts.push_back(new AssignNode(tok.value, vk, raw, tok.line));
            } else {
                // 変数出力
                prog->stmts.push_back(new VarOutputNode(tok.value, tok.line));
            }
        }
    }
    return prog;
}

void Parser::skipSpacesInTokens() {
    // トークン列にはスペースは含まれないので何もしない
}

} // namespace one
