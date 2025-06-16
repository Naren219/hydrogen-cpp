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
};

struct NodeStatement {
    std::variant<NodeStatementExit, NodeStatementLet, NodeStatementIf, NodeScope> expr;
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
        while (auto token = peek()) {
            if (token->type == TokenType::_exit) {
                if (!parse_exit_statement(program)) {
                    continue;
                }
                // Expect semicolon after exit statement
                if (!consume_expected(TokenType::semi)) {
                    std::cerr << "Parse error: expected ';' after exit statement" << std::endl;
                    continue;
                }
            } else if (token->type == TokenType::let) {
                if (!parse_let_statement(program)) {
                    continue;
                }
            } else if (token->type == TokenType::if_) {
                if (!parse_if_statement(program)) {
                    continue;
                }
            } else if (token->type == TokenType::open_brace) {
                auto scope = parse_scope();
                if (scope.has_value()) {
                    program.statements.push_back(NodeStatement{*scope});
                }
            } else if (token->type == TokenType::semi) {
                // Skip standalone semicolons
                consume();
            } else {
                std::cerr << "Parse error: unexpected token '" << token->value << "'" << std::endl;
                consume(); // Skip unknown tokens
            }
        }
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

    bool parse_exit_statement(NodeProgram& program) {
        consume(); // consume 'exit'
        if (!consume_expected(TokenType::open_paren)) {
            std::cerr << "Parse error: expected '(' after exit keyword" << std::endl;
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            std::cerr << "Parse error: expected expression in exit statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            std::cerr << "Parse error: expected ')' after expression in exit statement" << std::endl;
            return false;
        }
        NodeStatementExit exit_stmt{*expr};
        program.statements.push_back(NodeStatement{exit_stmt});
        return true;
    }

    bool parse_let_statement(NodeProgram& program) {
        consume(); // consume 'let'
        auto ident_token = consume_expected(TokenType::ident);
        if (!ident_token.has_value()) {
            std::cerr << "Parse error: expected identifier after 'let'" << std::endl;
            return false;
        }
        
        // Check if variable is already declared in current scope
        if (!m_symbols.declare(ident_token->value)) {
            std::cerr << "Parse error: variable '" << ident_token->value << "' already declared in this scope" << std::endl;
            return false;
        }

        if (!consume_expected(TokenType::eq)) {
            std::cerr << "Parse error: expected '=' after identifier in let statement" << std::endl;
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            std::cerr << "Parse error: expected expression after '=' in let statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::semi)) {
            std::cerr << "Parse error: expected ';' after let statement" << std::endl;
            return false;
        }
        NodeStatementLet let_stmt{*ident_token, *expr};
        program.statements.push_back(NodeStatement{let_stmt});
        return true;
    }

    bool parse_if_statement(NodeProgram& program) {
        consume(); // consume 'if'
        if (!consume_expected(TokenType::open_paren)) {
            std::cerr << "Parse error: expected '(' after if keyword" << std::endl;
            return false;
        }
        auto condition = parse_expr();
        if (!condition.has_value()) {
            std::cerr << "Parse error: expected condition in if statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            std::cerr << "Parse error: expected ')' after if condition" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::open_brace)) {
            std::cerr << "Parse error: expected '{' after if condition" << std::endl;
            return false;
        }
        auto then_scope = parse_scope();
        if (!then_scope.has_value()) {
            std::cerr << "Parse error: expected scope after if condition" << std::endl;
            return false;
        }
        NodeStatementIf if_stmt{*condition, *then_scope};
        program.statements.push_back(NodeStatement{if_stmt});
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
                std::cerr << "Parse error: variable '" << token->value << "' is not declared" << std::endl;
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
                std::cerr << "Parse error: expected expression after '('" << std::endl;
                return std::nullopt;
            }
            if (!consume_expected(TokenType::close_paren)) {
                std::cerr << "Parse error: expected ')' after expression" << std::endl;
                return std::nullopt;
            }
            return expr;
        }

        std::cerr << "Parse error: unexpected token '" << token->value << "' in expression" << std::endl;
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

    std::optional<NodeScope> parse_scope() {
        m_symbols.enter_scope();  // Enter new scope
        NodeScope scope;
        
        while (auto token = peek()) {
            if (token->type == TokenType::close_brace) {
                consume(); // consume '}'
                m_symbols.exit_scope();  // Exit scope
                return scope;
            }
            
            bool statement_parsed = false;
            if (token->type == TokenType::_exit) {
                statement_parsed = parse_exit_statement(scope);
                if (statement_parsed) {
                    // Expect semicolon after exit statement
                    if (!consume_expected(TokenType::semi)) {
                        std::cerr << "Parse error: expected ';' after exit statement" << std::endl;
                        m_symbols.exit_scope();
                        return std::nullopt;
                    }
                }
            } else if (token->type == TokenType::let) {
                statement_parsed = parse_let_statement(scope);
                if (!statement_parsed) {
                    m_symbols.exit_scope();
                    return std::nullopt;
                }
                // Expect semicolon after let statement
                if (!consume_expected(TokenType::semi)) {
                    std::cerr << "Parse error: expected ';' after let statement" << std::endl;
                    m_symbols.exit_scope();
                    return std::nullopt;
                }
            } else if (token->type == TokenType::if_) {
                statement_parsed = parse_if_statement(scope);
                if (!statement_parsed) {
                    m_symbols.exit_scope();
                    return std::nullopt;
                }
            } else if (token->type == TokenType::open_brace) {
                auto nested_scope = parse_scope();
                if (nested_scope.has_value()) {
                    scope.statements.push_back(NodeStatement{*nested_scope});
                    statement_parsed = true;
                } else {
                    m_symbols.exit_scope();
                    return std::nullopt;
                }
            } else if (token->type == TokenType::semi) {
                // Skip standalone semicolons
                consume();
                statement_parsed = true;
            }

            if (!statement_parsed) {
                std::cerr << "Parse error: unexpected token '" << token->value << "' in scope" << std::endl;
                m_symbols.exit_scope();
                return std::nullopt;
            }
        }
        
        std::cerr << "Parse error: expected '}' to close scope" << std::endl;
        m_symbols.exit_scope();  // Exit scope even on error
        return std::nullopt;
    }

    bool parse_exit_statement(NodeScope& scope) {
        consume(); // consume 'exit'
        if (!consume_expected(TokenType::open_paren)) {
            std::cerr << "Parse error: expected '(' after exit keyword" << std::endl;
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            std::cerr << "Parse error: expected expression in exit statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            std::cerr << "Parse error: expected ')' after expression in exit statement" << std::endl;
            return false;
        }
        NodeStatementExit exit_stmt{*expr};
        scope.statements.push_back(NodeStatement{exit_stmt});
        return true;
    }

    bool parse_let_statement(NodeScope& scope) {
        consume(); // consume 'let'
        auto ident_token = consume_expected(TokenType::ident);
        if (!ident_token.has_value()) {
            std::cerr << "Parse error: expected identifier after 'let'" << std::endl;
            return false;
        }
        
        // Check if variable is already declared in current scope
        if (!m_symbols.declare(ident_token->value)) {
            std::cerr << "Parse error: variable '" << ident_token->value << "' already declared in this scope" << std::endl;
            return false;
        }

        if (!consume_expected(TokenType::eq)) {
            std::cerr << "Parse error: expected '=' after identifier in let statement" << std::endl;
            return false;
        }
        auto expr = parse_expr();
        if (!expr.has_value()) {
            std::cerr << "Parse error: expected expression after '=' in let statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::semi)) {
            std::cerr << "Parse error: expected ';' after let statement" << std::endl;
            return false;
        }
        NodeStatementLet let_stmt{*ident_token, *expr};
        scope.statements.push_back(NodeStatement{let_stmt});
        return true;
    }

    bool parse_if_statement(NodeScope& scope) {
        consume(); // consume 'if'
        if (!consume_expected(TokenType::open_paren)) {
            std::cerr << "Parse error: expected '(' after if keyword" << std::endl;
            return false;
        }
        auto condition = parse_expr();
        if (!condition.has_value()) {
            std::cerr << "Parse error: expected condition in if statement" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::close_paren)) {
            std::cerr << "Parse error: expected ')' after if condition" << std::endl;
            return false;
        }
        if (!consume_expected(TokenType::open_brace)) {
            std::cerr << "Parse error: expected '{' after if condition" << std::endl;
            return false;
        }
        auto then_scope = parse_scope();
        if (!then_scope.has_value()) {
            std::cerr << "Parse error: expected scope after if condition" << std::endl;
            return false;
        }
        NodeStatementIf if_stmt{*condition, *then_scope};
        scope.statements.push_back(NodeStatement{if_stmt});
        return true;
    }
};