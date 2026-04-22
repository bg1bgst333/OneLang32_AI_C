#ifndef ONE_CODEGEN_H
#define ONE_CODEGEN_H

#include "ast.h"
#include <string>
#include <map>

namespace one {

class CodeGen {
public:
    std::string generate(const Program& prog);

private:
    std::map<std::string, ValKind> varTypes_; // 宣言済み変数と型
    std::string escapeString(const std::string& s);
    std::string genExpr(const Expr* e);
    ValKind exprType(const Expr* e);
};

} // namespace one

#endif
