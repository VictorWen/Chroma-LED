#ifndef COMMANDS_H
#define COMMANDS_H

#include <string>
#include <vector>
#include <memory>

enum ChromaType {
    String, Number, Effect, Object, List
};

class ChromaObject {
    ChromaType type;
    public:
        const ChromaType& get_type() const { return this->type; }
};

class ArgumentVerifier {
    public:
        virtual bool verify(const ChromaObject& argument) = 0;
};

class TypeVerifier : public ArgumentVerifier {
    private:
        ChromaType type;
    public:
        TypeVerifier(ChromaType type) : type(type) { }
        bool verify(const ChromaObject& argument) {
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
            return this->name + "(" + types[this->type] + ") - " + this->description; 
        }
};

class ChromaCommand {
    protected:
        ChromaCommand() { }
        std::string call_name;
        std::string description;
        std::vector<CommandArgument> arguments;
    public:
        ChromaCommand(const std::string& call_name, const std::string& description, const std::vector<CommandArgument>& args) :
            call_name(call_name), description(description), arguments(args) { }
        std::string get_format() const;
        std::string get_help() const;
};

class CommandBuilder : public ChromaCommand {
    public:
        CommandBuilder() { }
        CommandBuilder& new_command(const std::string& call_name);
        CommandBuilder& add_argument(const std::string& name, const ChromaType type, const std::string& descr);
        CommandBuilder& add_argument(const std::string& name, const ChromaType type, const std::shared_ptr<ArgumentVerifier> verifier, const std::string& descr);
        CommandBuilder& add_optional_argument(const std::string& name, const ChromaType type);
        CommandBuilder& set_description(const std::string& descr);
};

#endif