#include <iostream>
#include <string>
#include <unistd.h>
#include <deque>
#include <memory>
#include <math.h>

#include "chroma.h"
#include "chromatic.h"
#include "chroma_script.h"
#include "commands.h"
#include "evaluator.h"
#include "effects.h"

const auto RGB_CMD = CommandBuilder<ColorEffect>("rgb")
    .add_argument("R", NUMBER_TYPE, "red value 0-255")
    .add_argument("G", NUMBER_TYPE, "green value 0-255")
    .add_argument("B", NUMBER_TYPE, "blue value 0-255")
    .set_description("Displays a color with the given RGB values");
const auto SPLIT_CMD = CommandBuilder<SplitEffect>("split")
    .add_argument("EFFECT1", OBJECT_TYPE, "effect to split first")
    .add_argument("EFFECT2", OBJECT_TYPE, "effect to split second")
    .set_description("Splits the strip in half with two effects on each");
const auto GRADIENT_CMD = CommandBuilder<GradientEffect>("gradient")
    .add_argument("EFFECT1", OBJECT_TYPE, "effect to start with")
    .add_argument("EFFECT2", OBJECT_TYPE, "effect to gradually transition to")
    .set_description("Gradually transition from one effect to the other across the strip");
const auto RAINBOW_CMD = CommandBuilder<RainbowEffect>("rainbow")
    .set_description("Creates a rainbow along the entire strip");
const auto SLIDE_CMD = CommandBuilder<SlideEffect>("slide")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to slide around the strip") //TODO: have an effect verifier
    .add_argument("TIME", NUMBER_TYPE, "time to finish a loop in seconds")
    .set_description("Slides an effect with looping");


void callback(const std::vector<vec4>& pixels) {
    const int n = 150;
    vec4 buffer[n] = {0};
    std::copy(pixels.begin(), pixels.end(), buffer);
    fwrite(buffer, 16, n, stdout);
}

void process_input(const std::string& input, ChromaEnvironment& cenv, const bool verbose=false) {
    std::deque<ParseToken> tokens;
    Tokenizer tokenizer;

    if (verbose) 
        fprintf(stderr, "Starting tokenization\n");

    tokenizer.tokenize(input, tokens);

    if (verbose) {
        std::cerr << tokens.size() << std::endl;
        for (auto& token : tokens) {
            std::cerr << token.val << " ";
        }
        std::cerr << std::endl;
    }
    
    std::unique_ptr<Command> command(new Command());
    ParseEnvironment env;

    try {
        command->parse(tokens, env);
        if (!tokens.empty()) {
            std::string error = "Got too many tokens, unable to parse '" + tokens.front().val + "'";
            throw ParseException(error.c_str());
        }
    } catch (ParseException& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    if (verbose) {
        fprintf(stderr, "Done Parsing\n");

        PrintVisitor visitor;
        visitor.visit(*command);
        visitor.print(stderr);
    }

    try {
        Evaluator ev(cenv);
        ev.visit(*command);
        const ChromaData& ret_val = ev.get_env().ret_val;

        if (verbose) {
            fprintf(stderr, "Done Evaluating\n");
            if (ret_val.get_type() != NULL_TYPE)
                std::cerr << ret_val.get_type() /*<< " " << ret_val.get_obj()->get_typename()*/ << std::endl;
        }
    } catch (ChromaRuntimeException& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }
}


int main() {
    // for (std::string line; std::getline(std::cin, line);) {
    //     std::cout << line << std::endl;
    // }

    ChromaEnvironment cenv;

    fprintf(stderr, "%s\n", RGB_CMD.get_help().c_str());
    cenv.functions["rgb"] = std::make_unique<CommandBuilder<ColorEffect>>(RGB_CMD);

    fprintf(stderr, "%s\n", SPLIT_CMD.get_help().c_str());
    cenv.functions["split"] = std::make_unique<CommandBuilder<SplitEffect>>(SPLIT_CMD);

    fprintf(stderr, "%s\n", GRADIENT_CMD.get_help().c_str());
    cenv.functions["gradient"] = std::make_unique<CommandBuilder<GradientEffect>>(GRADIENT_CMD);

    fprintf(stderr, "%s\n", RAINBOW_CMD.get_help().c_str());
    cenv.functions["rainbow"] = std::make_unique<CommandBuilder<RainbowEffect>>(RAINBOW_CMD);

    fprintf(stderr, "%s\n", SLIDE_CMD.get_help().c_str());
    cenv.functions["slide"] = std::make_unique<CommandBuilder<SlideEffect>>(SLIDE_CMD);

    process_input("let r = rainbow", cenv, true);
    process_input("let ten = 10", cenv, true);
    process_input("func slide10 x = slide x ten", cenv, true);
    process_input("let RED = rgb 255 0 0", cenv, true);
    process_input("let BLUE = rgb 0 0 255", cenv, true);
    process_input("split RED (gradient RED BLUE)", cenv, true);

    try {
        ChromaController controller;
        controller.set_effect(cenv.ret_val.get_effect());
        controller.run(60, 150, callback);
    } catch (ChromaRuntimeException& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    return 0;
}
