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


class SlideEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
    public:
        SlideEffect(const std::vector<ChromaData>& args) : ChromaEffect("slide") {
            this->effect = args[0].get_effect();
            std::cerr << this->effect->get_typename() << std::endl;
            this->time = args[1].get_float();
            this->start = -1;
        }
        void tick(const ChromaState& state) {
            if (start < 0) this->start = state.time;
            this->effect->tick(state);
        }
        vec4 draw(float index, const ChromaState& state) const {
            float offset = fmod(state.get_time_diff(start) / this->time, 1);
            index = fmod(1 + index - offset, 1);
            return this->effect->draw(index, state);
        }
};

class RainbowEffect : public ChromaEffect {
    public:
        RainbowEffect(const std::vector<ChromaData>& args) : ChromaEffect("rainbow") {}
        vec4 draw(float index, const ChromaState& state) const {
            float i = index * 3;
            vec4 pixel = vec4(std::fmod(i + 1, 3), i, std::fmod(i - 1, 3), 1);
            return max(-abs(pixel - 1) + 1, 0);
        }
};


void callback(const std::vector<vec4>& pixels) {
    const int n = 150;
    vec4 buffer[n] = {0};
    std::copy(pixels.begin(), pixels.end(), buffer);
    fwrite(buffer, 16, n, stdout);
}

void process_input(const std::string& input, ChromaEnvironment& cenv, const bool verbose=false) {
    std::deque<ParseToken> tokens;
    Tokenizer tokenizer;

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
            if (ret_val.get_type() != Null)
                std::cerr << ret_val.get_type() << " " << ret_val.get_obj()->get_typename() << std::endl;
        }
    } catch (ChromaRuntimeException& e) {
        fprintf(stderr, "Error: %s\n", e.what()); //TODO: something about free(): invalid pointer??
    }
}


int main() {
    // for (std::string line; std::getline(std::cin, line);) {
    //     std::cout << line << std::endl;
    // }

    ChromaEnvironment cenv;

    auto slide_ptr = std::make_unique<CommandBuilder<SlideEffect>>("slide");
    auto& slide_builder = *slide_ptr;
    slide_builder
        .add_argument("EFFECT", Object, "effect to slide around the strip")
        .add_argument("TIME", Number, "time to finish a loop in seconds")
        .set_description("Slides an effect with looping");

    fprintf(stderr, "%s\n", slide_builder.get_help().c_str());

    cenv.functions["slide"] = move(slide_ptr);

    auto rainbow_ptr = std::make_unique<CommandBuilder<RainbowEffect>>("rainbow");
    auto rainbow_builder = *rainbow_ptr;
    rainbow_builder.set_description("Creates a rainbow along the entire strip");

    fprintf(stderr, "%s\n", rainbow_builder.get_help().c_str());

    cenv.functions["rainbow"] = move(rainbow_ptr);

    process_input("let r = rainbow", cenv, true);
    process_input("func slide10 x = slide x 10", cenv, true);
    process_input("slide10 r", cenv, true);

    try {
    //     Evaluator ev(cenv);
    //     ev.visit(*command);
    //     const ChromaData& ret_val = ev.get_env().ret_val;
    //     std::cerr << ret_val.get_type() << " " << ret_val.get_obj()->get_typename() << std::endl;

        ChromaController controller;
        controller.set_effect(cenv.ret_val.get_effect());
        controller.run(60, 150, callback);
    } catch (ChromaRuntimeException& e) {
        fprintf(stderr, "Error: %s\n", e.what()); //TODO: something about free(): invalid pointer??
    }

    return 0;
}
