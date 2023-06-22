#include "evaluator.h"


ChromaData ScriptFunction::call(const std::vector<ChromaData>& args, const ChromaEnvironment& env) {
    ChromaEnvironment scopedEnv = env;
    
    if (args.size() != this->var_names.size()) {
        std::string error = "Number of arguments (" + 
            std::to_string(args.size()) + 
            ") do not match number (" + std::to_string(this->var_names.size()) + 
            ") of parameters for function call " + this->get_name();
    }
    
    for (size_t i = 0; i < args.size(); i++) {
        scopedEnv.variables[this->var_names[i]] =  args[i];
    }

    Evaluator eval(scopedEnv);
    this->code->accept(eval);
    return eval.get_env().ret_val;
}

void Evaluator::visit(const Command &n) {
    n.children[0]->accept(*this);
}

void Evaluator::visit(const Statement &n) {
    n.children[0]->accept(*this);
}

void Evaluator::visit(const Expression &n) {
    n.children[0]->accept(*this);
}

void Evaluator::visit(const FuncDeclaration &n) {
    std::vector<std::string> var_names; 
    for (auto& node : n.get_var_names()) {
        var_names.push_back(node->get_var_name());
    }
    std::shared_ptr<ScriptFunction> func = std::make_shared<ScriptFunction>(n.get_func_name(), var_names, n.children[0]->clone());
    this->env.functions[n.get_func_name()] = func;
}

void Evaluator::visit(const SetVar &n) {
    this->env.ret_val = ChromaData();
    n.children[0]->accept(*this);
    if (this->env.ret_val.get_type() == Null) {
        std::string error = "Expected a return value from expression in set var";
        throw ChromaRuntimeException(error.c_str());
    }
    this->env.variables[n.get_var_name()] =  this->env.ret_val;
}

void Evaluator::visit(const InlineFuncCall& n) {
    std::vector<ChromaData> args;
    for (auto& node : n.children) {
        this->env.ret_val = ChromaData();
        node->accept(*this);
        if (this->env.ret_val.get_type() == Null) {
            std::string error = "Expected a return value from expression in inline function call";
            throw ChromaRuntimeException(error.c_str());
        }
        args.push_back(this->env.ret_val);
    }
    this->env.ret_val = this->env.functions[n.get_func_name()]->call(args, this->env);
}

void Evaluator::visit(const FuncCall& n) {
    std::vector<ChromaData> args;
    for (auto& node : n.children) {
        this->env.ret_val = ChromaData();
        node->accept(*this);
        if (this->env.ret_val.get_type() == Null) {
            std::string error = "Expected a return value from expression in function call";
            throw ChromaRuntimeException(error.c_str());
        }
        args.push_back(this->env.ret_val);
    }
    this->env.ret_val = this->env.functions[n.get_func_name()]->call(args, this->env);
}

void Evaluator::visit(const ListNode& n) {
    std::vector<ChromaData> list;
    for (auto& node : n.children) {
        this->env.ret_val = ChromaData();
        node->accept(*this);
        if (this->env.ret_val.get_type() == Null) {
            std::string error = "Expected a return value from expression in list";
            throw ChromaRuntimeException(error.c_str());
        }
        list.push_back(this->env.ret_val);
    }
    this->env.ret_val = ChromaData(list);
}

void Evaluator::visit(const VarName& n) {
    if (this->env.variables.count(n.get_var_name()) == 0) {
        std::string error = "Variable name " + n.get_var_name() + " is undefined";
        throw ChromaRuntimeException(error.c_str());
    }
    this->env.ret_val = this->env.variables[n.get_var_name()];
}

void Evaluator::visit(const StringLiteral& n) {
    this->env.ret_val = ChromaData(n.get_val());
}

void Evaluator::visit(const NumberLiteral& n) {
    this->env.ret_val = ChromaData(n.get_val());
}
