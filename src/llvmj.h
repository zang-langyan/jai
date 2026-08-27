#ifndef JAI_LLVM_CODEGEN_H
#define JAI_LLVM_CODEGEN_H

#include "symtable.h"
#include <memory>
#include <stack>
#include <unordered_map>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

class CodeGenContext {
public:
    CodeGenContext() = default;

    std::unordered_map<TypeInfo*, llvm::Type*>& getStructTypeRecord();
    std::unordered_map<TypeInfo*, llvm::Function*>& getFunctionTypeRecord();
    std::unordered_map<Symbol*, llvm::Value*>& getSymbolRecord();

    llvm::Function* current_function {nullptr};

    std::stack<llvm::BasicBlock*> loopExitBlocks;
    std::stack<llvm::BasicBlock*> loopContinueBlocks;
private:

    std::unordered_map<Symbol*, llvm::Value*> _symbol_value_record;
    std::unordered_map<TypeInfo*, llvm::Type*> _struct_type_creation_record;
    std::unordered_map<TypeInfo*, llvm::Function*> _function_type_creation_record;
};


class LLVMJ {
public:
    LLVMJ() {
        _context  = std::make_unique<llvm::LLVMContext>();
        _module   = std::make_unique<llvm::Module>("Jai JIT", *_context);
        _builder  = std::make_unique<llvm::IRBuilder<>>(*_context);

        add_builtin();

        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
        auto jitOrErr = llvm::orc::LLJITBuilder().create();
        if (!jitOrErr) {
            llvm::errs() << "Failed to create LLVM JIT Builder: " 
                        << llvm::toString(jitOrErr.takeError()) << "\n";
            return;
        }
        _jit = std::move(*jitOrErr);
    }

    static LLVMJ& instance() {
        static LLVMJ ins;
        return ins;
    }
    llvm::LLVMContext& getContext() { return *_context; }
    llvm::Module& getModule() { return *_module; }
    llvm::IRBuilder<>& getBuilder() { return *_builder; }
    CodeGenContext& getCodeGenContext() { return _cg; }
public:
    llvm::Type* toLLVMType(TypeInfo* type);
    void add_builtin();

    int execute();

private:
    std::unique_ptr<llvm::LLVMContext> _context;
    std::unique_ptr<llvm::Module> _module;
    std::unique_ptr<llvm::IRBuilder<>> _builder;

    std::unique_ptr<llvm::orc::LLJIT> _jit;

    CodeGenContext _cg;
};

#endif