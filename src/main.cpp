#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

enum class TokenType {
    _return,
    int_lit,
    semi
};

struct Token {
    TokenType type;
    std::string value;
};


std::vector<Token> tokenize(const std::string& str) {
    std::vector<Token> tokens;
    
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        
        if (isspace(c)) {
            continue;
        }
        
        if (isalpha(c)) {
            if (i + 5 < str.length() && str.substr(i, 6) == "return") {
                tokens.push_back({TokenType::_return, "return"});
                i += 5; // Skip the length of "return"
            }
        }
        else if (isdigit(c)) {
            std::string int_lit;
            while (i < str.length() && isdigit(str[i])) {
                int_lit += str[i];
                i++;
            }
            i--; // Adjust for the increment in the loop
            tokens.push_back({TokenType::int_lit, int_lit});
        }
        else if (c == ';') {
            tokens.push_back({TokenType::semi, ";"});
        }
    }
    
    return tokens;
}

std::string tokens_to_asm(const std::vector<Token>& tokens) {
    std::stringstream asm_code;
    asm_code << ".global _start\n_start:\n";
    for (const Token& token : tokens) {
        switch (token.type) {
            case TokenType::_return:
                asm_code << "\tmov rax, 0\n"; // Example assembly code for return
                break;
            case TokenType::int_lit:
                asm_code << "\tmov rbx, " << token.value << "\n"; // Example assembly code for int literal
                break;
            case TokenType::semi:
                break;
        }
    }
    return asm_code.str();
}

int main(int argc, char* argv[]) {
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1]);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    std::cout << "Contents of the file: " << contents << std::endl;
    if (contents.empty()) {
        std::cerr << "Error: File is empty or could not be read." << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<Token> tokens = tokenize(contents);

    for (const Token& token : tokens) {
        std::string type_str;
        switch (token.type) {
            case TokenType::_return:
                type_str = "return";
                break;
            case TokenType::int_lit:
                type_str = "int_lit";
                break;
            case TokenType::semi:
                type_str = "semi";
                break;
        }
        std::cout << "Type: " << type_str << ", Value: " << token.value << std::endl;
    }

    std::string asm_code = tokens_to_asm(tokens);
    std::cout << "Generated Assembly Code:\n" << asm_code << std::endl;
    return EXIT_SUCCESS;
}