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

// 条件式をC言語の文字列として生成
std::string CodeGen::genBoolExpr(const Expr* e) {
    if (e->kind == EXPR_COMPARE) {
        const CompareExpr* c = static_cast<const CompareExpr*>(e);
        std::string left  = genExpr(c->left);
        std::string right = genExpr(c->right);
        const char* op = "==";
        switch (c->op) {
            case CMP_EQ:  op = "=="; break;
            case CMP_NEQ: op = "!="; break;
            case CMP_GT:  op = ">";  break;
            case CMP_LT:  op = "<";  break;
            case CMP_GTE: op = ">="; break;
            case CMP_LTE: op = "<="; break;
        }
        return "(" + left + " " + op + " " + right + ")";
    }
    if (e->kind == EXPR_LOGIC) {
        const LogicExpr* l = static_cast<const LogicExpr*>(e);
        std::string left  = genBoolExpr(l->left);
        std::string right = genBoolExpr(l->right);
        std::string op = (l->op == '&') ? "&&" : "||";
        return "(" + left + " " + op + " " + right + ")";
    }
    if (e->kind == EXPR_NOT) {
        const NotExpr* n = static_cast<const NotExpr*>(e);
        return "(!" + genBoolExpr(n->operand) + ")";
    }
    // 算術式は非ゼロで真
    return genExpr(e);
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

// 条件ブロック内の代入文を if の外で事前宣言する
void CodeGen::preDeclare(std::ostringstream& out, Node* node, const std::string& indent) {
    if (node->kind == NODE_EXPR_ASSIGN) {
        ExprAssignNode* b = static_cast<ExprAssignNode*>(node);
        if (varTypes_.count(b->varName) == 0) {
            ValKind vk = exprType(b->expr);
            if (vk == VAL_INT) out << indent << "int " << b->varName << " = 0;\n";
            else               out << indent << "double " << b->varName << " = 0.0;\n";
            varTypes_[b->varName] = vk;
        }
    } else if (node->kind == NODE_ASSIGN) {
        AssignNode* b = static_cast<AssignNode*>(node);
        if (varTypes_.count(b->varName) == 0) {
            if (b->valKind == VAL_INT)        out << indent << "int " << b->varName << " = 0;\n";
            else if (b->valKind == VAL_FLOAT) out << indent << "double " << b->varName << " = 0.0;\n";
            else                              out << indent << "const char* " << b->varName << " = \"\";\n";
            varTypes_[b->varName] = b->valKind;
        }
    }
}

void CodeGen::emitStmt(std::ostringstream& out, Node* node, const std::string& indent) {
    if (node->kind == NODE_STRING_OUTPUT) {
        StringOutputNode* n = static_cast<StringOutputNode*>(node);
        out << indent << "puts(\"" << escapeString(n->text) << "\");\n";

    } else if (node->kind == NODE_NUMBER_OUTPUT) {
        NumberOutputNode* n = static_cast<NumberOutputNode*>(node);
        if (n->isFloat)
            out << indent << "printf(\"%g\\n\", (double)" << n->raw << ");\n";
        else
            out << indent << "printf(\"%d\\n\", (int)" << n->raw << ");\n";

    } else if (node->kind == NODE_ASSIGN) {
        AssignNode* n = static_cast<AssignNode*>(node);
        bool declared = varTypes_.count(n->varName) > 0;
        if (n->valKind == VAL_INT) {
            out << indent << (declared ? "" : "int ") << n->varName << " = " << n->rawValue << ";\n";
            varTypes_[n->varName] = VAL_INT;
        } else if (n->valKind == VAL_FLOAT) {
            out << indent << (declared ? "" : "double ") << n->varName << " = " << n->rawValue << ";\n";
            varTypes_[n->varName] = VAL_FLOAT;
        } else {
            out << indent << (declared ? "" : "const char* ") << n->varName << " = \"" << escapeString(n->rawValue) << "\";\n";
            varTypes_[n->varName] = VAL_STRING;
        }

    } else if (node->kind == NODE_INPUT) {
        InputNode* n = static_cast<InputNode*>(node);
        std::string buf = "_one_buf_" + n->varName;
        bool declared = varTypes_.count(n->varName) > 0;
        if (!declared)
            out << indent << "char " << buf << "[1024];\n";
        out << indent << "fgets(" << buf << ", sizeof(" << buf << "), stdin);\n";
        out << indent << "{ size_t _l = strlen(" << buf << "); "
            << "if (_l > 0 && " << buf << "[_l-1] == '\\n') "
            << buf << "[_l-1] = '\\0'; }\n";
        if (!declared)
            out << indent << "const char* " << n->varName << " = " << buf << ";\n";
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
            out << indent << "printf(\"%d\\n\", " << n->varName << ");\n";
        else if (vk == VAL_FLOAT)
            out << indent << "printf(\"%g\\n\", " << n->varName << ");\n";
        else
            out << indent << "puts(" << n->varName << ");\n";

    } else if (node->kind == NODE_EXPR_OUTPUT) {
        ExprOutputNode* n = static_cast<ExprOutputNode*>(node);
        if (n->expr->kind == EXPR_VAR) {
            const VarExpr* v = static_cast<const VarExpr*>(n->expr);
            if (varTypes_.count(v->name) && varTypes_[v->name] == VAL_STRING) {
                out << indent << "puts(" << v->name << ");\n";
                return;
            }
        }
        ValKind vk = exprType(n->expr);
        std::string cexpr = genExpr(n->expr);
        if (vk == VAL_INT)
            out << indent << "printf(\"%d\\n\", " << cexpr << ");\n";
        else
            out << indent << "printf(\"%g\\n\", (double)(" << cexpr << "));\n";

    } else if (node->kind == NODE_EXPR_ASSIGN) {
        ExprAssignNode* n = static_cast<ExprAssignNode*>(node);
        ValKind vk = exprType(n->expr);
        std::string cexpr = genExpr(n->expr);
        bool declared = varTypes_.count(n->varName) > 0;
        if (vk == VAL_INT) {
            out << indent << (declared ? "" : "int ") << n->varName << " = " << cexpr << ";\n";
            varTypes_[n->varName] = VAL_INT;
        } else {
            out << indent << (declared ? "" : "double ") << n->varName << " = " << cexpr << ";\n";
            varTypes_[n->varName] = VAL_FLOAT;
        }

    } else if (node->kind == NODE_BLOCK) {
        BlockNode* n = static_cast<BlockNode*>(node);
        for (size_t i = 0; i < n->stmts.size(); i++)
            emitStmt(out, n->stmts[i], indent);

    } else if (node->kind == NODE_COMPOUND_ASSIGN) {
        CompoundAssignNode* n = static_cast<CompoundAssignNode*>(node);
        std::string cexpr = genExpr(n->expr);
        out << indent << n->varName << " " << n->op << "= " << cexpr << ";\n";

    } else if (node->kind == NODE_FOR) {
        ForNode* n = static_cast<ForNode*>(node);

        // 変数の事前宣言
        if (n->init) preDeclare(out, n->init, indent);
        if (n->body->kind == NODE_BLOCK) {
            BlockNode* blk = static_cast<BlockNode*>(n->body);
            for (size_t i = 0; i < blk->stmts.size(); i++)
                preDeclare(out, blk->stmts[i], indent);
        } else {
            preDeclare(out, n->body, indent);
        }

        // init を実行
        if (n->init) emitStmt(out, n->init, indent);

        // while (cond) { body; incr; }
        std::string condStr = n->cond ? genBoolExpr(n->cond) : "1";
        out << indent << "while (" << condStr << ") {\n";
        emitStmt(out, n->body, indent + "    ");
        if (n->incr) emitStmt(out, n->incr, indent + "    ");
        out << indent << "}\n";

    } else if (node->kind == NODE_LOOP) {
        LoopNode* n = static_cast<LoopNode*>(node);

        // ループ本体で未宣言変数に代入する場合、while文の外で事前宣言する
        if (n->body->kind == NODE_BLOCK) {
            BlockNode* blk = static_cast<BlockNode*>(n->body);
            for (size_t i = 0; i < blk->stmts.size(); i++)
                preDeclare(out, blk->stmts[i], indent);
        } else {
            preDeclare(out, n->body, indent);
        }

        out << indent << "while (" << genBoolExpr(n->cond) << ") {\n";
        emitStmt(out, n->body, indent + "    ");
        out << indent << "}\n";

    } else if (node->kind == NODE_COND) {
        CondNode* n = static_cast<CondNode*>(node);

        // 条件本体で未宣言変数に代入する場合、if文の外で事前宣言する
        if (n->body->kind == NODE_BLOCK) {
            BlockNode* blk = static_cast<BlockNode*>(n->body);
            for (size_t i = 0; i < blk->stmts.size(); i++)
                preDeclare(out, blk->stmts[i], indent);
        } else {
            preDeclare(out, n->body, indent);
        }

        out << indent << "if (" << genBoolExpr(n->cond) << ") {\n";
        emitStmt(out, n->body, indent + "    ");
        out << indent << "}\n";
    }
}

std::string CodeGen::generate(const Program& prog) {
    varTypes_.clear();
    std::ostringstream out;

    out << "#include <stdio.h>\n";
    out << "#include <string.h>\n";
    out << "#include <windows.h>\n\n";
    out << "int main(void) {\n";
    out << "    SetConsoleOutputCP(65001);\n";

    for (size_t i = 0; i < prog.stmts.size(); i++)
        emitStmt(out, prog.stmts[i], "    ");

    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}

} // namespace one
