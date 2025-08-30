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
        if (emitter->llvm_builder->GetInsertBlock()->getTerminator()) {
            emitter->global_counter++;
            break;
        }
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

    emitter->continue_blocks.push(top_block);
    emitter->break_blocks.push(end_block);

    compound_stmt_->codegen(emitter);

    emitter->continue_blocks.pop();
    emitter->break_blocks.pop();

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
    if ((!temp or (temp and !temp->has_return())) and !emitter->llvm_builder->GetInsertBlock()->getTerminator()) {
        emitter->llvm_builder->CreateBr(bottom_block);
    }

    emitter->llvm_builder->SetInsertPoint(else_block);
    stmt_two_->codegen(emitter);
    stmt_three_->codegen(emitter);
    if (std::dynamic_pointer_cast<EmptyStmt>(stmt_three_)) {
        emitter->llvm_builder->CreateBr(bottom_block);
    }

    temp = std::dynamic_pointer_cast<CompoundStmt>(stmt_one_);
    if ((!temp or (temp and !temp->has_return())) and !emitter->llvm_builder->GetInsertBlock()->getTerminator()) {
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
    if ((!temp or (temp and !temp->has_return())) and !emitter->llvm_builder->GetInsertBlock()->getTerminator()) {
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

auto LoopStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto l_v = var_decl_;
    auto llvm_type = emitter->llvm_type(l_v->get_type());
    auto alloca = emitter->llvm_builder->CreateAlloca(llvm_type, nullptr, l_v->get_ident() + l_v->get_append());
    emitter->named_values[l_v->get_ident() + l_v->get_append()] = alloca;

    llvm::Value* val;
    if (lower_bound_.has_value()) {
        val = (*lower_bound_)->codegen(emitter);
        if (!val)
            return nullptr;
        emitter->llvm_builder->CreateStore(val, alloca);
    }
    else {
        val = emitter->llvm_builder->getInt64(0);
        emitter->llvm_builder->CreateStore(val, alloca);
    }
    auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();

    auto top_label_value = std::to_string(emitter->global_counter++);
    auto top_label = llvm::BasicBlock::Create(*(emitter->context), top_label_value, function);

    auto middle_label_value = std::to_string(emitter->global_counter++);
    auto middle_label = llvm::BasicBlock::Create(*(emitter->context), middle_label_value, function);

    auto iterate_label_value = std::to_string(emitter->global_counter++);
    auto iterate_label = llvm::BasicBlock::Create(*(emitter->context), iterate_label_value, function);

    auto bottom_label_value = std::to_string(emitter->global_counter++);
    auto bottom_label = llvm::BasicBlock::Create(*(emitter->context), bottom_label_value, function);

    emitter->llvm_builder->CreateBr(top_label);
    emitter->llvm_builder->SetInsertPoint(top_label);

    if (upper_bound_.has_value()) {
        auto lower = emitter->llvm_builder->CreateLoad(llvm_type, alloca);
        auto upper = (*upper_bound_)->codegen(emitter);
        auto cond = emitter->llvm_builder->CreateICmpSLT(lower, upper);
        emitter->llvm_builder->CreateCondBr(cond, middle_label, bottom_label);
    }
    else {
        emitter->llvm_builder->CreateBr(middle_label);
    }

    emitter->llvm_builder->SetInsertPoint(middle_label);

    emitter->continue_blocks.push(iterate_label);
    emitter->break_blocks.push(bottom_label);

    body_stmt_->codegen(emitter);

    emitter->continue_blocks.pop();
    emitter->break_blocks.pop();

    emitter->llvm_builder->CreateBr(iterate_label);

    emitter->llvm_builder->SetInsertPoint(iterate_label);

    auto lower = emitter->llvm_builder->CreateLoad(llvm_type, alloca);
    auto new_lower = emitter->llvm_builder->CreateAdd(lower, emitter->llvm_builder->getInt64(1));
    emitter->llvm_builder->CreateStore(new_lower, alloca);
    emitter->llvm_builder->CreateBr(top_label);

    emitter->llvm_builder->SetInsertPoint(bottom_label);

    return nullptr;
}

auto LoopStmt::print(std::ostream& os) const -> void {
    os << "loop " << var_name_;
    if (lower_bound_.has_value()) {
        os << " from ";
        (*lower_bound_)->print(os);
    }
    if (upper_bound_.has_value()) {
        os << " to ";
        (*upper_bound_)->print(os);
    }
    os << "{\n";
    body_stmt_->print(os);
    os << "}\n";
}

auto BreakStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    assert(emitter->break_blocks.size() > 0 && "BreakStmt codegen called without a break block");
    emitter->llvm_builder->CreateBr(emitter->break_blocks.top());
    return nullptr;
}

auto BreakStmt::print(std::ostream& os) const -> void {
    os << "break;\n";
}

auto ContinueStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    assert(emitter->continue_blocks.size() > 0 && "ContinueStmt codegen called without a continue block");
    emitter->llvm_builder->CreateBr(emitter->continue_blocks.top());
    return nullptr;
}

auto ContinueStmt::print(std::ostream& os) const -> void {
    os << "continue;\n";
}

auto DeleteStmt::get_pointer(std::shared_ptr<Emitter> emitter, std::shared_ptr<VarExpr> e) -> llvm::Value* {
    auto is_field_access = std::dynamic_pointer_cast<ClassFieldDecl>(e->get_ref());
    if (is_field_access) {
        assert(emitter->curr_class_ != nullptr);
        auto const field_index = emitter->curr_class_->get_index_for_field(is_field_access->get_ident());
        auto const this_ptr = emitter->named_values["this"];
        auto const class_type = emitter->llvm_type(emitter->curr_class_);

        auto const this_pointer = emitter->llvm_builder->CreateLoad(llvm::PointerType::getUnqual(class_type), this_ptr);
        auto const val = emitter->llvm_builder->CreateStructGEP(class_type, this_pointer, field_index);
        return emitter->llvm_builder->CreateLoad(emitter->llvm_type(e->get_type()), val);
    }

    return emitter->named_values[e->get_name() + e->get_ref()->get_append()];
}

auto DeleteStmt::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto val = expr_->codegen(emitter);
    auto t = expr_->get_type();
    if (t->is_pointer()) {
        auto free_func = emitter->llvm_module->getFunction("free");

        auto const& function = emitter->llvm_builder->GetInsertBlock()->getParent();
        auto const if_block_value = std::to_string(emitter->global_counter++);
        auto const if_block = llvm::BasicBlock::Create(*(emitter->context), if_block_value, function);

        auto const else_block_value = std::to_string(emitter->global_counter++);
        auto const else_block = llvm::BasicBlock::Create(*(emitter->context), else_block_value, function);

        auto cond = emitter->llvm_builder->CreateICmpNE(
            val,
            llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*(emitter->context)))));
        emitter->llvm_builder->CreateCondBr(cond, if_block, else_block);

        emitter->llvm_builder->SetInsertPoint(if_block);

        auto p_t = std::dynamic_pointer_cast<PointerType>(t);
        if (p_t->get_sub_type()->is_class()) {
            auto a_t = std::dynamic_pointer_cast<ClassType>(p_t->get_sub_type());
            auto destructor = emitter->llvm_module->getFunction("destructor." + a_t->get_ref()->get_ident());
            emitter->llvm_builder->CreateCall(destructor, {val});
        }

        emitter->llvm_builder->CreateCall(free_func, {val});
        if (auto e = std::dynamic_pointer_cast<VarExpr>(expr_)) {
            auto mem = get_pointer(emitter, e);
            emitter->llvm_builder->CreateStore(
                llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*(emitter->context)))),
                mem);
        }

        emitter->llvm_builder->CreateBr(else_block);
        emitter->llvm_builder->SetInsertPoint(else_block);
    }
    else {
        auto class_t = std::dynamic_pointer_cast<ClassType>(t);
        auto destructor = emitter->llvm_module->getFunction("destructor." + class_t->get_ref()->get_ident());
        emitter->llvm_builder->CreateCall(destructor, {val});
    }

    return nullptr;
}

auto DeleteStmt::print(std::ostream& os) const -> void {
    os << "delete ";
    expr_->print(os);
    os << ";\n";
}