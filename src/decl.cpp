#include "decl.hpp"

#include <iostream>

auto Function::operator==(const Function& other) const -> bool {
    if (this == &other) {
        return true;
    }
    if (ident_ != other.ident_) {
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
    auto name = get_ident();
    if (name != "main") {
        name += get_type_output();
    }
    auto func_type = llvm::FunctionType::get(return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, *emitter->llvm_module);

    // Setting names of function params
    auto idx = 0u;
    for (auto& arg : func->args()) {
        arg.setName(paras_[idx]->get_ident() + paras_[idx]->get_append());
        idx++;
    }

    auto entry_name = std::to_string(emitter->global_counter++);
    auto entry_block = llvm::BasicBlock::Create(*emitter->context, entry_name, func);
    emitter->llvm_builder->SetInsertPoint(entry_block);

    for (auto& arg : func->args()) {
        auto alloca = emitter->llvm_builder->CreateAlloca(arg.getType(), nullptr, arg.getName());
        emitter->llvm_builder->CreateStore(&arg, alloca);
        emitter->named_values[arg.getName().str()] = alloca;
    }

    stmts_->codegen(emitter);

    if (return_type->isVoidTy()) {
        emitter->llvm_builder->CreateRetVoid();
    }
    else if (!entry_block->getTerminator()) {
        emitter->llvm_builder->CreateRet(llvm::Constant::getNullValue(return_type));
    }

    return func;
}

auto Function::print(std::ostream& os) const -> void {
    os << "Function " << pos() << " " << ident_ << " : " << *t_ << "\n";

    for (auto const& para : paras_) {
        os << "\t\t";
        para->print(os);
    }
    stmts_->print(os);
    os << "\n";
}

auto Extern::operator==(Extern const& other) const -> bool {
    if (this == &other) {
        return true;
    }
    if (ident_ != other.ident_) {
        return false;
    }
    if (*t_ != *other.t_) {
        return false;
    }
    if (types_.size() != other.types_.size()) {
        return false;
    }
    for (size_t i = 0; i < types_.size(); ++i) {
        if (*types_[i] != *other.types_[i]) {
            return false;
        }
    }
    return true;
}

auto Extern::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const return_type = emitter->llvm_type(get_type());

    auto param_types = std::vector<llvm::Type*>{};
    auto i = 0u;
    for (auto& type : types_) {
        if (!(has_variatic_ and i == types_.size() - 1)) {
            param_types.push_back(emitter->llvm_type(type));
        }
        ++i;
    }

    auto func_type = llvm::FunctionType::get(return_type, param_types, has_variatic_);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, get_ident(), *emitter->llvm_module);

    return func;
}

auto Extern::print(std::ostream& os) const -> void {
    os << "Extern" << pos() << " " << ident_ << " : " << *t_ << "\n";

    for (auto const& type : types_) {
        os << ", " << *type;
    }
    os << "\n";
}

auto ParaDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto ParaDecl::print(std::ostream& os) const -> void {
    os << "ParaDecl " << pos();
    if (is_mut_) {
        os << "(is_mut) ";
    }
    os << "\t " << ident_ << " : " << *t_;
    return;
}

auto LocalVarDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto llvm_type = emitter->llvm_type(get_type());
    auto alloca = emitter->llvm_builder->CreateAlloca(llvm_type, nullptr, get_ident());

    if (get_type()->is_array()) {
        emitter->set_array_alloca(alloca);
    }

    auto init_val = expr_->codegen(emitter);
    auto is_array_init_expr = std::dynamic_pointer_cast<ArrayInitExpr>(expr_);
    if (!is_array_init_expr and init_val) {
        emitter->llvm_builder->CreateStore(init_val, alloca);
    }

    emitter->named_values[get_ident() + get_append()] = alloca;
    return alloca;
}

auto LocalVarDecl::print(std::ostream& os) const -> void {
    os << "let " << ident_ << " : " << *t_ << " = ";
    expr_->print(os);
    return;
}

auto GlobalVarDecl::handle_global_arr(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto const arr_type = std::dynamic_pointer_cast<ArrayType>(get_type());
    auto const arr_len = *arr_type->get_length();
    auto const llvm_type = static_cast<llvm::ArrayType*>(emitter->llvm_type(arr_type));

    auto const_elems = std::vector<llvm::Constant*>{};
    const_elems.reserve(arr_len);

    if (auto const l = std::dynamic_pointer_cast<ArrayInitExpr>(expr_)) {
        auto i = 0u;
        llvm::Constant* last_seen;
        for (auto& elem : l->get_exprs()) {
            auto const c_val = llvm::dyn_cast<llvm::Constant>(elem->codegen(emitter));
            last_seen = c_val;
            if (!c_val) {
                std::cout << "Global array initialiser is not constant\n";
                return nullptr;
            }
            const_elems.push_back(c_val);
            i++;
        }

        while (i and i < arr_len) {
            const_elems.push_back(last_seen);
            i++;
        }
    }

    auto const const_array = llvm::ConstantArray::get(llvm_type, const_elems);
    auto const global_var = new llvm::GlobalVariable(*emitter->llvm_module,
                                                     llvm_type,
                                                     false,
                                                     llvm::GlobalValue::ExternalLinkage,
                                                     const_array,
                                                     ident_ + get_append());
    emitter->named_values[ident_ + get_append()] = global_var;
    return global_var;
}

auto GlobalVarDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    if (get_type()->is_array()) {
        return handle_global_arr(emitter);
    }
    auto const llvm_type = emitter->llvm_type(get_type());
    auto global_var = new llvm::GlobalVariable(*emitter->llvm_module,
                                               llvm_type,
                                               false,
                                               llvm::GlobalValue::ExternalLinkage,
                                               nullptr,
                                               ident_);

    auto const has_expr = !(std::dynamic_pointer_cast<EmptyExpr>(expr_));
    if (has_expr) {
        auto val = expr_->codegen(emitter);
        global_var->setInitializer(llvm::dyn_cast<llvm::Constant>(val));
    }
    else {
        global_var->setInitializer(llvm::Constant::getNullValue(llvm_type));
    }

    emitter->named_values[ident_ + get_append()] = global_var;
    return global_var;
}

auto GlobalVarDecl::print(std::ostream& os) const -> void {
    os << "let " << ident_ << " : " << *t_ << " = ";
    expr_->print(os);
    os << ";\n";
    return;
}

auto EnumDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto EnumDecl::print(std::ostream& os) const -> void {
    os << "enum " << get_ident() << "{";
    for (auto const& field : fields_) {
        os << field << ", ";
    }
    os << "}\n";
    return;
}

auto EnumDecl::get_num(std::string field) const -> std::optional<int> {
    for (auto i = 0u; i < fields_.size(); ++i) {
        if (fields_[i] == field) {
            return std::optional{static_cast<int>(i)};
        }
    }
    return std::nullopt;
}

auto EnumDecl::find_duplicates() const -> std::vector<std::string> {
    auto count_map = std::unordered_map<std::string, int>{};
    auto duplicates = std::vector<std::string>{};

    for (const auto& str : fields_) {
        count_map[str]++;
    }

    for (const auto& [key, count] : count_map) {
        if (count > 1) {
            duplicates.push_back(key);
        }
    }

    return duplicates;
}