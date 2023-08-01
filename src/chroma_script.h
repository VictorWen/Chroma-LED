#ifndef CHROMA_SCRIPT_H
#define CHROMA_SCRIPT_H

#include <vector>
#include <deque>
#include <string>
#include <unordered_set>
#include <memory>

class NodeVisitor;

class ParseException : public std::exception {
    private:
        const std::string message;
    public:
        ParseException(const char * msg) : message(msg) { }
        const char* what () {
            return this->message.c_str();
        }
};

enum ChromaTokenType {
    STRING, NUMBER, IDENTIFIER, LITERAL
};

struct ParseEnvironment {
    // std::unordered_set<std::string> func_names;
};

struct ParseToken {
    ParseToken(std::string val, ChromaTokenType type) : val(val), type(type) {}
    std::string val;
    ChromaTokenType type;
};

class Tokenizer {
    public:
        Tokenizer() { }
        virtual ~Tokenizer() { }
        virtual void tokenize(std::string input, std::deque<ParseToken>& output);
};

class ParseNode {
    public:
        std::vector<std::unique_ptr<ParseNode>> children;
        
        ParseNode(const ParseNode& other) {
            for (const auto& child : other.children) {
                this->children.push_back(child->clone());
            }
        }
        ParseNode() {}

        virtual std::unique_ptr<ParseNode> clone() = 0;
        virtual void parse(std::deque<ParseToken>& tokens, ParseEnvironment env) = 0;
        virtual void accept(NodeVisitor& visitor) const = 0;
};

class Command : public ParseNode {
    public:
        Command(const Command& other) : ParseNode(other) { }
        Command() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<Command>(*this); }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class Statement : public ParseNode {
    public:
        Statement(const Statement& other) : ParseNode(other) { }
        Statement() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<Statement>(*this); }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class Expression : public ParseNode {
    public:
        Expression(const Expression& other) : ParseNode(other) { }
        Expression() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<Expression>(*this); }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class Identifier;

class FuncDeclaration : public ParseNode {
    private:
        std::string func_name;
        std::vector<std::unique_ptr<Identifier>> var_names;
    public:
        FuncDeclaration(const FuncDeclaration& other);
        FuncDeclaration() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<FuncDeclaration>(*this); }
        const std::string& get_func_name() const { return this->func_name; }
        const std::vector<std::unique_ptr<Identifier>>& get_var_names() const { return this->var_names; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};


class SetVar : public ParseNode {
    private:
        std::string var_name;
    public:
        SetVar(const SetVar& other) : ParseNode(other), var_name(other.var_name) { }
        SetVar() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<SetVar>(*this); }
        std::string get_var_name() const { return this->var_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class InlineFuncCall : public ParseNode {
    private:
        std::string func_name;
    public:
        InlineFuncCall(const InlineFuncCall& other) : ParseNode(other), func_name(other.func_name) { }
        InlineFuncCall() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<InlineFuncCall>(*this); }
        const std::string& get_func_name() const { return this->func_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class FuncCall : public ParseNode {
    private:
        std::string func_name;
    public:
        FuncCall(const FuncCall& other) : ParseNode(other), func_name(other.func_name) { }
        FuncCall() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<FuncCall>(*this); }
        const std::string& get_func_name() const { return this->func_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class ListNode : public ParseNode {
    public:
        ListNode(const ListNode& other) : ParseNode(other) { }
        ListNode() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<ListNode>(*this); }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class Identifier : public ParseNode {
    private:
        std::string name;
    public:
        Identifier(const Identifier& other) : ParseNode(other), name(other.name) { }
        Identifier() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<Identifier>(*this); }
        const std::string& get_name() const { return this->name; } 
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class StringLiteral : public ParseNode {
    private:
        std::string val;
    public:
        StringLiteral(const StringLiteral& other) : ParseNode(other), val(other.val) { }
        StringLiteral() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<StringLiteral>(*this); }
        const std::string& get_val() const { return this->val; } 
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};

class NumberLiteral : public ParseNode {
    private:
        float val;
    public:
        NumberLiteral(const NumberLiteral& other) : ParseNode(other), val(other.val) { }
        NumberLiteral() { } 
        std::unique_ptr<ParseNode> clone() { return std::make_unique<NumberLiteral>(*this); }
        const float& get_val() const { return this->val; } 
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
};


class NodeVisitor {
    public:
        virtual void visit(const Command& n) = 0;
        virtual void visit(const Statement& n) = 0;
        virtual void visit(const Expression& n) = 0;

        virtual void visit(const FuncDeclaration& n) = 0;
        virtual void visit(const SetVar& n) = 0;

        virtual void visit(const InlineFuncCall& n) = 0;
        virtual void visit(const FuncCall& n) = 0;

        virtual void visit(const ListNode& n) = 0;

        virtual void visit(const Identifier& n) = 0;
        virtual void visit(const StringLiteral& n) = 0;
        virtual void visit(const NumberLiteral& n) = 0;
};

class PrintVisitor : public NodeVisitor {
    private:
        std::vector<std::string> output; 
    public:
        void print(std::FILE* out_file = stderr) const;
        void expand_tree(const ParseNode& n, const std::string name);
        
        void visit(const Command& n);
        void visit(const Statement& n);
        void visit(const Expression& n);

        void visit(const FuncDeclaration& n);
        void visit(const SetVar& n);

        void visit(const InlineFuncCall& n);
        void visit(const FuncCall& n);

        void visit(const ListNode& n);

        void visit(const Identifier& n);
        void visit(const StringLiteral& n);
        void visit(const NumberLiteral& n);
};

#endif