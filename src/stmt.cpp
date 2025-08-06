#include "stmt.hpp"
#include "decl.hpp"
#include "emitter.hpp"

auto EmptyStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto EmptyStmt::print(std::ostream& os) const -> void {
    os << ";\n";
}

auto LocalVarStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    decl_->codegen(emitter);
    return nullptr;
}

auto LocalVarStmt::print(std::ostream& os) const -> void {
    decl_->print(os);
    os << ";";
    return;
}
auto ReturnStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (std::dynamic_pointer_cast<EmptyExpr>(expr_)) {
        return emitter->llvm_builder->CreateRetVoid();
    }
    auto const& val = expr_->codegen(emitter);
    if (!val)
        return nullptr;
    return emitter->llvm_builder->CreateRet(val);
}

auto ReturnStmt::print(std::ostream& os) const -> void {
    os << "return ";
    expr_->print(os);
    os << ";\n";
    return;
}

auto ExprStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    return expr_->codegen(emitter);
}

auto ExprStmt::print(std::ostream& os) const -> void {
    expr_->print(os);
    os << ";\n";
    return;
}

auto WhileStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto const top_block_value = std::to_string(emitter->global_counter++);
    auto const top_block = llvm::BasicBlock::Create(*(emitter->context), top_block_value, function);

    auto const stmt_block_value = std::to_string(emitter->global_counter++);
    auto const stmt_block = llvm::BasicBlock::Create(*(emitter->context), stmt_block_value, function);

    auto const end_block_value = std::to_string(emitter->global_counter++);
    auto const end_block = llvm::BasicBlock::Create(*(emitter->context), end_block_value, function);

    emitter->llvm_builder->CreateBr(top_block);
    emitter->llvm_builder->SetInsertPoint(top_block);

    auto const& val = cond_->codegen(emitter);
    if (!val)
        return nullptr;
    emitter->llvm_builder->CreateCondBr(val, stmt_block, end_block);

    emitter->llvm_builder->SetInsertPoint(stmt_block);
    for (auto& stmt : stmts_) {
        stmt->codegen(emitter);
    }
    emitter->llvm_builder->CreateBr(top_block);
    emitter->llvm_builder->SetInsertPoint(end_block);

    return nullptr;
}

auto WhileStmt::print(std::ostream& os) const -> void {
    os << "while ";
    cond_->print(os);
    os << "{";
    for (auto const& stmt : stmts_) {
        stmt->print(os);
    }
    os << "}\n";
    return;
}