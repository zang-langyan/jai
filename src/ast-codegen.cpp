#include "ast.h"
#include "llvmj.h"
#include <cstddef>

static llvm::Value* LValueAddress(ASTNode* node, LLVMJ& j, llvm::IRBuilder<>& builder) {
    if (!node) return nullptr;

    switch (node->type) {
        case ASTNodeType::IdentifierExpr: {
            auto* idExpr = static_cast<IdentifierExpr*>(node);
            Symbol* sym = idExpr->symbol;
            auto& symRec = j.getCodeGenContext().getSymbolRecord();
            auto it = symRec.find(sym);
            if (it != symRec.end()) {
                return it->second;
            }
            if (sym && sym->kind == SymKind::Variable && sym->scopeLevel == 0) {
                llvm::GlobalVariable* gv = j.getModule().getGlobalVariable(sym->name);
                if (gv) {
                    return gv;
                }
            }
            ERROR("Undefined variable in lvalue: " << sym->name);
            return nullptr;
        }

        case ASTNodeType::MemberAccessExpr: {
            auto* ma = static_cast<MemberAccessExpr*>(node);
            llvm::Value* objAddr = LValueAddress(ma->object, j, builder);
            if (!objAddr) {
                llvm::Value* objVal = ma->object->codegen<llvm::Value>();
                if (!objVal) return nullptr;
                llvm::Type* objTy = j.toLLVMType(ma->object->inferred_type);
                llvm::AllocaInst* temp = builder.CreateAlloca(objTy, nullptr, "objtemp");
                builder.CreateStore(objVal, temp);
                objAddr = temp;
            }

            TypeInfo* objTypeInfo = ma->object->inferred_type;
            llvm::Type* structTy = nullptr;
            llvm::Value* structPtr = objAddr;
            if (objTypeInfo->kind == TypeInfo::Kind::Pointer) {
                structTy = j.toLLVMType(objTypeInfo->baseType);
                structPtr = builder.CreateLoad(j.toLLVMType(objTypeInfo), objAddr, "ptrload");
            } else {
                structTy = j.toLLVMType(objTypeInfo);
            }
            int idx = 0;
            for (auto* f : (objTypeInfo->kind == TypeInfo::Kind::Pointer ? objTypeInfo->baseType->fields : objTypeInfo->fields)) {
                if (f->name == static_cast<Name*>(ma->member)->name) break;
                idx++;
            }
            llvm::Value* fieldPtr = builder.CreateStructGEP(structTy, structPtr, idx, "fieldptr");
            return fieldPtr;
        }

        case ASTNodeType::IndexExpr: {
            auto* ie = static_cast<IndexExpr*>(node);
            llvm::Value* baseAddr = LValueAddress(ie->base, j, builder);
            if (!baseAddr) {
                llvm::Value* baseVal = ie->base->codegen<llvm::Value>();
                if (!baseVal) return nullptr;
                llvm::Type* baseTy = j.toLLVMType(ie->base->inferred_type);
                llvm::AllocaInst* temp = builder.CreateAlloca(baseTy, nullptr, "basetemp");
                builder.CreateStore(baseVal, temp);
                baseAddr = temp;
            }

            TypeInfo* baseTypeInfo = ie->base->inferred_type;
            llvm::Value* indexVal = ie->index->codegen<llvm::Value>();
            if (!indexVal) return nullptr;

            llvm::Type* elemTy = nullptr;
            llvm::Value* elemPtr = nullptr;
            if (baseTypeInfo->kind == TypeInfo::Kind::Pointer) {
                elemTy = j.toLLVMType(baseTypeInfo->baseType);
                llvm::Value* ptr = builder.CreateLoad(j.toLLVMType(baseTypeInfo), baseAddr, "ptrload");
                elemPtr = builder.CreateGEP(elemTy, ptr, indexVal, "elemptr");
            } else if (baseTypeInfo->kind == TypeInfo::Kind::Array) {
                elemTy = j.toLLVMType(baseTypeInfo->baseType);
                elemPtr = builder.CreateGEP(j.toLLVMType(baseTypeInfo), baseAddr, {builder.getInt32(0), indexVal}, "elemptr");
            } else {
                ERROR("Indexing non-indexable type");
                return nullptr;
            }
            return elemPtr;
        }

        case ASTNodeType::UnaryExpr: {
            auto* ue = static_cast<UnaryExpr*>(node);
            if (ue->op && static_cast<Op*>(ue->op)->op_type == OpType::DEREF) {
                llvm::Value* ptrVal = ue->operand->codegen<llvm::Value>();
                if (!ptrVal) return nullptr;
                return ptrVal;
            }
            ERROR("Unary expression is not an lvalue");
            return nullptr;
        }

        case ASTNodeType::LiteralExpr:
        case ASTNodeType::CallExpr:
        case ASTNodeType::BinaryExpr:
        default:
            ERROR("Expression is not an lvalue");
            return nullptr;
    }
}


void* Module::codegen_impl() {
    if (statements) {
        return statements->codegen<llvm::Value>();
    }
    return nullptr;
}

void* Interactive::codegen_impl() {
    if (statements) {
        return statements->codegen<llvm::Value>();
    }
    return nullptr;
}

void* Name::codegen_impl() {
    return nullptr;
}

void* Op::codegen_impl() {
    return nullptr;
}

void* Literal::codegen_impl() {
    return nullptr;
}

void* VariableDecl::codegen_impl() {
    LLVMJ& llvmj = LLVMJ::instance();
    llvm::Value* address = nullptr;
    if (symbol->scopeLevel == 0) {
        // global variable
        llvm::Type* varType = llvmj.toLLVMType(symbol->type);
        llvm::GlobalVariable* gv = new llvm::GlobalVariable(
            llvmj.getModule(), varType, false,
            llvm::GlobalValue::ExternalLinkage,
            llvm::Constant::getNullValue(varType), symbol->name);
        address = gv;
    } else {
        // local variable
        llvm::Type* varType = llvmj.toLLVMType(symbol->type);
        address = llvmj.getBuilder().CreateAlloca(varType, nullptr, symbol->name);
    }
    llvmj.getCodeGenContext().getSymbolRecord()[symbol] = address;

    if (initializer) {
        llvm::Value* initVal = initializer->codegen<llvm::Value>();
        llvmj.getBuilder().CreateStore(initVal, address);
    }
    return address;
}

void* ConstantDecl::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();

    if (!symbol) {
        ERROR("Symbol is null in ConstantDecl::codegen");
        return nullptr;
    }

    llvm::Type* constType = j.toLLVMType(symbol->type);
    if (!constType) return nullptr;

    llvm::Value* address = nullptr;

    if (symbol->scopeLevel == 0) {
        llvm::Constant* initVal = nullptr;
        if (value) {
            if (auto* litExpr = static_cast<LiteralExpr*>(value)) {
                switch (litExpr->lit->litType) {
                    case Literal::LitType::Int:
                        initVal = llvm::ConstantInt::get(constType, litExpr->lit->intVal);
                        break;
                    case Literal::LitType::Float:
                        initVal = llvm::ConstantFP::get(constType, litExpr->lit->floatVal);
                        break;
                    case Literal::LitType::Bool:
                        initVal = llvm::ConstantInt::get(constType, litExpr->lit->boolVal ? 1 : 0);
                        break;
                    case Literal::LitType::Char:
                        initVal = llvm::ConstantInt::get(constType, litExpr->lit->intVal);
                        break;
                    case Literal::LitType::String:
                        initVal = builder.CreateGlobalStringPtr(std::string(litExpr->lit->stringVal.s, litExpr->lit->stringVal.len));
                        break;
                    default:
                        initVal = llvm::Constant::getNullValue(constType);
                }
            } else {
                // cannot eval at compile time, fill in null for now
                ERROR("Cannot eval constant decl at compile time.");
                initVal = llvm::Constant::getNullValue(constType);
            }
        } else {
            initVal = llvm::Constant::getNullValue(constType);
        }

        llvm::GlobalVariable* gv = new llvm::GlobalVariable(
            j.getModule(),
            constType,
            /* isConstant= */ true,
            llvm::GlobalValue::ExternalLinkage,
            initVal,
            symbol->name
        );
        address = gv;
    } else {
        address = builder.CreateAlloca(constType, nullptr, symbol->name);
    }

    j.getCodeGenContext().getSymbolRecord()[symbol] = address;

    if (symbol->scopeLevel > 0) {
        if (value) {
            llvm::Value* initVal = value->codegen<llvm::Value>();
            if (!initVal) return nullptr;
            builder.CreateStore(initVal, address);
        }
    }

    return address;
}

void* FuncDecl::codegen_impl() {
    LLVMJ& llvmj = LLVMJ::instance();

    llvm::Type* retType = llvmj.toLLVMType(symbol->type->returnType);
    std::vector<llvm::Type*> paramTypes;
    for (auto* p : symbol->type->params) {
        paramTypes.push_back(llvmj.toLLVMType(p->type));
    }
    llvm::FunctionType* ft = llvm::FunctionType::get(retType, paramTypes, false);

    llvm::Function* func = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, symbol->name, llvmj.getModule());
    llvmj.getCodeGenContext().getFunctionTypeRecord()[symbol->type] = func;

    size_t idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(symbol->type->params[idx]->name);
        idx++;
    }

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(llvmj.getContext(), "entry", func);
    llvmj.getBuilder().SetInsertPoint(entry);

    llvm::Function* oldFunc = llvmj.getCodeGenContext().current_function;
    llvmj.getCodeGenContext().current_function = func;

    idx = 0;
    for (auto& arg : func->args()) {
        Symbol* paramSym = symbol->type->params[idx];
        llvm::AllocaInst* alloca = llvmj.getBuilder().CreateAlloca(arg.getType(), nullptr, arg.getName());
        llvmj.getBuilder().CreateStore(&arg, alloca);
        llvmj.getCodeGenContext().getSymbolRecord()[paramSym] = alloca;
        ++idx;
    }

    if (body) {
        body->codegen<llvm::Value>();
    }

    if (!llvmj.getBuilder().GetInsertBlock()->getTerminator()) {
        if (retType->isVoidTy()) {
            llvmj.getBuilder().CreateRetVoid();
        } else {
            llvmj.getBuilder().CreateRet(llvm::Constant::getNullValue(retType));
        }
    }

    llvmj.getCodeGenContext().current_function = oldFunc;
    return func;
}

void* StructDecl::codegen_impl() {
    return nullptr;
}

void* UsingDecl::codegen_impl() {
    return nullptr;
}

void* CompoundStmts::codegen_impl() {
    for (auto* s : stmts) {
        if (s) s->codegen<llvm::Value>();
    }
    return nullptr;
}

void* SingleStmt::codegen_impl() {
    if (stmt) return stmt->codegen<llvm::Value>();
    return nullptr;
}

void* BlockStmt::codegen_impl() {
    if (stmts) return stmts->codegen<llvm::Value>();
    return nullptr;
}

void* ImportStmt::codegen_impl() {
    return nullptr;
}

void* IfStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::LLVMContext& ctx = j.getContext();
    llvm::Function* func = j.getCodeGenContext().current_function;
    if (!func) return nullptr;

    llvm::Value* cond = condition->codegen<llvm::Value>();
    if (!cond) return nullptr;

    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(ctx, "then", func);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(ctx, "else", func);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(ctx, "ifcont", func);

    builder.CreateCondBr(cond, thenBB, elseBB);

    builder.SetInsertPoint(thenBB);
    if (thenBranch) thenBranch->codegen<llvm::Value>();
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(mergeBB);

    builder.SetInsertPoint(elseBB);
    if (elseBranch) elseBranch->codegen<llvm::Value>();
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(mergeBB);

    builder.SetInsertPoint(mergeBB);
    return nullptr;
}

void* WhileStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::LLVMContext& ctx = j.getContext();
    llvm::Function* func = j.getCodeGenContext().current_function;
    if (!func) return nullptr;

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(ctx, "cond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(ctx, "body", func);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(ctx, "after", func);

    auto& exitStack = j.getCodeGenContext().loopExitBlocks;
    auto& contStack = j.getCodeGenContext().loopContinueBlocks;
    exitStack.push(afterBB);
    contStack.push(condBB);

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value* cond = condition->codegen<llvm::Value>();
    builder.CreateCondBr(cond, bodyBB, afterBB);

    builder.SetInsertPoint(bodyBB);
    if (body) body->codegen<llvm::Value>();
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(condBB);

    builder.SetInsertPoint(afterBB);

    exitStack.pop();
    contStack.pop();
    return nullptr;
}

void* ForStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::LLVMContext& ctx = j.getContext();
    llvm::Function* func = j.getCodeGenContext().current_function;
    if (!func) return nullptr;

    // 初始化
    if (init) init->codegen<llvm::Value>();

    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(ctx, "forcond", func);
    llvm::BasicBlock* bodyBB = llvm::BasicBlock::Create(ctx, "forbody", func);
    llvm::BasicBlock* incBB  = llvm::BasicBlock::Create(ctx, "forinc", func);
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(ctx, "forafter", func);

    auto& exitStack = j.getCodeGenContext().loopExitBlocks;
    auto& contStack = j.getCodeGenContext().loopContinueBlocks;
    exitStack.push(afterBB);
    contStack.push(incBB);

    builder.CreateBr(condBB);
    builder.SetInsertPoint(condBB);
    llvm::Value* cond = condition->codegen<llvm::Value>();
    builder.CreateCondBr(cond, bodyBB, afterBB);

    builder.SetInsertPoint(bodyBB);
    if (body) body->codegen<llvm::Value>();
    if (!builder.GetInsertBlock()->getTerminator())
        builder.CreateBr(incBB);

    builder.SetInsertPoint(incBB);
    if (increment) increment->codegen<llvm::Value>();
    builder.CreateBr(condBB);

    builder.SetInsertPoint(afterBB);

    exitStack.pop();
    contStack.pop();
    return nullptr;
}

void* ReturnStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    if (expr) {
        llvm::Value* retVal = expr->codegen<llvm::Value>();
        builder.CreateRet(retVal);
    } else {
        builder.CreateRetVoid();
    }
    return nullptr;
}

void* DeferStmt::codegen_impl() {
    return nullptr;
}

void* ExprStmt::codegen_impl() {
    if (expr) return expr->codegen<llvm::Value>();
    return nullptr;
}

void* BreakStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    auto& exitStack = j.getCodeGenContext().loopExitBlocks;
    if (exitStack.empty()) {
        ERROR("Not in a loop. Cannot Break.");
        return nullptr;
    }
    llvm::BasicBlock* target = exitStack.top();
    j.getBuilder().CreateBr(target);
    return nullptr;
}

void* ContinueStmt::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    auto& contStack = j.getCodeGenContext().loopContinueBlocks;
    if (contStack.empty()) {
        ERROR("Not in a loop. Cannot Continue.");
        return nullptr;
    }
    llvm::BasicBlock* target = contStack.top();
    j.getBuilder().CreateBr(target);
    return nullptr;
}

void* CompileStmt::codegen_impl() {
    return nullptr;
}

void* BinaryExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    OpType op_type = (op) ? static_cast<Op*>(op)->op_type : OpType::INVALID;
    if (op_type == OpType::ASSIGN) {
        llvm::Value* lhsAddr = LValueAddress(left, j, builder);
        if (!lhsAddr) {
            ERROR("Invalid left-hand side of assignment");
            return nullptr;
        }
        llvm::Value* rhs = right->codegen<llvm::Value>();
        if (!rhs) {
            ERROR("Failed to gen right hand side value.");
            return nullptr;
        }
        builder.CreateStore(rhs, lhsAddr);
        return rhs;
    }
    
    llvm::Value* L = left->codegen<llvm::Value>();
    llvm::Value* R = right->codegen<llvm::Value>();
    if (!L || !R) return nullptr;

    switch (op_type) {
        case OpType::ASSIGN:
        case OpType::OR:
        case OpType::AND:
        case OpType::PLUS:      return builder.CreateAdd(L, R, "addtmp");
        case OpType::MINUS:     return builder.CreateSub(L, R, "subtmp");
        case OpType::MULTIPLY:  return builder.CreateMul(L, R, "multmp");
        case OpType::DIVIDE:    return builder.CreateSDiv(L, R, "divtmp");
        case OpType::MOD:       return builder.CreateSRem(L, R, "modtmp");
        case OpType::LESS:      return builder.CreateICmpSLT(L, R, "cmptmp");
        case OpType::GREATER:   return builder.CreateICmpSGT(L, R, "cmptmp");
        case OpType::LESSEQ:    return builder.CreateICmpSLE(L, R, "cmptmp");
        case OpType::GREATEREQ: return builder.CreateICmpSGE(L, R, "cmptmp");
        case OpType::EQUAL:     return builder.CreateICmpEQ(L, R, "cmptmp");
        case OpType::UNEQUAL:   return builder.CreateICmpNE(L, R, "cmptmp");
        case OpType::LSHIFT:    return builder.CreateShl(L, R, "shltmp");
        case OpType::RSHIFT:    return builder.CreateAShr(L, R, "shrtmp");
        case OpType::BITAND:    return builder.CreateAnd(L, R, "andtmp");
        case OpType::BITOR:     return builder.CreateOr(L, R, "ortmp");
        case OpType::XOR:       return builder.CreateXor(L, R, "xortmp");
        default:                return nullptr;
    }
}

void* UnaryExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::Value* operandVal = operand->codegen<llvm::Value>();
    if (!operandVal) return nullptr;

    OpType op_type = (op) ? static_cast<Op*>(op)->op_type : OpType::INVALID;
    switch (op_type) {
        case OpType::NOT:
            return builder.CreateNot(operandVal, "nottmp");
        case OpType::NEGATIVE:
            return builder.CreateNeg(operandVal, "negtmp");
        case OpType::POSITIVE:
            return operandVal;
        case OpType::ADDRESS:
            return LValueAddress(operand, j, builder);
        case OpType::DEREF: {
            llvm::Value* ptr = operand->codegen<llvm::Value>();
            if (!ptr) return nullptr;
            TypeInfo* ptrType = operand->inferred_type;
            if (!ptrType || ptrType->kind != TypeInfo::Kind::Pointer || !ptrType->baseType) {
                ERROR("Dereference of non-pointer type");
                return nullptr;
            }
            llvm::Type* targetTy = j.toLLVMType(ptrType->baseType);
            return builder.CreateLoad(targetTy, ptr, "dereftmp");
        }
        default:
            return nullptr;
    }
}

void* CallExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::Function* calleeFunc = nullptr;

    if (auto* idExpr = dynamic_cast<IdentifierExpr*>(callee)) {
        Symbol* funcSym = idExpr->symbol;
        if (funcSym && funcSym->isBuiltin && funcSym->name == "print") {
            llvm::Function* printfFunc = j.getModule().getFunction("printf");
            if (!printfFunc) {
                ERROR("printf not declared");
                return nullptr;
            }

            std::vector<llvm::Value*> args;
            for (auto* arg : arguments) {
                llvm::Value* argVal = arg->codegen<llvm::Value>();
                if (!argVal) return nullptr;
                args.push_back(argVal);
            }

            llvm::CallInst* call = builder.CreateCall(printfFunc, args, "print_call");
            return nullptr;
        } else if (funcSym->isBuiltin && funcSym->name == "free") {
            llvm::Function* freeFunc = j.getModule().getFunction("free");
            llvm::Value* arg = arguments[0]->codegen<llvm::Value>();
            builder.CreateCall(freeFunc, {arg});
            return nullptr;
        }
    }

    TypeInfo* funcType = callee->inferred_type;
    auto& funcRec = j.getCodeGenContext().getFunctionTypeRecord();
    auto it = funcRec.find(funcType);
    if (it != funcRec.end()) {
        calleeFunc = it->second;
    } else {
        ERROR("Function " << funcType->name << " not found.");
        return nullptr;
    }
    
    if (!calleeFunc) {
        if (auto* idExpr = static_cast<IdentifierExpr*>(callee)) {
            std::string name = static_cast<Name*>(idExpr->name)->name;
            calleeFunc = j.getModule().getFunction(name);
        }
        if (!calleeFunc) return nullptr;
    }

    std::vector<llvm::Value*> args;
    for (auto* arg : arguments) {
        llvm::Value* argVal = arg->codegen<llvm::Value>();
        args.push_back(argVal);
    }

    return builder.CreateCall(calleeFunc, args, "calltmp");
}

void* IndexExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();

    llvm::Value* elemAddr = LValueAddress(this, j, builder);
    if (!elemAddr) return nullptr;

    llvm::Type* elemTy = j.toLLVMType(inferred_type);
    return builder.CreateLoad(elemTy, elemAddr, "elemval");
}

void* MemberAccessExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();

    llvm::Value* fieldAddr = LValueAddress(this, j, builder);
    if (!fieldAddr) return nullptr;

    TypeInfo* fieldType = inferred_type;
    llvm::Type* ty = j.toLLVMType(fieldType);
    return builder.CreateLoad(ty, fieldAddr, "fieldval");
}

void* IdentifierExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    if (!symbol) {
        ERROR("IndentifierExpr symbol is null!" << this->dump());
        return nullptr;
    }
    auto& symRec = j.getCodeGenContext().getSymbolRecord();
    auto it = symRec.find(symbol);
    if (it != symRec.end()) {
        if (symbol->kind == SymKind::Function) {
            return it->second;
        } else {
            llvm::Type* ty = j.toLLVMType(symbol->type);
            return j.getBuilder().CreateLoad(ty, it->second, symbol->name);
        }
    } else {
        ERROR(symbol->dump());
        if (symbol->kind == SymKind::Function) {
            return j.getModule().getFunction(symbol->name);
        } else if (symbol->kind == SymKind::Variable && symbol->scopeLevel == 0) {
            llvm::GlobalVariable* gv = j.getModule().getGlobalVariable(symbol->name);
            if (gv) {
                llvm::Type* ty = j.toLLVMType(symbol->type);
                return j.getBuilder().CreateLoad(ty, gv, symbol->name);
            }
        }
    }
    return nullptr;
}

void* LiteralExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    switch (lit->litType) {
        case Literal::LitType::Int:
            return llvm::ConstantInt::get(llvm::Type::getInt64Ty(j.getContext()), lit->intVal);
        case Literal::LitType::Float:
            return llvm::ConstantFP::get(llvm::Type::getDoubleTy(j.getContext()), lit->floatVal);
        case Literal::LitType::Bool:
            return llvm::ConstantInt::get(llvm::Type::getInt1Ty(j.getContext()), lit->boolVal ? 1 : 0);
        case Literal::LitType::Char:
            return llvm::ConstantInt::get(llvm::Type::getInt8Ty(j.getContext()), lit->intVal);
        case Literal::LitType::String: {
            return builder.CreateGlobalStringPtr(std::string(lit->stringVal.s, lit->stringVal.len));
        }
        case Literal::LitType::JNull: {
            llvm::PointerType* ptrTy = llvm::PointerType::getUnqual(j.getContext());
            return llvm::ConstantPointerNull::get(ptrTy);
        }
    }
    return nullptr;
}

void* CastExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::Value* val = expr->codegen<llvm::Value>();
    llvm::Type* targetTy = j.toLLVMType(type->inferred_type);
    return j.getBuilder().CreateCast(llvm::Instruction::CastOps::BitCast, val, targetTy, "casttmp");
}

void* ArrayLiteralExpr::codegen_impl() {
    return nullptr;
}

void* NewExpr::codegen_impl() {
    LLVMJ& j = LLVMJ::instance();
    llvm::IRBuilder<>& builder = j.getBuilder();
    llvm::Type* ty = j.toLLVMType(type->inferred_type);

    llvm::Value* size = nullptr;
    if (auto* structTy = llvm::dyn_cast<llvm::StructType>(ty)) {
        size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(j.getContext()),
                                      j.getModule().getDataLayout().getStructLayout(structTy)->getSizeInBytes());
    } else {
        size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(j.getContext()),
                                      j.getModule().getDataLayout().getTypeAllocSize(ty));
    }

    llvm::FunctionCallee mallocFunc = j.getModule().getOrInsertFunction("malloc",
        llvm::FunctionType::get(llvm::PointerType::getUnqual(j.getContext()),
                                {llvm::Type::getInt64Ty(j.getContext())}, false));
    llvm::Value* ptr = builder.CreateCall(mallocFunc, {size}, "newptr");
    return builder.CreatePointerCast(ptr, llvm::PointerType::getUnqual(j.getContext()), "newcast");
}

void* LambdaExpr::codegen_impl() {
    return nullptr;
}

void* IfExpr::codegen_impl() {
    return nullptr;
}

void* RunExpr::codegen_impl() {
    return nullptr;
}

void* NamedType::codegen_impl() {
    return nullptr;
}

void* PointerType::codegen_impl() {
    return nullptr;
}

void* ArrayType::codegen_impl() {
    return nullptr;
}

void* FunctionType::codegen_impl() {
    return nullptr;
}

