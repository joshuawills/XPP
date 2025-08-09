#ifndef EMITTER_HPP
#define EMITTER_HPP

class Module;
class AllModules;
class Type;
class Handler;

#include <map>
#include <memory>

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

    size_t global_counter = 0;
    llvm::BasicBlock* true_bottom = {};

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> llvm_module;
    std::unique_ptr<llvm::IRBuilder<>> llvm_builder;
    std::map<std::string, llvm::Value*> named_values;

    auto llvm_type(std::shared_ptr<Type> t) -> llvm::Type*;

 private:
    std::shared_ptr<AllModules> modules_;
    std::shared_ptr<Module> main_module_;
    std::shared_ptr<Handler> handler_;
};

#endif // EMITTER_HPP