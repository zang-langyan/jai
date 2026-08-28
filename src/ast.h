#ifndef JAI_AST_H
#define JAI_AST_H
#include "logging.h"
#include "token.h"
#include "symtable.h"
#include <vector>

enum class ASTNodeType {
    // root
    Module,
    Interactive,

    // atom
    Name,
    Op,
    Literal,

    // declaration
    FuncDecl,
    StructDecl,
    StructFields,
    VariableDecl,          // name := value / name : Type = value
    ConstantDecl,          // name :: value
    UsingDecl,

    // statements
    CompoundStmts,
    ImportStmt,
    SingleStmt,
    BlockStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt,
    DeferStmt,
    ExprStmt,
    BreakStmt,
    ContinueStmt,
    CompileStmt,

    // expressions
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    IndexExpr,
    MemberAccessExpr,
    IdentifierExpr,
    LiteralExpr,
    CastExpr,              // cast(Type) expr
    ArrayLiteralExpr,      // .[ ... ] or Type.[ ... ]
    NewExpr,               // New(Type)
    LambdaExpr,
    IfExpr,
    RunExpr,               // #run expr

    // Type annotation
    NamedType,
    PointerType,
    ArrayType,
    FunctionType,

    ENDOFASTTYPE
};

struct ASTContext {
    static size_t depth;
    ASTContext() {
        ++depth;
    }
    ~ASTContext() {
        --depth;
    }
};

struct SourcePos {
    int line, col;
};

class ASTNode {
public:
    ASTNodeType type;
    SourcePos startPos;
    SourcePos endPos;
    TypeInfo* inferred_type; /* for variable or expression*/

    ASTNode(ASTNodeType t) : type(t), inferred_type(nullptr) {}
    virtual ~ASTNode() = default;

    std::string dump() {
        ASTContext context;
        return dump_impl(); 
    }

    int visit(SymTable& symtable) {
        return visit_impl(symtable);
    }
    int visit2(SymTable& symtable) {
        return visit_impl2(symtable);
    }

    template<typename T>
    T* codegen() {
        return static_cast<T*>(codegen_impl());
    }
private:
    virtual std::string dump_impl() { return ""; };
    virtual int visit_impl(SymTable& symtable) = 0;
    virtual int visit_impl2(SymTable& symtable) = 0;
    virtual void* codegen_impl() = 0;
};

/* Root Ast */
class Module : public ASTNode {
public:
    ASTNode* statements;

    Module() : ASTNode(ASTNodeType::Module) {}

private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[Module]:\n";
        res += statements->dump();
        return res; 
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class Interactive : public ASTNode {
public:
    ASTNode* statements;

    Interactive() : ASTNode(ASTNodeType::Interactive) {}

private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[Interactive]:\n";
        if (statements) res += statements->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

/* Atom*/
struct Name: public ASTNode {
    char* name;
    size_t len;

    Name(): ASTNode(ASTNodeType::Name) {}

private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[Name]: ";
        if (name) {
            res += std::string(name, len) + '\n';
        } else {
            res += "(null)\n";
        }
        return res; 
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

enum class OpType {
    // Unary
    NEGATIVE,
    POSITIVE,
    NOT,
    ADDRESS,
    DEREF,
    // Binary
    ASSIGN,
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MOD,
    LESS,
    GREATER,
    LSHIFT,
    RSHIFT,
    OR,
    AND,
    XOR,
    BITOR,
    BITAND,
    LESSEQ,
    GREATEREQ,
    EQUAL,
    UNEQUAL,
    RANGE, /* .. */
    INVALID
};

struct Op: public ASTNode {
    OpType op_type;

    Op(): ASTNode(ASTNodeType::Op) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[Op]: ";
        switch (op_type) {
            case OpType::NEGATIVE:      res += "- (unary)";             break;
            case OpType::POSITIVE:      res += "+ (unary)";             break;
            case OpType::NOT:           res += "! (not)";               break;
            case OpType::DEREF:         res += "<< (deref)";            break;
            case OpType::ADDRESS:       res += "* (address of)";        break;

            case OpType::ASSIGN:        res += "= (assign)";            break;
            case OpType::PLUS:          res += "+ (plus)";              break;
            case OpType::MINUS:         res += "- (minus)";             break;
            case OpType::MULTIPLY:      res += "* (multiply)";          break;
            case OpType::DIVIDE:        res += "/ (divide)";            break;
            case OpType::MOD:           res += "% (mod)";               break;
            case OpType::LESS:          res += "< (less)";              break;
            case OpType::GREATER:       res += "> (greater)";           break;
            case OpType::LSHIFT:        res += "<< (left shift)";       break;
            case OpType::RSHIFT:        res += ">> (right shift)";      break;
            case OpType::OR:            res += "|| (or)";               break;
            case OpType::AND:           res += "&& (and)";              break;
            case OpType::XOR:           res += "^ (xor)";               break;
            case OpType::BITOR:         res += "| (bit or)";            break;
            case OpType::BITAND:        res += "& (bit and)";           break;
            case OpType::LESSEQ:        res += "<= (less or equal)";    break;
            case OpType::GREATEREQ:     res += ">= (greater or equal)"; break;
            case OpType::EQUAL:         res += "== (greater or equal)"; break;
            case OpType::UNEQUAL:       res += "!= (greater or equal)"; break;
            case OpType::RANGE:         res += ".. (range)";            break;
            default:                    res += "???";                   break;
        }
        res += "\n";
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

enum class LitType { Int, Float, Bool, String, Char, JNull };
struct Literal: public ASTNode {
    LitType litType;
    union {
        int64_t  intVal;
        double   floatVal;
        bool     boolVal;
        struct {
            char* s;
            size_t len;
        } stringVal;
    };

    Literal(): ASTNode(ASTNodeType::Literal) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[Literal]: ";
        switch (litType) {
            case LitType::Int:    res += "[INT] " + std::to_string(intVal); break;
            case LitType::Float:  res += "[FLOAT] " + std::to_string(floatVal); break;
            case LitType::Bool:   res += "[BOOL] " + (boolVal ? std::string("true") : std::string("false")); break;
            case LitType::Char:   res += "[BOOL] '" + std::string(1, (char)intVal) + "'"; break;
            case LitType::String: res += "[STRING] \"" + escapeString(stringVal.s, stringVal.len) + "\""; break;
            case LitType::JNull:  res += "[JNULL]"; break;
        }
        res += "\n";
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

/* Declaration Ast */
class VariableDecl : public ASTNode {
public:
    ASTNode* name;
    ASTNode* typeAnnotation;
    ASTNode* initializer;

    Symbol* symbol;

    VariableDecl()
        : ASTNode(ASTNodeType::VariableDecl),
        name(nullptr), typeAnnotation(nullptr), initializer(nullptr),
        symbol(nullptr) 
        {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[VariableDecl]:\n";
        if (name) {
            res += name->dump();
        } else {
            ERROR("VariableDecl name member is null!");
        }
        if (typeAnnotation) {
            ASTContext ctx;
            std::string type_label(ASTContext::depth * 4, ' ');
            type_label += "[Type]:\n";
            res += type_label + typeAnnotation->dump();
        }
        if (initializer) {
            ASTContext ctx;
            std::string init_label(ASTContext::depth * 4, ' ');
            init_label += "[Initializer]:\n";
            res += init_label + initializer->dump();
        }
        if (symbol) {
            res += symbol->dump() + '\n';
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ConstantDecl : public ASTNode {
public:
    ASTNode* name;
    ASTNode* value;            // :: x，x must be a compile time constant

    Symbol* symbol;

    ConstantDecl()
        : ASTNode(ASTNodeType::ConstantDecl),
        name(nullptr), value(nullptr),
        symbol(nullptr)
        {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ConstantDecl]:\n";
        if (name) res += name->dump();
        if (value) {
            ASTContext ctx;
            std::string val_label(ASTContext::depth * 4, ' ');
            val_label += "[Value]:\n";
            res += val_label + value->dump();
        }
        if (symbol) {
            res += symbol->dump() + '\n';
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class FuncDecl : public ASTNode {
public:
    ASTNode* name;
    ASTNode* params;           // arg names + type
    ASTNode* returnType;
    ASTNode* body;             // function body (BlockStmt)

    Symbol* symbol;

    FuncDecl()
        : ASTNode(ASTNodeType::FuncDecl),
        name(nullptr), params(nullptr), returnType(nullptr), body(nullptr),
        symbol(nullptr)
        {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[FuncDecl]:\n";
        if (name) res += name->dump();
        if (params) {
            ASTContext ctx;
            std::string par_label(ASTContext::depth * 4, ' ');
            par_label += "[Params]:\n";
            res += par_label + params->dump();
        }
        if (returnType) {
            ASTContext ctx;
            std::string ret_label(ASTContext::depth * 4, ' ');
            ret_label += "[ReturnType]:\n";
            res += ret_label + returnType->dump();
        }
        if (body) {
            ASTContext ctx;
            std::string body_label(ASTContext::depth * 4, ' ');
            body_label += "[Body]:\n";
            res += body_label + body->dump();
        }
        if (symbol) {
            res += symbol->dump() + '\n';
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class StructDecl : public ASTNode {
public:
    ASTNode* name;
    std::vector<VariableDecl*> fields;  // member variables

    Symbol* symbol;

    StructDecl() : ASTNode(ASTNodeType::StructDecl),
    name(nullptr), symbol(nullptr)
    {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[StructDecl]:\n";
        if (name) res += name->dump();
        if (!fields.empty()) {
            ASTContext ctx;
            std::string fields_label(ASTContext::depth * 4, ' ');
            fields_label += "[Fields]:\n";
            res += fields_label;
            for (auto* f : fields) {
                if (f) res += f->dump();
            }
        }
        if (symbol) {
            res += symbol->dump() + '\n';
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class UsingDecl : public ASTNode {
public:
    ASTNode* target;       // using module names

    UsingDecl() : ASTNode(ASTNodeType::UsingDecl) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[UsingDecl]:\n";
        if (target) res += target->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

/* Statement Ast */
class CompoundStmts: public ASTNode {
public:
    std::vector<ASTNode*> stmts;
    
    CompoundStmts() : ASTNode(ASTNodeType::CompoundStmts) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[CompoundStmts]:\n";
        for (auto* s : stmts) {
            if (s) res += s->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class SingleStmt: public ASTNode {
public:
    ASTNode* stmt;

    SingleStmt(): ASTNode(ASTNodeType::SingleStmt) {} 
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[SingleStmt]:\n";
        if (stmt) {
            res += stmt->dump();
        };
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class BlockStmt : public ASTNode {
public:
    ASTNode* stmts; /* CompoundStmts */

    BlockStmt() : ASTNode(ASTNodeType::BlockStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[BlockStmt]:\n";
        if (stmts) {
            res += stmts->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ImportStmt: public ASTNode {
public:
    ASTNode* import_name;

    Symbol* symbol;
    ImportStmt(): ASTNode(ASTNodeType::ImportStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ImportStmt]:\n";
        if (import_name) res += import_name->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class IfStmt : public ASTNode {
public:
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;

    IfStmt(): ASTNode(ASTNodeType::IfStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[IfStmt]:\n";
        if (condition) {
            ASTContext ctx;
            std::string cond_label(ASTContext::depth * 4, ' ');
            cond_label += "[Condition]:\n";
            res += cond_label + condition->dump();
        }
        if (thenBranch) {
            ASTContext ctx;
            std::string then_label(ASTContext::depth * 4, ' ');
            then_label += "[Then]:\n";
            res += then_label + thenBranch->dump();
        }
        if (elseBranch) {
            ASTContext ctx;
            std::string else_label(ASTContext::depth * 4, ' ');
            else_label += "[Else]:\n";
            res += else_label + elseBranch->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class WhileStmt : public ASTNode {
public:
    ASTNode* condition;
    ASTNode* body;

    WhileStmt(): ASTNode(ASTNodeType::WhileStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[WhileStmt]:\n";
        if (condition) {
            ASTContext ctx;
            std::string cond_label(ASTContext::depth * 4, ' ');
            cond_label += "[Condition]:\n";
            res += cond_label + condition->dump();
        }
        if (body) {
            ASTContext ctx;
            std::string body_label(ASTContext::depth * 4, ' ');
            body_label += "[Body]:\n";
            res += body_label + body->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ForStmt : public ASTNode {
public:
    ASTNode* init;
    ASTNode* condition;
    ASTNode* increment;
    ASTNode* body;

    ForStmt(): ASTNode(ASTNodeType::ForStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ForStmt]:\n";
        if (init) {
            ASTContext ctx;
            std::string init_label(ASTContext::depth * 4, ' ');
            init_label += "[Init]:\n";
            res += init_label + init->dump();
        }
        if (condition) {
            ASTContext ctx;
            std::string cond_label(ASTContext::depth * 4, ' ');
            cond_label += "[Condition]:\n";
            res += cond_label + condition->dump();
        }
        if (increment) {
            ASTContext ctx;
            std::string inc_label(ASTContext::depth * 4, ' ');
            inc_label += "[Increment]:\n";
            res += inc_label + increment->dump();
        }
        if (body) {
            ASTContext ctx;
            std::string body_label(ASTContext::depth * 4, ' ');
            body_label += "[Body]:\n";
            res += body_label + body->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ReturnStmt : public ASTNode {
public:
    ASTNode* expr;

    ReturnStmt() : ASTNode(ASTNodeType::ReturnStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ReturnStmt]:";
        if (expr) {
            res += "\n";
            ASTContext ctx;
            std::string expr_label(ASTContext::depth * 4, ' ');
            expr_label += "[Expr]:\n";
            res += expr_label + expr->dump();
        } else {
            res += " (void)\n";
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class DeferStmt : public ASTNode {
public:
    ASTNode* body;

    DeferStmt() : ASTNode(ASTNodeType::DeferStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[DeferStmt]:\n";
        if (body) res += body->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ExprStmt : public ASTNode {
public:
    ASTNode* expr;

    ExprStmt() : ASTNode(ASTNodeType::ExprStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ExprStmt]:\n";
        if (expr) res += expr->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class BreakStmt : public ASTNode {
public:
    BreakStmt() : ASTNode(ASTNodeType::BreakStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[BreakStmt]\n";
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ContinueStmt : public ASTNode {
public:
    ContinueStmt() : ASTNode(ASTNodeType::ContinueStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ContinueStmt]\n";
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

enum class CompileStmtType {
    RUN,
    INSERT,
    MODIFY,
    UNKNOWN
};

class CompileStmt : public ASTNode {
public:
    CompileStmtType type;
    ASTNode* body;

    CompileStmt() : ASTNode(ASTNodeType::CompileStmt) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[CompileStmt]";
        switch (type)
        {
        case CompileStmtType::RUN:
            res += "(#run)\n";
            break;
        case CompileStmtType::INSERT:
            res += "(#insert)\n";
            break;
        case CompileStmtType::MODIFY:
            res += "(#modify)\n";
            break;
        default:
            res += "(#unknown)\n";
            break;
        }
        res += body->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

/* Expression Ast */
class BinaryExpr : public ASTNode {
public:
    ASTNode* op;
    ASTNode* left;
    ASTNode* right;

    BinaryExpr(): ASTNode(ASTNodeType::BinaryExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[BinaryExpr]:\n";
        if (op) {
            ASTContext ctx;
            std::string op_label(ASTContext::depth * 4, ' ');
            op_label += "[Operator]:\n";
            res += op_label + op->dump();
        }
        if (left) {
            ASTContext ctx;
            std::string left_label(ASTContext::depth * 4, ' ');
            left_label += "[Left]:\n";
            res += left_label + left->dump();
        }
        if (right) {
            ASTContext ctx;
            std::string right_label(ASTContext::depth * 4, ' ');
            right_label += "[Right]:\n";
            res += right_label + right->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class UnaryExpr : public ASTNode {
public:
    ASTNode* op;
    ASTNode* operand;

    UnaryExpr()
        : ASTNode(ASTNodeType::UnaryExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[UnaryExpr]:\n";
        if (op) {
            ASTContext ctx;
            std::string op_label(ASTContext::depth * 4, ' ');
            op_label += "[Operator]:\n";
            res += op_label + op->dump();
        }
        if (operand) {
            ASTContext ctx;
            std::string opnd_label(ASTContext::depth * 4, ' ');
            opnd_label += "[Operand]:\n";
            res += opnd_label + operand->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class CallExpr : public ASTNode {
public:
    ASTNode* callee;          // call a function expression
    std::vector<ASTNode*> arguments;

    CallExpr(): ASTNode(ASTNodeType::CallExpr){}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[CallExpr]:\n";
        if (callee) {
            ASTContext ctx;
            std::string callee_label(ASTContext::depth * 4, ' ');
            callee_label += "[Callee]:\n";
            res += callee_label + callee->dump();
        }
        if (!arguments.empty()) {
            ASTContext ctx;
            std::string args_label(ASTContext::depth * 4, ' ');
            args_label += "[Arguments]:\n";
            res += args_label;
            for (auto* a : arguments) {
                if (a) res += a->dump();
            }
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class IndexExpr : public ASTNode {
public:
    ASTNode* base;
    ASTNode* index;

    IndexExpr(): ASTNode(ASTNodeType::IndexExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[IndexExpr]:\n";
        if (base) {
            ASTContext ctx;
            std::string base_label(ASTContext::depth * 4, ' ');
            base_label += "[Base]:\n";
            res += base_label + base->dump();
        }
        if (index) {
            ASTContext ctx;
            std::string idx_label(ASTContext::depth * 4, ' ');
            idx_label += "[Index]:\n";
            res += idx_label + index->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class MemberAccessExpr : public ASTNode {
public:
    ASTNode* object;
    ASTNode* member;

    MemberAccessExpr(): ASTNode(ASTNodeType::MemberAccessExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[MemberAccessExpr]:\n";
        if (object) {
            ASTContext ctx;
            std::string obj_label(ASTContext::depth * 4, ' ');
            obj_label += "[Object]:\n";
            res += obj_label + object->dump();
        }
        if (member) {
            ASTContext ctx;
            std::string mem_label(ASTContext::depth * 4, ' ');
            mem_label += "[Member]:\n";
            res += mem_label + member->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class IdentifierExpr : public ASTNode {
public:
    ASTNode* name;

    Symbol* symbol; /* which symbol this identifier it refers to */

    IdentifierExpr(): ASTNode(ASTNodeType::IdentifierExpr),
    name(nullptr), symbol(nullptr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[IdentifierExpr]:\n";
        if (name) res += name->dump();
        if (symbol) {
            res += symbol->dump() + '\n';
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class LiteralExpr : public ASTNode {
public:
    Literal* lit;

    LiteralExpr() : ASTNode(ASTNodeType::LiteralExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[LiteralExpr]:\n";
        if (lit) res += lit->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class CastExpr : public ASTNode {
public:
    ASTNode* type;
    ASTNode* expr;

    CastExpr()
        : ASTNode(ASTNodeType::CastExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[CastExpr]:\n";
        if (type) {
            ASTContext ctx;
            std::string type_label(ASTContext::depth * 4, ' ');
            type_label += "[Type]:\n";
            res += type_label + type->dump();
        }
        if (expr) {
            ASTContext ctx;
            std::string expr_label(ASTContext::depth * 4, ' ');
            expr_label += "[Expr]:\n";
            res += expr_label + expr->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ArrayLiteralExpr : public ASTNode {
public:
    ASTNode* type;
    std::vector<ASTNode*> elements;

    ArrayLiteralExpr()
        : ASTNode(ASTNodeType::ArrayLiteralExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ArrayLiteralExpr]:\n";
        if (type) {
            ASTContext ctx;
            std::string type_label(ASTContext::depth * 4, ' ');
            type_label += "[Element Type]:\n";
            res += type_label;
            res += type->dump();
        }
        if (!elements.empty()) {
            ASTContext ctx;
            std::string elems_label(ASTContext::depth * 4, ' ');
            elems_label += "[Elements]:\n";
            res += elems_label;
            for (auto* e : elements) {
                if (e) res += e->dump();
            }
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class NewExpr : public ASTNode {
public:
    ASTNode* type;

    NewExpr(): ASTNode(ASTNodeType::NewExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[NewExpr]:\n";
        if (type) {
            ASTContext ctx;
            std::string type_label(ASTContext::depth * 4, ' ');
            type_label += "[Type]:\n";
            res += type_label + type->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class LambdaExpr : public ASTNode {
public:
    ASTNode* parameters;
    ASTNode* returnType;
    ASTNode* body;

    LambdaExpr(): ASTNode(ASTNodeType::LambdaExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[LambdaExpr]:\n";
        {
            ASTContext ctx;
            std::string par_label(ASTContext::depth * 4, ' ');
            par_label += "[Params]:";
            res += par_label;
            if (parameters) {
                res += '\n';
                res += parameters->dump();
            } else {
                res += "[No parameters]\n";
            }
        }
        {
            ASTContext ctx;
            std::string returnType_label(ASTContext::depth * 4, ' ');
            returnType_label += "[Return Type]:";
            res += returnType_label;
            if (returnType) {
                res += '\n';
                res += returnType->dump();
            } else {
                res += "(Void)\n";
            }
        }
        if (body) {
            ASTContext ctx;
            std::string body_label(ASTContext::depth * 4, ' ');
            body_label += "[Body]:\n";
            res += body_label + body->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class IfExpr : public ASTNode {
public:
    ASTNode* condition;
    ASTNode* thenExpr;
    ASTNode* elseExpr;

    IfExpr(): ASTNode(ASTNodeType::IfExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[IfExpr]:\n";
        if (condition) {
            ASTContext ctx;
            std::string cond_label(ASTContext::depth * 4, ' ');
            cond_label += "[Condition]:\n";
            res += cond_label + condition->dump();
        }
        if (thenExpr) {
            ASTContext ctx;
            std::string then_label(ASTContext::depth * 4, ' ');
            then_label += "[Then]:\n";
            res += then_label + thenExpr->dump();
        }
        if (elseExpr) {
            ASTContext ctx;
            std::string else_label(ASTContext::depth * 4, ' ');
            else_label += "[Else]:\n";
            res += else_label + elseExpr->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class RunExpr : public ASTNode {
public:
    ASTNode* body;   // expression / block excuted at compile time

    RunExpr() : ASTNode(ASTNodeType::RunExpr) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[RunExpr]:\n";
        if (body) res += body->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

/* Type Annotation */
class NamedType : public ASTNode {
public:
    ASTNode* name;

    NamedType() : ASTNode(ASTNodeType::NamedType) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[NamedType]:\n";
        if (name) res += name->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class PointerType : public ASTNode {
public:
    ASTNode* baseType;

    PointerType() : ASTNode(ASTNodeType::PointerType) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[PointerType]:\n";
        if (baseType) res += baseType->dump();
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class ArrayType : public ASTNode {
public:
    ASTNode* elementType;
    ASTNode* sizeExpr;   // nullptr means dynamic, otherwise is static

    ArrayType(): ASTNode(ASTNodeType::ArrayType) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[ArrayType]:\n";
        if (elementType) {
            ASTContext ctx;
            std::string elem_label(ASTContext::depth * 4, ' ');
            elem_label += "[ElementType]:\n";
            res += elem_label + elementType->dump();
        }
        if (sizeExpr) {
            ASTContext ctx;
            std::string size_label(ASTContext::depth * 4, ' ');
            size_label += "[Size]:\n";
            res += size_label + sizeExpr->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};

class FunctionType : public ASTNode {
public:
    std::vector<ASTNode*> paramTypes;
    ASTNode* returnType;

    FunctionType(): ASTNode(ASTNodeType::FunctionType) {}
private:
    virtual std::string dump_impl() override {
        std::string res(ASTContext::depth * 4, ' ');
        res += "[FunctionType]:\n";
        if (!paramTypes.empty()) {
            ASTContext ctx;
            std::string pt_label(ASTContext::depth * 4, ' ');
            pt_label += "[ParamTypes]:\n";
            res += pt_label;
            for (auto* pt : paramTypes) {
                if (pt) res += pt->dump();
            }
        }
        if (returnType) {
            ASTContext ctx;
            std::string ret_label(ASTContext::depth * 4, ' ');
            ret_label += "[ReturnType]:\n";
            res += ret_label + returnType->dump();
        }
        return res;
    }
    virtual int visit_impl(SymTable& symtable) override;
    virtual int visit_impl2(SymTable& symtable) override;
    virtual void* codegen_impl() override;
};
#endif