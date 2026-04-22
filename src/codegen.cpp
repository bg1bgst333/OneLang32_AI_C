#include "codegen.h"
#include <sstream>
#include <stdexcept>

namespace one {

std::string CodeGen::escapeString(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

// 式をC言語の文字列として生成（未定義変数は0）
std::string CodeGen::genExpr(const Expr* e) {
    if (e->kind == EXPR_NUMBER) {
        return static_cast<const NumberExpr*>(e)->raw;
    }
    if (e->kind == EXPR_VAR) {
        const VarExpr* v = static_cast<const VarExpr*>(e);
        if (varTypes_.count(v->name) == 0) return "0";
        if (varTypes_[v->name] == VAL_STRING) return "0"; // 文字列変数は算術に使えない
        return v->name;
    }
    // EXPR_BINARY
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    std::string op(1, b->op);
    return "(" + genExpr(b->left) + " " + op + " " + genExpr(b->right) + ")";
}

// 式の型を推論（float成分があればVAL_FLOAT、なければVAL_INT）
ValKind CodeGen::exprType(const Expr* e) {
    if (e->kind == EXPR_NUMBER) {
        return static_cast<const NumberExpr*>(e)->isFloat ? VAL_FLOAT : VAL_INT;
    }
    if (e->kind == EXPR_VAR) {
        const VarExpr* v = static_cast<const VarExpr*>(e);
        if (!varTypes_.count(v->name)) return VAL_INT; // 未定義 → int(0)
        ValKind vk = varTypes_[v->name];
        return (vk == VAL_STRING) ? VAL_INT : vk;
    }
    // EXPR_BINARY
    const BinaryExpr* b = static_cast<const BinaryExpr*>(e);
    ValKind lt = exprType(b->left);
    ValKind rt = exprType(b->right);
    return (lt == VAL_FLOAT || rt == VAL_FLOAT) ? VAL_FLOAT : VAL_INT;
}

std::string CodeGen::generate(const Program& prog) {
    varTypes_.clear();
    std::ostringstream out;

    out << "#include <stdio.h>\n";
    out << "#include <string.h>\n";
    out << "#include <windows.h>\n\n";
    out << "int main(void) {\n";
    out << "    SetConsoleOutputCP(65001);\n";

    for (size_t i = 0; i < prog.stmts.size(); i++) {
        Node* node = prog.stmts[i];

        if (node->kind == NODE_STRING_OUTPUT) {
            StringOutputNode* n = static_cast<StringOutputNode*>(node);
            out << "    puts(\"" << escapeString(n->text) << "\");\n";

        } else if (node->kind == NODE_NUMBER_OUTPUT) {
            NumberOutputNode* n = static_cast<NumberOutputNode*>(node);
            if (n->isFloat)
                out << "    printf(\"%g\\n\", (double)" << n->raw << ");\n";
            else
                out << "    printf(\"%d\\n\", (int)" << n->raw << ");\n";

        } else if (node->kind == NODE_ASSIGN) {
            AssignNode* n = static_cast<AssignNode*>(node);
            bool declared = varTypes_.count(n->varName) > 0;

            if (n->valKind == VAL_INT) {
                if (!declared) out << "    int ";
                else           out << "    ";
                out << n->varName << " = " << n->rawValue << ";\n";
                varTypes_[n->varName] = VAL_INT;

            } else if (n->valKind == VAL_FLOAT) {
                if (!declared) out << "    double ";
                else           out << "    ";
                out << n->varName << " = " << n->rawValue << ";\n";
                varTypes_[n->varName] = VAL_FLOAT;

            } else { // VAL_STRING
                if (!declared) out << "    const char* ";
                else           out << "    ";
                out << n->varName << " = \"" << escapeString(n->rawValue) << "\";\n";
                varTypes_[n->varName] = VAL_STRING;
            }

        } else if (node->kind == NODE_INPUT) {
            InputNode* n = static_cast<InputNode*>(node);
            std::string buf = "_one_buf_" + n->varName;
            bool declared = varTypes_.count(n->varName) > 0;
            if (!declared)
                out << "    char " << buf << "[1024];\n";
            out << "    fgets(" << buf << ", sizeof(" << buf << "), stdin);\n";
            out << "    { size_t _l = strlen(" << buf << "); "
                << "if (_l > 0 && " << buf << "[_l-1] == '\\n') "
                << buf << "[_l-1] = '\\0'; }\n";
            if (!declared)
                out << "    const char* " << n->varName << " = " << buf << ";\n";
            varTypes_[n->varName] = VAL_STRING;

        } else if (node->kind == NODE_VAR_OUTPUT) {
            VarOutputNode* n = static_cast<VarOutputNode*>(node);
            if (varTypes_.count(n->varName) == 0) {
                std::ostringstream ss;
                ss << "line " << n->line << ": undefined variable '" << n->varName << "'";
                throw std::runtime_error(ss.str());
            }
            ValKind vk = varTypes_[n->varName];
            if (vk == VAL_INT)
                out << "    printf(\"%d\\n\", " << n->varName << ");\n";
            else if (vk == VAL_FLOAT)
                out << "    printf(\"%g\\n\", " << n->varName << ");\n";
            else
                out << "    puts(" << n->varName << ");\n";

        } else if (node->kind == NODE_EXPR_OUTPUT) {
            ExprOutputNode* n = static_cast<ExprOutputNode*>(node);
            // 文字列変数の単独出力は puts
            if (n->expr->kind == EXPR_VAR) {
                const VarExpr* v = static_cast<const VarExpr*>(n->expr);
                if (varTypes_.count(v->name) && varTypes_[v->name] == VAL_STRING) {
                    out << "    puts(" << v->name << ");\n";
                    continue;
                }
            }
            ValKind vk = exprType(n->expr);
            std::string cexpr = genExpr(n->expr);
            if (vk == VAL_INT)
                out << "    printf(\"%d\\n\", " << cexpr << ");\n";
            else
                out << "    printf(\"%g\\n\", (double)(" << cexpr << "));\n";

        } else if (node->kind == NODE_EXPR_ASSIGN) {
            ExprAssignNode* n = static_cast<ExprAssignNode*>(node);
            ValKind vk = exprType(n->expr);
            std::string cexpr = genExpr(n->expr);
            bool declared = varTypes_.count(n->varName) > 0;
            if (vk == VAL_INT) {
                if (!declared) out << "    int ";
                else           out << "    ";
                out << n->varName << " = " << cexpr << ";\n";
                varTypes_[n->varName] = VAL_INT;
            } else {
                if (!declared) out << "    double ";
                else           out << "    ";
                out << n->varName << " = " << cexpr << ";\n";
                varTypes_[n->varName] = VAL_FLOAT;
            }
        }
    }

    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}

} // namespace one
