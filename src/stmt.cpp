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

auto CompoundStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    for (auto& stmt : stmts_) {
        stmt->codegen(emitter);
    }
    return nullptr;
}

auto CompoundStmt::print(std::ostream& os) const -> void {
    for (auto const& stmt : stmts_) {
        os << "\t\t";
        stmt->print(os);
    }
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
    compound_stmt_->codegen(emitter);
    emitter->llvm_builder->CreateBr(top_block);
    emitter->llvm_builder->SetInsertPoint(end_block);

    return nullptr;
}

auto WhileStmt::print(std::ostream& os) const -> void {
    os << "while ";
    cond_->print(os);
    os << "{";
    compound_stmt_->print(os);
    os << "}\n";
    return;
}

auto IfStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto const middle_block_value = std::to_string(emitter->global_counter++);
    auto const middle_block = llvm::BasicBlock::Create(*(emitter->context), middle_block_value, function);

    auto const else_block_value = std::to_string(emitter->global_counter++);
    auto const else_block = llvm::BasicBlock::Create(*(emitter->context), else_block_value, function);

    auto const bottom_block_value = std::to_string(emitter->global_counter++);
    auto const bottom_block = llvm::BasicBlock::Create(*(emitter->context), bottom_block_value, function);
    emitter->true_bottom = bottom_block;

    auto const c = cond_->codegen(emitter);
    emitter->llvm_builder->CreateCondBr(c, middle_block, else_block);
    emitter->llvm_builder->SetInsertPoint(middle_block);

    stmt_one_->codegen(emitter);
    auto temp = std::dynamic_pointer_cast<CompoundStmt>(stmt_one_);
    if (!temp or (temp and !temp->has_return())) {
        emitter->llvm_builder->CreateBr(bottom_block);
    }

    emitter->llvm_builder->SetInsertPoint(else_block);
    stmt_two_->codegen(emitter);
    stmt_three_->codegen(emitter);
    temp = std::dynamic_pointer_cast<CompoundStmt>(stmt_one_);
    if (!temp or (temp and !temp->has_return())) {
        emitter->llvm_builder->CreateBr(bottom_block);
    }

    emitter->llvm_builder->SetInsertPoint(bottom_block);
    emitter->true_bottom = nullptr;
    return nullptr;
}

auto IfStmt::print(std::ostream& os) const -> void {
    os << "if ";
    cond_->print(os);
    os << "{";
    stmt_one_->print(os);
    os << "}\n";
    stmt_two_->print(os);
    stmt_three_->print(os);
    return;
}

auto ElseIfStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto const middle_block_value = std::to_string(emitter->global_counter++);
    auto const middle_block = llvm::BasicBlock::Create(*(emitter->context), middle_block_value, function);

    auto const next_block_value = std::to_string(emitter->global_counter++);
    auto const next_block = llvm::BasicBlock::Create(*(emitter->context), next_block_value, function);

    auto const c = cond_->codegen(emitter);
    emitter->llvm_builder->CreateCondBr(c, middle_block, next_block);

    emitter->llvm_builder->SetInsertPoint(middle_block);
    stmt_one_->codegen(emitter);

    auto temp = std::dynamic_pointer_cast<CompoundStmt>(stmt_one_);
    if (!temp or (temp and !temp->has_return())) {
        emitter->llvm_builder->CreateBr(emitter->true_bottom);
    }

    emitter->llvm_builder->SetInsertPoint(next_block);
    stmt_two_->codegen(emitter);

    return nullptr;
}

auto ElseIfStmt::print(std::ostream& os) const -> void {
    os << "else if ";
    cond_->print(os);
    os << "{";
    stmt_one_->print(os);
    os << "}\n";
    stmt_two_->print(os);
    return;
}