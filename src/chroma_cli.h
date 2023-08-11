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
        ChromaEnvironment& cenv;
        void handle_stdin_input();
    public:
        ChromaCLI(ChromaEnvironment& cenv) : cenv(cenv) { }
        int process_input(const std::string& input);
        template<typename T> void register_command(const T& cmd);
        void run_stdin(DiscoMaster& disco);
        void read_scriptfile(const std::string& filename = "scripts/startup.chroma");
};

template<typename T>
void ChromaCLI::register_command(const T& cmd) {
    fprintf(stderr, "%s\n", cmd.get_help().c_str());
    this->cenv.functions[cmd.get_name()] = std::make_unique<T>(cmd);
}

#endif