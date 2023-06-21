#include <iostream>
#include <string>
#include <unistd.h>
#include <deque>
#include <memory>

#include "chromatic.h"
#include "chroma_script.h"
#include "commands.h"


int main() {
    // for (std::string line; std::getline(std::cin, line);) {
    //     std::cout << line << std::endl;
    // }

    std::deque<ParseToken> tokens;
    tokens.push_back(ParseToken("func", LITERAL));
    tokens.push_back(ParseToken("a", IDENTIFIER));
    tokens.push_back(ParseToken("x", IDENTIFIER));
    tokens.push_back(ParseToken("y", IDENTIFIER));
    tokens.push_back(ParseToken("=", LITERAL));
    tokens.push_back(ParseToken("add", IDENTIFIER));
    tokens.push_back(ParseToken("x", IDENTIFIER));
    tokens.push_back(ParseToken("(", LITERAL));
    tokens.push_back(ParseToken("sub", IDENTIFIER));
    tokens.push_back(ParseToken("y", IDENTIFIER));
    tokens.push_back(ParseToken("x", IDENTIFIER));
    tokens.push_back(ParseToken(")", LITERAL));

    // tokens.push_back(ParseToken("[", LITERAL));
    // tokens.push_back(ParseToken("a", IDENTIFIER));
    // tokens.push_back(ParseToken("x", IDENTIFIER));
    // tokens.push_back(ParseToken("y", IDENTIFIER));
    // tokens.push_back(ParseToken("]", LITERAL));

    std::unique_ptr<Command> command(new Command());
    ParseEnvironment env;
    env.func_names.insert("add");
    env.func_names.insert("sub");
    
    try {
        command->parse(tokens, env);
    } catch (ParseException& e) {
        printf("Error: %s\n", e.what());
    }

    printf("Done Parsing\n");

    PrintVisitor visitor;
    visitor.visit(*command);
    visitor.print();

    CommandBuilder builder;
    builder.new_command("slide")
        .add_argument("EFFECT", Effect, "effect to slide")
        .add_argument("TIME", Number, "time to finish a loop in seconds")
        .set_description("Slides an effect with looping");
    ChromaCommand cmd = builder;

    printf("%s\n", builder.get_help().c_str());

    // Shader *ex = new ExampleShader();

    // int n = 150;
    // float time = 0;
    
    // vec4 pixels[150] = {0};
    // while (true) {
    //     ex->tick(time);
        
    //     for (int i = 0; i < n; i++) {
    //         ex->draw(time, ((float) i)/n, pixels[i]);
    //     }
    //     fwrite(pixels, 16, 150, stdout);

    //     usleep(10000);
    //     time += 0.01;
    // }

    // delete ex;
    return 0;
}