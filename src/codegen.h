#ifndef ONE_CODEGEN_H
#define ONE_CODEGEN_H

#include "ast.h"
#include <string>

namespace one {

// Win32 C++ コードを生成する
class CodeGen {
public:
    std::string generate(const Program& prog);

private:
    std::string escapeString(const std::string& s);
};

} // namespace one

#endif
