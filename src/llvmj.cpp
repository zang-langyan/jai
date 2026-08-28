#include "llvmj.h"

std::unordered_map<TypeInfo*, llvm::Type*>& CodeGenContext::getStructTypeRecord() {
    return _struct_type_creation_record;
}
std::unordered_map<TypeInfo*, llvm::Function*>& CodeGenContext::getFunctionTypeRecord() {
    return _function_type_creation_record;
}
std::unordered_map<Symbol*, llvm::Value*>& CodeGenContext::getSymbolRecord() {
    return _symbol_value_record;
}

llvm::Type* LLVMJ::toLLVMType(TypeInfo* type) {
    if (!_context) {
        ERROR("LLVM Context is NULL");
    }
    llvm::LLVMContext& context = *_context;
    switch (type->kind) {
        case TypeInfo::Kind::Void:   return llvm::Type::getVoidTy(context);
        case TypeInfo::Kind::Int:    return llvm::Type::getInt64Ty(context);   // s64
        case TypeInfo::Kind::Float:  return llvm::Type::getDoubleTy(context);
        case TypeInfo::Kind::Bool:   return llvm::Type::getInt1Ty(context);
        case TypeInfo::Kind::String: return llvm::PointerType::getUnqual(context); // i8*
        case TypeInfo::Kind::Char:   return llvm::Type::getInt8Ty(context);
        case TypeInfo::Kind::Pointer:
            return llvm::PointerType::getUnqual(context);
        case TypeInfo::Kind::Struct: {
            /* We might want to keep track of this creation and produce this type once and for all */
            if (_cg.getStructTypeRecord().find(type) != _cg.getStructTypeRecord().end()) {
                return _cg.getStructTypeRecord()[type];
            }
            llvm::StructType* st = llvm::StructType::create(context, type->name);
            std::vector<llvm::Type*> fieldTypes;
            for (auto* f : type->fields) {
                fieldTypes.push_back(toLLVMType(f->type));
            }
            st->setBody(fieldTypes);
            _cg.getStructTypeRecord()[type] = st;
            return st;
        }
        case TypeInfo::Kind::Array:
            return llvm::ArrayType::get(toLLVMType(type->elemType), type->arraySize);
        case TypeInfo::Kind::Function: {
            if (_cg.getFunctionTypeRecord().find(type) != _cg.getFunctionTypeRecord().end()) {
                return (llvm::Type*)_cg.getFunctionTypeRecord()[type];
            }
            llvm::Type* ret = toLLVMType(type->returnType);
            std::vector<llvm::Type*> params;
            for (auto* p : type->params) params.push_back(toLLVMType(p->type));

            _cg.getFunctionTypeRecord()[type] = (llvm::Function*)llvm::FunctionType::get(ret, params, false);
            return (llvm::Type*)_cg.getFunctionTypeRecord()[type];
        }
        default: return llvm::Type::getVoidTy(context);
    }
}

void LLVMJ::add_builtin() {
    llvm::LLVMContext& ctx = getContext();
    llvm::Module& module = getModule();

    llvm::FunctionType* printfType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(ctx),
        {llvm::PointerType::getUnqual(ctx)},
        true // variadic parameters
    );
    module.getOrInsertFunction("printf", printfType);

    llvm::FunctionType* mallocType = llvm::FunctionType::get(
        llvm::PointerType::getUnqual(ctx),
        {llvm::Type::getInt64Ty(ctx)},
        false
    );
    module.getOrInsertFunction("malloc", mallocType);

    llvm::FunctionType* freeType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(ctx),
        {llvm::PointerType::getUnqual(ctx)},
        false
    );
    module.getOrInsertFunction("free", freeType);
}

int LLVMJ::execute() {
    auto err = _jit->addIRModule(llvm::orc::ThreadSafeModule(std::move(_module), std::make_unique<llvm::LLVMContext>()));
    if (err) return -1;
    auto sym = _jit->lookup("main");
    if (!sym) return -1;
    int (*mainFn)() = sym->toPtr<int(*)()>();
    return mainFn();
}