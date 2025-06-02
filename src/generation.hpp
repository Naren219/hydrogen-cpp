#pragma once

class Generator {
    public:
        explicit Generator(const NodeExit& exit_node) : m_exit_node(exit_node) {}

        std::string generate() {
            std::string code;
            code += ".globl\t_main\n";
            code += ".p2align 2\n";
            code += "_main:\n";
            std::string exit_code = "0";
            if (!m_exit_node.exit.int_lit.value.empty()) {
                exit_code = m_exit_node.exit.int_lit.value;
            }
            code += "\tmov\tw0, #" + exit_code + "\n";
            code += "\tmov\tx16, #1\n";
            code += "\tsvc\t#0x80\n";
            return code;
        }
    private:
        NodeExit m_exit_node;

};