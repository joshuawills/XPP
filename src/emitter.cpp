#include "emitter.hpp"
#include "handler.hpp"
#include "module.hpp"
#include "type.hpp"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

#include <iostream>

auto Emitter::emit() -> void {
    // Forward declaring everything necessary
    for (auto& module : modules_->get_modules()) {
        for (auto& global : module->get_global_vars()) {
            if (global->is_used()) {
                if (!global->codegen(shared_from_this())) {
                    std::cerr << "LLVM failed to generate extern\n";
                    exit(EXIT_FAILURE);
                }
            }
        }

        for (auto& extern_ : module->get_externs()) {
            if (extern_->is_used()) {
                if (!extern_->codegen(shared_from_this())) {
                    std::cerr << "LLVM failed to generate extern\n";
                    exit(EXIT_FAILURE);
                }
            }
        }

        for (auto& function : module->get_functions()) {
            if (function->is_used() or function->get_ident() == "main") {
                forward_declare_func(function);
            }
        }

        for (auto& class_ : module->get_classes()) {
            if (class_->is_used()) {
                curr_class_ = class_;
                for (auto& method : class_->get_methods()) {
                    // Inadvertantly the types as well
                    forward_declare_method(method);
                }
                for (auto& constructor : class_->get_constructors()) {
                    forward_declare_constructor(constructor);
                }
            }
        }
    }

    for (auto& module : modules_->get_modules()) {
        for (auto& function : module->get_functions()) {
            if (function->is_used() or function->get_ident() == "main") {
                if (!function->codegen(shared_from_this())) {
                    std::cerr << "LLVM failed to generate function\n";
                    exit(EXIT_FAILURE);
                }
            }
        }
        for (auto& class_ : module->get_classes()) {
            if (class_->is_used()) {
                class_->codegen(shared_from_this());
            }
        }
    }

    if (handler_->llvm_mode()) {
        auto error_code = std::error_code{};
        auto dest = llvm::raw_fd_ostream{handler_->get_llvm_filename(), error_code};
        llvm_module->print(dest, nullptr);
        return;
    }

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    auto target_triple = llvm::sys::getDefaultTargetTriple();
    llvm_module->setTargetTriple(target_triple);

    auto error = std::string{};
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        std::cerr << "Failed to lookup target: " << error << "\n";
        exit(EXIT_FAILURE);
    }

    auto opt = llvm::TargetOptions{};
    auto reloc_model = std::optional<llvm::Reloc::Model>{};
    auto target_machine = target->createTargetMachine(target_triple, "generic", "", opt, reloc_model);

    llvm_module->setDataLayout(target_machine->createDataLayout());

    auto filename = (handler_->is_assembly()) ? handler_->get_assembly_filename() : handler_->get_object_filename();

    auto error_code = std::error_code{};
    auto dest = llvm::raw_fd_ostream{filename, error_code};

    auto pass = llvm::legacy::PassManager{};
    auto file_type = (handler_->is_assembly()) ? llvm::CodeGenFileType::CGFT_AssemblyFile
                                               : llvm::CodeGenFileType::CGFT_ObjectFile;

    target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type);
    pass.run(*llvm_module);
    dest.flush();

    if (!handler_->is_assembly()) {
        // Compile the executable
        auto command = std::string{"clang -no-pie "};
        command += filename;
        command += " -o ";
        command += handler_->get_output_filename();
        system(command.c_str());
        command.clear();
        command += "rm -f ";
        command += handler_->get_object_filename();
        system(command.c_str());
    }
}

auto Emitter::llvm_type(std::shared_ptr<Type> t) -> llvm::Type* {
    if (t->is_pointer()) {
        auto p_t = std::dynamic_pointer_cast<PointerType>(t);
        auto const element_type = llvm_type(p_t->get_sub_type());
        return llvm::PointerType::getUnqual(element_type);
    }
    else if (t->is_array()) {
        auto a_t = std::dynamic_pointer_cast<ArrayType>(t);
        auto const element_type = llvm_type(a_t->get_sub_type());
        auto const num_element = *a_t->get_length();
        return llvm::ArrayType::get(element_type, num_element);
    }
    else if (t->is_class()) {
        auto c_t = std::dynamic_pointer_cast<ClassType>(t);
        auto const class_name = "class." + c_t->get_ref()->get_class_type_name();
        auto lookup = llvm::StructType::getTypeByName(*context, class_name);
        if (lookup) {
            return lookup;
        }
        else {
            std::cout << "UNREACHABLE Emitter::llvm_type 1: " << *t << "\n";
            return nullptr;
        }
    }

    switch (t->get_type_spec()) {
    case TypeSpec::BOOL: return llvm::Type::getInt1Ty(*context);
    case TypeSpec::ENUM:
    case TypeSpec::I64:
    case TypeSpec::U64: return llvm::Type::getInt64Ty(*context);
    case TypeSpec::I32:
    case TypeSpec::U32: return llvm::Type::getInt32Ty(*context);
    case TypeSpec::I8:
    case TypeSpec::U8: return llvm::Type::getInt8Ty(*context);
    case TypeSpec::F32: return llvm::Type::getFloatTy(*context);
    case TypeSpec::F64: return llvm::Type::getDoubleTy(*context);
    case TypeSpec::VOID: return llvm::Type::getVoidTy(*context);
    default: std::cout << "UNREACHABLE Emitter::llvm_type: " << *t << "\n";
    }
    return nullptr;
}

auto Emitter::llvm_type(std::shared_ptr<ClassDecl> t) -> llvm::Type* {
    auto n = "class." + t->get_class_type_name();
    auto lookup = llvm::StructType::getTypeByName(*context, n);
    if (lookup) {
        return lookup;
    }
    else {
        auto s = llvm::StructType::create(*context, n);
        auto types = std::vector<llvm::Type*>{};
        for (auto& p : t->get_fields()) {
            types.push_back(llvm_type(p->get_type()));
        }
        s->setBody(types);
        return s;
    }
}

auto Emitter::forward_declare_func(std::shared_ptr<Function> function) -> void {
    auto return_type = llvm_type(function->get_type());

    // Handling params
    auto param_types = std::vector<llvm::Type*>{};
    for (auto& para : function->get_paras()) {
        param_types.push_back(llvm_type(para->get_type()));
    }

    // Instantiating function
    auto name = function->get_ident();
    if (name != "main") {
        name += function->get_type_output();
    }
    auto func_type = llvm::FunctionType::get(return_type, param_types, false);
    llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, *llvm_module);
}

auto Emitter::forward_declare_constructor(std::shared_ptr<ConstructorDecl> constructor) -> void {
    auto return_type = llvm::Type::getVoidTy(*context);
    auto param_types = std::vector<llvm::Type*>{};

    // Push in the pointer to itself
    param_types.push_back(llvm::PointerType::getUnqual(llvm_type(curr_class_)));

    // Handling params
    for (auto& para : constructor->get_paras()) {
        param_types.push_back(llvm_type(para->get_type()));
    }

    // Instantiating function
    auto name = "constructor." + curr_class_->get_ident() + constructor->get_type_output();
    auto const constructor_type = llvm::FunctionType::get(return_type, param_types, false);
    llvm::Function::Create(constructor_type, llvm::Function::ExternalLinkage, name, *llvm_module);
}

auto Emitter::forward_declare_method(std::shared_ptr<MethodDecl> method) -> void {
    auto return_type = llvm_type(method->get_type());
    auto param_types = std::vector<llvm::Type*>{};

    // Push in the pointer to itself
    param_types.push_back(llvm::PointerType::getUnqual(llvm_type(curr_class_)));

    // Handling params
    for (auto& para : method->get_paras()) {
        param_types.push_back(llvm_type(para->get_type()));
    }

    // Instantiating function
    auto name = "method." + method->get_ident() + method->get_type_output();
    auto const method_type = llvm::FunctionType::get(return_type, param_types, false);
    llvm::Function::Create(method_type, llvm::Function::ExternalLinkage, name, *llvm_module);
}