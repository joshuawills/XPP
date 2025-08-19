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

    auto name = get_ident();
    if (name != "main") {
        name += get_type_output();
    }
    auto func = emitter->llvm_module->getFunction(name);

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

auto MethodDecl::operator==(const MethodDecl& other) const -> bool {
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

auto MethodDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    auto return_type = emitter->llvm_type(get_type());

    auto name = "method." + get_ident() + get_type_output();
    auto method = emitter->llvm_module->getFunction(name);
    assert(method != nullptr);

    // Setting names of method params
    auto idx = 0u;
    for (auto& arg : method->args()) {
        if (idx == 0) {
            arg.setName("this");
        }
        else {
            arg.setName(paras_[idx - 1]->get_ident() + paras_[idx - 1]->get_append());
        }
        idx++;
    }

    auto entry_name = std::to_string(emitter->global_counter++);
    auto entry_block = llvm::BasicBlock::Create(*emitter->context, entry_name, method);
    emitter->llvm_builder->SetInsertPoint(entry_block);

    for (auto& arg : method->args()) {
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

    return method;
}

auto MethodDecl::print(std::ostream& os) const -> void {
    os << "Method " << pos() << " " << ident_ << " : " << *t_ << "\n";

    for (auto const& para : paras_) {
        os << "\t\t";
        para->print(os);
    }
    stmts_->print(os);
    os << "\n";
}

auto ConstructorDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    emitter->instantiating_constructor_ = true;
    auto name = "constructor." + ident_ + get_type_output();
    auto constructor = emitter->llvm_module->getFunction(name);

    auto idx = 0u;
    for (auto& arg : constructor->args()) {
        if (idx == 0) {
            arg.setName("this");
        }
        else {
            arg.setName(paras_[idx - 1]->get_ident() + paras_[idx - 1]->get_append());
        }
        ++idx;
    }

    auto entry_name = std::to_string(emitter->global_counter++);
    auto entry_block = llvm::BasicBlock::Create(*emitter->context, entry_name, constructor);
    emitter->llvm_builder->SetInsertPoint(entry_block);

    for (auto& arg : constructor->args()) {
        auto alloca = emitter->llvm_builder->CreateAlloca(arg.getType(), nullptr, arg.getName());
        emitter->llvm_builder->CreateStore(&arg, alloca);
        emitter->named_values[arg.getName().str()] = alloca;
    }

    stmts_->codegen(emitter);

    emitter->llvm_builder->CreateRetVoid();
    emitter->instantiating_constructor_ = false;
    return constructor;
}

auto ConstructorDecl::print(std::ostream& os) const -> void {
    os << "Constructor " << pos() << " " << ident_ << " : " << *t_ << "\n";

    for (auto const& para : paras_) {
        os << "\t\t";
        para->print(os);
    }
    stmts_->print(os);
    os << "\n";
}

auto ConstructorDecl::operator==(ConstructorDecl const& other) const -> bool {
    if (paras_.size() != other.paras_.size()) {
        return false;
    }
    for (auto i = 0u; i < paras_.size(); ++i) {
        if (*paras_[i] != *other.paras_[i]) {
            return false;
        }
    }
    return true;
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

    auto constructor_decl = std::dynamic_pointer_cast<ConstructorCallExpr>(expr_);
    auto alloca = emitter->llvm_builder->CreateAlloca(llvm_type, nullptr, get_ident() + get_append());
    if (constructor_decl) {
        emitter->alloca = alloca;
        expr_->codegen(emitter);
        emitter->alloca = nullptr;
        emitter->named_values[get_ident() + get_append()] = alloca;
        return alloca;
    }

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

auto ClassFieldDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    (void)emitter;
    return nullptr;
}

auto ClassFieldDecl::print(std::ostream& os) const -> void {
    os << "\t ";
    if (is_pub()) {
        os << "pub ";
    }
    if (is_mut()) {
        os << "mut ";
    }
    os << ident_ << " : " << *t_ << "\n";
    return;
}

auto ClassFieldDecl::operator==(ClassFieldDecl const& other) const -> bool {
    return get_ident() == other.get_ident();
}

auto ClassDecl::get_index_for_field(std::string field_name) const -> int {
    auto idx = 0;
    for (auto const& field : fields_) {
        if (field->get_ident() == field_name) {
            return idx;
        }
        ++idx;
    }
    std::cout << "UNREACHABLE ClassDecl::get_index_for_field. field not found: " << field_name << std::endl;
    return -1;
}

auto ClassDecl::method_exists(std::string const& method_name) const -> bool {
    for (auto const& method : methods_) {
        if (method->get_ident() == method_name) {
            return true;
        }
    }
    return false;
}

auto ClassDecl::field_exists(std::string const& field_name) const -> bool {
    for (auto const& field : fields_) {
        if (field->get_ident() == field_name) {
            return true;
        }
    }
    return false;
}

auto ClassDecl::get_field_type(std::string field_name) const -> std::shared_ptr<Type> {
    for (auto const& field : fields_) {
        if (field->get_ident() == field_name) {
            return field->get_type();
        }
    }
    std::cout << "UNREACHABLE ClassDecl::get_field_type. field not found: " << field_name << std::endl;
    return nullptr;
}

auto ClassDecl::field_is_private(std::string const& field_name) const -> bool {
    for (auto const& field : fields_) {
        if (field->get_ident() == field_name) {
            return !field->is_pub();
        }
    }
    return false;
}

auto ClassDecl::get_field(std::string const& name) const -> std::shared_ptr<ClassFieldDecl> {
    for (auto const& field : fields_) {
        if (field->get_ident() == name) {
            return field;
        }
    }
    std::cout << "UNREACHABLE ClassDecl::get_field. field not found: " << name << std::endl;
    return nullptr;
}

auto ClassDecl::get_method(std::shared_ptr<MethodAccessExpr> method) const -> std::optional<std::shared_ptr<MethodDecl>> {
    auto it = std::find_if(methods_.begin(), methods_.end(), [&](auto const& m) {
        if (m->get_ident() != method->get_method_name()) {
            return false;
        }

        auto call_args = method->get_args();
        auto method_params = m->get_paras();

        if (call_args.size() != method_params.size()) {
            return false;
        }

        for (size_t i = 0; i < call_args.size(); ++i) {
            if (!method_params[i]->get_type()->equal_soft(*call_args[i]->get_type())) {
                return false;
            }
        }

        return true;
    });
    if (it != methods_.end()) {
        return *it;
    }
    return std::nullopt;
}

auto ClassDecl::codegen(std::shared_ptr<Emitter> emitter) -> llvm::Value* {
    emitter->curr_class_ = shared_from_this();

    for (auto& constructor : constructors_) {
        constructor->codegen(emitter);
    }
    for (auto& method : methods_) {
        method->codegen(emitter);
    }

    emitter->curr_class_ = nullptr;
    return nullptr;
}

auto ClassDecl::print(std::ostream& os) const -> void {
    os << "class " << get_ident() << "{\n";
    os << "fields:\n";
    for (auto const& field : fields_) {
        field->print(os);
    }
    os << "constructors:\n";
    for (auto const& constructor : constructors_) {
        constructor->print(os);
    }
    os << "methods:\n";
    for (auto const& method : methods_) {
        method->print(os);
    }
    os << "\n}\n";
    return;
}

auto ClassDecl::operator==(ClassDecl const& other) const -> bool {
    return get_ident() == other.get_ident();
}

auto ClassDecl::get_class_type_name() -> std::string {
    if (type_name_.size() != 0) {
        return type_name_;
    }
    auto n = std::stringstream{};
    for (auto& field : fields_) {
        n << *field->get_type();
    }
    type_name_ = n.str();
    return type_name_;
}