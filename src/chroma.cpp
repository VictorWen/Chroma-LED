#include <boost/asio.hpp>

#include "chroma.h"

double get_now() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

void ChromaController::run(int fps, size_t pixel_length, std::function<int(const std::vector<vec4>&)> callback) {
    this->running = true;
    double us_per_frame = 1e6 / fps;

    ChromaState state;
    state.pixel_length = pixel_length;
    state.time = 0;

    std::vector<vec4> pixels(pixel_length);

    double start_time = get_now();
    
    boost::asio::thread_pool pool(4);

    while (this->running) {
        double curr_time = get_now();
        double delta_time = curr_time - start_time;
        start_time = curr_time;
        
        state.delta_time = delta_time / 1e6;
        state.time += delta_time / 1e6;

        for (auto& effect : this->layers) {
            if (effect == nullptr)
                continue;
            effect->tick(state);
        }
        
        std::vector<std::future<bool>> futures;
        for (size_t i = 0; i < pixel_length; i++) {
            // TODO: refactor?
            float index = static_cast<float>(i) / pixel_length;
            std::packaged_task<bool()> task([i, index, this, &state, &pixels](){
                float factor = 1;
                vec4 total_color;
                for (int j = this->layers.size() - 1; j >= 0; j--) {
                    auto& effect = this->layers[j];
                    if (effect == nullptr)
                        continue;
                    vec4 color = effect->draw(index, state); 
                    total_color += color * factor;
                    factor *= 1 - color.w;
                }
                pixels[i] = total_color;
                return true;
            });
            futures.push_back(boost::asio::post(pool, std::move(task)));
        }

        for (auto& future : futures) {
            future.wait();
        }

        if (callback(pixels) != 0)
            return; //TODO: do error handling

        usleep(us_per_frame);
    }
    pool.join();
}

void ChromaController::run(int fps, size_t pixel_length, DiscoMaster& disco) { //TODO: Maybe use a generic injection instead of DiscoMaster?
    this->run(fps, pixel_length, [&](const std::vector<vec4>& pixels){
        if (disco.write(this->component_id, pixels)) {
            return -1;
        }
        return 0;
    });
}