#include <iostream>
#include <regex>

#include "chroma_script.h"


void Tokenizer::tokenize(std::string input, std::deque<ParseToken>& output) {
    std::regex keyword_regex(R"(^(let|func))");
    std::regex number_regex(R"(^[+-]?((\d+\.?\d*)|(\d*\.?\d+)))");
    std::regex string_regex(R"(^\".*\")");
    std::regex identifier_regex(R"(^[A-Za-z_]+\w*)");
    std::regex white_space_regex(R"(^\s+)");

    size_t index = 0;

    while (index < input.size()) {
        std::smatch match;
        std::string substr = input.substr(index);
        if (regex_search(substr, match, keyword_regex)) {
            index += match.length();
            output.push_back(ParseToken(match.str(), LITERAL));
        }
        else if (regex_search(substr, match, number_regex)) {
            index += match.length();
            output.push_back(ParseToken(match.str(), NUMBER));
        }
        else if (regex_search(substr, match, string_regex)) {
            index += match.length();
            std::string found_string = match.str();
            output.push_back(ParseToken(found_string.substr(1, found_string.size() - 2), STRING));
        }
        else if (regex_search(substr, match, identifier_regex)) {
            index += match.length();
            output.push_back(ParseToken(match.str(), IDENTIFIER));
        }
        else if (regex_search(substr, match, white_space_regex)) {
            index += match.length();
        }
        else {
            output.push_back(ParseToken(input.substr(index, 1), LITERAL));
            index += 1;
        }
    }
}


void eat_literal(std::deque<ParseToken>& tokens, std::string literal) {
    if (tokens.front().type == LITERAL && tokens.front().val == literal) {
        tokens.pop_front();
        return;
    }
    std::string error = "Expected literal '" + literal + "'";
    if (!tokens.empty())
        error += ", got '" + tokens.front().val + "'";
    throw ParseException(error.c_str());
}

std::string consume_identifier(std::deque<ParseToken>& tokens, std::string want) {
    if (tokens.front().type == IDENTIFIER) {
        std::string ret = tokens.front().val;
        tokens.pop_front();
        return ret;
    }
    std::string error = "Expected identifier for " + want;
    if (!tokens.empty())
        error += ", got '" + tokens.front().val + "'";
    throw ParseException(error.c_str());
}

FuncDeclaration::FuncDeclaration(const FuncDeclaration& other) : 
    ParseNode(other), func_name(other.func_name) { 
    for (const auto& var_name : other.var_names) {
        Identifier* copied_var_name = static_cast<Identifier*>(var_name->clone().release());
        this->var_names.push_back(std::unique_ptr<Identifier>(copied_var_name));
    }
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
    if (!tokens.empty() && tokens.front().type == IDENTIFIER && tokens.size() > 1) {
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
    if (tokens.empty()) {
        throw ParseException("Unfinished expression, expected more tokens");
    }

    if (tokens.front().type == LITERAL && tokens.front().val == "(") {
        std::unique_ptr<ParseNode> next(new FuncCall());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == LITERAL && tokens.front().val == "[") {
        std::unique_ptr<ParseNode> next(new ListNode());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }
    else if (tokens.front().type == IDENTIFIER) {
        std::unique_ptr<ParseNode> next(new Identifier());
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
    else {
        std::string error = "Unfinished expression, got literal '" + tokens.front().val + "'";
        throw ParseException(error.c_str());
    }
}

void Expression::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void FuncDeclaration::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "func");
    
    this->func_name = consume_identifier(tokens, "function name");
    // env.func_names.insert(this->func_name);
    
    while (!tokens.empty() && !(tokens.front().type == LITERAL && tokens.front().val == "=")) {
        std::unique_ptr<Identifier> next(new Identifier());
        next->parse(tokens, env);
        this->var_names.push_back(move(next));
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

    // if (env.func_names.count(this->var_name) > 0)
        // env.func_names.erase(this->var_name);
}

void SetVar::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void InlineFuncCall::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    this->func_name = consume_identifier(tokens, "function name");

    std::unique_ptr<ParseNode> next(new Expression());
    next->parse(tokens, env);
    this->children.push_back(move(next));

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

void ListNode::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    eat_literal(tokens, "[");

    while (!tokens.empty() && !(tokens.front().type == LITERAL && tokens.front().val == "]")) {
        std::unique_ptr<ParseNode> next(new Expression());
        next->parse(tokens, env);
        this->children.push_back(move(next));
    }

    eat_literal(tokens, "]");
}

void ListNode::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void Identifier::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    this->name = consume_identifier(tokens, "this");
}

void Identifier::accept(NodeVisitor& visitor) const {
    visitor.visit(*this);
}

void StringLiteral::parse(std::deque<ParseToken>& tokens, ParseEnvironment env) {
    if (tokens.empty()) {
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
    this->expand_tree(n, "Statement");
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

void PrintVisitor::visit(const Identifier& n) {
    this->expand_tree(n, "Identifier (" + n.get_name() + ")");
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

void PrintVisitor::visit(const ListNode& n) {
    this->expand_tree(n, "ListNode");
}

void PrintVisitor::print(std::FILE* out_file) const {
    for (size_t i = 0; i < this->output.size(); i++) {
        fprintf(out_file, "%s\n", output[i].c_str());
    }
}