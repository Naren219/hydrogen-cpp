#pragma once

struct NodeExpr {
    Token int_lit;
};

struct NodeExit {
    NodeExpr exit;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)), m_pos(0) {}

    NodeExit parse() {
        NodeExit exit_node;
        while (auto token = next()) {
            if (token->type == TokenType::_exit) {
                if (auto int_token = next(); int_token && int_token->type == TokenType::int_lit) {
                    exit_node.exit.int_lit = *int_token;
                }
            } else if (token->type == TokenType::int_lit) {
                exit_node.exit.int_lit = *token;
            } // Ignore semicolons and unknown tokens
        }
        return exit_node;
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