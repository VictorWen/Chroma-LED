#include <iostream>
#include <string>
#include <unistd.h>
#include <deque>
#include <memory>
#include <math.h>
#include <thread>
#include <fstream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "chroma.h"
#include "chromatic.h"
#include "chroma_script.h"
#include "chroma_cli.h"
#include "commands.h"
#include "evaluator.h"
#include "effects.h"
#include "particles.h"
#include "disco.h"

// TODO: potential optimization, use string_view for read-only strings
const auto RGB_CMD = CommandBuilder<ColorEffect>("rgb")
    .add_argument("R", NUMBER_TYPE, "red value 0-255")
    .add_argument("G", NUMBER_TYPE, "green value 0-255")
    .add_argument("B", NUMBER_TYPE, "blue value 0-255")
    .set_description("Displays a color with the given RGB values");
const auto ALPHA_CMD = CommandBuilder<AlphaEffect>("alpha")
    .add_argument("EFFECT", OBJECT_TYPE, "effect to scale")
    .add_argument("ALPHA", NUMBER_TYPE, "value, 0 to 1, to scale effect by")
    .set_description("Scales the colors of an effect by an alpha value");
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


const auto ADD_LAYER_CMD = LambdaAdapter("addlayer", "Add a new layer to the Chroma Controller", std::vector<std::shared_ptr<CommandArgument>>(),
    [](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
        env.controller->add_layer();
        env.controller->set_current_layer(env.controller->get_num_layers() - 1);
        std::cerr << "Added a layer: " << env.controller->get_current_layer() << " of " << env.controller->get_num_layers() << std::endl;
        return ChromaData();
    }
);

const auto SET_LAYER_CMD = LambdaAdapter("setlayer", "Set current layer in the Chroma Controller", std::vector<std::shared_ptr<CommandArgument>>({
        std::make_shared<TypeArgument>("INDEX", NUMBER_TYPE, "index (starting from 0) of the layer")
    }),
    [](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
        env.controller->set_current_layer(args[0].get_int());
        std::cerr << "Set layer: " << env.controller->get_current_layer() << " of " << env.controller->get_num_layers() << std::endl;
        return ChromaData();
    }
);

const auto EXIT_CMD = LambdaAdapter("exit", "Exits the program", std::vector<std::shared_ptr<CommandArgument>>(),
    [](const std::vector<ChromaData>& args, ChromaEnvironment& env) {
        env.controller->stop();
        return ChromaData();
    }
);

void callback(const std::vector<vec4>& pixels) {
    const int n = 150;
    vec4 buffer[n] = {0};
    std::copy(pixels.begin(), pixels.end(), buffer);
    fwrite(buffer, 16, n, stdout);
}

int main() {
    ChromaController controller;
    ChromaEnvironment cenv;
    cenv.controller = &controller;

    ChromaCLI cli(cenv);

    cli.register_command(RGB_CMD);
    cli.register_command(ALPHA_CMD);
    cli.register_command(SPLIT_CMD);
    cli.register_command(GRADIENT_CMD);
    cli.register_command(RAINBOW_CMD);
    cli.register_command(SLIDE_CMD);
    cli.register_command(WIPE_CMD);
    cli.register_command(WORM_CMD);
    cli.register_command(BLINK_CMD);
    cli.register_command(BLINKFADE_CMD);
    cli.register_command(FADEIN_CMD);
    cli.register_command(FADEOUT_CMD);
    cli.register_command(WAVE_CMD);
    cli.register_command(WHEEL_CMD);

    cli.register_command(PBODY_CMD);
    cli.register_command(PARTICLE_CMD);
    cli.register_command(PSYSTEM_CMD);
    cli.register_command(EMITTER_CMD);
    cli.register_command(LIFE_CMD);

    cli.register_command(ADD_LAYER_CMD);
    cli.register_command(SET_LAYER_CMD);
    cli.register_command(EXIT_CMD);

    fprintf(stderr, "Ready to start...\n"); // TODO: do proper logging

    auto httpServer = std::make_unique<HTTPConfigManager>();
    
    // DiscoDiscoverer discoverer(httpServer.get());
    // if (discoverer.send_broadcast() != 0)
    //     return;
    
    httpServer->start();

    fprintf(stderr, "Waiting for Disco config...\n");
    httpServer->wait_for_any_config();

    UDPDisco disco(std::move(httpServer));
    
    cli.read_scriptfile();
    cli.run_stdin(disco);

    return 0;
}
