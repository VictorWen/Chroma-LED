#include "chroma_script.h"
#include <iostream>

void eat_literal(std::deque<ParseToken>& tokens, std::string literal) {
    if (tokens.front().type == LITERAL && tokens.front().val == literal) {
        tokens.pop_front();
        return;
    }
    std::string error = "Expected literal " + literal;
    // printf("HERE1\n");
    if (!tokens.empty())
        error += ", got " + tokens.front().val;
    throw ParseException(error.c_str());
}

std::string consume_identifier(std::deque<ParseToken>& tokens, std::string want) {
    if (tokens.front().type == IDENTIFIER) {
        std::string ret = tokens.front().val;
        tokens.pop_front();
        return ret;
    }
    // printf("HERE2\n");
    std::string error = "Expected identifier for " + want;
    if (!tokens.empty())
        error += ", got " + tokens.front().val;
    throw ParseException(error.c_str());
}

void Command::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    if (tokens.front().type == LITERAL && tokens.front().val == "func") {
        std::unique_ptr<ParseNode> next(new FuncDeclaration());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == LITERAL && tokens.front().val == "let") {
        std::unique_ptr<ParseNode> next(new SetVar());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else {
        std::unique_ptr<ParseNode> next(new Statement());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
}

void Command::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void Command::eval() {

}

void Statement::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) { 
    if (!tokens.empty() && tokens.front().type == IDENTIFIER && env.func_names.count(tokens.front().val) > 0) {
        std::unique_ptr<ParseNode> next(new InlineFuncCall());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else {
        std::unique_ptr<ParseNode> next(new Expression());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
}

void Statement::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void Expression::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    if (tokens.front().type == LITERAL && tokens.front().val == "(") {
        std::unique_ptr<ParseNode> next(new FuncCall());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == LITERAL && tokens.front().val == "[") {
        std::unique_ptr<ParseNode> next(new List());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == IDENTIFIER) {
        std::unique_ptr<ParseNode> next(new VarName());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == STRING) {
        std::unique_ptr<ParseNode> next(new StringLiteral());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == NUMBER) {
        std::unique_ptr<ParseNode> next(new NumberLiteral());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
}

void Expression::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void FuncDeclaration::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "func");
    
    this->func_name = consume_identifier(tokens, "function name");
    env.func_names.insert(this->func_name);
    
    while (!tokens.empty() && !(tokens.front().type == LITERAL && tokens.front().val == "=")) {
        std::unique_ptr<ParseNode> next(new VarName());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }

    eat_literal(tokens, "=");
    
    std::unique_ptr<ParseNode> next(new Statement());
    next->parse(tokens, env);
    this->children.push_back(move(next));
}

void FuncDeclaration::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void SetVar::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "let");
    
    this->var_name = consume_identifier(tokens, "variable name");

    eat_literal(tokens, "=");

    std::unique_ptr<ParseNode> next(new Expression());
    next->parse(tokens, env);
    this->children.push_back(move(next));

    if (env.func_names.count(this->var_name) > 0)
        env.func_names.erase(this->var_name);
}

void SetVar::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void InlineFuncCall::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    this->func_name = consume_identifier(tokens, "function name");
    // if (env.func_names.count(this->func_name) == 0) {
    //     std::string error = this->func_name + " is not a function";
    //     throw ParseException(error.c_str());
    // }

    while (!tokens.empty()) {
        std::unique_ptr<ParseNode> next(new Expression());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
}

void InlineFuncCall::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void FuncCall::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "(");
    
    this->func_name = consume_identifier(tokens, "function name");
    // if (env.func_names.count(this->func_name) == 0) {
    //     std::string error = this->func_name + " is not a function";
    //     throw ParseException(error.c_str());
    // }

    while (!tokens.empty() && !(tokens.front().type == LITERAL && tokens.front().val == ")")) {
        std::unique_ptr<ParseNode> next(new Expression());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }

    eat_literal(tokens, ")");
}

void FuncCall::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void List::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "[");

    while (!tokens.empty() && !(tokens.front().type == LITERAL && tokens.front().val == "]")) {
        std::unique_ptr<ParseNode> next(new Expression());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }

    eat_literal(tokens, "]");
}

void List::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void VarName::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    this->var_name = consume_identifier(tokens, "variable name");
    if (env.func_names.count(this->var_name) > 0) {
        std::string error = "Variable name " + this->var_name + " already in use by a function";
        // printf("HERE3\n");
        throw ParseException(error.c_str());
    }
}

void VarName::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void StringLiteral::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    if (tokens.empty()) {
        // printf("HERE4\n");
        throw ParseException("Expected a string");
    }
    this->val = tokens.front().val;
    tokens.pop_front();
}

void StringLiteral::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void NumberLiteral::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    if (tokens.empty()) {
        // printf("HERE5\n");
        throw ParseException("Expected a number");
    }
    //TODO: Insert num check!
    this->val = std::stof(tokens.front().val);
    tokens.pop_front();
}

void NumberLiteral::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void PrintVisitor::expand_tree(const ParseNode& n, const std::string name) {
    std::vector<std::string> new_output = output;
    output.clear();
    
    for (size_t i = 0; i < n.children.size(); i++) {
        n.children[i]->accept(*this);
    }

    new_output.push_back(name);
    
    for (size_t i = 0; i < this->output.size(); i++) {
        new_output.push_back("\t" + this->output[i]);
    }
    this->output = new_output;
}

void PrintVisitor::visit(const Command& n) {
    this->expand_tree(n, "Command");
}

void PrintVisitor::visit(const Statement& n) {
    this->expand_tree(n, "Statment");
}

void PrintVisitor::visit(const InlineFuncCall& n) {
    this->expand_tree(n, "InlineFuncCall (" + n.get_func_name() + ")");
}

void PrintVisitor::visit(const FuncDeclaration& n) {
    this->expand_tree(n, "FuncDeclaration (" + n.get_func_name() + ")");
}

void PrintVisitor::visit(const Expression& n) {
    this->expand_tree(n, "Expression");
}

void PrintVisitor::visit(const FuncCall& n) {
    this->expand_tree(n, "FuncCall (" + n.get_func_name() + ")");
}

void PrintVisitor::visit(const VarName& n) {
    this->expand_tree(n, "VarName (" + n.get_var_name() + ")");
}

void PrintVisitor::visit(const StringLiteral& n) {
    this->expand_tree(n, "StringLiteral (" + n.get_val() + ")");
}

void PrintVisitor::visit(const NumberLiteral& n) {
    this->expand_tree(n, "NumberLiteral (" + std::to_string(n.get_val()) + ")");
}

void PrintVisitor::visit(const SetVar& n) {
    this->expand_tree(n, "SetVar (" + n.get_var_name() + ")");
}

void PrintVisitor::visit(const List& n) {
    this->expand_tree(n, "List");
}

void PrintVisitor::print() const {
    for (size_t i = 0; i < this->output.size(); i++) {
        printf("%s\n", output[i].c_str());
    }
}