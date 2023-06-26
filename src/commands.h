#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>
#include <memory>

#include "chroma.h"


class ArgumentVerifier {
    public:
        virtual bool verify(const ChromaData& argument) = 0;
};

class TypeVerifier : public ArgumentVerifier {
    private:
        ChromaType type;
    public:
        TypeVerifier(ChromaType type) : type(type) { }
        bool verify(const ChromaData& argument) {
            return argument.get_type() == this->type;
        }
};

class CommandArgument {
    private:
        std::string name;
        ChromaType type;
        std::string description;
        std::shared_ptr<ArgumentVerifier> verifier;
    public:
        CommandArgument(std::string name, ChromaType type, std::string descr, std::shared_ptr<ArgumentVerifier> verifier):
            name(name), type(type), description(descr), verifier(verifier) { }
        std::string get_name() const { 
            return this->name;
        }
        std::string to_string() const {
            const std::string types[] { "string", "number", "effect", "object", "list" };
            return this->name + " (" + types[this->type] + ") - " + this->description; 
        }
        bool verify(const ChromaData& arg) { return this->verifier->verify(arg); }
};

template <class T>
class ChromaCommand : public ChromaFunction {
    protected: 
        std::string call_name;
        std::string description;
        std::vector<CommandArgument> arguments;
        ChromaCommand(const std::string& call_name) : ChromaFunction(call_name), call_name(call_name) { }
    public:
        ChromaCommand(const std::string& call_name, const std::string& description, const std::vector<CommandArgument>& args) :
            ChromaFunction(call_name), call_name(call_name), description(description), arguments(args) { }
        std::string get_format() const;
        std::string get_help() const;
        ChromaData call(const std::vector<ChromaData>& args, const ChromaEnvironment& env);
};

template <class T>
class CommandBuilder : public ChromaCommand<T> {
    public:
        CommandBuilder(const std::string& call_name) : ChromaCommand<T>(call_name) { }
        CommandBuilder& add_argument(const std::string& name, const ChromaType type, const std::string& descr);
        CommandBuilder& add_argument(const std::string& name, const ChromaType type, const std::shared_ptr<ArgumentVerifier> verifier, const std::string& descr);
        CommandBuilder& add_optional_argument(const std::string& name, const ChromaType type);
        CommandBuilder& set_description(const std::string& descr);
};

template <class T>
std::string ChromaCommand<T>::get_format() const {
    std::string output = this->call_name;
    for (auto arg : this->arguments) {
        output += " " + arg.get_name();
    }
    return output;
}

template <class T>
std::string ChromaCommand<T>::get_help() const {
    std::string output = this->get_format();
    output += "\n\t" + this->description;
    output += "\n";
    for (auto arg : this->arguments) {
        output += "\n\t" + arg.to_string();
    }
    return output;
}

template <class T>
ChromaData ChromaCommand<T>::call(const std::vector<ChromaData>& args, const ChromaEnvironment& env) {
    // TODO: optional args
    if (this->arguments.size() < args.size()) {
        throw ChromaRuntimeException("Command received too many arguments"); //TODO: make dynamic
    }

    for (size_t i = 0; i < args.size(); i++) {
        CommandArgument& cmd_arg = this->arguments[i];
        const ChromaData& arg_val = args[i];
        if (!cmd_arg.verify(arg_val)) {
            throw ChromaRuntimeException("Invalid argument type"); // TODO: make dynamic
        }
    }

    return ChromaData(std::make_shared<T>(args));
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::add_argument(const std::string& name, const ChromaType type, const std::string& descr) {
    CommandArgument arg(name, type, descr, std::shared_ptr<TypeVerifier>(new TypeVerifier(type)));
    this->arguments.push_back(arg);
    return *this;
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::add_argument(const std::string& name, const ChromaType type, const std::shared_ptr<ArgumentVerifier> verifier, const std::string& descr) {
    CommandArgument arg(name, type, descr, verifier);
    this->arguments.push_back(arg);
    return *this;
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::add_optional_argument(const std::string& name, const ChromaType type) {
    return *this;
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::set_description(const std::string& descr) {
    this->description = descr;
    return *this;
}

#endif