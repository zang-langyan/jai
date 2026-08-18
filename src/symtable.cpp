#include "symtable.h"
#include "logging.h"
#include <cstddef>
#include <tuple>

std::string TypeInfo::dump() {
    if (_dumping) {
        return "(TypeInfo: " + name + " [recursive])";
    }
    _dumping = true;
    std::string res = "(TypeInfo [" + name + "], [";
    switch (kind)
    {
    case Kind::Void:     res += "Void";      break;
    case Kind::Int:      res += "Int";       break;
    case Kind::Float:    res += "Float";     break;
    case Kind::Bool:     res += "Bool";      break;
    case Kind::String:   res += "String";    break;
    case Kind::Char:     res += "Char";      break;
    case Kind::Pointer:  res += "Pointer of Base Type " + baseType->dump();   break;
    case Kind::Array:    res += "Array";     break;
    case Kind::Struct:   {
        res += "Struct";
        for (auto* f: fields) {
            res += " " + f->dump();
        }
        break;
    }
    case Kind::Function: {
        res += "Function";
        if (returnType) {
            res += " return type: " + returnType->dump();
        }
        if (params.size() > 0) {
            res += " params: ";
        }
        for (auto* p: params) {
            res += p->dump();
        }
        break;
    }
    case Kind::Unknown:  res += "Unknown";   break;
    default:
        break;
    }
    res += "])";
    _dumping = false;
    return res;
}

std::string Symbol::dump() {
    std::string res = "(Symbol [" + name + "], [";
    switch (kind)
    {
    case SymKind::Module:    res += "Module";     break;
    case SymKind::Variable:  res += "Variable";   break;
    case SymKind::Constant:  res += "Constant";   break;
    case SymKind::Function:  res += "Function";   break;
    case SymKind::Parameter: res += "Parameter";  break;
    case SymKind::Type:      res += "Type";       break;
    default:
        break;
    }
    res += "]";
    if (type) {
        res += type->dump();
    } else {
        res += "[No Type]";
    }
    return res;
}

void SymTable::enterScope() {
    _scopes.emplace_back();
}

void SymTable::exitScope() {
    if (_scopes.empty()) {
        return;
    }
    _scopes.pop_back();
}

void SymTable::add_builtin() {
    std::vector<std::string> builtin_funcs {
        "print",
        "free",
    };
    for (const std::string& name: builtin_funcs) {
        Symbol* funcSym = _ar_symbol->New<Symbol>();
        TypeInfo* funcType = _ar_symbol->New<TypeInfo>();
        TypeInfo* returnType = _ar_symbol->New<TypeInfo>();
        funcSym->name = name;
        funcSym->kind = SymKind::Function;
        funcSym->type = funcType;
        
        funcType->name = name;
        funcType->kind = TypeInfo::Kind::Function;
        funcType->returnType = returnType;
        
        returnType->kind = TypeInfo::Kind::Void;
        insert(name, funcSym);
    }
    std::vector<std::tuple<std::string, TypeInfo::Kind>> builtin_types {
        {"s64", TypeInfo::Kind::Int},
    };
    for (const auto& [name, kind]: builtin_types) {
        Symbol* typeSym = _ar_symbol->New<Symbol>();
        TypeInfo* typeType = _ar_symbol->New<TypeInfo>();
        typeSym->name = name;
        typeSym->kind = SymKind::Type;
        typeSym->type = typeType;
        
        typeType->name = name;
        typeType->kind = kind;
        
        insert(name, typeSym);
    }
}

bool SymTable::insert(const std::string& name, Symbol* symbol) {
    if (_scopes.empty()) {
        ERROR("symtable is empty " << name);
        return false;
    }
    auto& scope = _scopes.back();
    auto& set = scope[name];

    if (symbol->kind == SymKind::Function) {
        // overload
        set.push_back(symbol);
    } else {
        if (!set.empty()) {
            ERROR(name << " is not empty in symtable" << ToString(_scopes));
            return false;
        }
        set.push_back(symbol);
    }
    symbol->scopeLevel = _scopes.size() - 1;
    return true;
}

Symbol* SymTable::lookup(const std::string& name) {
    int cur = _scopes.size();
    while (cur-- > 0) {
        const Scope& tbl = _scopes[cur];
        auto it = tbl.find(name);
        if (it != tbl.end()) {
            return it->second[0];
        }
    }
    return nullptr;
}

Symbol* SymTable::lookupLocal(const std::string& name) {
    if (
        !_scopes.empty()
    ) {
        auto it = _scopes.back().find(name);
        if (it != _scopes.back().end()) {
            return it->second[0];
        }
    }
    return nullptr;
}

SymTable::Scope* SymTable::globalScope() {
    if (_scopes.empty()) {
        ERROR("_scopes is empty, failed to get global scope.");
        return nullptr;
    }
    return &(*_scopes.begin());
}

void SymTable::resetToGlobal() {
    _scopes.resize(1);
}

