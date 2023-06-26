#include "effects.h"


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

RainbowEffect::RainbowEffect(const std::vector<ChromaData>& args) : ChromaEffect("rainbow") { }

vec4 RainbowEffect::draw(float index, const ChromaState& state) const {
    float i = index * 3;
    vec4 pixel = vec4(std::fmod(i + 1, 3), i, std::fmod(i - 1, 3), 1);
    return max(-abs(pixel - 1) + 1, 0);
}