#pragma once
#include <string>
#include <vector>
#include <variant>
#include <unordered_map> // Corrected typo
#include <map>
#include <iostream> // For std::cerr
#include "parser.hpp" // Assuming parser.hpp defines NodeProgram, NodeStatement, NodeExpr etc.


class Generator {
    public:
        inline explicit Generator(const NodeProgram program )
            : m_program(std::move(program)) {
            // Initialize with global scope
            m_scope_stack.push_back({});
        }

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
                auto var_opt = find_in_any_scope(ident_name);
                if (var_opt) {
                    size_t var_stack_pos = var_opt->stack_offset;
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
                auto var_opt = find_in_any_scope(ident_name);
                if (var_opt) {
                    size_t var_stack_pos = var_opt->stack_offset;
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
            // Add immediate exit after setting exit value
            code += "\tmov\tx16, #1\n"; // syscall for exit
            code += "\tsvc\t#0x80\n";   // make the syscall
            return code;
        }

        std::string generate_let(const NodeStatementLet& let_stmt) {
            std::string code;
            const std::string& ident = let_stmt.ident.value;
            
            if (find_in_current_scope(ident)) {
                std::cerr << "Error: Variable '" << ident << "' already declared in current scope.\n";
                return code;
            }

            if (std::holds_alternative<NodeExprIntLit>(let_stmt.value.var)) {
                const auto& int_lit = std::get<NodeExprIntLit>(let_stmt.value.var).int_lit;
                code += "\tmov\tw1, #" + int_lit.value + "\n";
                code += "\tstr\tw1, [sp, #-16]!\n";
                m_scope_stack.back()[ident] = Var{m_stack_size};
                m_stack_size += 16;
            } else if (std::holds_alternative<NodeExprIdent>(let_stmt.value.var)) {
                const auto& ident_expr = std::get<NodeExprIdent>(let_stmt.value.var);
                const std::string& source_ident = ident_expr.ident.value;
                auto var_opt = find_in_any_scope(source_ident);
                if (var_opt) {
                    size_t source_stack_pos = var_opt->stack_offset;
                    size_t source_offset_from_sp = m_stack_size - source_stack_pos - 16;
                    code += "\tldr\tw1, [sp, #" + std::to_string(source_offset_from_sp) + "]\n";
                    code += "\tstr\tw1, [sp, #-16]!\n";
                    m_scope_stack.back()[ident] = Var{m_stack_size};
                    m_stack_size += 16;
                } else {
                    std::cerr << "Error: Undeclared variable '" << source_ident << "' used in let statement.\n";
                }
            } else if (std::holds_alternative<BinExpr>(let_stmt.value.var)) {
                // For let with complex expression, evaluate it and the result stays on stack
                size_t old_stack_size = m_stack_size;
                code += generate_expr(let_stmt.value);
                // The result is already on the stack, just record the variable
                m_scope_stack.back()[ident] = Var{old_stack_size};
                // Stack size is already updated by generate_expr
            }
            return code;
        }

        std::string generate_if(const NodeStatementIf& if_stmt) {
            std::string code;
            size_t current_label = m_label_counter++;
            
            // Save current stack size
            size_t old_stack_size = m_stack_size;
            
            // Generate code for condition
            code += generate_expr(if_stmt.condition);
            
            code += "\tldr\tw0, [sp], #16\n";
            m_stack_size = old_stack_size; // Restore stack size after popping condition
            
            // Compare with 0 and branch if equal (false)
            code += "\tcmp\tw0, #0\n";
            const std::string skip_label = ".L" + std::to_string(current_label) + "_skip";
            code += "\tb.eq\t" + skip_label + "\n";
            
            // Generate code for then scope
            code += generate_scope(if_stmt.then_scope);
            if (if_stmt.predicate) {
                const std::string end_label = ".L" + std::to_string(current_label) + "_end";
                code += "\tb\t" + end_label + "\n";
                code += skip_label + ":\n";

                code += generate_predicate(*if_stmt.predicate, end_label);
                code += end_label + ":\n";
            } else {
                code += skip_label + ":\n";
            }
            return code;
        }

        std::string generate_predicate(const NodeIfPredicate& predicate, const std::string& end_label) {
            std::string code;
            if (std::holds_alternative<NodeIfPredicateElse>(predicate.var)) {
                const auto& else_predicate = std::get<NodeIfPredicateElse>(predicate.var);
                code += generate_scope(else_predicate.scope);
            } else if (std::holds_alternative<NodeIfPredicateElif>(predicate.var)) {
                const auto& elif_predicate = std::get<NodeIfPredicateElif>(predicate.var);
                size_t current_label = m_label_counter++;
                
                // Save current stack size
                size_t old_stack_size = m_stack_size;
                
                // Generate code for condition
                code += generate_expr(elif_predicate.condition);
                
                code += "\tldr\tw0, [sp], #16\n";
                m_stack_size = old_stack_size; // Restore stack size after popping condition
                
                // Compare with 0 and branch if equal (false)
                const std::string skip_label = ".L" + std::to_string(current_label) + "_skip";
                code += "\tcmp\tw0, #0\n";
                code += "\tb.eq\t" + skip_label + "\n";
                
                // Generate code for then scope
                code += generate_scope(elif_predicate.scope);
                
                code += "\tb\t" + end_label + "\n";
                code += skip_label + ":\n";
                
                // Generate code for next predicate (if any)
                if (elif_predicate.predicate) {
                    code += generate_predicate(*elif_predicate.predicate, end_label);
                }
            }
            return code;
        }

        std::string generate_scope(const NodeScope& scope) {
            std::string code;
            // Push new scope
            m_scope_stack.push_back({});
            
            for (const auto& statement : scope.statements) {
                if (std::holds_alternative<NodeStatementExit>(statement.expr)) {
                    const auto& exit_stmt = std::get<NodeStatementExit>(statement.expr);
                    code += generate_exit(exit_stmt.exit);
                } else if (std::holds_alternative<NodeStatementLet>(statement.expr)) {
                    const auto& let_stmt = std::get<NodeStatementLet>(statement.expr);
                    code += generate_let(let_stmt);
                } else if (std::holds_alternative<NodeStatementIf>(statement.expr)) {
                    const auto& if_stmt = std::get<NodeStatementIf>(statement.expr);
                    code += generate_if(if_stmt);
                } else if (std::holds_alternative<NodeScope>(statement.expr)) {
                    const auto& nested_scope = std::get<NodeScope>(statement.expr);
                    code += generate_scope(nested_scope);
                }
            }
            
            // Pop scope
            m_scope_stack.pop_back();
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
                } else if (std::holds_alternative<NodeStatementIf>(statement.expr)) {
                    const auto& if_stmt = std::get<NodeStatementIf>(statement.expr);
                    code += generate_if(if_stmt);
                } else if (std::holds_alternative<NodeScope>(statement.expr)) {
                    const auto& scope = std::get<NodeScope>(statement.expr);
                    code += generate_scope(scope);
                }
            }

            // Add default exit with code 0 if no exit statement was encountered
            code += "\tmov\tw0, #0\n";
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
        std::vector<std::map<std::string, Var>> m_scope_stack; // Stack of variable maps
        size_t m_label_counter = 0;

        // Helper function to find variable in current scope
        bool find_in_current_scope(const std::string& ident) {
            return m_scope_stack.back().find(ident) != m_scope_stack.back().end();
        }

        // Helper function to find variable in any scope
        std::optional<Var> find_in_any_scope(const std::string& ident) {
            for (auto it = m_scope_stack.rbegin(); it != m_scope_stack.rend(); ++it) {
                if (it->find(ident) != it->end()) {
                    return it->at(ident);
                }
            }
            return std::nullopt;
        }
};