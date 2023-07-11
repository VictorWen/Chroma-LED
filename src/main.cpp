#include <iostream>
#include <string>
#include <unistd.h>
#include <deque>
#include <memory>
#include <math.h>
#include <thread>

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
#include "particles.h"

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
const auto WORM_CMD = CommandBuilder<WormEffect>("worm")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to worm around the strip")
    .add_argument("TIME", NUMBER_TYPE, "time to complete the worm")
    .set_description("Grows a worm from one end of the strip to the other");
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


const auto PBODY_CMD = CommandBuilder<PhysicsBody>("pbody")
    .add_argument("POSITION", NUMBER_TYPE, "position of the body (in pixels)")
    .add_optional_argument("VELOCITY", NUMBER_TYPE, "velocity of the body (in pixels per second)")
    .add_optional_argument("ACCELERATION", NUMBER_TYPE, "acceleration of the body (in pixels per second squared)")
    .add_optional_argument("MASS", NUMBER_TYPE, "mass of the body")
    .set_description("Creates a physics body object to be used with a particle");
const auto PARTICLE_CMD = CommandBuilder<ParticleEffect>("particle")
    .add_argument("EFFECT", OBJECT_TYPE, "colors the particle will display")
    .add_argument("BODY", OBJECT_TYPE, "physics body of the particle")
    .add_argument("RADIUS", NUMBER_TYPE, "radius of the particle body")
    .add_optional_argument("BEHAVIORS", LIST_TYPE, "list of behaviors the particle will have")
    .set_description("Creates a particle object for a physics system");
const auto PSYSTEM_CMD = CommandBuilder<ParticleSystem>("psystem")
    .add_argument("PARTICLE_LIST", LIST_TYPE, "list of particle objects to display")
    .set_description("Displays a set of particles with physics simulation");
const auto EMITTER_CMD = CommandBuilder<EmitterBehavior>("emitter")
    .add_argument("PARTICLE", OBJECT_TYPE, "particle emission")
    .add_argument("DENSITY", NUMBER_TYPE, "number of particles per second")
    .set_description("Emits copies of the given particle at a consistent rate");
const auto LIFE_CMD = CommandBuilder<LifetimeBehavior>("life")
    .add_argument("LIFETIME", NUMBER_TYPE, "time in seconds for the particle to live")
    .set_description("Destroys the particle after a certain amount of time");


void callback(const std::vector<vec4>& pixels) {
    const int n = 150;
    vec4 buffer[n] = {0};
    std::copy(pixels.begin(), pixels.end(), buffer);
    fwrite(buffer, 16, n, stdout);
}

bool process_input(const std::string& input, ChromaEnvironment& cenv, const bool verbose=false) {
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
        cenv.ret_val = ChromaData();
        fprintf(stderr, "Error: %s\n", e.what());
        return false;
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
        return true;
    } catch (ChromaRuntimeException& e) {
        cenv.ret_val = ChromaData();
        fprintf(stderr, "Error: %s\n", e.what());
        return false;
    }
}


void handle_stdin_input(ChromaEnvironment& cenv, ChromaController& controller) {
    fprintf(stderr, "Input Command: ");
    for (std::string line; std::getline(std::cin, line);) {
        std::cerr << line << std::endl;
        if (process_input(line, cenv, false) && cenv.ret_val.get_type() == OBJECT_TYPE) {
            try { // TODO: need a better way to check effect type       
                controller.set_effect(cenv.ret_val.get_effect());
            } catch (ChromaRuntimeException& e) {
                fprintf(stderr, "Error: %s\n", e.what());
            }
        }
        fprintf(stderr, "Input Command: ");
    }
}

void run_stdin(ChromaEnvironment& cenv) {
    #ifdef _WIN32
        _setmode(_fileno(stdout), _O_BINARY);
    #endif
    ChromaController controller;

    std::vector<ChromaData> data;
    controller.set_effect(std::make_shared<RainbowEffect>(data));

    fprintf(stderr, "Starting input thread\n");
    std::thread thread(handle_stdin_input, std::ref(cenv), std::ref(controller));
    fprintf(stderr, "Starting controller\n");
    controller.run(60, 150, callback);
    thread.join();
}


int main() {
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

    fprintf(stderr, "%s\n", WORM_CMD.get_help().c_str());
    cenv.functions["worm"] = std::make_unique<CommandBuilder<WormEffect>>(WORM_CMD);

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

    fprintf(stderr, "%s\n", PBODY_CMD.get_help().c_str());
    cenv.functions["pbody"] = std::make_unique<CommandBuilder<PhysicsBody>>(PBODY_CMD);

    fprintf(stderr, "%s\n", PARTICLE_CMD.get_help().c_str());
    cenv.functions["particle"] = std::make_unique<CommandBuilder<ParticleEffect>>(PARTICLE_CMD);

    fprintf(stderr, "%s\n", PSYSTEM_CMD.get_help().c_str());
    cenv.functions["psystem"] = std::make_unique<CommandBuilder<ParticleSystem>>(PSYSTEM_CMD);

    fprintf(stderr, "%s\n", EMITTER_CMD.get_help().c_str());
    cenv.functions["emitter"] = std::make_unique<CommandBuilder<EmitterBehavior>>(EMITTER_CMD);

    fprintf(stderr, "%s\n", LIFE_CMD.get_help().c_str());
    cenv.functions["life"] = std::make_unique<CommandBuilder<LifetimeBehavior>>(LIFE_CMD);
    

    // process_input("let r = rainbow", cenv, true);
    // process_input("let ten = 10", cenv, true);
    // process_input("func slide10 x = slide x ten", cenv, true);
    process_input("let RED = rgb 255 0 0", cenv, false);
    process_input("let GREEN = rgb 0 255 0", cenv, false);
    process_input("let BLUE = rgb 0 0 255", cenv, false);
    // process_input("let p = split RED (gradient RED BLUE)", cenv, true);
    // process_input("gradient (slide10 p) (slide10 r)", cenv, true);


    // process_input("let r = gradient RED GREEN BLUE", cenv, true);
    // process_input("let a = wipe r 10", cenv, true);
    // process_input("let b = blink r 10", cenv, true);
    // process_input("let c = blinkfade r 10", cenv, true);
    // process_input("let d = fadein r 10", cenv, true);
    // process_input("let e = fadeout r 10", cenv, true);
    // process_input("let f = wave r 10 10", cenv, true);
    // process_input("let g = wheel r 10 10", cenv, true);
    // process_input("let h = worm r 10", cenv, true);

    // process_input("split a b c d e f g", cenv, true);
    // process_input("h", cenv, true);

    process_input("let testParticle = particle RED (pbody 10 5) 2 [(life 10)]", cenv, false);
    process_input("let emitterParticle = particle GREEN (pbody 10) 1 [(emitter testParticle 0.1)]", cenv, false);

    process_input("let particleA = particle RED (pbody 0 10) 2", cenv, false);
    process_input("let particleB = particle BLUE (pbody 150 -10) 2", cenv, false);

    // fprintf(stderr, "Done processing input\n");

    run_stdin(cenv);

    return 0;
}
