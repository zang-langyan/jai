#include "ast.h"
#include "symtable.h"

int Module::visit_impl2(SymTable& symtable) {
    if (statements) {
        return statements->visit2(symtable);
    }
    return 0;
}

int Interactive::visit_impl2(SymTable& symtable) {
    if (statements) {
        return statements->visit2(symtable);
    }
    return 0;
}

int Name::visit_impl2(SymTable& symtable) {
    return 0;
}

int Op::visit_impl2(SymTable& symtable) {
    return 0;
}

int Literal::visit_impl2(SymTable& symtable) {
    return 0;
}

int VariableDecl::visit_impl2(SymTable& symtable) {
    if (!symbol) {
        ERROR("symbol not constructed in pass 1");
    }

    if (symtable.getScopeLevel() > 0) {
        if (!symtable.insert(symbol->name, symbol)) {
            ERROR("Failed to insert variable decl to symtable. " << symbol->name << ": " << symbol);
        }
    }

    // if (!inferred_type || inferred_type->kind == TypeInfo::Kind::Unknown) {
        TypeInfo* initType = nullptr;
        if (initializer) {
            initializer->visit2(symtable);
            initType = initializer->inferred_type;
        }

        TypeInfo* declaredType = nullptr;
        if (typeAnnotation) {
            typeAnnotation->visit2(symtable);
            declaredType = typeAnnotation->inferred_type;
        }
        if (initType && initType->kind != TypeInfo::Kind::Unknown
            && declaredType && declaredType->kind != TypeInfo::Kind::Unknown
            && !sameType(initType, declaredType)) {
            ERROR("VariableDecl Type mismatch: " << initType->dump() << " != " << declaredType->dump());
        }
        TypeInfo* finalType = declaredType ? declaredType : initType;
        if (!finalType) {
            ERROR(
                "Cannot infer type for Variable Declaration: " 
                << this->dump()
            );
            return -1;
        }
        inferred_type = finalType;
        if (inferred_type->kind == TypeInfo::Kind::Unknown) {
            ERROR("Inferred Type is still unknown");
            return -1;
        }
    // }

    return 0;
}

int ConstantDecl::visit_impl2(SymTable& symtable) {
    if (!symbol) {
        ERROR("symbol not constructed in pass 1");
    }
    if (symtable.getScopeLevel() > 0) {
        if (!symtable.insert(symbol->name, symbol)) {
            ERROR("Failed to insert variable decl to symtable.");
        }
    }
    if (!inferred_type || inferred_type->kind == TypeInfo::Kind::Unknown) {
        if (value) {
            value->visit2(symtable);
        }
        TypeInfo* type = value ? value->inferred_type : nullptr;
    }
    return 0;
}

int FuncDecl::visit_impl2(SymTable& symtable) {
    if (!symbol) {
        ERROR("symbol not constructed in pass 1");
    }
    if (symtable.getScopeLevel() > 0) {
        if (!symtable.insert(symbol->name, symbol)) {
            ERROR("Failed to insert variable decl to symtable.");
        }
    }
    // TODO: move function name construct to here pass 2
    TypeInfo* funcType = symbol->type;
    if (params) {
        CompoundStmts* paramList = static_cast<CompoundStmts*>(params);
        for (int i = 0; i < funcType->params.size(); ++i) {
            Symbol* p = funcType->params[i];
            if (!(p->type) || p->type->kind == TypeInfo::Kind::Unknown) {
                VariableDecl* paramDecl = static_cast<VariableDecl*>(paramList->stmts[i]);
                paramDecl->visit2(symtable);
                TypeInfo* paramType = paramDecl->inferred_type
                                    ? paramDecl->inferred_type
                                    : nullptr;
                if (!paramType) {
                    ERROR("Parameter " << i << " type not found");
                    return -1;
                }
            }
        }
    }

    if (!(funcType->returnType)) {
        ERROR("Unlikely to happen, someone altered the function symbol.");
        return -1;
    }
    TypeInfo* retType = nullptr;
    if (funcType->returnType->kind == TypeInfo::Kind::Unknown) {
        if (returnType) {
            returnType->visit2(symtable);
            retType = returnType->inferred_type;
        }
        if (!retType || retType->kind == TypeInfo::Kind::Unknown) {
            ERROR("Cannot find return type symbol in pass 2!");
        }
        funcType->returnType = retType;
    }

    // enter function body scope
    symtable.enterScope();
    if (params) {
        CompoundStmts* paramList = static_cast<CompoundStmts*>(params);
        for (ASTNode* pnode : paramList->stmts) {
            VariableDecl* paramDecl = static_cast<VariableDecl*>(pnode);
            if (paramDecl && paramDecl->symbol) {
                if (!symtable.insert(paramDecl->symbol->name, paramDecl->symbol)) {
                    ERROR("Failed to insert " << paramDecl->symbol->dump());
                    return -1;
                }
            }
        }
    }
    
    TypeInfo* oldRetType = symtable.get_current_function_return_type();
    symtable.set_current_function_return_type(funcType->returnType);
    if (body) {
        body->visit2(symtable);
    }

    symtable.exitScope();
    symtable.set_current_function_return_type(oldRetType);
    return 0;
}

int StructDecl::visit_impl2(SymTable& symtable) {
    if (!symbol) {
        ERROR("symbol not constructed in pass 1");
    }
    if (symtable.getScopeLevel() > 0) {
        if (!symtable.insert(symbol->name, symbol)) {
            ERROR("Failed to insert variable decl to symtable.");
        }
    }
    for (int i = 0; i < fields.size(); ++i) {
        Symbol* field = symbol->type->fields[i];
        if (!(field->type) || field->type->kind == TypeInfo::Kind::Unknown) {
            VariableDecl* field_decl = fields[i];
            if (field_decl->visit2(symtable) != 0) {
                return -1;
            }
            field->type = field_decl->typeAnnotation->inferred_type;
        }
    }   
    return 0;
}

int UsingDecl::visit_impl2(SymTable& symtable) {
    return 0;
}

int CompoundStmts::visit_impl2(SymTable& symtable) {
    for (auto* s : stmts) {
        if (s) s->visit2(symtable);
    }
    return 0;
}

int SingleStmt::visit_impl2(SymTable& symtable) {
    if (stmt) stmt->visit2(symtable);
    return 0;
}

int BlockStmt::visit_impl2(SymTable& symtable) {
    symtable.enterScope();
    if (stmts) stmts->visit2(symtable);
    symtable.exitScope();
    return 0;
}

int ImportStmt::visit_impl2(SymTable& symtable) {
    if (!symbol) {
        ERROR("symbol not constructed in pass 1");
    }

    if (symtable.getScopeLevel() > 0) {
        if (!symtable.insert(symbol->name, symbol)) {
            ERROR("Failed to insert variable decl to symtable.");
        }
    }
    return 0;
}

int IfStmt::visit_impl2(SymTable& symtable) {
    if (!condition || !thenBranch) return -1;
    condition->visit2(symtable);
    if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
        ERROR("Condition must be bool");
        return -1;
    }
    thenBranch->visit2(symtable);
    if (elseBranch) elseBranch->visit2(symtable);
    return 0;
}

int WhileStmt::visit_impl2(SymTable& symtable) {
    if (!condition || !body) return -1;
    condition->visit2(symtable);
    if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
        ERROR("Condition must be bool");
        return -1;
    }
    body->visit2(symtable);
    return 0;
}

int ForStmt::visit_impl2(SymTable& symtable) {
    if (init) init->visit2(symtable);
    if (condition) {
        condition->visit2(symtable);
        if (condition->inferred_type && condition->inferred_type->kind != TypeInfo::Kind::Bool) {
            ERROR("Condition must be bool");
            return -1;
        }
    }
    if (increment) increment->visit2(symtable);
    if (body) body->visit2(symtable);
    return 0;
}

int ReturnStmt::visit_impl2(SymTable& symtable) {
    TypeInfo* retType = symtable.get_current_function_return_type();
    if (!retType) {
        ERROR("return type is null" << retType);
    }
    if (!expr) {
        if (retType->kind != TypeInfo::Kind::Void) {
            ERROR("current function need to return something " << retType->dump());
            return -1;
        }
        return 0;
    }
    if (expr->visit2(symtable) != 0) return -1;

    if (!retType || !sameType(retType, expr->inferred_type)) {
        ERROR("Failed to get return Type or Return type mismatch" << this->dump());
        return -1;
    }
    return 0;
}

int DeferStmt::visit_impl2(SymTable& symtable) {
    return 0;
}

int ExprStmt::visit_impl2(SymTable& symtable) {
    if (!expr) {
        ERROR("expr in ExprStmt is null");
        return -1;
    }
    return expr->visit2(symtable);
}

int BreakStmt::visit_impl2(SymTable& symtable) {
    return 0;
}

int ContinueStmt::visit_impl2(SymTable& symtable) {
    return 0;
}

int CompileStmt::visit_impl2(SymTable& symtable) {
    return 0;
}

int BinaryExpr::visit_impl2(SymTable& symtable) {
    if (!left || !right) return -1;
    if (left->visit2(symtable) != 0) return -1;
    if (right->visit2(symtable) != 0) return -1;

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

int UnaryExpr::visit_impl2(SymTable& symtable) {
    if (!operand) return -1;
    if (operand->visit2(symtable) != 0) return -1;
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

int CallExpr::visit_impl2(SymTable& symtable) {
    if (!callee) return -1;
    callee->visit2(symtable);
    TypeInfo* calleeType = callee->inferred_type;

    if (!calleeType || calleeType->kind != TypeInfo::Kind::Function) {
        ERROR("Attempting to call a non-function" << this->dump());
        return -1;
    }

    std::vector<TypeInfo*> argTypes;
    for (auto* arg : arguments) {
        if (arg) {
            arg->visit2(symtable);
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

int IndexExpr::visit_impl2(SymTable& symtable) {
    base->visit2(symtable);
    inferred_type = base->inferred_type;
    index->visit(symtable);
    return 0;
}

int MemberAccessExpr::visit_impl2(SymTable& symtable) {
    if (!object) return -1;
    object->visit2(symtable);
    TypeInfo* obj_type = object->inferred_type;
    if (obj_type && obj_type->kind == TypeInfo::Kind::Pointer && obj_type->baseType) {
        obj_type = obj_type->baseType;
    }
    if (!obj_type || obj_type->kind != TypeInfo::Kind::Struct) {
        ERROR("obj_type: " << obj_type->dump());
        ERROR("Cannot find struct object" << object->dump() 
            << "\n SymTable:\n" << ToString(symtable.get_environments()));
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

int IdentifierExpr::visit_impl2(SymTable& symtable) {
    Symbol* s = symtable.lookup(static_cast<Name*>(name)->name);
    if (!s) return -1;
    symbol = s;
    inferred_type = s->type;
    return 0;
}

int LiteralExpr::visit_impl2(SymTable& symtable) {
    return 0;
}

int CastExpr::visit_impl2(SymTable& symtable) {
    if (type) {
        type->visit2(symtable);
        inferred_type = type->inferred_type;
    }
    if (expr) {
        expr->visit2(symtable);
    }
    return 0;
}

int ArrayLiteralExpr::visit_impl2(SymTable& symtable) {
    if (!type) {
        ERROR("Expect a type in array literal expr");
    }
    type->visit(symtable);
    TypeInfo* eType = type->inferred_type;
    if (eType->kind == TypeInfo::Kind::Unknown) {
        ERROR("Fail to get eType Kind, still unknown.");
        return -1;
    }
    inferred_type->name = eType->name + "[](array)";
    inferred_type->elemType = eType;

    for (ASTNode* e: elements) {
        e->visit2(symtable);
    }
    return 0;
}

int NewExpr::visit_impl2(SymTable& symtable) {
    if (type && !type->inferred_type || type->inferred_type->kind == TypeInfo::Kind::Unknown) {
        type->visit2(symtable);
        TypeInfo* target = type->inferred_type;
        if (!target) return -1;

        inferred_type->baseType = target;
        return 0;
    }
    return -1;
}

int LambdaExpr::visit_impl2(SymTable& symtable) {
    return 0;
}

int IfExpr::visit_impl2(SymTable& symtable) {
    return 0;
}

int RunExpr::visit_impl2(SymTable& symtable) {
    return 0;
}

int NamedType::visit_impl2(SymTable& symtable) {
    Symbol* s = symtable.lookup(static_cast<Name*>(name)->name);
    if (!s) {
        ERROR("Cannot Find: " << static_cast<Name*>(name)->name);
        return -1;
    }
    if (s->kind != SymKind::Type) {
        ERROR("Named Type is not a Type identifier");
        return -1;
    }
    inferred_type = s->type;
    return 0;
}

int PointerType::visit_impl2(SymTable& symtable) {
    if (!baseType) {
        return -1;
    }
    baseType->visit2(symtable);
    TypeInfo* base = baseType->inferred_type;
    if (!base) {
        ERROR("Cannot find base type");
        return -1;
    }
    inferred_type->baseType = base;
    return 0;    
}

int ArrayType::visit_impl2(SymTable& symtable) {
    if (!elementType) return -1;
    elementType->visit2(symtable);
    TypeInfo* elem = elementType->inferred_type;
    if (!elem) return -1;
    Arena* ar = symtable.get_symbol_arena();
    inferred_type = ar->New<TypeInfo>();
    inferred_type->elemType = elem;
    /* TODO Evalute array size */
    // inferred_type->arraySize = evaluate_size(sizeExpr);
    return 0;
}

int FunctionType::visit_impl2(SymTable& symtable) {
    // TODO: Fill in param func type info
    return 0;
}

