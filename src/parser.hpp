#pragma once

#include <variant>

struct NodeExprIntLit {
    Token int_lit;
};

struct NodeExprIdent {
    Token ident;
};

struct NodeExpr {
    std::variant<NodeExprIntLit, NodeExprIdent> var;
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

struct NodeExit {
    NodeExpr exit;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_pos(0) {}

    NodeProgram parse() {
        NodeProgram program;
        while (auto token = next()) {
            if (token->type == TokenType::_exit) {
                NodeStatementExit exit_stmt;
                auto open_paren = next();
                if (!open_paren || open_paren->type != TokenType::open_paren) {
                    std::cerr << "Parse error: expected '(' after exit keyword" << std::endl;
                    continue;
                }
                auto expr_token = next();
                if (expr_token && expr_token->type == TokenType::int_lit) {
                    NodeExprIntLit int_lit_expr{*expr_token};
                    exit_stmt.exit.var = int_lit_expr;
                } else if (expr_token && expr_token->type == TokenType::ident) {
                    NodeExprIdent ident_expr{*expr_token};
                    exit_stmt.exit.var = ident_expr;
                } else {
                    std::cerr << "Parse error: expected integer literal or identifier after '(' in exit statement" << std::endl;
                    continue;
                }
                auto close_paren = next();
                if (!close_paren || close_paren->type != TokenType::close_paren) {
                    std::cerr << "Parse error: expected ')' after expression in exit statement" << std::endl;
                    continue;
                }
                program.statements.push_back(NodeStatement{exit_stmt});
            } else if (token->type == TokenType::let) {
                NodeStatementLet let_stmt;
                auto ident_token = next();
                if (!ident_token || ident_token->type != TokenType::ident) {
                    std::cerr << "Parse error: expected identifier after 'let'" << std::endl;
                    continue;
                }
                let_stmt.ident = *ident_token;
                auto eq_token = next();
                if (!eq_token || eq_token->type != TokenType::eq) {
                    std::cerr << "Parse error: expected '=' after identifier in let statement" << std::endl;
                    continue;
                }
                auto value_token = next();
                if (value_token && value_token->type == TokenType::int_lit) {
                    NodeExprIntLit int_lit_expr{*value_token};
                    let_stmt.value.var = int_lit_expr;
                } else if (value_token && value_token->type == TokenType::ident) {
                    NodeExprIdent ident_expr{*value_token};
                    let_stmt.value.var = ident_expr;
                } else {
                    std::cerr << "Parse error: expected value after '=' in let statement" << std::endl;
                    continue;
                }
                auto semi_token = next();
                if (!semi_token || semi_token->type != TokenType::semi) {
                    std::cerr << "Parse error: expected ';' after let statement" << std::endl;
                    continue;
                }
                program.statements.push_back(NodeStatement{let_stmt});
            } else {
                // Skip unknown or unsupported tokens
            }
        }
        return program;
    }

private:
    std::vector<Token> m_tokens;
    size_t m_pos;

    std::optional<Token> next() {
        if (m_pos < m_tokens.size()) {
            return m_tokens[m_pos++];
        }
        return std::nullopt;
    }
};