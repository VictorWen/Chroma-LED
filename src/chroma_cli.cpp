#include "chroma_cli.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <thread>
#include <iostream>
#include <memory>
#include <fstream>

#include "evaluator.h"
#include "commands.h"

void ChromaCLI::handle_stdin_input() {
    int result = -1;
    fprintf(stderr, "Input Command: ");
    for (std::string line; this->cenv.controller->is_running() && std::getline(std::cin, line);) {
        result = this->process_input(line);
        if (result == PARSE_GOOD && this->cenv.ret_val.get_type() == OBJECT_TYPE) {
            try { // TODO: need a better way to check effect type       
                this->cenv.controller->set_effect(this->cenv.ret_val.get_effect());
            } catch (ChromaRuntimeException& e) {
                fprintf(stderr, "Error: %s\n", e.what());
            }
        }
        if (result != PARSE_CONTINUE && this->cenv.controller->is_running())
            fprintf(stderr, "Input Command: ");
    }
}

int ChromaCLI::process_input(const std::string& input) {
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

void ChromaCLI::run_stdin(DiscoMaster& disco) {
    // #ifdef _WIN32
    //     _setmode(_fileno(stdout), _O_BINARY);
    // #endif

    this->register_command(LambdaAdapter("read", "Read in a ChromaScript file", std::vector<std::shared_ptr<CommandArgument>>({
        std::make_shared<TypeArgument>("FILENAME", STRING_TYPE, "File name of the ChromaScript file")
    }),
        [&](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
            this->read_scriptfile(args[0].get_string());
            return ChromaData();
        }
    ));

    fprintf(stderr, "Starting input thread\n");
    std::thread thread([&](){this->handle_stdin_input();});
    fprintf(stderr, "Starting controller\n");
    cenv.controller->run(60, 150, disco);
    thread.join();
}

void ChromaCLI::read_scriptfile(const std::string& filename) {
    int result = -1;
    std::ifstream startup_file (filename);
    for (std::string line; std::getline(startup_file, line);) {
        result = this->process_input(line);
        if (result == PARSE_GOOD && this->cenv.ret_val.get_type() == OBJECT_TYPE) {
            try { // TODO: need a better way to check effect type       
                this->cenv.controller->set_effect(this->cenv.ret_val.get_effect());
            } catch (ChromaRuntimeException& e) {
                fprintf(stderr, "Error: %s from %s\n", e.what(), line.c_str());
            }
        }
    }
}