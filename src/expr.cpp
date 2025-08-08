#include "expr.hpp"

#include "emitter.hpp"

auto operator<<(std::ostream& os, Op const& o) -> std::ostream& {
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
    case DEREF: os << "*"; break;
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

    if (lhs) {
        auto alloca = emitter->named_values[lhs->get_name()];
        emitter->llvm_builder->CreateStore(rhs, alloca);
    }
    else {
        auto const& lhs = std::dynamic_pointer_cast<UnaryExpr>(left_);
        auto ptr = lhs->get_expr()->codegen(emitter);
        emitter->llvm_builder->CreateStore(rhs, ptr);
    }

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
    if (op_ == Op::LOGICAL_OR) {
        return handle_logical_or(emitter);
    }
    else if (op_ == Op::LOGICAL_AND) {
        return handle_logical_and(emitter);
    }

    auto const& l = left_->codegen(emitter);
    auto const& r = right_->codegen(emitter);
    if (!l or !r) {
        return nullptr;
    }

    switch (op_) {
    case Op::PLUS: {
        if (is_pointer_arithmetic_) {
            auto inner_type = *left_->get_type().sub_type;
            return emitter->llvm_builder->CreateInBoundsGEP(emitter->llvm_type(inner_type), l, r);
        }
        return emitter->llvm_builder->CreateAdd(l, r);
    }
    case Op::MINUS: {
        if (is_pointer_arithmetic_) {
            auto neg = emitter->llvm_builder->CreateNeg(r);
            auto inner_type = *left_->get_type().sub_type;
            return emitter->llvm_builder->CreateInBoundsGEP(emitter->llvm_type(inner_type), l, neg);
        }
        return emitter->llvm_builder->CreateSub(l, r);
    }
    case Op::MULTIPLY: return emitter->llvm_builder->CreateMul(l, r);
    case Op::DIVIDE: return emitter->llvm_builder->CreateSDiv(l, r);
    case Op::EQUAL: return emitter->llvm_builder->CreateICmpEQ(l, r);
    case Op::NOT_EQUAL: return emitter->llvm_builder->CreateICmpNE(l, r);
    case Op::LESS_THAN: return emitter->llvm_builder->CreateICmpSLT(l, r);
    case Op::LESS_EQUAL: return emitter->llvm_builder->CreateICmpSLE(l, r);
    case Op::GREATER_THAN: return emitter->llvm_builder->CreateICmpSGT(l, r);
    case Op::GREATER_EQUAL: return emitter->llvm_builder->CreateICmpSGE(l, r);
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
    if (op_ == Op::ADDRESS_OF) {
        auto var_e = std::dynamic_pointer_cast<VarExpr>(expr_);
        return emitter->named_values[var_e->get_name()];
    }

    auto const& value = expr_->codegen(emitter);
    if (!value) {
        return nullptr;
    }

    if (op_ == Op::NEGATE) {
        return emitter->llvm_builder->CreateSub(llvm::ConstantInt::get(llvm::Type::getInt64Ty(*(emitter->context)), 0),
                                                value);
    }
    else if (op_ == Op::MINUS) {
        return emitter->llvm_builder->CreateICmpEQ(value, llvm::ConstantInt::get(value->getType(), 0));
    }
    else if (op_ == Op::DEREF) {
        return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), value);
    }
    else {
        std::cout << "UNREACHABLE UnaryExpr::codegen " << op_ << "\n";
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
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(width_, value_, true));
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

auto StringExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto str_const = llvm::ConstantDataArray::getString(*emitter->context, value_, true);

    auto const global_name = ".str" + std::to_string(emitter->global_counter++);

    auto global_str = new llvm::GlobalVariable(*emitter->llvm_module,
                                               str_const->getType(),
                                               true,
                                               llvm::GlobalValue::PrivateLinkage,
                                               str_const,
                                               global_name);

    global_str->setAlignment(llvm::Align(1));

    auto zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*emitter->context), 0);
    llvm::Value* indices[] = {zero, zero};
    return llvm::ConstantExpr::getInBoundsGetElementPtr(str_const->getType(), global_str, indices);
}

auto StringExpr::print(std::ostream& os) const -> void {
    os << "\"" << value_ << "\"";
}

auto CharExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(8, value_, true));
}

auto CharExpr::print(std::ostream& os) const -> void {
    os << "\'" << value_ << "\'";
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

auto CastExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto value = expr_->codegen(emitter);
    auto expr_type = expr_->get_type();

    if (to_ == expr_type) {
        return value;
    }

    if (to_.is_int() and expr_type.is_int()) {
        auto src_bits = value->getType()->getIntegerBitWidth();
        auto dest_bits = emitter->llvm_type(to_)->getIntegerBitWidth();

        if (dest_bits > src_bits) {
            // Extend
            return emitter->llvm_builder->CreateSExt(value, emitter->llvm_type(to_));
        }
        else if (dest_bits < src_bits) {
            // Truncate
            return emitter->llvm_builder->CreateTrunc(value, emitter->llvm_type(to_));
        }
        else {
            // They're equal
            return emitter->llvm_builder->CreateBitCast(value, emitter->llvm_type(to_));
        }
    }
    else {
        std::cout << "UNREACHABLE CastExpr::codegen\n";
    }

    return nullptr;
}

auto CastExpr::print(std::ostream& os) const -> void {
    expr_->print(os);
    os << " as " << to_;
}