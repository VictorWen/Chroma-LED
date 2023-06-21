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

enum TokenType {
    STRING, NUMBER, IDENTIFIER, LITERAL
};

struct ParseEnvironment {
    std::unordered_set<std::string> func_names;
};

struct ParseToken {
    ParseToken(std::string val, TokenType type) : val(val), type(type) {}
    std::string val;
    TokenType type;
};

class ParseNode {
    public:
        std::vector<std::unique_ptr<ParseNode>> children;
        virtual void parse(std::deque<ParseToken>& tokens, ParseEnvironment env) = 0;
        virtual void accept(NodeVisitor& visitor) const = 0;
        virtual void eval() = 0;
};

class Command : public ParseNode {
    public:
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval();
};

class Statement : public ParseNode {
    public:
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class Expression : public ParseNode {
    public:
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class FuncDeclaration : public ParseNode {
    private:
        std::string func_name;
    public:
        const std::string& get_func_name() const { return this->func_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};


class SetVar : public ParseNode {
    private:
        std::string var_name;
    public:
        std::string get_var_name() const { return this->var_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class InlineFuncCall : public ParseNode {
    private:
        std::string func_name;
    public:
        const std::string& get_func_name() const { return this->func_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class FuncCall : public ParseNode {
    private:
        std::string func_name;
    public:
        const std::string& get_func_name() const { return this->func_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class ListNode : public ParseNode {
    public:
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class FuncName : public ParseNode {
    public:
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class VarName : public ParseNode {
    private:
        std::string var_name;
    public:
        const std::string& get_var_name() const { return this->var_name; }
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class StringLiteral : public ParseNode {
    private:
        std::string val;
    public:
        const std::string& get_val() const { return this->val; } 
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
};

class NumberLiteral : public ParseNode {
    private:
        float val;
    public:
        const float& get_val() const { return this->val; } 
        void parse(std::deque<ParseToken>& tokens, ParseEnvironment env);
        void accept(NodeVisitor& visitor) const;
        void eval() {};
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

        virtual void visit(const VarName& n) = 0;
        virtual void visit(const StringLiteral& n) = 0;
        virtual void visit(const NumberLiteral& n) = 0;
};

class PrintVisitor : public NodeVisitor {
    private:
        std::vector<std::string> output; 
    public:
        void print() const;
        void expand_tree(const ParseNode& n, const std::string name);
        
        void visit(const Command& n);
        void visit(const Statement& n);
        void visit(const Expression& n);

        void visit(const FuncDeclaration& n);
        void visit(const SetVar& n);

        void visit(const InlineFuncCall& n);
        void visit(const FuncCall& n);

        void visit(const ListNode& n);

        void visit(const VarName& n);
        void visit(const StringLiteral& n);
        void visit(const NumberLiteral& n);
};

#endif