#pragma once

#include <variant>
#include <vector>
#include <optional>
#include <iostream>
#include "tokenization.hpp"
#include "arena.hpp"

// Forward declarations
struct NodeExpr;

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct BinExprAdd {
    NodeExpr* left;
    NodeExpr* right;
};

struct BinExprMul {
    NodeExpr* left;
    NodeExpr* right;
};

struct BinExpr {
    std::variant<BinExprAdd, BinExprMul> var;
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

struct NodeStatement {
    std::variant<NodeStatementExit, NodeStatementLet> expr;
};

struct NodeProgram {
    std::vector<NodeStatement> statements;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_pos(0), m_arena() {}

    NodeProgram parse() {
        NodeProgram program;
        while (auto token = peek()) {
            if (token->type == TokenType::_exit) {
                consume(); // consume 'exit'
                if (!consume_expected(TokenType::open_paren)) {
                    std::cerr << "Parse error: expected '(' after exit keyword" << std::endl;
                    continue;
                }
                auto expr = parse_expr();
                if (!expr.has_value()) {
                    std::cerr << "Parse error: expected expression in exit statement" << std::endl;
                    continue;
                }
                if (!consume_expected(TokenType::close_paren)) {
                    std::cerr << "Parse error: expected ')' after expression in exit statement" << std::endl;
                    continue;
                }
                NodeStatementExit exit_stmt{*expr};
                program.statements.push_back(NodeStatement{exit_stmt});
            } else if (token->type == TokenType::let) {
                consume(); // consume 'let'
                auto ident_token = consume_expected(TokenType::ident);
                if (!ident_token.has_value()) {
                    std::cerr << "Parse error: expected identifier after 'let'" << std::endl;
                    continue;
                }
                if (!consume_expected(TokenType::eq)) {
                    std::cerr << "Parse error: expected '=' after identifier in let statement" << std::endl;
                    continue;
                }
                auto expr = parse_expr();
                if (!expr.has_value()) {
                    std::cerr << "Parse error: expected expression after '=' in let statement" << std::endl;
                    continue;
                }
                if (!consume_expected(TokenType::semi)) {
                    std::cerr << "Parse error: expected ';' after let statement" << std::endl;
                    continue;
                }
                NodeStatementLet let_stmt{*ident_token, *expr};
                program.statements.push_back(NodeStatement{let_stmt});
            } else {
                consume(); // Skip unknown tokens
            }
        }
        return program;
    }

    // Get arena statistics
    void print_arena_stats() const {
        std::cout << "Arena Stats:" << std::endl;
        std::cout << "  Total allocated: " << m_arena.total_allocated() << " bytes" << std::endl;
        std::cout << "  Memory used: " << m_arena.memory_used() << " bytes" << std::endl;
        std::cout << "  Efficiency: " << (100.0 * m_arena.memory_used() / m_arena.total_allocated()) << "%" << std::endl;
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos;
    mutable Arena m_arena; // mutable because allocation happens in const methods

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

    // Expression parsing with operator precedence
    // expr -> term (('+') term)*
    std::optional<NodeExpr> parse_expr() {
        auto left = parse_term();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (auto token = peek()) {
            if (token->type == TokenType::plus) {
                consume(); // consume '+'
                auto right = parse_term();
                if (!right.has_value()) {
                    std::cerr << "Parse error: expected expression after '+'" << std::endl;
                    return std::nullopt;
                }
                
                // Allocate nodes in arena
                NodeExpr* left_ptr = m_arena.allocate<NodeExpr>(*left);
                NodeExpr* right_ptr = m_arena.allocate<NodeExpr>(*right);
                
                BinExprAdd add_expr{left_ptr, right_ptr};
                NodeExpr new_expr;
                new_expr.var = BinExpr{add_expr};
                left = new_expr;
            } else {
                break;
            }
        }

        return left;
    }

    // term -> factor (('*') factor)*
    std::optional<NodeExpr> parse_term() {
        auto left = parse_factor();
        if (!left.has_value()) {
            return std::nullopt;
        }

        while (auto token = peek()) {
            if (token->type == TokenType::star) {
                consume(); // consume '*'
                auto right = parse_factor();
                if (!right.has_value()) {
                    std::cerr << "Parse error: expected expression after '*'" << std::endl;
                    return std::nullopt;
                }
                
                // Allocate nodes in arena
                NodeExpr* left_ptr = m_arena.allocate<NodeExpr>(*left);
                NodeExpr* right_ptr = m_arena.allocate<NodeExpr>(*right);
                
                BinExprMul mul_expr{left_ptr, right_ptr};
                NodeExpr new_expr;
                new_expr.var = BinExpr{mul_expr};
                left = new_expr;
            } else {
                break;
            }
        }

        return left;
    }

    // factor -> int_lit | ident | '(' expr ')'
    std::optional<NodeExpr> parse_factor() {
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

        return std::nullopt;
    }
};