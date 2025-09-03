#ifndef EMITTER_HPP
#define EMITTER_HPP

class Module;
class ClassDecl;
class MethodDecl;
class Function;
class AllModules;
class Type;
class ConstructorDecl;
class Handler;

#include <map>
#include <memory>
#include <stack>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

class Emitter : public std::enable_shared_from_this<Emitter> {
 public:
    Emitter(std::shared_ptr<AllModules> modules, std::shared_ptr<Module> main_module, std::shared_ptr<Handler> handler)
    : modules_(modules)
    , main_module_(main_module)
    , handler_{handler} {
        context = std::make_unique<llvm::LLVMContext>();
        llvm_module = std::make_unique<llvm::Module>("my module", *context);
        llvm_builder = std::make_unique<llvm::IRBuilder<>>(*context);
    }

    auto emit() -> void;

    bool instantiating_constructor_ = false;
    size_t global_counter = 0;
    llvm::BasicBlock* true_bottom = {};
    llvm::Value* alloca = nullptr;
    bool is_this_ = false;

    std::shared_ptr<ClassDecl> curr_class_;

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> llvm_module;
    std::unique_ptr<llvm::IRBuilder<>> llvm_builder;
    std::map<std::string, llvm::Value*> named_values;

    std::stack<llvm::BasicBlock*> break_blocks;
    std::stack<llvm::BasicBlock*> continue_blocks;

    auto llvm_type(std::shared_ptr<Type> t) -> llvm::Type*;
    auto llvm_type(std::shared_ptr<ClassDecl> t) -> llvm::Type*;

    auto forward_declare_func(std::shared_ptr<Function> function) -> void;
    auto forward_declare_method(std::shared_ptr<MethodDecl> method) -> void;
    auto forward_declare_constructor(std::shared_ptr<ConstructorDecl> constructor) -> void;
    auto forward_declare_copy_constructor() -> void;
    auto forward_declare_destructor(std::shared_ptr<ClassDecl> class_) -> void;

    auto set_array_alloca(llvm::AllocaInst* a) -> void {
        array_alloca_ = a;
    }

    auto get_array_alloca() -> llvm::AllocaInst* {
        return array_alloca_;
    }

 private:
    std::shared_ptr<AllModules> modules_;
    llvm::AllocaInst* array_alloca_;
    std::shared_ptr<Module> main_module_;
    std::shared_ptr<Handler> handler_;
};

#endif // EMITTER_HPP