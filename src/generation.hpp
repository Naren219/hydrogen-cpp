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
                    // Variable found, use its assigned register
                    std::string var_reg = "w" + std::to_string(m_vars.at(ident_name).register_index);
                    code += "\tmov\tw0, " + var_reg + "\n";
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
                    std::string reg = "w" + std::to_string(m_current_register_idx);
                    code += "\tmov\t" + reg + ", #" + int_lit.value + "\n";
                    m_vars[ident] = Var{m_current_register_idx};
                    m_current_register_idx++; // Increment for the next variable
                } else {
                    std::cerr << "Error: Variable '" << ident << "' already declared.\n";
                }
            } else if (std::holds_alternative<NodeExprIdent>(let_stmt.value.var)) { // handle when setting variable to an variable

                if (m_vars.find(ident) == m_vars.end()) {
                    const auto& ident_expr = std::get<NodeExprIdent>(let_stmt.value.var);
                    const std::string& source_ident = ident_expr.ident.value;
                    if (m_vars.count(source_ident)) {
                        // Source variable found, copy its value to a new register
                        std::string source_reg = "w" + std::to_string(m_vars.at(source_ident).register_index);
                        std::string dest_reg = "w" + std::to_string(m_current_register_idx);
                        code += "\tmov\t" + dest_reg + ", " + source_reg + "\n";
                        m_vars[ident] = Var{m_current_register_idx};
                        m_current_register_idx++;
                    } else {
                        std::cerr << "Error: Undeclared variable '" << source_ident << "' used in let statement.\n";
                    }
                } else {
                    std::cerr << "Error: Variable '" << ident << "' already declared.\n";
                }
            }
            // TODO: Handle other types for let_stmt.value.var (e.g., expressions)
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
            // Let's rename stack_location to register_index to be more accurate for current usage
            size_t register_index; 
        };

        NodeProgram m_program;
        size_t m_current_register_idx = 1; // Start allocating from w1 (w0 is for exit status)
        std::unordered_map<std::string, Var> m_vars;
};