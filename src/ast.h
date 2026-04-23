#ifndef ONE_AST_H
#define ONE_AST_H

#include <string>
#include <vector>

namespace one {

enum NodeKind {
    NODE_STRING_OUTPUT,
    NODE_NUMBER_OUTPUT,
    NODE_ASSIGN,       // x = 値
    NODE_VAR_OUTPUT,   // x (変数出力)
    NODE_INPUT,        // x: (標準入力→変数)
    NODE_EXPR_OUTPUT,  // 式の評価・出力
    NODE_EXPR_ASSIGN,  // 変数 = 式（代入）
    NODE_COND          // 条件 -> 実行
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

// ── 式ノード ──────────────────────────────────────────────

enum ExprKind { EXPR_NUMBER, EXPR_VAR, EXPR_BINARY };

struct Expr {
    ExprKind kind;
    explicit Expr(ExprKind k) : kind(k) {}
    virtual ~Expr() {}
};

// 数値リテラル式
struct NumberExpr : Expr {
    std::string raw;
    bool isFloat;
    NumberExpr(const std::string& r, bool f)
        : Expr(EXPR_NUMBER), raw(r), isFloat(f) {}
};

// 変数参照式
struct VarExpr : Expr {
    std::string name;
    explicit VarExpr(const std::string& n) : Expr(EXPR_VAR), name(n) {}
};

// 二項演算式
struct BinaryExpr : Expr {
    char op; // '+' '-' '*' '/'
    Expr* left;
    Expr* right;
    BinaryExpr(char o, Expr* l, Expr* r)
        : Expr(EXPR_BINARY), op(o), left(l), right(r) {}
    ~BinaryExpr() { delete left; delete right; }
};

// 式を評価して出力するノード
struct ExprOutputNode : Node {
    Expr* expr;
    ExprOutputNode(Expr* e, int l) : Node(NODE_EXPR_OUTPUT, l), expr(e) {}
    ~ExprOutputNode() { delete expr; }
};

// 変数 = 式（代入）ノード
struct ExprAssignNode : Node {
    std::string varName;
    Expr* expr;
    ExprAssignNode(const std::string& n, Expr* e, int l)
        : Node(NODE_EXPR_ASSIGN, l), varName(n), expr(e) {}
    ~ExprAssignNode() { delete expr; }
};

// 比較演算子
enum CompOp { CMP_EQ, CMP_NEQ, CMP_GT, CMP_LT, CMP_GTE, CMP_LTE };

// 条件実行ノード: 式 比較演算子 式 -> 実行内容
struct CondNode : public Node {
    Expr* left;
    CompOp op;
    Expr* right;
    Node* body;
    CondNode(Expr* l, CompOp o, Expr* r, Node* b, int line)
        : Node(NODE_COND, line), left(l), op(o), right(r), body(b) {}
    ~CondNode() { delete left; delete right; delete body; }
};

// ─────────────────────────────────────────────────────────

struct Program {
    std::vector<Node*> stmts;
    ~Program() {
        for (size_t i = 0; i < stmts.size(); i++)
            delete stmts[i];
    }
};

} // namespace one

#endif
