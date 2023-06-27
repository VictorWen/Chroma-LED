#include <math.h>

#include "effects.h"


ColorEffect::ColorEffect(const std::vector<ChromaData>& args) {
    this->color.x = args[0].get_int() / 255.0;
    this->color.y = args[1].get_int() / 255.0;
    this->color.z = args[2].get_int() / 255.0;
    if (args.size() > 3)
        this->color.w = args[3].get_int() / 255.0;
    else
        this->color.w = 1;
}

RainbowEffect::RainbowEffect(const std::vector<ChromaData>& args) : ChromaEffect("rainbow") { }

vec4 RainbowEffect::draw(float index, const ChromaState& state) const {
    float i = index * 3;
    vec4 pixel = vec4(std::fmod(i + 1, 3), i, std::fmod(i - 1, 3), 1);
    return max(-abs(pixel - 1) + 1, 0);
}

SplitEffect::SplitEffect(const std::vector<ChromaData>& args) {
    for (auto& data : args) {
        this->effects.push_back(data.get_effect());
    }
}

vec4 SplitEffect::draw(float index, const ChromaState& state) const {
    index = index * this->effects.size();
    int i = floor(index);
    return this->effects[i]->draw(fmod(index, 1), state);
}

GradientEffect::GradientEffect(const std::vector<ChromaData>& args) {
    for (auto& data : args) {
        this->effects.push_back(data.get_effect());
    }
}

vec4 GradientEffect::draw(float index, const ChromaState& state) const {
    if (this->effects.size() == 1)
        return this->effects[0]->draw(index, state); // TODO: add verifier for at least two args?
    
    index = index * (this->effects.size() - 1);
    int i = floor(index);
    float lerp = index - i;

    vec4 a = this->effects[i]->draw(0, state);
    vec4 b = this->effects[i + 1]->draw(0, state);
    return a * (1 - lerp) + b * lerp; 
}

SlideEffect::SlideEffect(const std::vector<ChromaData>& args) : ChromaEffect("slide") {
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void SlideEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);
}

vec4 SlideEffect::draw(float index, const ChromaState& state) const {
    float offset = fmod(state.get_time_diff(start) / this->time, 1);
    index = fmod(1 + index - offset, 1);
    return this->effect->draw(index, state);
}
