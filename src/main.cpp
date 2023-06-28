#include <iostream>
#include <string>
#include <unistd.h>
#include <deque>
#include <memory>
#include <math.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

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
    .add_expanding_argument("EFFECTS...", OBJECT_TYPE, "two or more effects to split between", 2)
    .set_description("Splits the strip in half with two effects on each");
const auto GRADIENT_CMD = CommandBuilder<GradientEffect>("gradient")
    .add_expanding_argument("EFFECTS...", OBJECT_TYPE, "two or more effects to gradually transition between", 2)
    .set_description("Gradually transition from one effect to the other across the strip");
const auto RAINBOW_CMD = CommandBuilder<RainbowEffect>("rainbow")
    .set_description("Creates a rainbow along the entire strip");
const auto SLIDE_CMD = CommandBuilder<SlideEffect>("slide")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to slide around the strip") //TODO: have an effect verifier
    .add_argument("TIME", NUMBER_TYPE, "time to finish a loop in seconds")
    .set_description("Slides an effect with looping");
const auto WIPE_CMD = CommandBuilder<WipeEffect>("wipe")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to wipe around the strip")
    .add_argument("TIME", NUMBER_TYPE, "time to finish a wipe in seconds")
    .set_description("Wipes an effect with looping");
const auto BLINK_CMD = CommandBuilder<BlinkEffect>("blink")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to blink")
    .add_argument("TIME", NUMBER_TYPE, "time between blinking on and off")
    .set_description("Blinks an effect on and off");
const auto BLINKFADE_CMD = CommandBuilder<BlinkFadeEffect>("blinkfade")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to fade")
    .add_argument("TIME", NUMBER_TYPE, "time between fading on and off")
    .set_description("Fades an effect on and off");
const auto FADEIN_CMD = CommandBuilder<FadeInEffect>("fadein")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to fade")
    .add_argument("TIME", NUMBER_TYPE, "time from off to on")
    .set_description("Fades an effect from off to on");
const auto FADEOUT_CMD = CommandBuilder<FadeOutEffect>("fadeout")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to fade")
    .add_argument("TIME", NUMBER_TYPE, "time from on to off")
    .set_description("Fades an effect from on to off");
const auto WAVE_CMD = CommandBuilder<WaveEffect>("wave")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to wave")
    .add_argument("PERIOD", NUMBER_TYPE, "period of the wave")
    .add_argument("WAVELENGTH", NUMBER_TYPE, "wavelength (in pixels) of the wave")
    .set_description("Moves the effect across the strip with a wave distribution");
const auto WHEEL_CMD = CommandBuilder<WheelEffect>("wheel")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to spin")
    .add_argument("PERIOD", NUMBER_TYPE, "period of the spin")
    .set_description("Spins the effect across the strip like a wheel (wave-like version of wipe)");

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
        cenv.ret_val = ChromaData();
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

    fprintf(stderr, "%s\n", WIPE_CMD.get_help().c_str());
    cenv.functions["wipe"] = std::make_unique<CommandBuilder<WipeEffect>>(WIPE_CMD);

    fprintf(stderr, "%s\n", BLINK_CMD.get_help().c_str());
    cenv.functions["blink"] = std::make_unique<CommandBuilder<BlinkEffect>>(BLINK_CMD);

    fprintf(stderr, "%s\n", BLINKFADE_CMD.get_help().c_str());
    cenv.functions["blinkfade"] = std::make_unique<CommandBuilder<BlinkFadeEffect>>(BLINKFADE_CMD);

    fprintf(stderr, "%s\n", FADEIN_CMD.get_help().c_str());
    cenv.functions["fadein"] = std::make_unique<CommandBuilder<FadeInEffect>>(FADEIN_CMD);

    fprintf(stderr, "%s\n", FADEOUT_CMD.get_help().c_str());
    cenv.functions["fadeout"] = std::make_unique<CommandBuilder<FadeOutEffect>>(FADEOUT_CMD);

    fprintf(stderr, "%s\n", WAVE_CMD.get_help().c_str());
    cenv.functions["wave"] = std::make_unique<CommandBuilder<WaveEffect>>(WAVE_CMD);

    fprintf(stderr, "%s\n", WHEEL_CMD.get_help().c_str());
    cenv.functions["wheel"] = std::make_unique<CommandBuilder<WheelEffect>>(WHEEL_CMD);
    

    // process_input("let r = rainbow", cenv, true);
    // process_input("let ten = 10", cenv, true);
    // process_input("func slide10 x = slide x ten", cenv, true);
    process_input("let RED = rgb 255 0 0", cenv, true);
    process_input("let GREEN = rgb 0 255 0", cenv, true);
    process_input("let BLUE = rgb 0 0 255", cenv, true);
    // process_input("let p = split RED (gradient RED BLUE)", cenv, true);
    // process_input("gradient (slide10 p) (slide10 r)", cenv, true);


    process_input("let r = gradient RED GREEN BLUE", cenv, true);
    process_input("let a = wipe r 10", cenv, true);
    process_input("let b = blink r 10", cenv, true);
    process_input("let c = blinkfade r 10", cenv, true);
    process_input("let d = fadein r 10", cenv, true);
    process_input("let e = fadeout r 10", cenv, true);
    process_input("let f = wave r 10 10", cenv, true);
    process_input("let g = wheel r 10", cenv, true);

    process_input("split a b c d e f g", cenv, true);
    // process_input("g", cenv, true);

    fprintf(stderr, "Done processing input\n");

    try {
        if (cenv.ret_val.get_type() != OBJECT_TYPE)
            throw ChromaRuntimeException("Invalid effect type");
        ChromaController controller;
        controller.set_effect(cenv.ret_val.get_effect());
#ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY );
#endif
        controller.run(60, 150, callback);
    } catch (ChromaRuntimeException& e) {
        fprintf(stderr, "Error: %s\n", e.what());
    }

    return 0;
}
