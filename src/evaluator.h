#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "chroma.h"
#include "chroma_script.h"


class ScriptFunction : public ChromaFunction {
    private:
        std::vector<std::string> var_names;
        std::unique_ptr<ParseNode> code;
    public:
        ScriptFunction(const std::string& name, const std::vector<std::string>& var_names, std::unique_ptr<ParseNode>&& code):
            ChromaFunction(name), var_names(var_names), code(move(code)) {}
        ChromaData call(const std::vector<ChromaData>& args, const ChromaEnvironment& env);
};

class Evaluator : public NodeVisitor {
    private:
        ChromaEnvironment env;
    public:
        const ChromaEnvironment& get_env() const { return this->env; }

        Evaluator(const ChromaEnvironment& env) : env(env) {}

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