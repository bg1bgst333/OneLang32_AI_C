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

std::string CodeGen::generate(const Program& prog) {
    varTypes_.clear();
    std::ostringstream out;

    out << "#include <stdio.h>\n";
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
        }
    }

    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}

} // namespace one
