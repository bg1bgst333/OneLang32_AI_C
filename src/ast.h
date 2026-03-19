#ifndef ONE_AST_H
#define ONE_AST_H

#include <string>
#include <vector>

namespace one {

enum NodeKind {
    NODE_STRING_OUTPUT
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

struct Program {
    std::vector<Node*> stmts;
    ~Program() {
        for (size_t i = 0; i < stmts.size(); i++)
            delete stmts[i];
    }
};

} // namespace one

#endif
