#include <iostream>

#include "commands.h"

std::string ChromaCommand::get_format() const
{
    std::string output = this->call_name;
    for (auto arg : this->arguments) {
        output += " " + arg.get_name();
    }
    return output;
}

std::string ChromaCommand::get_help() const
{
    std::string output = this->get_format();
    output += "\n\t" + this->description;
    output += "\n";
    for (auto arg : this->arguments) {
        output += "\n\t" + arg.to_string();
    }
    return output;
}

CommandBuilder &CommandBuilder::new_command(const std::string& call_name)
{
    this->call_name = std::string(call_name);
    return *this;
}

CommandBuilder &CommandBuilder::add_argument(const std::string& name, const ChromaType type, const std::string& descr)
{
    CommandArgument arg(name, type, descr, nullptr);
    this->arguments.push_back(arg);
    return *this;
}

CommandBuilder &CommandBuilder::add_argument(const std::string& name, const ChromaType type, const std::shared_ptr<ArgumentVerifier> verifier, const std::string& descr)
{
    CommandArgument arg(name, type, descr, verifier);
    this->arguments.push_back(arg);
    return *this;
}

CommandBuilder &CommandBuilder::add_optional_argument(const std::string& name, const ChromaType type)
{
    return *this;
}

CommandBuilder &CommandBuilder::set_description(const std::string& descr)
{
    this->description = descr;
    return *this;
}
