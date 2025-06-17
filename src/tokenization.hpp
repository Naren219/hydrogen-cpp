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
    else_,
    elif,
};

struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class Tokenizer {
    public:
        inline Tokenizer(const std::string& src) : m_src(std::move(src)) {}


        inline std::vector<Token> tokenize() {
            std::string buf;
            std::vector<Token> tokens;
            size_t line = 1;
            size_t column = 1;

            while(peek().has_value()) {
                char c = consume();
                if (isspace(c)) {
                    if (c == '\n') {
                        line++;
                        column = 1;
                    } else {
                        column++;
                    }
                    continue;
                }
                if (isalpha(c)) {
                    buf += c;
                    while (peek().has_value() && isalpha(peek().value())) {
                        buf += consume();
                    }
                    if (buf == "exit") {
                        tokens.push_back(Token{TokenType::_exit, buf, line, column});
                        buf.clear();
                    } else if (buf == "let") {
                        tokens.push_back(Token{TokenType::let, buf, line, column});
                        buf.clear();
                    } else if (buf == "if") {
                        tokens.push_back(Token{TokenType::if_, buf, line, column});
                        buf.clear();
                    } else if (buf == "elif") {
                        tokens.push_back(Token{TokenType::elif, buf, line, column});
                        buf.clear();
                    } else if (buf == "else") {
                        tokens.push_back(Token{TokenType::else_, buf, line, column});
                        buf.clear();
                    } else {
                        tokens.push_back(Token{TokenType::ident, buf, line, column});
                        buf.clear();
                    }
                } else if (isdigit(c)) {
                    buf += c;
                    while (peek().has_value() && isdigit(peek().value())) {
                        buf += consume();
                    }
                    tokens.push_back(Token{TokenType::int_lit, buf, line, column});
                    buf.clear();
                } else if (c == '(') {
                    tokens.push_back(Token{TokenType::open_paren, "(", line, column});
                } else if (c == ')') {
                    tokens.push_back(Token{TokenType::close_paren, ")", line, column});
                } else if (c == ';') {
                    tokens.push_back(Token{TokenType::semi, ";", line, column});
                } else if (c == '=') {
                    tokens.push_back(Token{TokenType::eq, "=", line, column});
                } else if (c == '+') {
                    tokens.push_back(Token{TokenType::plus, "+", line, column});
                } else if (c == '*') {
                    tokens.push_back(Token{TokenType::star, "*", line, column});
                } else if (c == '/') {
                    if (peek().has_value() && peek().value() == '/') {
                        consume(); // consume the second '/'
                        while (peek().has_value() && peek().value() != '\n') {
                            consume(); // consume the rest of the line
                            column++;
                        }
                        if (peek().has_value()) {
                            consume(); // consume the newline
                            line++;
                            column = 1;
                        }
                    } else if (peek().has_value() && peek().value() == '*') {
                        consume(); // consume the '*'
                        while (peek().has_value()) {
                            if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/') {
                                consume(); // consume the '*'
                                consume(); // consume the '/'
                                column += 2;
                                break;
                            }
                            if (peek().value() == '\n') {
                                line++;
                                column = 1;
                            } else {
                                column++;
                            }
                            consume();
                        }
                    } else {
                        tokens.push_back(Token{TokenType::slash, "/", line, column});
                    }
                } else if (c == '-') {
                    tokens.push_back(Token{TokenType::minus, "-", line, column});
                } else if (c == '{') {
                    tokens.push_back(Token{TokenType::open_brace, "{", line, column});
                } else if (c == '}') {
                    tokens.push_back(Token{TokenType::close_brace, "}", line, column});
                } else {
                    throw std::runtime_error("Unexpected character: " + std::string(1, c) + " at line " + std::to_string(line) + " column " + std::to_string(column));
                }
            }

            return tokens;
        }

    private:

        std::optional<char> peek(int offset = 0) const {
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