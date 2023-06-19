#include <iostream>
#include <string>
#include <unistd.h>
#include "chromatic.h"

void write_num(char* out, uint32_t in) {
    out[0] = (in >> 24) && 0xFF;
    out[1] = (in >> 16) && 0xFF;
    out[2] = (in >> 8) && 0xFF;
    out[3] = (in >> 0) && 0xFF;
}

void write_vec4(char* out, const vec4& v) {
    write_num(out + 0, v.x);
    write_num(out + 4, v.y);
    write_num(out + 8, v.z);
    write_num(out + 12, v.w);
}

int main() {
    // for (std::string line; std::getline(std::cin, line);) {
    //     std::cout << line << std::endl;
    // }
    
    Shader *ex = new ExampleShader();

    int n = 150;
    float time = 0;
    
    vec4 pixels[150] = {0};
    while (true) {
        ex->tick(time);
        
        for (int i = 0; i < n; i++) {
            ex->draw(time, ((float) i)/n, pixels[i]);
        }
        fwrite(pixels, 16, 150, stdout);

        usleep(10000);
        time += 0.01;
    }

    delete ex;
    return 0;
}