#include "symtable.h"
#include "logging.h"
#include <cstddef>

std::string TypeInfo::dump() {
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
        funcSym->kind = SymKind::Function;
        funcSym->type = funcType;
        
        funcType->kind = TypeInfo::Kind::Function;
        funcType->returnType = returnType;
        
        returnType->kind = TypeInfo::Kind::Void;
        insert(name, funcSym);
    }
}

bool SymTable::insert(const std::string& name, Symbol* symbol) {
    if (_scopes.empty()) return false;
    auto& scope = _scopes.back();
    auto& set = scope[name];

    if (symbol->kind == SymKind::Function) {
        // overload
        set.push_back(symbol);
    } else {
        if (!set.empty()) {
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

