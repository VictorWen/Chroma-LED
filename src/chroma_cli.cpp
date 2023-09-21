#include "chroma_cli.hpp"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <thread>
#include <iostream>
#include <memory>
#include <fstream>
#include <ostream>

#include "evaluator.hpp"

void ChromaCLI::handle_stdin_input() {
    int result = -1;
    fprintf(stderr, "Input Command: ");
    for (std::string line; this->cenv.controller->is_running() && std::getline(std::cin, line);) {
        result = this->process_input(line, std::cerr);
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

int ChromaCLI::process_text(const std::string& input, std::ostream& output) {
    if (!this->cenv.controller->is_running())
        return -1;

    int result = -1;
    size_t left_index = 0;
    size_t right_index = input.find('\n', left_index);
    if (right_index >= input.size())
        right_index = input.size();

    while (left_index < right_index) {
        std::string line = input.substr(left_index, right_index - left_index);

        result = this->process_input(line, output);
        if (result == PARSE_GOOD && this->cenv.ret_val.get_type() == OBJECT_TYPE) {
            try { // TODO: need a better way to check effect type       
                this->cenv.controller->set_effect(this->cenv.ret_val.get_effect());
            } catch (ChromaRuntimeException& e) {
                output << "Error: " << e.what() << "from " << line << "\n";
                return -1;
            }
        }
        else if (result == PARSE_BAD)
            return -1;

        left_index = right_index + 1;
        right_index = input.find('\n', left_index);
        if (right_index >= input.size())
            right_index = input.size();
    }
    return result;
}

int ChromaCLI::process_input(const std::string& input, std::ostream& output) {
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
        output << "Error: " << e.what() << "\n";
        return PARSE_BAD;
    }

    try {
        Evaluator ev(cenv);
        ev.visit(*command);
        // const ChromaData& ret_val = ev.get_env().ret_val;
        return PARSE_GOOD;
    } catch (ChromaRuntimeException& e) {
        cenv.ret_val = ChromaData();
        output << "Error: " << e.what() << "\n";
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

    // this->register_command(LambdaAdapter("help", "Get the help page for a command", std::vector<std::shared_ptr<CommandArgument>>({
    //     std::make_shared<TypeArgument>("COMMAND", STRING_TYPE, "name of the command to get help")
    // }),
    //     [&](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
    //         std::string cmd_name = args[0].get_string();
    //         if (this->commands.count(cmd_name) > 0)
    //             fprintf("=== HELP ===\n%s\n===========\n", this->commands[cmd_name].get_help().c_str());
    //         else
    //             fprintf("ERROR: command %s does not exists", cmd_name.c_str());
    //         return ChromaData();
    //     }
    // ));

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
        result = this->process_input(line, std::cerr);
        if (result == PARSE_GOOD && this->cenv.ret_val.get_type() == OBJECT_TYPE) {
            try { // TODO: need a better way to check effect type       
                this->cenv.controller->set_effect(this->cenv.ret_val.get_effect());
            } catch (ChromaRuntimeException& e) {
                fprintf(stderr, "Error: %s from %s\n", e.what(), line.c_str());
            }
        }
    }
}