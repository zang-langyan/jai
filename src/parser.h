#ifndef JAI_PARSER_H
#define JAI_PARSER_H
#include <stdio.h>
#include <vector>
#include "arena.h"
#include "ast.h"
#include "token.h"
#include "lexer.h"
#include "state.h"

class Parser {
    FILE* _f;
    Arena* _ast_ar;
    Arena* _token_ar;
    size_t _mark = 0;
    std::vector<Token*> _toks;
    CompilerType _type;
    Lexer _l;
    bool _initialized = false;
public:
    Parser() = default;
    ~Parser() = default;

    int initialize(FILE* fp, CompilerType t, Arena* lexer_ar, Arena* tok_ar, Arena* ast_ar) {
        if (fp) {
            _l.setfile(fp);
            _f = fp;
        }
        if (_l.setArena(lexer_ar)) {
            return -1;
        }
        set_type(t);
        if (set_ast_ar(ast_ar)){
            return -1;
        }
        if (set_token_ar(tok_ar)) {
            return -1;
        }
        _initialized = true;
        return 0;
    }

    void setsource(const char* src, size_t n) {
        _l.setsource(src, n);
    }

    void set_type(CompilerType t);
    int set_ast_ar(Arena* a);
    int set_token_ar(Arena* a);
    ASTNode* parse(const std::vector<Token*>& tokens);
    ASTNode* parse();
private:
    Token* peek();
    Token* advance();
    Token* expect(TokenType);
    Token* expect(TokenType, const char*);
private:
    #include "parser-impl.h"
};
#endif