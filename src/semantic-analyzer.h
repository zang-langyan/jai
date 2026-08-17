#ifndef JAI_SEMANTIC_ANALYZER_H
#define JAI_SEMANTIC_ANALYZER_H
#include "arena.h"
#include "ast.h"
#include "symtable.h"
#include "logging.h"

class SemanticAnalyzer {
private:
    Arena* _ar_symbol {nullptr};
    SymTable _symtable;
public:
    SemanticAnalyzer() = default;

    int set_symbol_arena(Arena* ar) {
        if (!ar) {
            ERROR("Symbol Arena is Null!");
            return -1;
        }
        _ar_symbol = ar;
        _symtable.set_symbol_arena(_ar_symbol);
        return 0;
    }

    int analyze(ASTNode* root) {
        _symtable.add_builtin();
        return root->visit(_symtable);
    }
};

#endif