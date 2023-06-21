#ifndef CHROMA_H
#define CHROMA_H

#include <unordered_map>

#include "chroma_script.h"
#include "commands.h"

class ChromaData {

}

class ChromaFunction {
    std::string name;
    public:
        ChromaData call(std::string alias, std::vector<ChromaData> args);    
};

class ChromaEnvironment {
    std::unordered_map<std::string, ChromaFunction> commands;
    std::unordered_map<std::string, ChromaObject> variables;
};

class Evaluator : public NodeVisitor {
    private:
        ChromaEnvironment env;
    public:
        const ChromaEnvironment& env() const { return this->env; }

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