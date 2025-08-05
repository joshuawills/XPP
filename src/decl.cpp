#include "decl.hpp"

#include <iostream>

auto Function::operator==(const Function& other) const -> bool {
    if (this == &other) {
        return true;
    }
    if (ident_ != other.ident_) {
        return false;
    }
    if (t_ != other.t_) {
        return false;
    }
    if (paras_.size() != other.paras_.size()) {
        return false;
    }
    for (size_t i = 0; i < paras_.size(); ++i) {
        if (*paras_[i] != *other.paras_[i]) {
            return false;
        }
    }
    return true;
}

auto Function::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto return_type = emitter->llvm_type(get_type());

    // Handling params
    auto param_types = std::vector<llvm::Type*>{};
    for (auto& para : paras_) {
        param_types.push_back(emitter->llvm_type(para->get_type()));
    }

    // Instantiating function
    auto func_type = llvm::FunctionType::get(return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, get_ident(), *emitter->llvm_module);

    // Setting names of function params
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(paras_[idx++]->get_ident());
    }

    auto entry_name = std::to_string(emitter->global_counter++);
    auto entry_block = llvm::BasicBlock::Create(*emitter->context, entry_name, func);
    emitter->llvm_builder->SetInsertPoint(entry_block);

    for (auto& arg : func->args()) {
        auto alloca = emitter->llvm_builder->CreateAlloca(arg.getType(), nullptr, arg.getName());
        emitter->llvm_builder->CreateStore(&arg, alloca);
        emitter->named_values[arg.getName().str()] = alloca;
    }

    for (auto& stmt : stmts_) {
        stmt->codegen(emitter);
    }

    if (return_type->isVoidTy()) {
        emitter->llvm_builder->CreateRetVoid();
    }
    else if (!entry_block->getTerminator()) {
        emitter->llvm_builder->CreateRet(llvm::Constant::getNullValue(return_type));
    }

    return func;
}

auto ParaDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto LocalVarDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto llvm_type = emitter->llvm_type(get_type());
    auto alloca = emitter->llvm_builder->CreateAlloca(llvm_type, nullptr, get_ident());

    auto init_val = e_->codegen(emitter);
    if (init_val) {
        emitter->llvm_builder->CreateStore(init_val, alloca);
    }

    emitter->named_values[get_ident()] = alloca;
    return alloca;
}