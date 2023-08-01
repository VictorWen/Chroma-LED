#include "commands.h"

bool TypeArgument::consume(const ChromaData &arg)
{
    if (this->done) 
        return false;
    
    if (arg.get_type() == this->get_type()) {
        this->stored = arg;
        done = true;
    }
    
    return this->done;
}

bool TypeArgument::is_satisfied() const
{
    return this->done;
}

ChromaData TypeArgument::convert() const
{
    return this->stored;
}

bool ExpandingTypeArgument::consume(const ChromaData &arg)
{
    if (arg.get_type() == this->get_type()) {
        this->stored.push_back(arg);
        return true;
    }
    return false;
}

bool ExpandingTypeArgument::is_satisfied() const
{
    if (this->min_num == 0)
        return this->stored.size() > 0;
    else 
        return this->stored.size() >= this->min_num;
}

ChromaData ExpandingTypeArgument::convert() const
{
    return ChromaData(this->stored);
}

std::string LambdaAdapter::get_format() const {
    std::string output = this->call_name;
    for (const auto& arg : this->arguments) {
        output += " " + arg->get_name();
    }
    return output;
}

std::string LambdaAdapter::get_help() const {
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
ChromaData LambdaAdapter::call(const std::vector<ChromaData>& args, ChromaEnvironment& env) { 
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

    return this->lambda(converted, env);
}