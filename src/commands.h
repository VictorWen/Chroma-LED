#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>
#include <memory>

#include "chroma.h"


class CommandArgument {
    private:
        bool optional;
        std::string name;
        ChromaType type;
        std::string description;
    public:
        CommandArgument(std::string name, ChromaType type, std::string descr, bool optional=false):
            optional(optional), name(name), type(type), description(descr) { } // TODO: have a constructor that uses a string for type implying object base
        virtual ~CommandArgument() { }
        std::string get_name() const { 
            return this->name;
        }
        std::string to_string() const {
            const std::string types[] { "string", "number", "object", "list" };
            return this->name + " (" + types[this->type] + ") - " + this->description; 
        }
        bool is_optional() const { return this->optional; }
        ChromaType get_type() const { return this->type; }
        virtual void clean() = 0;
        virtual bool consume(const ChromaData& arg) = 0;
        virtual bool is_satisfied() const = 0;
        virtual ChromaData convert() const = 0;
};

class TypeArgument : public CommandArgument {
    private:
        bool done;
        ChromaData stored;
    public:
        TypeArgument(std::string name, ChromaType type, std::string descr, bool optional=false) : 
            CommandArgument(name, type, descr, optional), done(false) { }
        void clean() { this->done = false; }
        bool consume(const ChromaData& arg);
        bool is_satisfied() const;
        ChromaData convert() const;
};

class ExpandingTypeArgument : public CommandArgument {
    private:
        size_t min_num;
        std::vector<ChromaData> stored;
    public:
        ExpandingTypeArgument(std::string name, ChromaType type, std::string descr, size_t min_num = 0) :
            CommandArgument(name, type, descr, min_num == 0), min_num(min_num) { }
        void clean() { this->stored.clear(); }
        bool consume(const ChromaData& arg);
        bool is_satisfied() const;
        ChromaData convert() const;
};


// TODO: refactor, remove the template from ChromaCommand
template <class T>
class ChromaCommand : public ChromaFunction {
    protected: 
        std::string call_name;
        std::string description;
        std::vector<std::shared_ptr<CommandArgument>> arguments;
        ChromaCommand(const std::string& call_name) : ChromaFunction(call_name), call_name(call_name) { }
    public:
        ChromaCommand(const std::string& call_name, const std::string& description, const std::vector<CommandArgument>& args) :
            ChromaFunction(call_name), call_name(call_name), description(description), arguments(args) { }
        std::string get_format() const;
        std::string get_help() const;
        ChromaData call(const std::vector<ChromaData>& args, ChromaEnvironment& env);
};

class LambdaAdapter : public ChromaFunction {
    protected:
        std::string call_name;
        std::string description;
        std::vector<std::shared_ptr<CommandArgument>> arguments;
        std::function<ChromaData(const std::vector<ChromaData>& args, ChromaEnvironment& env)> lambda;
        LambdaAdapter(const std::string& call_name) : ChromaFunction(call_name), call_name(call_name) { }
    public:
        // TODO: fix arguments
        LambdaAdapter(const std::string& call_name, const std::string& description, const std::vector<std::shared_ptr<CommandArgument>>& args, 
            std::function<ChromaData(const std::vector<ChromaData>& args, ChromaEnvironment& env)> lambda) :
            ChromaFunction(call_name), call_name(call_name), description(description), arguments(args), lambda(lambda) { }
        std::string get_format() const;
        std::string get_help() const;
        ChromaData call(const std::vector<ChromaData>& args, ChromaEnvironment& env);
};

template <class T>
class CommandBuilder : public ChromaCommand<T> {
    public:
        CommandBuilder(const std::string& call_name) : ChromaCommand<T>(call_name) { }
        // CommandBuilder<T>& add_argument(const CommandArgument& argument);
        CommandBuilder<T>& add_argument(const std::string& name, const ChromaType type, const std::string& descr);
        CommandBuilder<T>& add_optional_argument(const std::string& name, const ChromaType type, const std::string& descr);
        CommandBuilder<T>& add_expanding_argument(const std::string& name, const ChromaType type, const std::string& descr, const size_t min_num);
        CommandBuilder<T>& set_description(const std::string& descr);
};

template <class T>
std::string ChromaCommand<T>::get_format() const {
    std::string output = this->call_name;
    for (const auto& arg : this->arguments) {
        output += " " + arg->get_name();
    }
    return output;
}

template <class T>
std::string ChromaCommand<T>::get_help() const {
    std::string output = this->get_format();
    output += "\n\t" + this->description;
    output += "\n";
    for (const auto& arg : this->arguments) {
        output += "\n\t" + arg->to_string();
    }
    return output;
}


// Matching data to argument is roughly equivalent to regex parsing,
// which is not something I want to implement/learn how to do right now
// SO, using a greedy approach instead
// TODO: figure out regex parsing like algorithm?
template <class T>
ChromaData ChromaCommand<T>::call(const std::vector<ChromaData>& args, ChromaEnvironment& env) { 
    std::vector<ChromaData> converted;
    size_t i = 0, j = 0, prev_i = 0;
    
    for (size_t k = 0; k < this->arguments.size(); k++) {
        this->arguments[k]->clean();
    }

    while (j < this->arguments.size()) {
        std::shared_ptr<CommandArgument>& cmd_arg = this->arguments[j];
        if (i < args.size() && cmd_arg->consume(args[i]))
            i++;
        else if (cmd_arg->is_satisfied()) {
            converted.push_back(cmd_arg->convert());
            j++;
            prev_i = i;
        }
        else if (cmd_arg->is_optional()) {
            i = prev_i;
            j++;
        }
        else if (i == args.size()) {
            std::string error = "Command received not enough arguments, " + cmd_arg->get_name() + " is required but is missing a value";
            throw ChromaRuntimeException(error.c_str());
        }
        else {
            std::string error = "Command received a invalid argument, " + cmd_arg->get_name() + " is required but could not accept the value at position " + std::to_string(i);
            throw ChromaRuntimeException(error.c_str());
        }
    }

    return ChromaData(std::make_shared<T>(converted));
}

// template <class T>
// CommandBuilder<T> &CommandBuilder<T>::add_argument(const CommandArgument& argument) {
//     this->arguments.push_back(argument);
//     return *this;
// }

template <class T>
CommandBuilder<T> &CommandBuilder<T>::add_argument(const std::string& name, const ChromaType type, const std::string& descr) {
    this->arguments.push_back(std::make_shared<TypeArgument>(name, type, descr));
    return *this;
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::add_optional_argument(const std::string& name, const ChromaType type, const std::string& descr) {
    this->arguments.push_back(std::make_shared<TypeArgument>(name, type, descr, true));
    return *this;
}

template <class T>
CommandBuilder<T>& CommandBuilder<T>::add_expanding_argument(const std::string& name, const ChromaType type, const std::string& descr, const size_t min_num) {
    this->arguments.push_back(std::make_shared<ExpandingTypeArgument>(name, type, descr, min_num));
    return *this;
}

template <class T>
CommandBuilder<T> &CommandBuilder<T>::set_description(const std::string& descr) {
    this->description = descr;
    return *this;
}

#endif