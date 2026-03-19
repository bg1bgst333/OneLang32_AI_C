#ifndef ONE_AST_H
#define ONE_AST_H

#include <string>
#include <vector>

namespace one {

enum NodeKind {
    NODE_STRING_OUTPUT,
    NODE_NUMBER_OUTPUT,
    NODE_ASSIGN,      // x = 値
    NODE_VAR_OUTPUT,  // x (変数出力)
    NODE_INPUT        // x: (標準入力→変数)
};

struct Node {
    NodeKind kind;
    int line;
    explicit Node(NodeKind k, int l) : kind(k), line(l) {}
    virtual ~Node() {}
};

// 文字列出力ノード
struct StringOutputNode : public Node {
    std::string text;
    StringOutputNode(const std::string& t, int l)
        : Node(NODE_STRING_OUTPUT, l), text(t) {}
};

// 数値出力ノード
struct NumberOutputNode : public Node {
    std::string raw;
    bool isFloat;
    NumberOutputNode(const std::string& r, bool f, int l)
        : Node(NODE_NUMBER_OUTPUT, l), raw(r), isFloat(f) {}
};

// 変数の値の種類
enum ValKind { VAL_INT, VAL_FLOAT, VAL_STRING };

// 代入ノード: x = 値
struct AssignNode : public Node {
    std::string varName;
    ValKind valKind;
    std::string rawValue; // 数値はそのまま、文字列はクォートなし
    AssignNode(const std::string& name, ValKind vk, const std::string& val, int l)
        : Node(NODE_ASSIGN, l), varName(name), valKind(vk), rawValue(val) {}
};

// 変数出力ノード: x
struct VarOutputNode : public Node {
    std::string varName;
    VarOutputNode(const std::string& name, int l)
        : Node(NODE_VAR_OUTPUT, l), varName(name) {}
};

// 入力ノード: x:
struct InputNode : public Node {
    std::string varName;
    InputNode(const std::string& name, int l)
        : Node(NODE_INPUT, l), varName(name) {}
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
