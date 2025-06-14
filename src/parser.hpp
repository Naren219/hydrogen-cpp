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

struct NodeStatement {
    std::variant<NodeStatementExit, NodeStatementLet> expr;
};

struct NodeProgram {
    std::vector<NodeStatement> statements;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_pos(0) {}

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

private:
    std::vector<Token> m_tokens;
    size_t m_pos;
    
    std::unordered_map<TokenType, int> m_precedence = {
        {TokenType::plus, 1},
        {TokenType::minus, 1},
        {TokenType::star, 2},
        {TokenType::slash, 2}
    };

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