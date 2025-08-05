#include "expr.hpp"

#include "emitter.hpp"

auto EmptyExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto AssignmentExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto rhs = right_->codegen(emitter);
    if (!rhs)
        return nullptr;
    auto lhs = std::dynamic_pointer_cast<VarExpr>(left_);
    emitter->named_values[lhs->get_name()] = rhs;
    return rhs;
}

auto BinaryExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (op_ == Operator::LOGICAL_OR) {
        return handle_logical_or(emitter);
    }
    else if (op_ == Operator::LOGICAL_AND) {
        return handle_logical_and(emitter);
    }

    auto l = left_->codegen(emitter);
    auto r = right_->codegen(emitter);
    if (!l or !r) {
        return nullptr;
    }

    switch (op_) {
    case Operator::PLUS: return emitter->llvm_builder->CreateAdd(l, r);
    case Operator::MINUS: return emitter->llvm_builder->CreateSub(l, r);
    case Operator::MULTIPLY: return emitter->llvm_builder->CreateMul(l, r);
    case Operator::DIVIDE: return emitter->llvm_builder->CreateSDiv(l, r);
    case Operator::EQUAL: return emitter->llvm_builder->CreateICmpEQ(l, r);
    case Operator::NOT_EQUAL: return emitter->llvm_builder->CreateICmpNE(l, r);
    case Operator::LESS_THAN: return emitter->llvm_builder->CreateICmpSLT(l, r);
    case Operator::LESS_EQUAL: return emitter->llvm_builder->CreateICmpSLE(l, r);
    case Operator::GREATER_THAN: return emitter->llvm_builder->CreateICmpSGT(l, r);
    case Operator::GREATER_EQUAL: return emitter->llvm_builder->CreateICmpSGE(l, r);
    default: std::cout << "UNREACHABLE BinaryExpr::codegen\n";
    }

    return nullptr;
}

auto UnaryExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto value = expr_->codegen(emitter);
    if (!value) {
        return nullptr;
    }

    if (op_ == Operator::NEGATE) {
        return emitter->llvm_builder->CreateSub(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*(emitter->context)), 0),
                                                value);
    }
    else if (op_ == Operator::MINUS) {
        return emitter->llvm_builder->CreateICmpEQ(value, llvm::ConstantInt::get(value->getType(), 0));
    }
    else {
        std::cout << "UNREACHABLE UnaryExpr::codegen\n";
    }
    return nullptr;
}

auto IntExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(64, value_, true));
}

auto BoolExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(1, value_, true));
}

auto VarExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return emitter->named_values[name_];
}

auto BinaryExpr::handle_logical_and(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto lhs_false_value = std::to_string(emitter->global_counter++);
    auto lhs_false_block = llvm::BasicBlock::Create(*(emitter->context), lhs_false_value, function);

    auto rhs_value = std::to_string(emitter->global_counter++);
    auto rhs_block = llvm::BasicBlock::Create(*(emitter->context), rhs_value, function);

    auto merge_value = std::to_string(emitter->global_counter++);
    auto merge_block = llvm::BasicBlock::Create(*(emitter->context), merge_value, function);

    auto lhs_val = left_->codegen(emitter);
    if (!lhs_val)
        return nullptr;

    emitter->llvm_builder->CreateCondBr(lhs_val, rhs_block, lhs_false_block);

    emitter->llvm_builder->SetInsertPoint(lhs_false_block);
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(rhs_block);
    auto rhs_val = right_->codegen(emitter);
    if (!rhs_val)
        return nullptr;
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(merge_block);
    auto phi = emitter->llvm_builder->CreatePHI(llvm::Type::getInt1Ty(*(emitter->context)), 2);

    phi->addIncoming(llvm::ConstantInt::getFalse(*(emitter->context)), lhs_false_block);
    phi->addIncoming(rhs_val, rhs_block);

    return phi;
}

auto BinaryExpr::handle_logical_or(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto lhs_true_value = std::to_string(emitter->global_counter++);
    auto lhs_true_block = llvm::BasicBlock::Create(*(emitter->context), lhs_true_value, function);

    auto rhs_value = std::to_string(emitter->global_counter++);
    auto rhs_block = llvm::BasicBlock::Create(*(emitter->context), rhs_value, function);

    auto merge_value = std::to_string(emitter->global_counter++);
    auto merge_block = llvm::BasicBlock::Create(*(emitter->context), merge_value, function);

    auto lhs_val = left_->codegen(emitter);
    if (!lhs_val)
        return nullptr;

    emitter->llvm_builder->CreateCondBr(lhs_val, lhs_true_block, rhs_block);

    emitter->llvm_builder->SetInsertPoint(lhs_true_block);
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(rhs_block);
    auto rhs_val = right_->codegen(emitter);
    if (!rhs_val)
        return nullptr;
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(merge_block);
    auto phi = emitter->llvm_builder->CreatePHI(llvm::Type::getInt1Ty(*(emitter->context)), 2);

    phi->addIncoming(llvm::ConstantInt::getTrue(*(emitter->context)), lhs_true_block);
    phi->addIncoming(rhs_val, rhs_block);

    return phi;
}