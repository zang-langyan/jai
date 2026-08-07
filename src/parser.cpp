#include "parser.h"
#include "logging.h"
#include "token.h"
#include <cstddef>

size_t ASTContext::depth = -1;

ASTNode* Parser::parse(const std::vector<Token*>& tokens) {
    if (!_initialized) {
        ERROR("Parser not initialized!");
        return nullptr;
    }
    if (!_ast_ar) {
        ERROR("Ast Arena is null!");
    }
    if (!_token_ar) {
        ERROR("Tokens Arena is null!");
    }
    _toks = tokens;
    switch (_type) {
    case CompilerType::File:
        return file_rule();
    case CompilerType::Interactive:
        return interactive_rule();
    default:
        DEBUG("Unknown Compiler Type.");
    }
}

ASTNode* Parser::parse() {
    // std::vector<Token> tokseq;
    // _l.scan(&tokseq);
    // dumpTokens(tokseq);
    // return nullptr;
    if (!_initialized) {
        ERROR("Parser not initialized!");
        return nullptr;
    }
    if (!_ast_ar) {
        ERROR("Ast Arena is null!");
    }
    if (!_token_ar) {
        ERROR("Tokens Arena is null!");
    }

    _mark = 0;
    _toks.clear();

    switch (_type) {
    case CompilerType::File:
        return file_rule();
    case CompilerType::Interactive:
        return interactive_rule();
    default:
        DEBUG("Unknown Compiler Type.");
    }
    return nullptr;
}

void Parser::set_type(CompilerType t) {
    _type = t;
}

int Parser::set_ast_ar(Arena* a) {
    if (!a) {
        ERROR("Failed to set AST Arena, a is nullptr");
        return -1;
    }
    _ast_ar = a;
    return 0;
}

int Parser::set_token_ar(Arena* a) {
    if (!a) {
        ERROR("Failed to set Tokens Arena, a is nullptr");
        return -1;
    }
    _token_ar = a;
    return 0;
}

Token* Parser::peek() {
    if (_toks.size() > _mark) {
        return _toks[_mark];
    }
    Token* t = nullptr;
    while (!t || t->ignore) {
        t = _token_ar->New<Token>();
        _l.getNextToken(t);
    }
    _toks.emplace_back(t);
    return t;
}

Token* Parser::advance() {
    int mk = _mark;
    ++_mark;
    if (_toks.size() > mk) {
        return _toks[mk];
    }
    Token* t = nullptr;
    while (!t || t->ignore) {
        t = _token_ar->New<Token>();
        _l.getNextToken(t);
    }
    _toks.emplace_back(t);
    return t;
}

Token* Parser::expect(TokenType ty) {
    Token* t = nullptr;
    if (peek()->type == ty) {
        t = advance();
    }
    return t;
}

Token* Parser::expect(TokenType ty, const char* target) {
    Token* t = nullptr;
    if (peek()->type == ty) {
        Token* candidate = advance();
        if (strcmp(candidate->data.lexeme, target) == 0) {
            t = candidate;
        }
    }
    return t;
}