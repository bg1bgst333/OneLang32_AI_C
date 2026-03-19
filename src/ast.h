#ifndef ONE_AST_H
#define ONE_AST_H

#include <string>
#include <vector>

namespace one {

enum NodeKind {
    NODE_STRING_OUTPUT,
    NODE_NUMBER_OUTPUT
};

struct Node {
    NodeKind kind;
    int line;
    explicit Node(NodeKind k, int l) : kind(k), line(l) {}
    virtual ~Node() {}
};

// 文字列出力ノード
struct StringOutputNode : public Node {
    std::string text; // クォートを除いた文字列
    StringOutputNode(const std::string& t, int l)
        : Node(NODE_STRING_OUTPUT, l), text(t) {}
};

// 数値出力ノード
struct NumberOutputNode : public Node {
    std::string raw; // 元のテキスト (例: "42", "3.14")
    bool isFloat;
    NumberOutputNode(const std::string& r, bool f, int l)
        : Node(NODE_NUMBER_OUTPUT, l), raw(r), isFloat(f) {}
};

struct Program {
    std::vector<Node*> stmts;
    ~Program() {
        for (size_t i = 0; i < stmts.size(); i++)
            delete stmts[i];
    }
};

} // namespace one

#endif
