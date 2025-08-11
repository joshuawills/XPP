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
    for (auto& global : main_module_->get_global_vars()) {
        if (!global->codegen(shared_from_this())) {
            std::cerr << "LLVM failed to generate extern\n";
            exit(EXIT_FAILURE);
        }
    }

    for (auto& extern_ : main_module_->get_externs()) {
        if (!extern_->codegen(shared_from_this())) {
            std::cerr << "LLVM failed to generate extern\n";
            exit(EXIT_FAILURE);
        }
    }

    for (auto& function : main_module_->get_functions()) {
        if (!function->codegen(shared_from_this())) {
            std::cerr << "LLVM failed to generate function\n";
            exit(EXIT_FAILURE);
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

    switch (t->get_type_spec()) {
    case TypeSpec::BOOL: return llvm::Type::getInt1Ty(*context);
    case TypeSpec::I64:
    case TypeSpec::U64: return llvm::Type::getInt64Ty(*context);
    case TypeSpec::I32:
    case TypeSpec::U32: return llvm::Type::getInt32Ty(*context);
    case TypeSpec::I8:
    case TypeSpec::U8: return llvm::Type::getInt8Ty(*context);
    case TypeSpec::F32: return llvm::Type::getFloatTy(*context);
    case TypeSpec::F64: return llvm::Type::getDoubleTy(*context);
    case TypeSpec::VOID: return llvm::Type::getVoidTy(*context);
    default: std::cout << "UNREACHABLE Emitter::llvm_type: " << t << "\n";
    }
    return nullptr;
}