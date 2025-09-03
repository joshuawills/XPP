#include "expr.hpp"

#include "decl.hpp"
#include "emitter.hpp"

#include <cassert>

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
        auto is_field_access = std::dynamic_pointer_cast<ClassFieldDecl>(lhs->get_ref());
        if (is_field_access) {
            auto const t = emitter->named_values["this"];
            auto const class_type = emitter->llvm_type(emitter->curr_class_);
            auto this_ptr = emitter->llvm_builder->CreateLoad(llvm::PointerType::getUnqual(class_type), t);
            ptr = emitter->llvm_builder->CreateStructGEP(
                class_type,
                this_ptr,
                emitter->curr_class_->get_index_for_field(is_field_access->get_ident()));
        }
        else {
            ptr = emitter->named_values[lhs->get_name() + lhs->get_ref()->get_append()];
        }
    }
    else if (auto const& lhs = std::dynamic_pointer_cast<UnaryExpr>(left_)) {
        ptr = lhs->get_expr()->codegen(emitter);
    }
    else if (auto const& lhs = std::dynamic_pointer_cast<ArrayIndexExpr>(left_)) {
        ptr = lhs->codegen(emitter);
    }
    else if (auto const& lhs = std::dynamic_pointer_cast<FieldAccessExpr>(left_)) {
        if (auto l = std::dynamic_pointer_cast<VarExpr>(lhs->get_class_instance())) {
            emitter->is_this_ = l->get_name() == "this";
        }
        auto const class_instance = lhs->get_class_instance()->codegen(emitter);
        if (!class_instance) {
            return nullptr;
        }
        emitter->is_this_ = false;
        auto const class_type = emitter->llvm_type(lhs->get_class_ref());
        ptr = emitter->llvm_builder->CreateStructGEP(class_type, class_instance, lhs->get_field_num());
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
        if (var_e) {
            return emitter->named_values[var_e->get_name() + var_e->get_ref()->get_append()];
        }
        auto index_e = std::dynamic_pointer_cast<ArrayIndexExpr>(expr_);
        if (index_e) {
            return index_e->codegen(emitter);
        }
        std::cout << "UNREACHABLE UnaryExpr::codegen\n";
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
        auto is_field_access = std::dynamic_pointer_cast<ClassFieldDecl>(l->get_ref());
        if (is_field_access) {
            auto const t = emitter->named_values["this"];
            auto const class_type = emitter->llvm_type(emitter->curr_class_);
            auto this_ptr = emitter->llvm_builder->CreateLoad(llvm::PointerType::getUnqual(class_type), t);
            ptr = emitter->llvm_builder->CreateStructGEP(
                class_type,
                this_ptr,
                emitter->curr_class_->get_index_for_field(is_field_access->get_ident()));
        }
        else {
            ptr = emitter->named_values[l->get_name() + l->get_ref()->get_append()];
        }
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
        if (get_type()->is_class()) {
            return value;
        }
        else {
            return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), value);
        }
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

auto NullExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const t = llvm::PointerType::get(llvm::Type::getVoidTy(*emitter->context), 0);
    return llvm::ConstantPointerNull::get(t);
}

auto NullExpr::print(std::ostream& os) const -> void {
    os << "null";
}

auto IntExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    return llvm::ConstantInt::get(*(emitter->context), llvm::APInt(width_, value_, true));
}

auto IntExpr::print(std::ostream& os) const -> void {
    os << value_;
}

auto UIntExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
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
    auto is_field_access = std::dynamic_pointer_cast<ClassFieldDecl>(get_ref());
    if (emitter->is_this_ and !is_field_access) {
        auto this_ = emitter->named_values["this"];
        auto t = emitter->llvm_type(get_type());
        return emitter->llvm_builder->CreateLoad(t, this_);
    }
    else if (is_field_access) {
        assert(emitter->curr_class_ != nullptr);
        auto const field_index = emitter->curr_class_->get_index_for_field(is_field_access->get_ident());
        auto const this_ptr = emitter->named_values["this"];
        auto const class_type = emitter->llvm_type(emitter->curr_class_);

        auto const this_pointer = emitter->llvm_builder->CreateLoad(llvm::PointerType::getUnqual(class_type), this_ptr);
        auto const val = emitter->llvm_builder->CreateStructGEP(class_type, this_pointer, field_index);
        return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), val);
    }

    auto const ptr = emitter->named_values[name_ + get_ref()->get_append()];
    if (get_type()->is_class() or get_type()->is_array()) {
        return ptr;
    }
    else {
        return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), ptr, name_);
    }
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
    if (!callee) {
        std::cout << "UNREACHABLE CallExpr::codegen, function not found: " << name << "\n";
        return nullptr;
    }

    auto arg_vals = std::vector<llvm::Value*>{};
    for (auto& arg : args_) {
        auto val = arg->codegen(emitter);
        if (arg->get_type()->is_class() and std::dynamic_pointer_cast<VarExpr>(arg)) {
            // Call the copy constructor
            auto alloca = emitter->llvm_builder->CreateAlloca(emitter->llvm_type(arg->get_type()));
            auto class_type = std::dynamic_pointer_cast<ClassType>(arg->get_type());
            auto constructor =
                emitter->llvm_module->getFunction("copy_constructor." + class_type->get_ref()->get_ident());

            emitter->llvm_builder->CreateCall(constructor, {alloca, val});
            val = emitter->llvm_builder->CreateLoad(emitter->llvm_type(arg->get_type()), alloca);
        }
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

auto ConstructorCallExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto constructor_ref = std::dynamic_pointer_cast<ConstructorDecl>(ref_);
    if (!constructor_ref) {
        // Assume it's a default copy constructor call
        auto callee = emitter->llvm_module->getFunction("copy_constructor." + name_);
        auto arg_vals = std::vector<llvm::Value*>{};
        arg_vals.push_back(emitter->alloca);
        for (auto& arg : args_) {
            auto val = arg->codegen(emitter);
            arg_vals.push_back(val);
        }
        return emitter->llvm_builder->CreateCall(callee, arg_vals);
    }

    auto is_copy_constructor = false;
    if (args_.size() == 1) {
        if (auto l = std::dynamic_pointer_cast<PointerType>(args_[0]->get_type())) {
            if (*l->get_sub_type() == *emitter->curr_class_->get_type()) {
                is_copy_constructor = true;
            }
        }
    }

    auto name = std::string{};
    if (is_copy_constructor) {
        name = "copy_constructor." + constructor_ref->get_ident();
    }
    else {
        name = "constructor." + constructor_ref->get_ident() + constructor_ref->get_type_output();
    }

    auto callee = emitter->llvm_module->getFunction(name);

    llvm::Value* class_ptr;
    if (emitter->instantiating_constructor_ and name_ == emitter->curr_class_->get_ident()) {
        auto val = emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), emitter->named_values["this"]);
        auto arg_vals = std::vector<llvm::Value*>{};
        arg_vals.push_back(val);
        for (auto& arg : args_) {
            auto const val = arg->codegen(emitter);
            arg_vals.push_back(val);
        }
        emitter->llvm_builder->CreateCall(callee, arg_vals);
        return nullptr;
    }
    else if (emitter->alloca) {
        class_ptr = emitter->alloca;
    }
    else {
        class_ptr = emitter->llvm_builder->CreateAlloca(emitter->llvm_type(get_type()),
                                                        nullptr,
                                                        std::to_string(emitter->global_counter++));
    }
    assert(class_ptr != nullptr);

    auto arg_vals = std::vector<llvm::Value*>{};

    arg_vals.push_back(class_ptr);

    for (auto& arg : args_) {
        auto const val = arg->codegen(emitter);
        arg_vals.push_back(val);
    }
    emitter->llvm_builder->CreateCall(callee, arg_vals);
    return class_ptr;
}

auto ConstructorCallExpr::print(std::ostream& os) const -> void {
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

// Assume this is NOT for GlobalVariableDecl (handled there)
auto ArrayInitExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const array_t = std::dynamic_pointer_cast<ArrayType>(get_type());
    if (!array_t)
        return nullptr;

    auto const llvm_type = emitter->llvm_type(array_t);

    auto alloca = emitter->get_array_alloca();

    auto i = 0u;
    llvm::Value* last_val;
    for (auto& expr : exprs_) {
        auto elem_val = expr->codegen(emitter);
        if (!elem_val)
            return nullptr;

        llvm::Value* indices[] = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*emitter->context), 0),
                                  llvm::ConstantInt::get(llvm::Type::getInt32Ty(*emitter->context), i)};

        auto elem_ptr = emitter->llvm_builder->CreateInBoundsGEP(llvm_type, alloca, indices);
        emitter->llvm_builder->CreateStore(elem_val, elem_ptr);
        ++i;
        last_val = elem_val;
    }
    while (i != 0 and i < *array_t->get_length()) {
        llvm::Value* indices[] = {llvm::ConstantInt::get(llvm::Type::getInt32Ty(*emitter->context), 0),
                                  llvm::ConstantInt::get(llvm::Type::getInt32Ty(*emitter->context), i)};

        auto elem_ptr = emitter->llvm_builder->CreateInBoundsGEP(llvm_type, alloca, indices);
        emitter->llvm_builder->CreateStore(last_val, elem_ptr);
        ++i;
    }

    return alloca;
}

auto ArrayInitExpr::print(std::ostream& os) const -> void {
    os << "[";
    for (auto& expr : exprs_) {
        expr->print(os);
        os << " ";
    }
    os << "]";
}

auto ArrayIndexExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const base_ptr = array_expr_->codegen(emitter);
    if (!base_ptr)
        return nullptr;

    auto const index_val = index_expr_->codegen(emitter);
    if (!index_val)
        return nullptr;

    auto const elem_type = emitter->llvm_type(get_type());
    auto const zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*emitter->context), 0);

    llvm::Value* gep_ptr;
    if (auto l = std::dynamic_pointer_cast<ArrayType>(array_expr_->get_type())) {
        llvm::Value* indices[] = {zero, index_val};
        gep_ptr = emitter->llvm_builder->CreateInBoundsGEP(emitter->llvm_type(l), base_ptr, indices);
    }
    else if (auto l2 = std::dynamic_pointer_cast<PointerType>(array_expr_->get_type())) {
        gep_ptr = emitter->llvm_builder->CreateInBoundsGEP(elem_type, base_ptr, index_val);
    }
    else {
        std::cout << "UNREACHABLE ArrayIndexExpr::codegen\n";
    }

    auto v = std::dynamic_pointer_cast<UnaryExpr>(get_parent());
    if (v) {
        if (v->get_operator() == Op::ADDRESS_OF) {
            return gep_ptr;
        }
        else {
            return emitter->llvm_builder->CreateLoad(elem_type, gep_ptr);
        }
    }

    if (auto l = std::dynamic_pointer_cast<AssignmentExpr>(get_parent())) {
        return gep_ptr;
    }
    else {
        return emitter->llvm_builder->CreateLoad(elem_type, gep_ptr);
    }
}

auto ArrayIndexExpr::print(std::ostream& os) const -> void {
    array_expr_->print(os);
    os << "[";
    index_expr_->print(os);
    os << "]";
}

auto EnumAccessExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    return llvm::ConstantInt::get((*emitter->context), llvm::APInt(64, field_num_, true));
}

auto EnumAccessExpr::print(std::ostream& os) const -> void {
    os << enum_name_ << "::" << field_;
}

auto FieldAccessExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (auto l = std::dynamic_pointer_cast<VarExpr>(class_instance_)) {
        emitter->is_this_ = l->get_name() == "this";
    }
    auto class_val = class_instance_->codegen(emitter);
    if (!class_val)
        return nullptr;
    emitter->is_this_ = false;
    auto const class_type = emitter->llvm_type(class_ref_);
    auto const val = emitter->llvm_builder->CreateStructGEP(class_type, class_val, field_num_);
    return emitter->llvm_builder->CreateLoad(emitter->llvm_type(get_type()), val);
}

auto FieldAccessExpr::print(std::ostream& os) const -> void {
    class_instance_->print(os);
    os << "." << field_name_;
}

auto MethodAccessExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    // First field in the method call
    if (auto l = std::dynamic_pointer_cast<VarExpr>(class_instance_)) {
        emitter->is_this_ = l->get_name() == "this";
    }
    auto class_val = class_instance_->codegen(emitter);
    emitter->is_this_ = false;

    auto name = "method." + ref_->get_class_ref()->get_ident() + method_name_ + ref_->get_type_output();
    auto function = emitter->llvm_module->getFunction(name);
    assert(function != nullptr);

    auto arg_vals = std::vector<llvm::Value*>{};
    arg_vals.push_back(class_val);
    for (auto& arg : args_) {
        arg_vals.push_back(arg->codegen(emitter));
    }
    return emitter->llvm_builder->CreateCall(function, arg_vals);
}

auto MethodAccessExpr::print(std::ostream& os) const -> void {
    class_instance_->print(os);
    os << "." << method_name_ << "(";
    for (size_t i = 0; i < args_.size(); ++i) {
        args_[i]->print(os);
        if (i < args_.size() - 1) {
            os << ", ";
        }
    }
    os << ")";
}

auto SizeOfExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    uint64_t size_ = 0;
    auto& data_layout = emitter->llvm_module->getDataLayout();
    if (is_type_) {
        auto t = emitter->llvm_type(type_to_size_);
        size_ = data_layout.getTypeAllocSize(t);
    }
    else {
        auto e_t = expr_to_size_->get_type();
        size_ = data_layout.getTypeAllocSize(emitter->llvm_type(e_t));
    }
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*emitter->context), size_);
}

auto SizeOfExpr::print(std::ostream& os) const -> void {
    os << "sizeof(";
    if (is_type_) {
        os << *type_to_size_;
    }
    else {
        expr_to_size_->print(os);
    }
    os << ")";
}

auto ImportExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    return expr_->codegen(emitter);
}

auto ImportExpr::print(std::ostream& os) const -> void {
    os << alias_name_ << "::";
    expr_->print(os);
}

auto NewExpr::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto& data_layout = emitter->llvm_module->getDataLayout();

    auto const t = emitter->llvm_type(new_type_);
    auto const size = data_layout.getTypeAllocSize(t);
    auto const malloc_func = emitter->llvm_module->getFunction("malloc");

    if (!call_expr_ and !array_size_args_.has_value()) {
        auto size_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*emitter->context), size);
        return emitter->llvm_builder->CreateCall(malloc_func, size_val);
    }
    else if (array_size_args_.has_value()) {
        auto array_size = (*array_size_args_)->codegen(emitter);
        auto total_size =
            emitter->llvm_builder->CreateMul(array_size,
                                             llvm::ConstantInt::get(llvm::Type::getInt64Ty(*emitter->context), size));
        return emitter->llvm_builder->CreateCall(malloc_func, total_size);
    }
    else {
        // Constructor call
        assert(emitter->alloca != nullptr);
        auto size_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*emitter->context), size);
        auto void_p = emitter->llvm_builder->CreateCall(malloc_func, size_val);
        emitter->llvm_builder->CreateStore(
            emitter->llvm_builder->CreateBitCast(void_p, emitter->llvm_type(new_type_)->getPointerTo()),
            emitter->alloca);

        assert(call_expr_ != nullptr);
        auto loaded_alloca = emitter->llvm_builder->CreateLoad(emitter->llvm_type(new_type_), emitter->alloca);
        emitter->alloca = static_cast<llvm::Value*>(loaded_alloca);
        call_expr_->codegen(emitter);
        return loaded_alloca;
    }

    return nullptr;
}

auto NewExpr::print(std::ostream& os) const -> void {
    os << "new ";
    if (new_type_) {
        os << *new_type_;
    }
    if (constructor_args_) {
        os << "(";
        for (const auto& arg : *constructor_args_) {
            arg->print(os);
            os << ", ";
        }
        os.seekp(-2, std::ios_base::end);
        os << ")";
    }
}