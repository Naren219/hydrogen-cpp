#pragma once
#include <string>
#include <vector>
#include <variant>
#include <unordered_map> // Corrected typo
#include <iostream> // For std::cerr
#include "parser.hpp" // Assuming parser.hpp defines NodeProgram, NodeStatement, NodeExpr etc.


class Generator {
    public:
    
        inline explicit Generator(const NodeProgram program )
            : m_program(std::move(program)) {}

        std::string generate_expr(const NodeExpr& expr) {
            std::string code;
            if (std::holds_alternative<NodeExprIntLit>(expr.var)) {
                const auto& int_lit = std::get<NodeExprIntLit>(expr.var).int_lit;
                code += "\tmov\tw0, #" + int_lit.value + "\n";
                code += "\tstr\tw0, [sp, #-16]!\n"; // Push to stack
                m_stack_size += 16;
            } else if (std::holds_alternative<NodeExprIdent>(expr.var)) {
                const auto& ident_expr = std::get<NodeExprIdent>(expr.var);
                const std::string& ident_name = ident_expr.ident.value;
                if (m_vars.count(ident_name)) {
                    size_t var_stack_pos = m_vars.at(ident_name).stack_offset;
                    size_t offset_from_current_sp = m_stack_size - var_stack_pos - 16;
                    code += "\tldr\tw0, [sp, #" + std::to_string(offset_from_current_sp) + "]\n";
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push to stack
                    m_stack_size += 16;
                } else {
                    std::cerr << "Error: Undeclared variable '" << ident_name << "' used in expression.\n";
                    code += "\tmov\tw0, #0\n";
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push default value
                    m_stack_size += 16;
                }
            } else if (std::holds_alternative<BinExpr>(expr.var)) {
                const auto& bin_expr = std::get<BinExpr>(expr.var);
                if (std::holds_alternative<BinExprAdd>(bin_expr.var)) {
                    const auto& add_expr = std::get<BinExprAdd>(bin_expr.var);
                    // Generate code for left operand (pushes to stack)
                    code += generate_expr(*add_expr.left);
                    // Generate code for right operand (pushes to stack)  
                    code += generate_expr(*add_expr.right);
                    // Pop both operands and add them
                    code += "\tldr\tw1, [sp], #16\n";  // Pop right operand to w1
                    code += "\tldr\tw0, [sp], #16\n";  // Pop left operand to w0
                    code += "\tadd\tw0, w0, w1\n";     // Add them
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push result back
                    m_stack_size -= 16; // Net effect: two pops, one push = -16
                } else if (std::holds_alternative<BinExprMul>(bin_expr.var)) {
                    const auto& mul_expr = std::get<BinExprMul>(bin_expr.var);
                    // Generate code for left operand (pushes to stack)
                    code += generate_expr(*mul_expr.left);
                    // Generate code for right operand (pushes to stack)
                    code += generate_expr(*mul_expr.right);
                    // Pop both operands and multiply them
                    code += "\tldr\tw1, [sp], #16\n";  // Pop right operand to w1
                    code += "\tldr\tw0, [sp], #16\n";  // Pop left operand to w0
                    code += "\tmul\tw0, w0, w1\n";     // Multiply them
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push result back
                    m_stack_size -= 16; // Net effect: two pops, one push = -16
                } else if (std::holds_alternative<BinExprDiv>(bin_expr.var)) {
                    const auto& div_expr = std::get<BinExprDiv>(bin_expr.var);
                    // Generate code for left operand (pushes to stack)
                    code += generate_expr(*div_expr.left);
                    // Generate code for right operand (pushes to stack)
                    code += generate_expr(*div_expr.right);
                    // Pop both operands and divide them
                    code += "\tldr\tw1, [sp], #16\n";  // Pop right operand to w1
                    code += "\tldr\tw0, [sp], #16\n";  // Pop left operand to w0
                    code += "\tudiv\tw0, w0, w1\n";    // Divide them
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push result back
                    m_stack_size -= 16; // Net effect: two pops, one push = -16
                } else if (std::holds_alternative<BinExprSub>(bin_expr.var)) {
                    const auto& sub_expr = std::get<BinExprSub>(bin_expr.var);
                    // Generate code for left operand (pushes to stack)
                    code += generate_expr(*sub_expr.left);
                    // Generate code for right operand (pushes to stack)
                    code += generate_expr(*sub_expr.right);
                    // Pop both operands and subtract them
                    code += "\tldr\tw1, [sp], #16\n";  // Pop right operand to w1
                    code += "\tldr\tw0, [sp], #16\n";  // Pop left operand to w0
                    code += "\tsub\tw0, w0, w1\n";     // Subtract them (left - right)
                    code += "\tstr\tw0, [sp, #-16]!\n"; // Push result back
                    m_stack_size -= 16; // Net effect: two pops, one push = -16
                }
            }
            return code;
        }

        std::string generate_exit(const NodeExpr& exit_expr) {
            std::string code;
            if (std::holds_alternative<NodeExprIntLit>(exit_expr.var)) {
                const auto& int_lit = std::get<NodeExprIntLit>(exit_expr.var).int_lit;
                code += "\tmov\tw0, #" + int_lit.value + "\n";
            } else if (std::holds_alternative<NodeExprIdent>(exit_expr.var)) {
                const auto& ident_expr = std::get<NodeExprIdent>(exit_expr.var);
                const std::string& ident_name = ident_expr.ident.value;
                if (m_vars.count(ident_name)) {
                    size_t var_stack_pos = m_vars.at(ident_name).stack_offset;
                    size_t offset_from_current_sp = m_stack_size - var_stack_pos - 16;
                    code += "\tldr\tw0, [sp, #" + std::to_string(offset_from_current_sp) + "]\n";
                } else {
                    std::cerr << "Error: Undeclared variable '" << ident_name << "' used in exit statement.\n";
                    code += "\tmov\tw0, #0\n";
                }
            } else if (std::holds_alternative<BinExpr>(exit_expr.var)) {
                // For exit with complex expression, evaluate it and pop result
                size_t old_stack_size = m_stack_size;
                code += generate_expr(exit_expr);
                code += "\tldr\tw0, [sp], #16\n"; // Pop result for exit
                m_stack_size = old_stack_size; // Restore stack size
            }
            return code;
        }

        std::string generate_let(const NodeStatementLet& let_stmt) {
            std::string code;
            const std::string& ident = let_stmt.ident.value;
            
            if (m_vars.find(ident) != m_vars.end()) {
                std::cerr << "Error: Variable '" << ident << "' already declared.\n";
                return code;
            }

            if (std::holds_alternative<NodeExprIntLit>(let_stmt.value.var)) {
                const auto& int_lit = std::get<NodeExprIntLit>(let_stmt.value.var).int_lit;
                code += "\tmov\tw1, #" + int_lit.value + "\n";
                code += "\tstr\tw1, [sp, #-16]!\n";
                m_vars[ident] = Var{m_stack_size};
                m_stack_size += 16;
            } else if (std::holds_alternative<NodeExprIdent>(let_stmt.value.var)) {
                const auto& ident_expr = std::get<NodeExprIdent>(let_stmt.value.var);
                const std::string& source_ident = ident_expr.ident.value;
                if (m_vars.count(source_ident)) {
                    size_t source_stack_pos = m_vars.at(source_ident).stack_offset;
                    size_t source_offset_from_sp = m_stack_size - source_stack_pos - 16;
                    code += "\tldr\tw1, [sp, #" + std::to_string(source_offset_from_sp) + "]\n";
                    code += "\tstr\tw1, [sp, #-16]!\n";
                    m_vars[ident] = Var{m_stack_size};
                    m_stack_size += 16;
                } else {
                    std::cerr << "Error: Undeclared variable '" << source_ident << "' used in let statement.\n";
                }
            } else if (std::holds_alternative<BinExpr>(let_stmt.value.var)) {
                // For let with complex expression, evaluate it and the result stays on stack
                size_t old_stack_size = m_stack_size;
                code += generate_expr(let_stmt.value);
                // The result is already on the stack, just record the variable
                m_vars[ident] = Var{old_stack_size};
                // Stack size is already updated by generate_expr
            }
            return code;
        }

        std::string generate_program() {
            std::string code;
            code += ".globl\t_main\n.p2align 2\n_main:\n";

            for (const auto& statement : m_program.statements) {
                if (std::holds_alternative<NodeStatementExit>(statement.expr)) {

                    const auto& exit_stmt = std::get<NodeStatementExit>(statement.expr);
                    code += generate_exit(exit_stmt.exit);
                } else if (std::holds_alternative<NodeStatementLet>(statement.expr)) {

                    const auto& let_stmt = std::get<NodeStatementLet>(statement.expr);
                    code += generate_let(let_stmt);
                }
            }

            code += "\tmov\tx16, #1\n"; // syscall for exit
            code += "\tsvc\t#0x80\n";   // make the syscall
            return code;
        }

        std::string generate_statement() {
            std::string code;
            code += generate_program();
            return code;
        }

    private:

        struct Var {
            size_t stack_offset; // Offset from current stack pointer
        };

        NodeProgram m_program;
        size_t m_stack_size = 0; // Track total stack space used
        std::unordered_map<std::string, Var> m_vars;
};