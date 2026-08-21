#ifndef JAI_SYMTABLE_H
#define JAI_SYMTABLE_H
#include "arena.h"
#include "logging.h"
#include <cstddef>
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
    TypeInfo* elemType = nullptr;
    size_t arraySize = 0;

    // for function
    TypeInfo* returnType = nullptr;
    std::vector<Symbol*> params;

    // for struct
    std::vector<Symbol*> fields;

    std::string dump();
private:
    bool _dumping = false; /* mark is during dumping */
};

bool sameType(TypeInfo* t1, TypeInfo* t2);
bool convertableType(TypeInfo* t1, TypeInfo* t2);

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

    std::string dump();
};

class SymTable {
public:
    using OverloadSet = std::vector<Symbol*>;
    using Scope = std::unordered_map<std::string, OverloadSet>;
    Arena* _ar_symbol {nullptr};
public:
    SymTable() {
        enterScope();
    }
    int set_symbol_arena(Arena* ar) {
        _ar_symbol = ar;
        return 0;
    }
    Arena* get_symbol_arena() {
        if (!_ar_symbol) {
            ERROR("Symbol Arena in SymTable is NULL!");
            return nullptr;
        }
        return _ar_symbol;
    }

    void enterScope();
    void exitScope();

    bool insert(const std::string& name, Symbol* symbol);

    void set_current_function_return_type(TypeInfo*);
    TypeInfo* get_current_function_return_type();

    Symbol* lookup(const std::string& name);

    Symbol* lookupLocal(const std::string& name);

    Symbol* lookupOverload(const std::string& name, const std::vector<TypeInfo*>& sig);
    
    Scope* globalScope();

    void resetToGlobal();
    void reset();

    void add_builtin();
    int getScopeLevel();

    const std::vector<Scope>& get_environments() const {
        return _scopes;
    };
private:
    std::vector<Scope> _scopes;
    TypeInfo* _current_function_return_type = nullptr;
};

#endif