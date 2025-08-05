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