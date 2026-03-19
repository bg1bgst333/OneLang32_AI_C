#include "codegen.h"
#include <sstream>

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
    std::ostringstream out;

    out << "#include <stdio.h>\n";
    out << "#include <windows.h>\n\n";
    out << "int main(void) {\n";
    out << "    SetConsoleOutputCP(65001);\n";

    for (size_t i = 0; i < prog.stmts.size(); i++) {
        if (prog.stmts[i]->kind == NODE_STRING_OUTPUT) {
            StringOutputNode* n = static_cast<StringOutputNode*>(prog.stmts[i]);
            out << "    puts(\"" << escapeString(n->text) << "\");\n";
        } else if (prog.stmts[i]->kind == NODE_NUMBER_OUTPUT) {
            NumberOutputNode* n = static_cast<NumberOutputNode*>(prog.stmts[i]);
            if (n->isFloat)
                out << "    printf(\"%g\\n\", (double)" << n->raw << ");\n";
            else
                out << "    printf(\"%d\\n\", (int)" << n->raw << ");\n";
        }
    }

    out << "    return 0;\n";
    out << "}\n";

    return out.str();
}

} // namespace one
