#pragma once

#include <variant>
#include <vector>
#include <optional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include "tokenization.hpp"

// Forward declarations
struct NodeExpr;
struct NodeStatement;
struct NodeIfPredicate;

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct BinExprAdd {
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct BinExprMul {
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct BinExprDiv {
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct BinExprSub {
    std::shared_ptr<NodeExpr> left;
    std::shared_ptr<NodeExpr> right;
};

struct BinExpr {
    std::variant<BinExprAdd, BinExprMul, BinExprDiv, BinExprSub> var;
};  

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent, BinExpr> var;
};

struct NodeStatementExit {
    NodeExpr exit;
};

struct NodeStatementLet {
    Token ident;
    NodeExpr value;
};

struct NodeScope {
    std::vector<NodeStatement> statements;
};

struct NodeStatementIf {
    NodeExpr condition;
    NodeScope then_scope;
    std::shared_ptr<NodeIfPredicate> predicate;
};

struct NodeIfPredicateElse {
    NodeScope scope;
};

struct NodeIfPredicateElif {
    NodeExpr condition;
    NodeScope scope;
    std::shared_ptr<NodeIfPredicate> predicate;
};

struct NodeIfPredicate {
    std::variant<NodeIfPredicateElse, NodeIfPredicateElif> var;
};

struct NodeStatementAssign {
    Token ident;
    NodeExpr value;
};

struct NodeStatement {
    std::variant<NodeStatementExit, NodeStatementLet, NodeStatementIf, NodeIfPredicate, NodeScope, NodeStatementAssign> expr;
};

struct NodeProgram {
    std::vector<NodeStatement> statements;
};

// Symbol table for variable resolution
class SymbolTable {
public:
    SymbolTable() {
        // Start with global scope
        m_scopes.push_back(std::unordered_map<std::string, bool>());
    }

    void enter_scope() {
        m_scopes.push_back(std::unordered_map<std::string, bool>());
    }

    void exit_scope() {
        if (m_scopes.size() > 1) {  // Don't pop the global scope
            m_scopes.pop_back();
        }
    }

    bool declare(const std::string& name) {
        if (m_scopes.empty()) {
            std::cerr << "Internal error: symbol table is empty" << std::endl;
            return false;
        }
        
        // Check if variable is already declared in current scope
        if (m_scopes.back().find(name) != m_scopes.back().end()) {
            return false;
        }
        
        m_scopes.back()[name] = true;
        return true;
    }

    bool is_declared(const std::string& name) const {
        if (m_scopes.empty()) {
            std::cerr << "Internal error: symbol table is empty" << std::endl;
            return false;
        }

        // Search from innermost to outermost scope
        for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return true;
            }
        }
        return false;
    }

    // Check if a variable is accessible in the current scope
    bool is_accessible(const std::string& name) const {
        if (m_scopes.empty()) {
            return false;
        }

        // A variable is accessible if it's declared in any scope
        return is_declared(name);
    }

    size_t scope_depth() const {
        return m_scopes.size();
    }

private:
    std::vector<std::unordered_map<std::string, bool>> m_scopes;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_pos(0) {}

    NodeProgram parse() {
        NodeProgram program;
        m_symbols.enter_scope();  // Enter global scope
        
        if (!parse_statements(program.statements, true)) {
            m_symbols.exit_scope();
            return program;
        }
        
        m_symbols.exit_scope();  // Exit global scope
        return program;
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos;
    SymbolTable m_symbols;
    
    std::unordered_map<TokenType, int> m_precedence = {
        {TokenType::plus, 1},
        {TokenType::minus, 1},
        {TokenType::star, 2},
        {TokenType::slash, 2}
    };

    // Helper method for error reporting
    void report_error(const std::string& message, const Token& token) {
        std::cerr << "Error at line " << token.line << ", column " << token.column << ": " << message << std::endl;
    }

    bool parse_statements(std::vector<NodeStatement>& statements, bool is_program_scope = false) {
        while (auto token = peek()) {
            if (token->type == TokenType::close_brace) {
                return true;  // End of scope
            }
            
            bool statement_parsed = false;
            if (token->type == TokenType::_exit) {
                statement_parsed = parse_exit_statement(statements);
                if (statement_parsed) {
                    // Expect semicolon after exit statement
                    if (!consume_expected(TokenType::semi)) {
                        report_error("expected ';' after exit statement", *token);
                        return false;
                    }
                }
            } else if (token->type == TokenType::let) {
                statement_parsed = parse_let_statement(statements);
                if (!statement_parsed) {
                    return false;
                }
            } else if (token->type == TokenType::if_) {
                statement_parsed = parse_if_statement(statements);
                if (!statement_parsed) {
                    return false;
                }
            } else if (token->type == TokenType::open_brace) {
                auto nested_scope = parse_scope();
                if (nested_scope.has_value()) {
                    statements.push_back(NodeStatement{*nested_scope});
                    statement_parsed = true;
                } else {
                    return false;
                }
            } else if (token->type == TokenType::ident) {
                statement_parsed = parse_assign_statement(statements);
                if (!statement_parsed) {
                    return false;
                }
            } else if (token->type == TokenType::semi) {
                // Skip standalone semicolons
                consume();
                statement_parsed = true;
            }

            if (!statement_parsed) {
                report_error("unexpected token '" + token->value + "' in scope", *token);
                return false;
            }
        }
        
        // If we're at the end of input and this is the program scope, that's valid
        if (is_program_scope) {
            return true;
        }
        
        report_error("expected '}' to close scope", peek().value());
        return false;
    }

    std::optional<NodeScope> parse_scope() {
        m_symbols.enter_scope();  // Enter new scope
        NodeScope scope;
        
        if (!parse_statements(scope.statements)) {
            m_symbols.exit_scope();  // Exit scope on error
            return std::nullopt;
        }
        
        consume(); // consume '}'
        m_symbols.exit_scope();  // Exit scope
        return scope;
    }

    bool parse_exit_statement(std::vector<NodeStatement>& statements) {
        auto exit_token = *consume(); // consume 'exit'
        if (!consume_expected(TokenType::open_paren)) {
            report_error("expected '(' after exit keyword", exit_token);
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            report_error("expected expression in exit statement", exit_token);
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            report_error("expected ')' after expression in exit statement", exit_token);
            return false;
        }
        NodeStatementExit exit_stmt{*expr};
        statements.push_back(NodeStatement{exit_stmt});
        return true;
    }

    bool parse_let_statement(std::vector<NodeStatement>& statements) {
        auto let_token = *consume(); // consume 'let'
        auto ident_token = consume_expected(TokenType::ident);
        if (!ident_token.has_value()) {
            report_error("expected identifier after 'let'", let_token);
            return false;
        }
        
        // Check if variable is already declared in current scope
        if (!m_symbols.declare(ident_token->value)) {
            report_error("variable '" + ident_token->value + "' already declared in this scope", *ident_token);
            return false;
        }

        if (!consume_expected(TokenType::eq)) {
            report_error("expected '=' after identifier in let statement", *ident_token);
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            report_error("expected expression after '=' in let statement", *ident_token);
            return false;
        }
        if (!consume_expected(TokenType::semi)) {
            report_error("expected ';' after let statement", *ident_token);
            return false;
        }
        NodeStatementLet let_stmt{*ident_token, *expr};
        statements.push_back(NodeStatement{let_stmt});
        return true;
    }

    bool parse_if_statement(std::vector<NodeStatement>& statements) {
        auto if_token = *consume(); // consume 'if'
        if (!consume_expected(TokenType::open_paren)) {
            report_error("expected '(' after if keyword", if_token);
            return false;
        }
        auto condition = parse_expr();
        if (!condition.has_value()) {
            report_error("expected condition in if statement", if_token);
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            report_error("expected ')' after if condition", if_token);
            return false;
        }
        if (!consume_expected(TokenType::open_brace)) {
            report_error("expected '{' after if condition", if_token);
            return false;
        }
        auto then_scope = parse_scope();
        if (!then_scope.has_value()) {
            report_error("expected scope after if condition", if_token);
            return false;
        }
        auto predicate = parse_predicate();
        NodeStatementIf if_stmt{*condition, *then_scope, predicate};
        statements.push_back(NodeStatement{if_stmt});
        return true;
    }

    std::shared_ptr<NodeIfPredicate> parse_predicate() {
        if (peek()->type == TokenType::elif) {
            auto elif_token = *consume(); // consume 'elif'
            if (!consume_expected(TokenType::open_paren)) {
                report_error("expected '(' after elif keyword", elif_token);
                return nullptr;
            }
            auto condition = parse_expr();
            if (!condition.has_value()) {
                report_error("expected condition in elif predicate", elif_token);
                return nullptr;
            }
            if (!consume_expected(TokenType::close_paren)) {
                report_error("expected ')' after elif predicate", elif_token);
                return nullptr;
            }
            if (!consume_expected(TokenType::open_brace)) {
                report_error("expected '{' after elif predicate", elif_token);
                return nullptr;
            }
            auto elif_scope = parse_scope();
            if (!elif_scope.has_value()) {
                report_error("expected scope after elif predicate", elif_token);
                return nullptr;
            }
            auto next_predicate = parse_predicate();
            return std::make_shared<NodeIfPredicate>(NodeIfPredicateElif{*condition, *elif_scope, next_predicate});
        } else if (peek()->type == TokenType::else_) {
            auto else_token = *consume(); // consume 'else'
            if (!consume_expected(TokenType::open_brace)) {
                report_error("expected '{' after else keyword", else_token);
                return nullptr;
            }
            auto else_scope = parse_scope();
            if (!else_scope.has_value()) {
                report_error("expected scope after else predicate", else_token);
                return nullptr;
            }
            return std::make_shared<NodeIfPredicate>(NodeIfPredicateElse{*else_scope});
        }
        return nullptr; // No elif or else
    }

    bool parse_assign_statement(std::vector<NodeStatement>& statements) {
        auto ident_token = consume_expected(TokenType::ident);
        if (!ident_token.has_value()) {
            report_error("expected identifier in assignment", peek().value());
            return false;
        }

        // Check if variable exists in any scope
        if (!m_symbols.is_declared(ident_token->value)) {
            report_error("variable '" + ident_token->value + "' is not declared", *ident_token);
            return false;
        }

        if (!consume_expected(TokenType::eq)) {
            report_error("expected '=' after identifier in assign statement", *ident_token);
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            report_error("expected expression after '=' in assign statement", *ident_token);
            return false;
        }
        if (!consume_expected(TokenType::semi)) {
            report_error("expected ';' after assign statement", *ident_token);
            return false;
        }
        NodeStatementAssign assign_stmt{*ident_token, *expr};
        statements.push_back(NodeStatement{assign_stmt});
        return true;
    }
    
    std::optional<Token> peek(size_t offset = 0) const {
        if (m_pos + offset < m_tokens.size()) {
            return m_tokens[m_pos + offset];
        }
        return std::nullopt;
    }

    std::optional<Token> consume() {
        if (m_pos < m_tokens.size()) {
            return m_tokens[m_pos++];
        }
        return std::nullopt;
    }

    std::optional<Token> consume_expected(TokenType expected_type) {
        auto token = consume();
        if (token && token->type == expected_type) {
            return token;
        }
        return std::nullopt;
    }

    int get_precedence(TokenType type) {
        auto it = m_precedence.find(type);
        if (it != m_precedence.end()) {
            return it->second;
        }
        return 0; // Default precedence for non-operators
    }

    // Parse primary expressions (numbers, identifiers, parentheses)
    std::optional<NodeExpr> parse_primary() {
        auto token = peek();
        if (!token.has_value()) {
            return std::nullopt;
        }

        if (token->type == TokenType::int_lit) {
            consume();
            NodeExpr expr;
            expr.var = NodeExprIntLit{*token};
            return expr;
        } else if (token->type == TokenType::ident) {
            // Check if variable is accessible in current scope
            if (!m_symbols.is_accessible(token->value)) {
                report_error("variable '" + token->value + "' is not declared", *token);
                return std::nullopt;
            }
            consume();
            NodeExpr expr;
            expr.var = NodeExprIdent{*token};
            return expr;
        } else if (token->type == TokenType::open_paren) {
            consume(); // consume '('
            auto expr = parse_expr();
            if (!expr.has_value()) {
                report_error("expected expression after '('", *token);
                return std::nullopt;
            }
            if (!consume_expected(TokenType::close_paren)) {
                report_error("expected ')' after expression", *token);
                return std::nullopt;
            }
            return expr;
        }

        report_error("unexpected token '" + token->value + "' in expression", *token);
        return std::nullopt;
    }

    // Precedence climbing expression parser
    std::optional<NodeExpr> parse_expr(int min_precedence = 0) {
        auto left = parse_primary();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (true) {
            auto op_token = peek();
            if (!op_token.has_value()) {
                break; // End of input
            }

            int precedence = get_precedence(op_token->type);
            if (precedence == 0 || precedence < min_precedence) {
                break; // Not an operator or precedence too low
            }

            auto op = consume(); // consume the operator
            auto right = parse_expr(precedence + 1);
            if (!right.has_value()) {
                std::cerr << "Parse error: expected expression after operator '" << op->value << "'" << std::endl;
                return std::nullopt;
            }

            // Create binary expression based on operator type
            if (op->type == TokenType::plus) {
                BinExprAdd add_expr{
                    std::make_shared<NodeExpr>(*left),
                    std::make_shared<NodeExpr>(*right)
                };
                NodeExpr new_expr;
                new_expr.var = BinExpr{add_expr};
                left = new_expr;
            } else if (op->type == TokenType::minus) {
                BinExprSub sub_expr{
                    std::make_shared<NodeExpr>(*left),
                    std::make_shared<NodeExpr>(*right)
                };
                NodeExpr new_expr;
                new_expr.var = BinExpr{sub_expr};
                left = new_expr;
            } else if (op->type == TokenType::star) {
                BinExprMul mul_expr{
                    std::make_shared<NodeExpr>(*left),
                    std::make_shared<NodeExpr>(*right)
                };
                NodeExpr new_expr;
                new_expr.var = BinExpr{mul_expr};
                left = new_expr;
            } else if (op->type == TokenType::slash) {
                BinExprDiv div_expr{
                    std::make_shared<NodeExpr>(*left),
                    std::make_shared<NodeExpr>(*right)
                };
                NodeExpr new_expr;
                new_expr.var = BinExpr{div_expr};
                left = new_expr;
            }
        }

        return left;
    }
};