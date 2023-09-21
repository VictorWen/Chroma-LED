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

ChromaCLI::ChromaCLI(ChromaEnvironment& cenv, DiscoMaster& disco) : cenv(cenv), disco(disco) {
    this->register_command(LambdaAdapter("read", "Read in a ChromaScript file", std::vector<std::shared_ptr<CommandArgument>>({
        std::make_shared<TypeArgument>("FILENAME", STRING_TYPE, "File name of the ChromaScript file")
    }),
        [&](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
            this->read_script_file(args[0].get_string());
            return ChromaData();
        }
    ));

    this->register_command(LambdaAdapter("disco-scan", "Scan for disco devices on the local network", std::vector<std::shared_ptr<CommandArgument>>({
        std::make_shared<TypeArgument>("TIMEOUT", NUMBER_TYPE, "Max time alloted to complete scan in seconds", true)
    }),
        [&](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
            mDNSDiscoverer discoverer(this->disco);
            double timeout = 2;
            if (args.size() > 0)
                timeout = args[0].get_float();
            discoverer.run(timeout);
            return ChromaData();
        }
    ));

    this->register_command(LambdaAdapter("disco-list", "List all disco devices found on the local network", std::vector<std::shared_ptr<CommandArgument>>(),
        [&](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
            for (const std::string& name : this->disco.get_connection_names()) {
                const DiscoConnection& connection = this->disco.get_connection(name);
                const DiscoConfig& config = this->disco.get_manager().get_config(name);

                fprintf(stderr, "- %s - address: %s, status: %s\n", name.c_str(), config.address.c_str(), conn_to_string(connection.get_status()).c_str());
            }
            return ChromaData();
        }
    ));
}

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
        return PARSE_GOOD;
    } catch (ChromaRuntimeException& e) {
        cenv.ret_val = ChromaData();
        output << "Error: " << e.what() << "\n";
        return PARSE_BAD;
    }
}

void ChromaCLI::run_stdin() {
    fprintf(stderr, "Starting input thread\n");
    std::thread thread([&](){this->handle_stdin_input();});
    fprintf(stderr, "Starting controller\n");
    cenv.controller->run(60, 150, this->disco);
    thread.join();
}

void ChromaCLI::read_script_file(const std::string& filename) {
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