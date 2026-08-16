#include "ast.h"

int Module::visit_impl(SymTable& symtable) {
    if (statements) {
        return statements->visit(symtable);
    }
    return 0;
}

int Interactive::visit_impl(SymTable& symtable) {
    if (statements) {
        return statements->visit(symtable);
    }
    return 0;
}

int Name::visit_impl(SymTable& symtable) {
    return 0;
}

int Op::visit_impl(SymTable& symtable) {
    return 0;
}

int Literal::visit_impl(SymTable& symtable) {
    return 0;
}

int VariableDecl::visit_impl(SymTable& symtable) {
    TypeInfo* initType = nullptr;
    if (initializer) {
        initializer->visit(symtable);
        initType = initializer->inferred_type;
    }

    TypeInfo* declaredType = nullptr;
    if (typeAnnotation) {
        typeAnnotation->visit(symtable); // Type Node fill in inferred_type
        declaredType = typeAnnotation->inferred_type;
    }
    TypeInfo* finalType = declaredType ? declaredType : initType;
    if (!finalType) {
        ERROR(
            "Cannot infer type for Variable Declaration: " 
            << std::string(
                static_cast<Name*>(name)->name, 
                static_cast<Name*>(name)->len
            )
        );
        return -1;
    }
    Arena* ar = symtable.get_symbol_arena();
    Symbol* variable_symbol = ar->New<Symbol>();
    variable_symbol->name = std::string(
                static_cast<Name*>(name)->name, 
                static_cast<Name*>(name)->len
            );
    variable_symbol->kind = SymKind::Variable;
    variable_symbol->type = finalType;
    variable_symbol->isMutable = true;
    variable_symbol->declNode = this;
    symbol = variable_symbol;
    return symtable.insert(variable_symbol->name, variable_symbol) ? 0 : -1;
}

int ConstantDecl::visit_impl(SymTable& symtable) {
    if (value) {
        value->visit(symtable);
    }
    TypeInfo* type = value ? value->inferred_type : nullptr;

    Arena* ar = symtable.get_symbol_arena();
    Symbol* const_symbol = ar->New<Symbol>();
    const_symbol->name = static_cast<Name*>(name)->name;
    const_symbol->kind = SymKind::Constant;
    const_symbol->type = type;
    const_symbol->isMutable = false;
    const_symbol->isCompileTime = true;
    const_symbol->declNode = this;

    symbol = const_symbol;
    return symtable.insert(const_symbol->name, const_symbol) ? 0 : -1;
}

int FuncDecl::visit_impl(SymTable& symtable) {
    Arena* ar = symtable.get_symbol_arena();

    Symbol* funcSym = ar->New<Symbol>();
    funcSym->name = static_cast<Name*>(name)->name;
    funcSym->kind = SymKind::Function;
    funcSym->declNode = this;
    funcSym->isBuiltin = false;

    TypeInfo* funcType = ar->New<TypeInfo>();
    funcType->kind = TypeInfo::Kind::Function;

    if (params) {
        CompoundStmts* paramList = static_cast<CompoundStmts*>(params);
        for (ASTNode* pnode : paramList->stmts) {
            VariableDecl* paramDecl = static_cast<VariableDecl*>(pnode);
            if (!paramDecl) continue;

            if (paramDecl->typeAnnotation) {
                paramDecl->typeAnnotation->visit(symtable);
            }
            TypeInfo* paramType = paramDecl->typeAnnotation
                                  ? paramDecl->typeAnnotation->inferred_type
                                  : nullptr;
            if (!paramType) {
                paramType = ar->New<TypeInfo>();
                paramType->kind = TypeInfo::Kind::Unknown;
            }

            Symbol* paramSym = ar->New<Symbol>();
            paramSym->name = static_cast<Name*>(paramDecl->name)->name;
            paramSym->kind = SymKind::Parameter;
            paramSym->type = paramType;
            paramSym->isMutable = true;
            paramSym->declNode = paramDecl;

            funcType->params.push_back(paramSym);

            paramDecl->symbol = paramSym;
        }
    }

    TypeInfo* retType = nullptr;
    if (returnType) {
        returnType->visit(symtable);
        retType = returnType->inferred_type;
    } else {
        retType = ar->New<TypeInfo>();
        retType->kind = TypeInfo::Kind::Void;
    }
    funcType->returnType = retType;

    funcSym->type = funcType;

    if (!symtable.insert(funcSym->name, funcSym)) {
        ERROR("Redefinition of function: " << funcSym->name);
        return -1;
    }
    symbol = funcSym;

    // enter function body scope
    symtable.enterScope();
    if (params) {
        CompoundStmts* paramList = static_cast<CompoundStmts*>(params);
        for (ASTNode* pnode : paramList->stmts) {
            VariableDecl* paramDecl = static_cast<VariableDecl*>(pnode);
            if (paramDecl && paramDecl->symbol) {
                symtable.insert(paramDecl->symbol->name, paramDecl->symbol);
            }
        }
    }

    if (body) {
        body->visit(symtable);
    }

    symtable.exitScope();
    return 0;
}

int StructDecl::visit_impl(SymTable& symtable) {
    Arena* ar = symtable.get_symbol_arena();

    Symbol* struct_symbol = ar->New<Symbol>();
    struct_symbol->name = static_cast<Name*>(name)->name;
    struct_symbol->kind = SymKind::Type;
    struct_symbol->declNode = this;
    symbol = struct_symbol;

    TypeInfo* struct_type = ar->New<TypeInfo>();
    struct_type->kind = TypeInfo::Kind::Struct;
    struct_type->name = struct_symbol->name;
    struct_symbol->type = struct_type;

    for (VariableDecl* field : fields) {
        Symbol* field_symbol = ar->New<Symbol>();
        field_symbol->name = static_cast<Name*>(field->name)->name;
        field_symbol->kind = SymKind::Variable;
        field_symbol->declNode = field;

        if (field->typeAnnotation) {
            field->typeAnnotation->visit(symtable);
            field_symbol->type = field->typeAnnotation->inferred_type;
        }
        struct_type->fields.push_back(field_symbol);
    }

    return symtable.insert(struct_symbol->name, struct_symbol) ? 0 : -1;
}

int UsingDecl::visit_impl(SymTable& symtable) {
    return 0;
}

int CompoundStmts::visit_impl(SymTable& symtable) {
    for (auto* s : stmts) {
        if (s) s->visit(symtable);
    }
    return 0;
}

int SingleStmt::visit_impl(SymTable& symtable) {
    if (stmt) stmt->visit(symtable);
    return 0;
}

int BlockStmt::visit_impl(SymTable& symtable) {
    symtable.enterScope();
    if (stmts) stmts->visit(symtable);
    symtable.exitScope();
    return 0;
}

int ImportStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int IfStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int WhileStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int ForStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int ReturnStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int DeferStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int ExprStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int BreakStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int ContinueStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int CompileStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int BinaryExpr::visit_impl(SymTable& symtable) {
    if (!left || !right) return -1;
    if (left->visit(symtable) != 0) return -1;
    if (right->visit(symtable) != 0) return -1;

    TypeInfo* ltype = left->inferred_type;
    TypeInfo* rtype = right->inferred_type;
    if (!ltype || !rtype) return -1;

    // 比较运算符返回 bool
    OpType op_type = (op) ? static_cast<Op*>(op_node)->op_type : OpType::INVALID;
    switch (op_type) {
        case OpType::LESS:
        case OpType::GREATER:
        case OpType::LESSEQ:
        case OpType::GREATEREQ:
        case OpType::EQUAL:
        case OpType::UNEQUAL: {
            Arena* ar = symtable.get_symbol_arena();
            inferred_type = ar->New<TypeInfo>();
            inferred_type->kind = TypeInfo::Kind::Bool;
            return 0;
        }
        default:
            // TODO: check ltype and rtype compatibility
            inferred_type = ltype;
            return 0;
    }
}

int UnaryExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int CallExpr::visit_impl(SymTable& symtable) {
    if (!callee) return -1;
    if (callee->visit(symtable) != 0) {
        ERROR("Failed to analyze callee expression.");
        return -1;
    }
    TypeInfo* calleeType = callee->inferred_type;

    if (!calleeType || calleeType->kind != TypeInfo::Kind::Function) {
        ERROR("Callee is not a Function!");
        return -1;
    }
    for (auto* arg : arguments) {
        if (arg) {
            if (arg->visit(symtable) != 0) {
                ERROR("Failed to analyze arg symbol.");
                return -1;
            }
        }
    }
    inferred_type = calleeType->returnType;
    return 0;
}

int IndexExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int MemberAccessExpr::visit_impl(SymTable& symtable) {
    if (!object) return -1;
    object->visit(symtable);
    TypeInfo* obj_type = object->inferred_type;

    if (!obj_type || obj_type->kind != TypeInfo::Kind::Struct) {
        return -1;
    }
    std::string member_name = static_cast<Name*>(member)->name;
    for (Symbol* f : obj_type->fields) {
        if (f->name == member_name) {
            inferred_type = f->type;
            return 0;
        }
    }
    return -1;
}

int IdentifierExpr::visit_impl(SymTable& symtable) {
    Symbol* s = symtable.lookup(static_cast<Name*>(name)->name);
    if (!s) return -1;
    symbol = s;
    inferred_type = s->type;
    return 0;
}

int LiteralExpr::visit_impl(SymTable& symtable) {
    Arena* ar = symtable.get_symbol_arena();
    TypeInfo* t = ar->New<TypeInfo>();
    if (lit) {
        switch (lit->litType) {
            case Literal::LitType::Int:    t->kind = TypeInfo::Kind::Int;
            case Literal::LitType::Float:  t->kind = TypeInfo::Kind::Float;
            case Literal::LitType::Bool:   t->kind = TypeInfo::Kind::Bool;
            case Literal::LitType::String: t->kind = TypeInfo::Kind::String;
            case Literal::LitType::Char:   t->kind = TypeInfo::Kind::Char;
        }
        inferred_type = t;
    }
    return 0;
}

int CastExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int ArrayLiteralExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int NewExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int LambdaExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int IfExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int RunExpr::visit_impl(SymTable& symtable) {
    return 0;
}

int NamedType::visit_impl(SymTable& symtable) {
    return 0;
}

int PointerType::visit_impl(SymTable& symtable) {
    return 0;
}

int ArrayType::visit_impl(SymTable& symtable) {
    return 0;
}

int FunctionType::visit_impl(SymTable& symtable) {
    return 0;
}

