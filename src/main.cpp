#include <windows.h>
#include <stdio.h>
#include <string>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

static std::string readFile(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool writeFile(const char* path, const std::string& content) {
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f << content;
    return true;
}

// foo.1 -> foo.exe / foo.cpp
static std::string makeExePath(const std::string& input) {
    size_t dot = input.rfind('.');
    std::string base = (dot != std::string::npos) ? input.substr(0, dot) : input;
    return base + ".exe";
}
static std::string makeCppPath(const std::string& input) {
    size_t dot = input.rfind('.');
    std::string base = (dot != std::string::npos) ? input.substr(0, dot) : input;
    return base + ".cpp";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "One Compiler\n");
        fprintf(stderr, "Usage: onec <source.1> [-o output.exe]\n");
        return 1;
    }

    const char* srcPath = argv[1];

    // -o オプション
    std::string exePath;
    for (int i = 2; i < argc - 1; i++) {
        if (std::string(argv[i]) == "-o") {
            exePath = argv[i + 1];
            break;
        }
    }
    if (exePath.empty())
        exePath = makeExePath(srcPath);

    std::string cppPath = makeCppPath(srcPath);

    // ソース読み込み
    std::string src = readFile(srcPath);
    if (src.empty()) {
        fprintf(stderr, "Error: cannot read '%s'\n", srcPath);
        return 1;
    }

    // 字句解析
    one::Lexer lexer(src);
    std::vector<one::Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::exception& e) {
        fprintf(stderr, "Lexer error: %s\n", e.what());
        return 1;
    }

    // 構文解析
    one::Parser parser(tokens);
    one::Program* prog = NULL;
    try {
        prog = parser.parse();
    } catch (const std::exception& e) {
        fprintf(stderr, "Parse error: %s\n", e.what());
        return 1;
    }

    // コード生成 -> 一時 .cpp
    one::CodeGen codegen;
    std::string code = codegen.generate(*prog);
    delete prog;

    if (!writeFile(cppPath.c_str(), code)) {
        fprintf(stderr, "Error: cannot write '%s'\n", cppPath.c_str());
        return 1;
    }

    // g++ で .exe にコンパイル
    std::string cmd = "g++ \"" + cppPath + "\" -o \"" + exePath + "\"";
    int ret = system(cmd.c_str());

    // 中間 .cpp を削除
    DeleteFileA(cppPath.c_str());

    if (ret != 0) {
        fprintf(stderr, "Error: g++ failed\n");
        return 1;
    }

    printf("Compiled: %s -> %s\n", srcPath, exePath.c_str());
    return 0;
}
