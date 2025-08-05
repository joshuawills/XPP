#include "expr.hpp"

#include "emitter.hpp"

auto operator<<(std::ostream& os, Operator const& o) -> std::ostream& {
    switch (o) {
    case ASSIGN: os << "="; break;
    case LOGICAL_OR: os << "||"; break;
    case LOGICAL_AND: os << "&&"; break;
    case EQUAL: os << "=="; break;
    case NOT_EQUAL: os << "!="; break;
    case NEGATE: os << "!"; break;
    case PLUS: os << "+"; break;
    case MINUS: os << "-"; break;
    case MULTIPLY: os << "*"; break;
    case DIVIDE: os << "/"; break;
    case LESS_THAN: os << "<"; break;
    case GREATER_THAN: os << ">"; break;
    case LESS_EQUAL: os << "<="; break;
    case GREATER_EQUAL: os << ">="; break;
    default: os << "UNRECOGNISED OPERATOR";
    };
    return os;
}

auto EmptyExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto EmptyExpr::print(std::ostream& os) const -> void {
    os << "EmptyExpr " << pos();
}

auto AssignmentExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& rhs = right_->codegen(emitter);
    if (!rhs)
        return nullptr;
    auto const& lhs = std::dynamic_pointer_cast<VarExpr>(left_);
    emitter->named_values[lhs->get_name()] = rhs;
    return rhs;
}

auto AssignmentExpr::print(std::ostream& os) const -> void {
    os << "AssignmentExpr " << pos();
    os << "\t";
    left_->print(os);
    os << " " << op_ << " ";
    right_->print(os);
}

auto BinaryExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (op_ == Operator::LOGICAL_OR) {
        return handle_logical_or(emitter);
    }
    else if (op_ == Operator::LOGICAL_AND) {
        return handle_logical_and(emitter);
    }

    auto const& l = left_->codegen(emitter);
    auto const& r = right_->codegen(emitter);
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

auto BinaryExpr::print(std::ostream& os) const -> void {
    left_->print(os);
    os << " " << op_ << " ";
    right_->print(os);
}

auto UnaryExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& value = expr_->codegen(emitter);
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

auto UnaryExpr::print(std::ostream& os) const -> void {
    os << "UnaryExpr " << pos();
    os << "\t " << op_ << " ";
    expr_->print(os);
}

auto IntExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(64, value_, true));
}

auto IntExpr::print(std::ostream& os) const -> void {
    os << value_;
}

auto BoolExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(1, value_, true));
}

auto BoolExpr::print(std::ostream& os) const -> void {
    if (value_) {
        os << "true";
    }
    else {
        os << "false";
    }
}

auto VarExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto ptr = emitter->named_values[name_];
    return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), ptr, name_);
}

auto VarExpr::print(std::ostream& os) const -> void {
    os << name_;
}

auto BinaryExpr::handle_logical_and(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto const& lhs_false_value = std::to_string(emitter->global_counter++);
    auto const& lhs_false_block = llvm::BasicBlock::Create(*(emitter->context), lhs_false_value, function);

    auto const& rhs_value = std::to_string(emitter->global_counter++);
    auto const& rhs_block = llvm::BasicBlock::Create(*(emitter->context), rhs_value, function);

    auto const& merge_value = std::to_string(emitter->global_counter++);
    auto const& merge_block = llvm::BasicBlock::Create(*(emitter->context), merge_value, function);

    auto const& lhs_val = left_->codegen(emitter);
    if (!lhs_val)
        return nullptr;

    emitter->llvm_builder->CreateCondBr(lhs_val, rhs_block, lhs_false_block);

    emitter->llvm_builder->SetInsertPoint(lhs_false_block);
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(rhs_block);
    auto const& rhs_val = right_->codegen(emitter);
    if (!rhs_val)
        return nullptr;
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(merge_block);
    auto const& phi = emitter->llvm_builder->CreatePHI(llvm::Type::getInt1Ty(*(emitter->context)), 2);

    phi->addIncoming(llvm::ConstantInt::getFalse(*(emitter->context)), lhs_false_block);
    phi->addIncoming(rhs_val, rhs_block);

    return phi;
}

auto BinaryExpr::handle_logical_or(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
    auto const& lhs_true_value = std::to_string(emitter->global_counter++);
    auto const& lhs_true_block = llvm::BasicBlock::Create(*(emitter->context), lhs_true_value, function);

    auto const& rhs_value = std::to_string(emitter->global_counter++);
    auto const& rhs_block = llvm::BasicBlock::Create(*(emitter->context), rhs_value, function);

    auto const& merge_value = std::to_string(emitter->global_counter++);
    auto const& merge_block = llvm::BasicBlock::Create(*(emitter->context), merge_value, function);

    auto const& lhs_val = left_->codegen(emitter);
    if (!lhs_val)
        return nullptr;

    emitter->llvm_builder->CreateCondBr(lhs_val, lhs_true_block, rhs_block);

    emitter->llvm_builder->SetInsertPoint(lhs_true_block);
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(rhs_block);
    auto const& rhs_val = right_->codegen(emitter);
    if (!rhs_val)
        return nullptr;
    emitter->llvm_builder->CreateBr(merge_block);

    emitter->llvm_builder->SetInsertPoint(merge_block);
    auto const& phi = emitter->llvm_builder->CreatePHI(llvm::Type::getInt1Ty(*(emitter->context)), 2);

    phi->addIncoming(llvm::ConstantInt::getTrue(*(emitter->context)), lhs_true_block);
    phi->addIncoming(rhs_val, rhs_block);

    return phi;
}

auto CallExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto callee = emitter->llvm_module->getFunction(name_);
    auto arg_vals = std::vector<llvm::Value*>{};
    for (auto& arg : args_) {
        auto const val = arg->codegen(emitter);
        arg_vals.push_back(val);
    }
    return emitter->llvm_builder->CreateCall(callee, arg_vals);
}

auto CallExpr::print(std::ostream& os) const -> void {
    os << name_ << "(";
    for (auto const& arg : args_) {
        arg->print(os);
        if (arg != args_.back()) {
            os << ", ";
        }
    }
    os << ")";
    return;
}