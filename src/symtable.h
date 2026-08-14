#ifndef JAI_SYMTABLE_H
#define JAI_SYMTABLE_H
#include <string>
#include <vector>
#include <unordered_map>

class ASTNode;
class VariableDecl;
struct Symbol;

enum class SymKind {
    Module,
    Variable,
    Constant,
    Function,
    Parameter,
    Type,
};

struct TypeInfo {
    enum class Kind {
        Void, Int, Float, Bool, String, Char, Pointer, Array, Struct, Function, Unknown
    };
    Kind kind = Kind::Unknown;
    std::string name{};

    // for pointer
    TypeInfo* baseType = nullptr;

    // for array
    size_t arraySize = 0;

    // for function
    TypeInfo* returnType = nullptr;
    std::vector<Symbol*> params;

    // for struct
    std::vector<Symbol*> fields;
};

struct Symbol {
    std::string name;
    SymKind kind;

    /*
     * 1. Module: NULL
     * 2. Variable, Constant, Parameter: depict the type of itself
     * 3. Function: depict the function signiture
     * 4. Type: depicts the type (Pointer or Struct, both originated from base types)
     */
    TypeInfo* type;

    bool isMutable       = true;
    bool isCompileTime   = false;
    bool isBuiltin       = false;
    // bool isPublic        = true;

    size_t scopeLevel    = 0;
    ASTNode* declNode    = nullptr;
};

class SymTable {
public:
    using Scope = std::unordered_map<std::string, Symbol*>;
public:
    SymTable() {
        enterScope();
    }

    void enterScope();
    void exitScope();

    bool insert(const std::string& name, Symbol symbol);

    Symbol* lookup(const std::string& name);

    Symbol* lookupLocal(const std::string& name);

    Scope& globalScope();

    void resetToGlobal();
private:
    std::vector<Scope> _scopes;
};

#endif