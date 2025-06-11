#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "tokenization.hpp"
#include "parser.hpp"
#include "generation.hpp"

int main(int argc, char* argv[]) {
    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1]);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens));
    NodeProgram program = parser.parse();

    Generator generator(program);
    std::string asm_code = generator.generate_program();
    {
        std::fstream file("out.s", std::ios::out | std::ios::trunc);
        file << asm_code;
    }

    std::cout << "Generated Assembly Code:\n" << asm_code << std::endl;

    int assemble_status = system("as -o out.o out.s");
    int link_status = system("ld -arch arm64 -o out_exec out.o -lSystem -syslibroot `xcrun --show-sdk-path` -e _main");
    int run_status = system("./out_exec");

    return WEXITSTATUS(run_status);
};