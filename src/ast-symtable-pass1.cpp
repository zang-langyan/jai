#include "ast.h"
#include "symtable.h"

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
    if (finalType) {
        inferred_type = finalType;
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
    variable_symbol->scopeLevel = symtable.getScopeLevel();
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
    const_symbol->scopeLevel = symtable.getScopeLevel();
    const_symbol->declNode = this;

    symbol = const_symbol;
    return symtable.insert(const_symbol->name, const_symbol) ? 0 : -1;
}

int FuncDecl::visit_impl(SymTable& symtable) {
    Arena* ar = symtable.get_symbol_arena();

    Symbol* funcSym = ar->New<Symbol>();
    funcSym->name = static_cast<Name*>(name)->name;
    funcSym->kind = SymKind::Function;
    funcSym->scopeLevel = symtable.getScopeLevel();
    funcSym->declNode = this;
    funcSym->isBuiltin = false;

    TypeInfo* funcType = ar->New<TypeInfo>();
    funcType->name = static_cast<Name*>(name)->name;
    funcType->kind = TypeInfo::Kind::Function;

    funcType->name += "(";
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
                paramType->name = "unknown, ";
                paramType->kind = TypeInfo::Kind::Unknown;
            }
            funcType->name += paramType->name + ", ";

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
    funcType->name += ")";
    
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
    struct_symbol->scopeLevel = symtable.getScopeLevel();
    struct_symbol->declNode = this;
    symbol = struct_symbol;

    if (!symtable.insert(struct_symbol->name, struct_symbol)) {
        ERROR("Failed to Insert Struct Decl " << struct_symbol->name);
        return -1;
    }

    TypeInfo* struct_type = ar->New<TypeInfo>();
    struct_type->kind = TypeInfo::Kind::Struct;
    struct_type->name = struct_symbol->name;
    struct_symbol->type = struct_type;

    symtable.enterScope();
    for (VariableDecl* field : fields) {
        Symbol* field_symbol = ar->New<Symbol>();
        field_symbol->name = static_cast<Name*>(field->name)->name;
        field_symbol->kind = SymKind::Variable;
        field_symbol->declNode = field;

        field->visit(symtable);
        field_symbol->type = field->typeAnnotation->inferred_type;
        struct_type->fields.push_back(field_symbol);
    }
    symtable.exitScope();

    return 0;
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
    std::string modName = std::string(
        static_cast<Literal*>(import_name)->stringVal.s,
        static_cast<Literal*>(import_name)->stringVal.len
    );
    Symbol* modSym = symtable.get_symbol_arena()->New<Symbol>();
    modSym->name = modName;
    modSym->kind = SymKind::Module;
    modSym->type = nullptr;
    modSym->scopeLevel = symtable.getScopeLevel();
    modSym->declNode = this;
    symbol = modSym;
    return symtable.insert(modSym->name, modSym) ? 0 : -1;
}

int IfStmt::visit_impl(SymTable& symtable) {
    if (!condition || !thenBranch) return -1;
    condition->visit(symtable);
    if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
        ERROR("Condition must be bool");
        return -1;
    }
    thenBranch->visit(symtable);
    if (elseBranch) elseBranch->visit(symtable);
    return 0;
}

int WhileStmt::visit_impl(SymTable& symtable) {
    if (!condition || !body) return -1;
    condition->visit(symtable);
    if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
        ERROR("Condition must be bool");
        return -1;
    }
    body->visit(symtable);
    return 0;
}

int ForStmt::visit_impl(SymTable& symtable) {
    if (init) init->visit(symtable);
    if (condition) {
        condition->visit(symtable);
        if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
            ERROR("Condition must be bool");
            return -1;
        }
    }
    if (increment) increment->visit(symtable);
    if (body) body->visit(symtable);
    return 0;
}

int ReturnStmt::visit_impl(SymTable& symtable) {
    if (!expr) {
        return 0;
    }
    if (expr->visit(symtable) != 0) return -1;

    /* TODO: check return type */
    // TypeInfo* retType = symtable.get_current_function_return_type();
    // if (retType && !symtable.type_compatible(retType, expr->inferred_type)) {
    //     ERROR("Return type mismatch");
    //     return -1;
    // }
    return 0;
}

int DeferStmt::visit_impl(SymTable& symtable) {
    return 0;
}

int ExprStmt::visit_impl(SymTable& symtable) {
    if (!expr) {
        ERROR("expr in ExprStmt is null");
        return -1;
    }
    return expr->visit(symtable);
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

    OpType op_type = (op) ? static_cast<Op*>(op)->op_type : OpType::INVALID;
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
    }
    return 0;
}

int UnaryExpr::visit_impl(SymTable& symtable) {
    if (!operand) return -1;
    if (operand->visit(symtable) != 0) return -1;
    TypeInfo* operand_type = operand->inferred_type;

    OpType op_type = (op) ? static_cast<Op*>(op)->op_type : OpType::INVALID;
    switch (op_type) {
        case OpType::NOT:{
            Arena* ar = symtable.get_symbol_arena();
            inferred_type = ar->New<TypeInfo>();
            inferred_type->kind = TypeInfo::Kind::Bool;
            break;
        }
        case OpType::NEGATIVE:
        case OpType::POSITIVE:
            inferred_type = operand_type;
            break;
        case OpType::ADDRESS:{
            Arena* ar = symtable.get_symbol_arena();
            inferred_type = ar->New<TypeInfo>();
            inferred_type->kind = TypeInfo::Kind::Pointer;
            inferred_type->baseType = operand_type;
            break;
        }
        case OpType::DEREF:
            if (operand_type->kind == TypeInfo::Kind::Pointer && operand_type->baseType) {
                inferred_type = operand_type->baseType;
            } else {
                ERROR("operand is not a pointer");
                return -1;
            }
            break;
        default:
            return -1;
    }
    return 0;
}

int CallExpr::visit_impl(SymTable& symtable) {
    if (!callee) return -1;
    callee->visit(symtable);
    TypeInfo* calleeType = callee->inferred_type;

    if (!calleeType || calleeType->kind != TypeInfo::Kind::Function) {
        ERROR("Attempting to call a non-function" << this->dump());
        return -1;
    }

    std::vector<TypeInfo*> argTypes;
    for (auto* arg : arguments) {
        if (arg) {
            arg->visit(symtable);
            argTypes.push_back(arg->inferred_type);
        }
    }

    // if (argTypes.size() != calleeType->params.size()) {
    //     ERROR("Argument count mismatch");
    //     return -1;
    // }
    /* TODO: Check parameter signiture */
    // for (size_t i = 0; i < argTypes.size(); ++i) {
    //     if (!symtable.type_compatible(calleeType->params[i]->type, argTypes[i])) {
    //         ERROR("Argument type mismatch");
    //         return -1;
    //     }
    // }
    inferred_type = calleeType->returnType;
    return 0;
}

int IndexExpr::visit_impl(SymTable& symtable) {
    if (!base) {
        ERROR("base is null" << this->dump());
        return -1;
    }
    base->visit(symtable);
    inferred_type = base->inferred_type;
    index->visit(symtable);
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
    if (!lit) {
        ERROR("Literal is Null!");
    }
    inferred_type = makeLiteralType(symtable, lit->litType);
    return 0;
}

int CastExpr::visit_impl(SymTable& symtable) {
    if (type) {
        type->visit(symtable);
        inferred_type = type->inferred_type;
    }
    if (expr) {
        expr->visit(symtable);
    }
    return 0;
}

int ArrayLiteralExpr::visit_impl(SymTable& symtable) {
    if (!type) {
        ERROR("Expect a type in array literal expr");
    }
    type->visit(symtable);
    TypeInfo* eType = type->inferred_type;
    if (!eType) {
        ERROR("Fail to create eType");
        return -1;
    }
    Arena* ar = symtable.get_symbol_arena();
    inferred_type = ar->New<TypeInfo>();
    inferred_type->name = eType->name + "[](array)";
    inferred_type->kind = TypeInfo::Kind::Array;
    inferred_type->elemType = eType;
    inferred_type->arraySize = elements.size();
    return 0;
}

int NewExpr::visit_impl(SymTable& symtable) {
    if (!type) {
        ERROR("Expect a type in New Expression!");
        return -1;
    }
    type->visit(symtable);
    TypeInfo* target = type->inferred_type;
    if (!target) return -1;

    inferred_type = makePointerType(symtable, target);
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
    Symbol* s = symtable.lookup(static_cast<Name*>(name)->name);
    if (!s) return 0;
    if (s->kind != SymKind::Type) {
        ERROR("NAMED Type is not a Type identifier");
        return -1;
    }
    inferred_type = s->type;
    return 0;
}

int PointerType::visit_impl(SymTable& symtable) {
    if (!baseType) {
        ERROR("base type is Null");
        return -1;
    }
    baseType->visit(symtable);
    TypeInfo* base = baseType->inferred_type;

    inferred_type = makePointerType(symtable, base);
    return 0;    
}

int ArrayType::visit_impl(SymTable& symtable) {
    if (!elementType) return -1;
    elementType->visit(symtable);
    TypeInfo* elem = elementType->inferred_type;

    Arena* ar = symtable.get_symbol_arena();
    inferred_type = ar->New<TypeInfo>();
    inferred_type->kind = TypeInfo::Kind::Array;
    /* TODO Evalute array size */
    // inferred_type->arraySize = evaluate_size(sizeExpr);
    return 0;
}

int FunctionType::visit_impl(SymTable& symtable) {
    Arena* ar = symtable.get_symbol_arena();
    inferred_type = ar->New<TypeInfo>();
    inferred_type->kind = TypeInfo::Kind::Function;
    return 0;
}

