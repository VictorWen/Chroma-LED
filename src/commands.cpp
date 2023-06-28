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

