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
