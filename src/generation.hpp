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

        std::string generate_exit(const NodeExpr& exit_expr) {
            std::string code;
            if (std::holds_alternative<NodeExprIntLit>(exit_expr.var)) {

                const auto& int_lit = std::get<NodeExprIntLit>(exit_expr.var).int_lit;
                code += "\tmov\tw0, #" + int_lit.value + "\n";
            } else if (std::holds_alternative<NodeExprIdent>(exit_expr.var)) {

                const auto& ident_expr = std::get<NodeExprIdent>(exit_expr.var);
                const std::string& ident_name = ident_expr.ident.value;
                if (m_vars.count(ident_name)) {
                    // Variable found, calculate offset from current stack pointer
                    size_t var_stack_pos = m_vars.at(ident_name).stack_offset;
                    size_t offset_from_current_sp = m_stack_size - var_stack_pos - 16;
                    code += "\tldr\tw0, [sp, #" + std::to_string(offset_from_current_sp) + "]\n";
                } else {
                    // Variable not found, error or default
                    std::cerr << "Error: Undeclared variable '" << ident_name << "' used in exit statement.\n";
                    code += "\tmov\tw0, #0\n"; // Default to exit code 0 on error
                }
            }
            return code;
        }

        std::string generate_let(const NodeStatementLet& let_stmt) {
            std::string code;
            const std::string& ident = let_stmt.ident.value;
            if (std::holds_alternative<NodeExprIntLit>(let_stmt.value.var)) { // handle when setting variable to an integer literal

                if (m_vars.find(ident) == m_vars.end()) {
                    const auto& int_lit = std::get<NodeExprIntLit>(let_stmt.value.var).int_lit;
                    code += "\tmov\tw1, #" + int_lit.value + "\n";
                    code += "\tstr\tw1, [sp, #-16]!\n";
                    m_vars[ident] = Var{m_stack_size};
                    m_stack_size += 16; // Each variable takes 16 bytes on stack
                } else {
                    std::cerr << "Error: Variable '" << ident << "' already declared.\n";
                }
            } else if (std::holds_alternative<NodeExprIdent>(let_stmt.value.var)) { // handle when setting variable to an variable

                if (m_vars.find(ident) == m_vars.end()) {
                    const auto& ident_expr = std::get<NodeExprIdent>(let_stmt.value.var);
                    const std::string& source_ident = ident_expr.ident.value;
                    if (m_vars.count(source_ident)) {
                        // Source variable found, load from its stack offset and store to new location
                        size_t source_stack_pos = m_vars.at(source_ident).stack_offset;
                        size_t source_offset_from_sp = m_stack_size - source_stack_pos - 16;
                        code += "\tldr\tw1, [sp, #" + std::to_string(source_offset_from_sp) + "]\n";
                        code += "\tstr\tw1, [sp, #-16]!\n";
                        m_vars[ident] = Var{m_stack_size};
                        m_stack_size += 16;
                    } else {
                        std::cerr << "Error: Undeclared variable '" << source_ident << "' used in let statement.\n";
                    }
                } else {
                    std::cerr << "Error: Variable '" << ident << "' already declared.\n";
                }
            }
            // TODO: Handle other types for let_stmt.value.var (e.g., expressions)
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