#include "chroma_cli.h"
#include "evaluator.h"

int ChromaCLI::process_input(const std::string& input, ChromaEnvironment& cenv) {
    this->tokenizer.tokenize(input, this->tokens, this->brackets);

    if (!this->brackets.empty())
        return PARSE_CONTINUE;
    
    std::unique_ptr<Command> command(new Command());
    ParseEnvironment env; // TODO: remove ParseEnvironment

    try {
        command->parse(tokens, env);
        if (!tokens.empty()) {
            std::string error = "Got too many tokens, unable to parse '" + tokens.front().val + "'";
            throw ParseException(error.c_str());
        }
    } catch (ParseException& e) {
        cenv.ret_val = ChromaData();
        fprintf(stderr, "Error: %s\n", e.what());
        return PARSE_BAD;
    }

    try {
        Evaluator ev(cenv);
        ev.visit(*command);
        // const ChromaData& ret_val = ev.get_env().ret_val;
        return PARSE_GOOD;
    } catch (ChromaRuntimeException& e) {
        cenv.ret_val = ChromaData();
        fprintf(stderr, "Error: %s\n", e.what());
        return PARSE_BAD;
    }
}