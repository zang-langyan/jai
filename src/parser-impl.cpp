#include "ast.h"
#include "logging.h"
#include "parser.h"
#include "token.h"
#include <vector>


ASTNode* Parser::file_rule(){
    Module* res = nullptr;
    ASTNode* a = nullptr;
    size_t m = _mark;
    if (
        (a = statements()) &&
        (res || (res = _ast_ar->New<Module>()))
    ) {
        res->statements = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return res;
}

ASTNode* Parser::interactive_rule(){
    Interactive* res = nullptr;
    size_t m = _mark;
    ASTNode* a = nullptr;
    if (
        (a = statements())
        && (res || (res = _ast_ar->New<Interactive>()))
    ) {
        res->statements = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    // try expr
    if (
        (a = expression())
        && (res || (res = _ast_ar->New<Interactive>()))
    ) {
        res->statements = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

/* atom rules */
ASTNode* Parser::name_rule() {
    Name* res = nullptr;
    Token* name = nullptr;
    size_t m = _mark;
    if (
        (name = expect(TokenType::Identifier)) &&
        (res || (res = _ast_ar->New<Name>()))
    ) {
        res->name = name->data.lexeme;
        res->len = name->length;
        res->startPos = {name->pos.sr, name->pos.sc};
        res->endPos   = {name->pos.er, name->pos.ec};
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return res;
}

ASTNode* Parser::op_rule(TokenType exp, OpType target) {
    Op* res = nullptr;
    size_t m = _mark;
    if (
        expect(exp)
        && (res || (res = _ast_ar->New<Op>()))
    ) {
        res->op_type = target;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return res;
}

ASTNode* Parser::literal_rule() {
    Literal* res = nullptr;
    size_t m = _mark;
    Token* t;
    if (
        (t = expect(TokenType::ILiteral))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::Int;
        res->intVal = t->data.intValue;
    } else if (
        (t = expect(TokenType::FLiteral))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::Float;
        res->floatVal = t->data.floatValue;
    } else if (
        (t = expect(TokenType::CLiteral))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::Char;
        res->intVal = t->data.intValue;
    } else if (
        (t = expect(TokenType::SLiteral))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::String;
        res->stringVal = {.s = t->data.lexeme, .len = t->length};
    } else if (
        (t = expect(TokenType::TRUE))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::Bool;
        res->boolVal = true;
    } else if (
        (t = expect(TokenType::FALSE))
        && (res || (res = _ast_ar->New<Literal>()))
    ) {
        res->litType = Literal::LitType::Bool;
        res->boolVal = false;
    } else {
        // dumpTokens(_toks);
    }
    if (!res) {
        _mark = m;
        return nullptr;
    }
    res->startPos = {t->pos.sr, t->pos.sc};
    res->endPos   = {t->pos.er, t->pos.ec};
    DBPRINT(res->dump());
    return res;
}

ASTNode* Parser::statements(){
    CompoundStmts* res = nullptr;
    ASTNode* stmts = nullptr;
    size_t m = _mark;
    while (
        (stmts = statement()) &&
        (res || (res = _ast_ar->New<CompoundStmts>()))
    ) {
        res->stmts.emplace_back(stmts);
    }
    if (!res) {
        _mark = m;
        return nullptr;
    }
    DBPRINT(res->dump());
    return res;
}

ASTNode* Parser::statement(){
    SingleStmt* res = nullptr;
    ASTNode* a = nullptr;
    size_t m = _mark;
    if (
        (
            (a = import_stmt()) ||
            (a = func_def()) ||
            (a = struct_def()) ||
            (a = variable_decl()) ||
            (a = constant_decl()) ||
            (a = using_stmt()) ||
            (a = if_stmt()) ||
            (a = while_stmt()) ||
            (a = for_stmt()) ||
            (a = return_stmt()) ||
            (a = defer_stmt()) ||
            (a = break_stmt()) ||
            (a = continue_stmt()) ||
            (a = expr_stmt()) ||
            (a = compile_stmt())
        ) &&
        (res || (res = _ast_ar->New<SingleStmt>()))
    ) {
        res->stmt = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return res;
}

ASTNode* Parser::import_stmt(){
    ImportStmt* res = nullptr;
    size_t m = _mark;
    Literal* s = nullptr;
    if (
        expect(TokenType::SHARP)
        && expect(TokenType::IMPORT)
        && (
            (s = static_cast<Literal*>(literal_rule())) && 
            s->litType == Literal::LitType::String
        )
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<ImportStmt>()))
    ) {
        res->import_name = s;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::func_def(){
    FuncDecl* res = nullptr;
    ASTNode* func_name = nullptr;
    ASTNode* paramters = nullptr;
    ASTNode* ret_type = nullptr;
    ASTNode* body = nullptr;
    size_t m = _mark;
    if (
        (func_name = name_rule())
        && expect(TokenType::DCOLON)
        && expect(TokenType::LPAR)
        && ((paramters = params()) || true)
        && expect(TokenType::RPAR)
        && (
            (expect(TokenType::RARROW) && (ret_type = type_rule()))
            || true
        )
        && (body = block())
        && (res || (res = _ast_ar->New<FuncDecl>()))
    ) {
        res->name = func_name;
        res->params = paramters;
        res->returnType = ret_type;
        res->body = body;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::params(){
    size_t m = _mark;
    CompoundStmts* comp = _ast_ar->New<CompoundStmts>();
    ASTNode* p = param();
    if (!p) { _mark = m; return nullptr; }
    comp->stmts.push_back(p);
    while (expect(TokenType::COMMA)) {
        p = param();
        if (!p) break;
        comp->stmts.push_back(p);
    }
    return comp;
}

ASTNode* Parser::param(){
    size_t m = _mark;
    ASTNode* pname = nullptr;
    ASTNode* ptype = nullptr;
    VariableDecl* res = nullptr;
    if (
        (pname = name_rule())
        && expect(TokenType::COLON)
        && (ptype = type_rule())
        && (res || (res = _ast_ar->New<VariableDecl>()))
    ) {
        res->name = pname;
        res->typeAnnotation = ptype;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}


int Parser::struct_fields_loop(std::vector<VariableDecl*>& res) {
    VariableDecl* field = nullptr;
    while ((field = static_cast<VariableDecl*>(struct_field()))) {
        res.emplace_back(field);
    }
    return 0;
}

ASTNode* Parser::struct_def(){
    StructDecl* res = nullptr;
    size_t m = _mark;
    ASTNode* stname = nullptr;
    std::vector<VariableDecl*> fields;
    if ((stname = name_rule()) &&
        expect(TokenType::DCOLON) &&
        expect(TokenType::STRUCT) &&
        expect(TokenType::LBRACE) &&
        (0 == struct_fields_loop(fields)) &&
        expect(TokenType::RBRACE) &&
        (res || (res = _ast_ar->New<StructDecl>()))
    ) {
        res->name = stname;
        res->fields = fields;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::struct_field(){
    VariableDecl* res = nullptr;
    size_t m = _mark;
    ASTNode* fname = nullptr;
    ASTNode* ftype = nullptr;
    ASTNode* finit = nullptr;

    if (
        (fname = name_rule())
        && expect(TokenType::COLON)
        && (ftype = type_rule())
    ) {
        size_t mm = _mark;
        if (
            expect(TokenType::EQUAL)
            && (finit = expression())
            && expect(TokenType::SEMI)
            && (res || (res = _ast_ar->New<VariableDecl>()))
        ) {
            res->name = fname;
            res->typeAnnotation = ftype;
            res->initializer = finit;
            DBPRINT(res->dump());
            return res;
        }
        _mark = mm;
        if (
            expect(TokenType::SEMI)
            && (res || (res = _ast_ar->New<VariableDecl>()))
        ) {
            res->name = fname;
            res->typeAnnotation = ftype;
            res->initializer = nullptr;
            DBPRINT(res->dump());
            return res;
        }
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::variable_decl(){
    VariableDecl* res = nullptr;
    size_t m = _mark;
    ASTNode* vname = nullptr;
    ASTNode* vtype = nullptr;
    ASTNode* vinit = nullptr;
    if (
        (vname = name_rule())
        && expect(TokenType::COLONEQUAL)
        && (vinit = expression())
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<VariableDecl>()))
    ) {
        res->name = vname;
        res->typeAnnotation = vtype;
        res->initializer = vinit;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (vname = name_rule())
        && expect(TokenType::COLON)
        && (vtype = type_rule())
    ) {
        size_t mm = _mark;
        if (
            expect(TokenType::EQUAL)
            && (vinit = expression())
            && expect(TokenType::SEMI)
            && (res || (res = _ast_ar->New<VariableDecl>()))
        ) {
            res->name = vname;
            res->typeAnnotation = vtype;
            res->initializer = vinit;
            DBPRINT(res->dump());
            return res;
        }
        _mark = mm;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::constant_decl(){
    ConstantDecl* res = nullptr;
    size_t m = _mark;
    ASTNode* name  = nullptr;
    ASTNode* value = nullptr;
    if (
        (name = name_rule())
        && (expect(TokenType::DCOLON))
        && (value = expression())
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<ConstantDecl>()))
    ) {
        res->name = name;
        res->value = value;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::using_stmt(){
    UsingDecl* res = nullptr;
    size_t m = _mark;
    ASTNode* target = nullptr;
    if (
        expect(TokenType::USING)
        && (target = name_rule())
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<UsingDecl>()))
    ) {
        res->target = target;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::if_stmt(){
    IfStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* condition  = nullptr;
    ASTNode* thenBranch = nullptr;
    ASTNode* elseBranch = nullptr;
    if (
        expect(TokenType::IF)
        && (condition = expression())
        && (thenBranch = block())
    ) {
        size_t mm = _mark;
        if (expect(TokenType::ELSE)) {
            size_t mmm = _mark;
            if (
                (elseBranch = block())
                && (res || (res = _ast_ar->New<IfStmt>()))
            ) {
                res->condition = condition;
                res->thenBranch = thenBranch;
                res->elseBranch = elseBranch;
                DBPRINT(res->dump());
                return res;
            }
            _mark = mmm;
            if (
                (elseBranch = if_stmt())
                && (res || (res = _ast_ar->New<IfStmt>()))
            ) {
                res->condition = condition;
                res->thenBranch = thenBranch;
                res->elseBranch = elseBranch;
                DBPRINT(res->dump());
                return res;
            }
            // TODO: ERROR only 'else', no following
            _mark = mmm;
        }
        _mark = mm;
        if ((res || (res = _ast_ar->New<IfStmt>()))) {
            res->condition = condition;
            res->thenBranch = thenBranch;
            res->elseBranch = elseBranch;
            DBPRINT(res->dump());
            return res;
        }
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::while_stmt(){
    WhileStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* condition = nullptr;
    ASTNode* body      = nullptr;
    if (expect(TokenType::WHILE)
        && (condition = expression())
        && (body = block())
        && (res || (res = _ast_ar->New<WhileStmt>()))
    ) {
        res->condition = condition;
        res->body = body;
        DBPRINT(res->dump());
        return res;
    } 
    _mark = m;
    return nullptr;
}

ASTNode* Parser::for_stmt(){
    ForStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* init      = nullptr;
    ASTNode* condition = nullptr;
    ASTNode* increment = nullptr;
    ASTNode* body      = nullptr;
    if (
        expect(TokenType::FOR)
        && (init = expression())
        && expect(TokenType::SEMI)
        && (condition = expression())
        && expect(TokenType::SEMI)
        && (increment = expression())
        && (body = block())
        && (res || (res = _ast_ar->New<ForStmt>()))
    ) {
        res->init = init;
        res->condition = condition;
        res->increment = increment;
        res->body = body;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::return_stmt(){
    ReturnStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* expr = nullptr;
    if (
        expect(TokenType::RETURN)
        && (expr = expression())
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<ReturnStmt>()))
    ) {
        res->expr = expr;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::defer_stmt(){
    DeferStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* body = nullptr;
    if (
        expect(TokenType::RETURN)
        && (body = block())
        && (res || (res = _ast_ar->New<DeferStmt>()))
    ) {
        res->body = body;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::break_stmt(){
    BreakStmt* res = nullptr;
    size_t m = _mark;
    if (
        expect(TokenType::BREAK)
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<BreakStmt>()))
    ) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::continue_stmt(){
    ContinueStmt* res = nullptr;
    size_t m = _mark;
    if (
        expect(TokenType::CONTINUE)
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<ContinueStmt>()))
    ) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::expr_stmt(){
    ExprStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* expr = nullptr;
    if (
        (expr = expression())
        && expect(TokenType::SEMI)
        && (res || (res = _ast_ar->New<ExprStmt>()))
    ) {
        res->expr = expr;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::compile_stmt(){
    CompileStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* body;
    if (
        expect(TokenType::SHARP)
    ) {
        size_t mm = _mark;
        if (
            expect(TokenType::Identifier, "run")
            && (body = block())
            && (res || (res = _ast_ar->New<CompileStmt>()))
        ) {
            res->body = body;
            DBPRINT(res->dump());
            return res;
        }
        _mark = mm;
        if (
            expect(TokenType::Identifier, "insert")
            && (body = expression())
            && expect(TokenType::SEMI)
            && (res || (res = _ast_ar->New<CompileStmt>()))
        ) {
            res->body = body;
            DBPRINT(res->dump());
            return res;
        }
        _mark = mm;
        if (
            expect(TokenType::Identifier, "modify")
            && (body = expression())
            && expect(TokenType::SEMI)
            && (res || (res = _ast_ar->New<CompileStmt>()))
        ) {
            res->body = body;
            DBPRINT(res->dump());
            return res;
        }
        _mark = mm;
        // TODO: ERROR, must have following block or expression
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::block(){
    BlockStmt* res = nullptr;
    size_t m = _mark;
    ASTNode* stmts = nullptr;
    if (
        expect(TokenType::LBRACE)
        && (stmts = statements())
        && expect(TokenType::RBRACE)
        && (res || (res = _ast_ar->New<BlockStmt>()))
    ) {
        res->stmts = stmts;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::expression(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    if (
        (res = assignment())
    ) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::assignment(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = logical_or())
        && (op = op_rule(TokenType::EQUAL, OpType::ASSIGN))
        && (right = expression())
        && (res || (res = _ast_ar->New<BinaryExpr>()))
    ) {
        BinaryExpr* a = static_cast<BinaryExpr*>(res);
        a->left = left;
        a->op = op;
        a->right = right;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (res = logical_or())
    ) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::logical_or(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = logical_and())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                (op = op_rule(TokenType::DVBAR, OpType::OR))
                && (right = logical_and())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::logical_and(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = bitwise_or())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                (op = op_rule(TokenType::DAMPER, OpType::AND))
                && (right = bitwise_or())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::bitwise_or(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = bitwise_xor())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                (op = op_rule(TokenType::VBAR, OpType::BITOR))
                && (right = bitwise_xor())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::bitwise_xor(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = bitwise_and())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                (op = op_rule(TokenType::XOR, OpType::XOR))
                && (right = bitwise_and())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::bitwise_and(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = shift_expr())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                (op = op_rule(TokenType::AMPER, OpType::BITAND))
                && (right = shift_expr())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::shift_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = additive_expr())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                ( /* op_rule will unwind _mark if not match */
                    (op = op_rule(TokenType::LSHIFT, OpType::LSHIFT))
                    || (op = op_rule(TokenType::RSHIFT, OpType::RSHIFT))
                )
                && (right = additive_expr())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::additive_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = multiplicative_expr())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                ( /* op_rule will unwind _mark if not match */
                    (op = op_rule(TokenType::PLUS, OpType::PLUS))
                    || (op = op_rule(TokenType::MINUS, OpType::MINUS))
                )
                && (right = multiplicative_expr())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::multiplicative_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* left  = nullptr;
    ASTNode* op    = nullptr;
    ASTNode* right = nullptr;
    if (
        (left = unary_expr())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                ( /* op_rule will unwind _mark if not match */
                    (op = op_rule(TokenType::STAR, OpType::MULTIPLY))
                    || (op = op_rule(TokenType::SLASH, OpType::DIVIDE))
                    || (op = op_rule(TokenType::PERCENT, OpType::MOD))
                )
                && (right = unary_expr())
            ) {
                BinaryExpr* a = _ast_ar->New<BinaryExpr>();
                a->left = left;
                a->op = op;
                a->right = right;
                left = a;
                continue;
            } else {
                _mark = mm;
                break;
            }
        }
        res = left;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::unary_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* op  = nullptr;
    ASTNode* operand    = nullptr;
    if (
        (
            (op = op_rule(TokenType::MINUS, OpType::NEGATIVE))
            || (op = op_rule(TokenType::EXCLAMATION, OpType::NOT))
            || (op = op_rule(TokenType::LSHIFT, OpType::DEREF))
            || (op = op_rule(TokenType::STAR, OpType::ADDRESS))
        )
        && (operand = unary_expr())
        && (res || (res = _ast_ar->New<UnaryExpr>()))
    ) {
        UnaryExpr* a = static_cast<UnaryExpr*>(res);
        a->op = op;
        a->operand = operand;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (res = postfix_expr())
    ) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::postfix_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* primary = nullptr;
    ASTNode* postfix = nullptr;
    if (
        (primary = primary_expr())
    ) {
        while (true) {
            size_t mm = _mark;
            if (
                expect(TokenType::LPAR)
            ) {
                std::vector<ASTNode*> args;
                if (
                    (0 == arguments(args))
                    && expect(TokenType::RPAR)
                    && (res || (res = _ast_ar->New<CallExpr>()))
                ) {
                    CallExpr* a = static_cast<CallExpr*>(res);
                    a->callee = primary;
                    a->arguments = args;
                    primary = res;
                    continue;
                } else {
                    /* TODO ERROR */
                    ERROR("'(' is not closed.");
                    _mark = mm;
                    break;
                }
            }
            _mark = mm;
            if (
                expect(TokenType::LSQB)
            ) {
                ASTNode* index;
                if (
                    (index = expression())
                    && expect(TokenType::RSQB)
                    && (res || (res = _ast_ar->New<IndexExpr>()))
                ) {
                    IndexExpr* a = static_cast<IndexExpr*>(res);
                    a->base = primary;
                    a->index = index;
                    primary = res;
                    continue;
                } else {
                    /* TODO ERROR */
                    ERROR("'[' is not closed.");
                    _mark = mm;
                    break;
                }
            }
            _mark = mm;
            if (
                expect(TokenType::DOT)
            ) {
                ASTNode* name;
                if (
                    (name = name_rule())
                    && (res || (res = _ast_ar->New<MemberAccessExpr>()))
                ) {
                    MemberAccessExpr* a = static_cast<MemberAccessExpr*>(res);
                    a->object = primary;
                    a->member = name;
                    primary = res;
                    continue;
                } else {
                    /* TODO ERROR */
                    ERROR("not a valid member identifier.");
                    _mark = mm;
                    break;
                }
            }
            _mark = mm;
            break;
        }
        res = primary;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::primary_expr(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    ASTNode* a = nullptr;
    ASTNode* b = nullptr;
    if (
        (a = name_rule())
        && (res || (res = _ast_ar->New<IdentifierExpr>()))
    ){
        static_cast<IdentifierExpr*>(res)->name = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (a = literal_rule())
        && (res || (res = _ast_ar->New<LiteralExpr>()))
    ){
        static_cast<LiteralExpr*>(res)->lit = static_cast<Literal*>(a);
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        expect(TokenType::LPAR)
        && (a = expression())
        && expect(TokenType::RPAR)
    ){
        res = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        expect(TokenType::CAST)
        && expect(TokenType::LPAR)
        && (a = type_rule())
        && expect(TokenType::RPAR)
        && (b = expression())
        && (res || (res = _ast_ar->New<CastExpr>()))
    ){
        static_cast<CastExpr*>(res)->type = a;
        static_cast<CastExpr*>(res)->expr = b;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        expect(TokenType::NEW)
        && expect(TokenType::LPAR)
        && (a = type_rule())
        && expect(TokenType::RPAR)
        && (res || (res = _ast_ar->New<NewExpr>()))
    ){
        static_cast<NewExpr*>(res)->type = a;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (res = lambda_expr())
    ){
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (res = if_expr())
    ){
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if (
        (res = array_literal())
    ){
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

int Parser::arguments(std::vector<ASTNode*>& args){
    size_t m = _mark;
    ASTNode* a = expression();
    if (!a) { _mark = m; return 0; }
    args.emplace_back(a);
    while (expect(TokenType::COMMA)) {
        a = expression();
        if (!a) break;
        args.emplace_back(a);
    }
    return 0;
}

ASTNode* Parser::lambda_expr(){
    LambdaExpr* res = nullptr;
    size_t m = _mark;
    ASTNode* parameters = nullptr;
    ASTNode* returnType = nullptr;
    ASTNode* body       = nullptr;
    if (
        expect(TokenType::LPAR)
        && ((parameters = params()) || true)
        && expect(TokenType::RPAR)
        && expect(TokenType::RARROW)
        && ((returnType = type_rule()) || true)
        && (body = block())
        && (res || (res = _ast_ar->New<LambdaExpr>()))
    ) {
        res->parameters = parameters;
        res->returnType = returnType;
        res->body = body;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::if_expr(){
    IfExpr* res = nullptr;
    size_t m = _mark;
    ASTNode* condition = nullptr;
    ASTNode* thenExpr  = nullptr;
    ASTNode* elseExpr  = nullptr;
    if (
        expect(TokenType::IF)
        && (condition = expression())
        && (thenExpr = expression())
        && expect(TokenType::ELSE)
        && (condition = expression())
        && (res || (res = _ast_ar->New<IfExpr>()))
    ) {
        res->condition = condition;
        res->thenExpr = thenExpr;
        res->elseExpr = elseExpr;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::array_literal(){
    ArrayLiteralExpr* res = nullptr;
    size_t m = _mark;
    ASTNode* type;
    std::vector<ASTNode*> elements;
    if (
        ((type = type_rule()) || true)
        && expect(TokenType::DOT)
        && expect(TokenType::LSQB)
        && (0 == expression_list(elements))
        && expect(TokenType::RSQB)
        && (res || (res = _ast_ar->New<ArrayLiteralExpr>()))
    ) {
        res->type = type;
        res->elements = elements;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

int Parser::expression_list(std::vector<ASTNode*>& exprs){
    size_t m = _mark;
    ASTNode* a = expression();
    if (!a) { _mark = m; return 0; }
    exprs.emplace_back(a);
    while (expect(TokenType::COMMA)) {
        a = expression();
        if (!a) break;
        exprs.emplace_back(a);
    }
    return 0;
}

ASTNode* Parser::type_rule(){
    ASTNode* res = nullptr;
    size_t m = _mark;
    if ((res = named_type())) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if ((res = pointer_type())) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if ((res = array_type())) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    if ((res = function_type())) {
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::named_type(){
    NamedType* res = nullptr;
    size_t m = _mark;
    ASTNode* name = nullptr;
    if (
        (name = name_rule())
        && (res || (res = _ast_ar->New<NamedType>()))
    ) {
        res->name = name;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::pointer_type(){
    PointerType* res = nullptr;
    size_t m = _mark;
    ASTNode* baseType = nullptr;
    if (
        expect(TokenType::STAR)
        && (baseType = type_rule())
        && (res || (res = _ast_ar->New<PointerType>()))
    ) {
        res->baseType = baseType;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::array_type(){
    ArrayType* res = nullptr;
    size_t m = _mark;
    ASTNode* elementType = nullptr;
    ASTNode* sizeExpr    = nullptr;
    if (
        expect(TokenType::LSQB)
        && (
            expect(TokenType::DDOT)
            || (sizeExpr = expression())
            || true
        )
        && expect(TokenType::RSQB)
        && (elementType = type_rule())
        && (res || (res = _ast_ar->New<ArrayType>()))
    ) {
        res->elementType = elementType;
        res->sizeExpr = sizeExpr;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

ASTNode* Parser::function_type(){
    FunctionType* res = nullptr;
    size_t m = _mark;
    std::vector<ASTNode*> paramTypes;
    ASTNode* returnType = nullptr;
    if (
        expect(TokenType::LPAR)
        && (0 == type_list(paramTypes))
        && expect(TokenType::RPAR)
        && expect(TokenType::RARROW)
        && (returnType = type_rule())
        && (res || (res = _ast_ar->New<FunctionType>()))
    ) {
        res->paramTypes = paramTypes;
        res->returnType = returnType;
        DBPRINT(res->dump());
        return res;
    }
    _mark = m;
    return nullptr;
}

int Parser::type_list(std::vector<ASTNode*>& vt){
    size_t m = _mark;
    ASTNode* a = type_rule();
    if (!a) { _mark = m; return 0; }
    vt.emplace_back(a);
    while (expect(TokenType::COMMA)) {
        a = type_rule();
        if (!a) break;
        vt.emplace_back(a);
    }
    return 0;
}

