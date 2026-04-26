#ifndef ONE_CODEGEN_H
#define ONE_CODEGEN_H

#include "ast.h"
#include <string>
#include <map>
#include <vector>

namespace one {

class CodeGen {
public:
    std::string generate(const Program& prog);

private:
    std::map<std::string, ValKind> varTypes_; // 宣言済み変数と型
    std::map<std::string, ValKind> funcRetTypes_; // 関数の戻り値型
    int strTmpCount_;                          // 文字列一時変数のカウンター
    std::string escapeString(const std::string& s);
    std::string genExpr(const Expr* e);
    std::string genBoolExpr(const Expr* e);
    ValKind exprType(const Expr* e);
    bool isStringExpr(const Expr* e);
    void collectStrParts(const Expr* e, std::string& fmt, std::vector<std::string>& args);
    std::string genStrExpr(std::ostringstream& out, const Expr* e, const std::string& indent);
    void preDeclare(std::ostringstream& out, Node* node, const std::string& indent);
    void emitStmt(std::ostringstream& out, Node* node, const std::string& indent);
    ValKind inferReturnType(Node* body);
    void emitFuncDef(std::ostringstream& out, FuncDefNode* n);
};

} // namespace one

#endif
