#pragma once

#include <string>
#include <vector>

enum class TokenType {
    _exit,
    int_lit,
    semi
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
                    }
                } else if (isdigit(c)) {
                    buf += c;
                    while (peak().has_value() && isdigit(peak().value())) {
                        buf += consume();
                    }
                    tokens.push_back(Token{TokenType::int_lit, buf});
                    buf.clear();
                } else if (c == ';') {
                    tokens.push_back(Token{TokenType::semi, ";"});
                } else {
                    throw std::runtime_error("Unexpected character: " + std::string(1, c));
                }
            }

            return tokens;
        }

    private:

        std::optional<char> peak(int ahead = 1) const {
            if (m_pos + ahead < m_src.length()) {
                return m_src[m_pos];
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