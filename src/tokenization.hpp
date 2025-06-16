#pragma once

#include <string>
#include <vector>

enum class TokenType {
    _exit,
    int_lit,
    semi,
    open_paren,
    close_paren,
    ident,
    let,
    eq,
    plus,
    star,
    slash,
    minus,
    open_brace,
    close_brace,
    if_,
};

struct Token {
    TokenType type;
    std::string value;
};

class Tokenizer {
    public:
        inline Tokenizer(const std::string& src) : m_src(std::move(src)) {}


        inline std::vector<Token> tokenize() {
            std::string buf;
            std::vector<Token> tokens;

            while(peak().has_value()) {
                char c = consume();
                if (isspace(c)) {
                    continue;
                }
                if (isalpha(c)) {
                    buf += c;
                    while (peak().has_value() && isalpha(peak().value())) {
                        buf += consume();
                    }
                    if (buf == "exit") {
                        tokens.push_back(Token{TokenType::_exit, buf});
                        buf.clear();
                    } else if (buf == "let") {
                        tokens.push_back(Token{TokenType::let, buf});
                        buf.clear();
                    } else if (buf == "if") {
                        tokens.push_back(Token{TokenType::if_, buf});
                        buf.clear();
                    } else {
                        tokens.push_back(Token{TokenType::ident, buf});
                        buf.clear();
                    }
                } else if (isdigit(c)) {
                    buf += c;
                    while (peak().has_value() && isdigit(peak().value())) {
                        buf += consume();
                    }
                    tokens.push_back(Token{TokenType::int_lit, buf});
                    buf.clear();
                } else if (c == '(') {
                    tokens.push_back(Token{TokenType::open_paren, "("});
                } else if (c == ')') {
                    tokens.push_back(Token{TokenType::close_paren, ")"});
                } else if (c == ';') {
                    tokens.push_back(Token{TokenType::semi, ";"});
                } else if (c == '=') {
                    tokens.push_back(Token{TokenType::eq, "="});
                } else if (c == '+') {
                    tokens.push_back(Token{TokenType::plus, "+"});
                } else if (c == '*') {
                    tokens.push_back(Token{TokenType::star, "*"});
                } else if (c == '/') {
                    tokens.push_back(Token{TokenType::slash, "/"});
                } else if (c == '-') {
                    tokens.push_back(Token{TokenType::minus, "-"});
                } else if (c == '{') {
                    tokens.push_back(Token{TokenType::open_brace, "{"});
                } else if (c == '}') {
                    tokens.push_back(Token{TokenType::close_brace, "}"});
                } else {
                    throw std::runtime_error("Unexpected character: " + std::string(1, c));
                }
            }

            return tokens;
        }

    private:

        std::optional<char> peak(int offset = 0) const {
            if (m_pos + offset < m_src.length()) {
                return m_src[m_pos + offset];
            }
            return std::nullopt;
        }

        char consume() {
            if (m_pos < m_src.length()) {
                return m_src[m_pos++];
            }
            throw std::out_of_range("No more characters to consume");
        }
        const std::string& m_src;
        int m_pos = 0;
};