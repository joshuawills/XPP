#include "expr.hpp"

#include "decl.hpp"
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
    case ADDRESS_OF: os << "&"; break;
    case POSTFIX_ADD:
    case PREFIX_ADD: os << "++"; break;
    case POSTFIX_MINUS:
    case PREFIX_MINUS: os << "--"; break;
    case MODULO: os << "%"; break;
    case PLUS_ASSIGN: os << "+="; break;
    case MINUS_ASSIGN: os << "-="; break;
    case MULTIPLY_ASSIGN: os << "*="; break;
    case DIVIDE_ASSIGN: os << "/="; break;
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
    auto rhs = right_->codegen(emitter);
    if (!rhs)
        return nullptr;

    auto const is_decimal = get_type()->is_decimal();
    auto const is_unsigned = get_type()->is_unsigned_int();
    auto const is_pointer = get_type()->is_pointer();

    llvm::Value* ptr = nullptr;
    if (auto const& lhs = std::dynamic_pointer_cast<VarExpr>(left_)) {
        ptr = emitter->named_values[lhs->get_name() + lhs->get_ref()->get_append()];
    }
    else if (auto const& lhs = std::dynamic_pointer_cast<UnaryExpr>(left_)) {
        ptr = lhs->get_expr()->codegen(emitter);
    }
    else {
        std::cout << "UNREACHABLE AssignmentExpr::codegen";
    }
    auto const loaded_ptr = emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), ptr);

    auto result = rhs;
    if (op_ == Op::PLUS_ASSIGN or op_ == Op::MINUS_ASSIGN) {
        auto const is_plus = op_ == Op::PLUS_ASSIGN;
        if (is_pointer) {
            auto index = rhs;
            if (!is_plus) {
                auto const zero = llvm::ConstantInt::get(rhs->getType(), 0);
                index = emitter->llvm_builder->CreateSub(zero, rhs);
            }
            auto p_t = std::dynamic_pointer_cast<PointerType>(left_->get_type());
            auto inner_type = emitter->llvm_type(p_t->get_sub_type());
            result = emitter->llvm_builder->CreateGEP(inner_type, loaded_ptr, index);
        }
        else if (is_decimal) {
            result = (is_plus) ? emitter->llvm_builder->CreateFAdd(loaded_ptr, rhs)
                               : emitter->llvm_builder->CreateFSub(loaded_ptr, rhs);
        }
        else {
            result = (is_plus) ? emitter->llvm_builder->CreateAdd(loaded_ptr, rhs)
                               : emitter->llvm_builder->CreateSub(loaded_ptr, rhs);
        }
    }
    else if (op_ == Op::MULTIPLY_ASSIGN) {
        if (is_decimal) {
            result = emitter->llvm_builder->CreateFMul(loaded_ptr, rhs);
        }
        else {
            result = emitter->llvm_builder->CreateMul(loaded_ptr, rhs);
        }
    }
    else if (op_ == Op::DIVIDE_ASSIGN) {
        if (is_decimal) {
            result = emitter->llvm_builder->CreateFDiv(loaded_ptr, rhs);
        }
        else if (is_unsigned) {
            result = emitter->llvm_builder->CreateUDiv(loaded_ptr, rhs);
        }
        else {
            result = emitter->llvm_builder->CreateSDiv(loaded_ptr, rhs);
        }
    }

    emitter->llvm_builder->CreateStore(result, ptr);
    return result;
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

    auto const is_decimal = left_->get_type()->is_decimal();
    auto const is_unsigned = left_->get_type()->is_unsigned_int();

    switch (op_) {
    case Op::MODULO: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFRem(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateURem(l, r);
        }
        else {
            return emitter->llvm_builder->CreateSRem(l, r);
        }
    }
    case Op::PLUS: {
        if (is_pointer_arithmetic_) {
            auto p_t = std::dynamic_pointer_cast<PointerType>(left_->get_type());
            auto inner_type = p_t->get_sub_type();
            return emitter->llvm_builder->CreateInBoundsGEP(emitter->llvm_type(inner_type), l, r);
        }
        return (is_decimal) ? emitter->llvm_builder->CreateFAdd(l, r) : emitter->llvm_builder->CreateAdd(l, r);
    }
    case Op::MINUS: {
        if (is_pointer_arithmetic_) {
            auto neg = emitter->llvm_builder->CreateNeg(r);
            auto p_t = std::dynamic_pointer_cast<PointerType>(left_->get_type());
            auto inner_type = p_t->get_sub_type();
            return emitter->llvm_builder->CreateInBoundsGEP(emitter->llvm_type(inner_type), l, neg);
        }
        return (is_decimal) ? emitter->llvm_builder->CreateFSub(l, r) : emitter->llvm_builder->CreateSub(l, r);
    }
    case Op::MULTIPLY:
        return (is_decimal) ? emitter->llvm_builder->CreateFMul(l, r) : emitter->llvm_builder->CreateMul(l, r);
    case Op::DIVIDE: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFDiv(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateUDiv(l, r);
        }
        else {
            return emitter->llvm_builder->CreateSDiv(l, r);
        }
    }
    case Op::EQUAL:
        return (is_decimal) ? emitter->llvm_builder->CreateFCmpOEQ(l, r) : emitter->llvm_builder->CreateICmpEQ(l, r);
    case Op::NOT_EQUAL:
        return (is_decimal) ? emitter->llvm_builder->CreateFCmpONE(l, r) : emitter->llvm_builder->CreateICmpNE(l, r);
    case Op::LESS_THAN: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFCmpOLT(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateICmpULT(l, r);
        }
        else {
            return emitter->llvm_builder->CreateICmpSLT(l, r);
        }
    }
    case Op::LESS_EQUAL: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFCmpOLE(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateICmpULE(l, r);
        }
        else {
            return emitter->llvm_builder->CreateICmpSLE(l, r);
        }
    }
    case Op::GREATER_THAN: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFCmpOGT(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateICmpUGT(l, r);
        }
        else {
            return emitter->llvm_builder->CreateICmpSGT(l, r);
        }
    }
    case Op::GREATER_EQUAL: {
        if (is_decimal) {
            return emitter->llvm_builder->CreateFCmpOGE(l, r);
        }
        else if (is_unsigned) {
            return emitter->llvm_builder->CreateICmpUGE(l, r);
        }
        else {
            return emitter->llvm_builder->CreateICmpSGE(l, r);
        }
    }
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
        return emitter->named_values[var_e->get_name() + var_e->get_ref()->get_append()];
    }

    auto const& value = expr_->codegen(emitter);
    if (!value) {
        return nullptr;
    }
    auto const is_decimal = expr_->get_type()->is_decimal();
    auto const is_pointer = expr_->get_type()->is_pointer();

    auto const int_one = llvm::ConstantInt::get(value->getType(), 1);
    auto const decimal_one = llvm::ConstantFP::get(*(emitter->context), llvm::APFloat(1.0));

    llvm::Value* ptr = nullptr;
    if (auto l = std::dynamic_pointer_cast<VarExpr>(expr_)) {
        ptr = emitter->named_values[l->get_name() + l->get_ref()->get_append()];
    }
    else if (auto l = std::dynamic_pointer_cast<UnaryExpr>(expr_)) {
        ptr = l->get_expr()->codegen(emitter);
    }

    if (op_ == Op::PREFIX_ADD or op_ == Op::PREFIX_MINUS) {
        llvm::Value* new_val;
        if (is_pointer) {
            auto const index =
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*(emitter->context)), (op_ == Op::PREFIX_ADD) ? 1 : -1);
            auto const p_t = std::dynamic_pointer_cast<PointerType>(expr_->get_type());
            auto const inner_type = p_t->get_sub_type();
            new_val = emitter->llvm_builder->CreateGEP(emitter->llvm_type(inner_type), value, index);
        }
        else if (is_decimal) {
            new_val = (op_ == Op::PREFIX_ADD) ? emitter->llvm_builder->CreateFAdd(value, decimal_one)
                                              : emitter->llvm_builder->CreateFSub(value, decimal_one);
        }
        else {
            new_val = (op_ == Op::PREFIX_ADD) ? emitter->llvm_builder->CreateAdd(value, int_one)
                                              : emitter->llvm_builder->CreateSub(value, int_one);
        }
        emitter->llvm_builder->CreateStore(new_val, ptr);
        return new_val;
    }
    else if (op_ == Op::POSTFIX_ADD or op_ == Op::POSTFIX_MINUS) {
        llvm::Value* new_val;
        if (is_pointer) {
            auto const index =
                llvm::ConstantInt::get(llvm::Type::getInt32Ty(*(emitter->context)), (op_ == Op::POSTFIX_ADD) ? 1 : -1);
            auto const p_t = std::dynamic_pointer_cast<PointerType>(expr_->get_type());
            auto const inner_type = p_t->get_sub_type();
            new_val = emitter->llvm_builder->CreateGEP(emitter->llvm_type(inner_type), value, index);
        }
        else if (is_decimal) {
            new_val = (op_ == Op::POSTFIX_ADD) ? emitter->llvm_builder->CreateFAdd(value, decimal_one)
                                               : emitter->llvm_builder->CreateFSub(value, decimal_one);
        }
        else {
            new_val = (op_ == Op::POSTFIX_ADD) ? emitter->llvm_builder->CreateAdd(value, int_one)
                                               : emitter->llvm_builder->CreateSub(value, int_one);
        }
        emitter->llvm_builder->CreateStore(new_val, ptr);
        return value;
    }
    else if (op_ == Op::NEGATE) {
        return emitter->llvm_builder->CreateICmpEQ(value, llvm::ConstantInt::get(value->getType(), 0));
    }
    else if (op_ == Op::MINUS) {
        auto ty = value->getType();
        auto int_zero = llvm::ConstantInt::get(ty, 0);
        auto decimal_zero = llvm::ConstantFP::get(*(emitter->context), llvm::APFloat(0.0));
        if (is_decimal) {
            return emitter->llvm_builder->CreateFSub(decimal_zero, value);
        }
        else {
            return emitter->llvm_builder->CreateSub(int_zero, value);
        }
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

auto UIntExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(width_, value_, false));
}

auto UIntExpr::print(std::ostream& os) const -> void {
    os << value_;
}

auto DecimalExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (width_ == 64) {
        return llvm::ConstantFP::get(*(emitter->context), llvm::APFloat(value_));
    }
    else if (width_ == 32) {
        return llvm::ConstantFP::get(*(emitter->context), llvm::APFloat((float)value_));
    }
    else {
        std::cout << "UNREACHABLE DecimalExpr::codegen\n";
        return nullptr;
    }
}

auto DecimalExpr::print(std::ostream& os) const -> void {
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
    auto ptr = emitter->named_values[name_ + get_ref()->get_append()];
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
    auto name = std::string{};
    if (auto l = std::dynamic_pointer_cast<Function>(ref_)) {
        name = l->get_ident() + l->get_type_output();
    }
    else {
        name = name_;
    }
    auto callee = emitter->llvm_module->getFunction(name);
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

    auto const llvm_type = emitter->llvm_type(to_);

    // int -> int
    if (to_->is_int() and expr_type->is_int()) {
        auto src_bits = value->getType()->getIntegerBitWidth();
        auto dest_bits = llvm_type->getIntegerBitWidth();

        if (dest_bits > src_bits) {
            if (expr_type->is_unsigned_int()) {
                return emitter->llvm_builder->CreateZExt(value, llvm_type);
            }
            else {
                return emitter->llvm_builder->CreateSExt(value, llvm_type);
            }
        }
        else if (dest_bits < src_bits) {
            return emitter->llvm_builder->CreateTrunc(value, llvm_type);
        }
        else {
            return emitter->llvm_builder->CreateBitCast(value, llvm_type);
        }
    }
    // decimal -> decimal
    else if (to_->is_decimal() and expr_type->is_decimal()) {
        auto src_bits = value->getType()->getPrimitiveSizeInBits();
        auto dest_bits = llvm_type->getPrimitiveSizeInBits();

        if (dest_bits > src_bits) {
            return emitter->llvm_builder->CreateFPExt(value, llvm_type);
        }
        else if (dest_bits < src_bits) {
            return emitter->llvm_builder->CreateFPTrunc(value, llvm_type);
        }
        else {
            return emitter->llvm_builder->CreateBitCast(value, llvm_type);
        }
    }
    // int -> decimal
    else if (to_->is_decimal() and expr_type->is_int()) {
        if (expr_type->is_unsigned_int()) {
            return emitter->llvm_builder->CreateUIToFP(value, llvm_type);
        }
        else {
            return emitter->llvm_builder->CreateSIToFP(value, llvm_type);
        }
    }
    // decimal -> int
    else if (to_->is_int() and expr_type->is_decimal()) {
        if (expr_type->is_unsigned_int()) {
            return emitter->llvm_builder->CreateFPToUI(value, llvm_type);
        }
        else {
            return emitter->llvm_builder->CreateFPToSI(value, llvm_type);
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