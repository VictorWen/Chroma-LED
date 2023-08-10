#ifndef CHROMA_CLI_H
#define CHROMA_CLI_H

#include <deque>

#include "chroma.h"
#include "chroma_script.h"

#define PARSE_GOOD 0
#define PARSE_BAD 1
#define PARSE_CONTINUE 2

class ChromaCLI {
    private:
        std::deque<ParseToken> tokens;
        std::stack<char> brackets;
        Tokenizer tokenizer;
    public:
        int process_input(const std::string& input, ChromaEnvironment& cenv);
};

#endif