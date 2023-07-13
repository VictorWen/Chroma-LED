#include "chroma.h"


double get_now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

void ChromaController::run(int fps, size_t pixel_length, void callback (const std::vector<vec4>&)) {
    this->running = true;
    double us_per_frame = 1e6 / fps;

    ChromaState state;
    state.pixel_length = pixel_length;
    state.time = 0;

    std::vector<vec4> pixels(pixel_length);

    double start_time = get_now();
    
    while (this->running) {
        double curr_time = get_now();
        double delta_time = curr_time - start_time;
        start_time = curr_time;
        
        state.delta_time = delta_time / 1e6;
        state.time += delta_time / 1e6;

        this->_curr_effect->tick(state);
        // TODO: add multithreading
        for (size_t i = 0; i < pixel_length; i++) {
            float index = static_cast<float>(i) / pixel_length;
            pixels[i] = this->_curr_effect->draw(index, state);
        }
        callback(pixels);

        usleep(us_per_frame);
    }
}

void ChromaController::run(int fps, size_t pixel_length, DiscoController& disco) {
        this->running = true;
    double us_per_frame = 1e6 / fps;

    ChromaState state;
    state.pixel_length = pixel_length;
    state.time = 0;

    std::vector<vec4> pixels(pixel_length);

    double start_time = get_now();
    
    while (this->running) {
        double curr_time = get_now();
        double delta_time = curr_time - start_time;
        start_time = curr_time;
        
        state.delta_time = delta_time / 1e6;
        state.time += delta_time / 1e6;

        this->_curr_effect->tick(state);
        // TODO: add multithreading
        for (size_t i = 0; i < pixel_length; i++) {
            float index = static_cast<float>(i) / pixel_length;
            pixels[i] = this->_curr_effect->draw(index, state);
        }
        if (disco.write(pixels)) {
            return;
        }

        usleep(us_per_frame);
    }
}