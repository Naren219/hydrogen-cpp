## learnings
for our expressions, we have a circular dependency with a `BinExpr` and `NodeExpr` which isn't allowed since we don't know the exact size to allocate when defining each object. to solve this, we use pointers instead. however, this creates issues with cpu cache as we'll get pointers to random spots in memory which is not optimal for cache (which queries for objects in chunks). thus, we can use an arena to get a huge block of memory and store all pointers in a single chain. 

### Recursive descent
see [alternative parser](./src/parser-recurse.hpp) for implementation

**function call hierarchy creates precedence!** top-down parser.

```
parse_expr()     ← Lowest precedence (+, -)
    ↓ calls
parse_term()     ← Higher precedence (*, /)  
    ↓ calls
parse_factor()   ← Highest precedence (numbers, parens)
    ↓ can call back to
parse_expr()     ← For parentheses expressions!
```

### Precedence climbing
see [parser](./src/parser.hpp) for implementation

```cpp
std::optional<NodeExpr> parse_expr(int min_precedence = 0) {
    auto left = parse_primary();

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

        // Create binary expression based on operator type
        if (op->type == TokenType::plus) {
            BinExprAdd add_expr{
                std::make_shared<NodeExpr>(*left),
                std::make_shared<NodeExpr>(*right)
            };
            NodeExpr new_expr;
            new_expr.var = BinExpr{add_expr};
            left = new_expr;
        } ...
    }

    return left;
}
```

We keep iterating min precedence by 1 to prevent an infinite loop. Once we get an operator with a lower precedence than what was passed into the recursive call, we break the recursive loop and go up the stack (from the most recent called). This higher-order expression is set to `right` and we keep the terms set to left to create the binary tree. 

### variable management
```cpp
class SymbolTable {
    std::vector<std::unordered_map<std::string, bool>> m_scopes;
```
Each scope is a map where the keys are the variable names and values are the respective declaration *stati* (plural for statuses lol). 

```cpp
// Search from innermost to outermost scope
for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
    if (it->find(name) != it->end()) {
        return true;
    }
}
return false;
```
Used for variable shadowing (vars declared in the inner scopes should take precedence over global ones).

### if elif else statements
@[generation.hpp](./src/generation.hpp)
```cpp
code += "\tldr\tw0, [sp], #16\n";

// Compare with 0 and branch if equal (false)
code += "\tcmp\tw0, #0\n";
const std::string skip_label = ".L" + std::to_string(current_label) + "_skip";
code += "\tb.eq\t" + skip_label + "\n";

code += generate_scope(if_stmt.then_scope);
if (if_stmt.predicate) { // predicate meaning elif or else present
    const std::string end_label = ".L" + std::to_string(current_label) + "_end";
    code += "\tb\t" + end_label + "\n"; // b means to JUMP!
    code += skip_label + ":\n";

    code += generate_predicate(*if_stmt.predicate, end_label);
    code += end_label + ":\n";
} else {
    code += skip_label + ":\n";
}
```

Skip logic: If a given condition is false, the code jumps to a `skip_label` which is past the jump to the `end_label` (in case any clause is executed, the program should skip over all other clauses (aka be triggered by the end_label jump) before continuing onto the predicates).