#include "symtable.h"
#include "logging.h"
#include <cstddef>

void SymTable::enterScope() {
    _scopes.emplace_back();
}

void SymTable::exitScope() {
    if (_scopes.empty()) {
        return;
    }
    _scopes.pop_back();
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

