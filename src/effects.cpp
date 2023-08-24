#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <math.h>

#include "effects.hpp"


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
    for (auto& data : args[0].get_list()) {
        this->effects.push_back(data.get_effect());
    }
}

vec4 SplitEffect::draw(float index, const ChromaState& state) const {
    if (index != 1)
        index = index * this->effects.size();
    else
        index = this->effects.size() - 1;
    int i = floor(index);
    return this->effects[i]->draw(fmod(index, 1), state);
}

GradientEffect::GradientEffect(const std::vector<ChromaData>& args) {
    for (auto& data : args[0].get_list()) {
        this->effects.push_back(data.get_effect());
    }
}

vec4 GradientEffect::draw(float index, const ChromaState& state) const {
    if (this->effects.size() == 1)
        return this->effects[0]->draw(index, state); // TODO: add verifier for at least two args?
    
    if (index == 1)
        return this->effects[this->effects.size() - 1]->draw(1, state);

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

WipeEffect::WipeEffect(const std::vector<ChromaData>& args) : ChromaEffect("slide") {
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void WipeEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);
}

vec4 WipeEffect::draw(float index, const ChromaState& state) const {
    float offset = fmod(state.get_time_diff(start) / this->time, 1);
    return this->effect->draw(offset, state);
}

BlinkEffect::BlinkEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void BlinkEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);

    int i = floor(state.get_time_diff(this->start) / this->time);
    this->on = i % 2 == 0;
}

vec4 BlinkEffect::draw(float index, const ChromaState& state) const {
    vec4 color = this->effect->draw(index, state);
    if (this->on)
        return color;
    else
        return vec4(0, 0, 0, 0); // TODO: decide to use premultiplied or straight alpha
}

BlinkFadeEffect::BlinkFadeEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void BlinkFadeEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);

    float t = fmod(state.get_time_diff(this->start) / this->time, 2);
    this->transition = 1.0 - abs(t - 1.0);
}

vec4 BlinkFadeEffect::draw(float index, const ChromaState& state) const {
    vec4 color = this->effect->draw(index, state);
    return color * this->transition;
}

WormEffect::WormEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void WormEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);

    this->cutoff = state.get_time_diff(this->start) / this->time;
}

vec4 WormEffect::draw(float index, const ChromaState& state) const {
    if (index <= this->cutoff)
        return this->effect->draw(index, state);
    else
        return vec4(0, 0, 0, 0);
}

FadeInEffect::FadeInEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void FadeInEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);

    float t = state.get_time_diff(this->start) / this->time;
    if (t > 1)
        t = 1;
    this->transition = t;
}

vec4 FadeInEffect::draw(float index, const ChromaState& state) const {
    vec4 color = this->effect->draw(index, state);
    return color * this->transition;
}

FadeOutEffect::FadeOutEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->time = args[1].get_float();
    this->start = -1;
}

void FadeOutEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);

    float t = 1.0 - state.get_time_diff(this->start) / this->time;
    if (t < 0)
        t = 0;
    this->transition = t;
}

vec4 FadeOutEffect::draw(float index, const ChromaState& state) const {
    vec4 color = this->effect->draw(index, state);
    return color * this->transition;
}

WaveEffect::WaveEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->period = args[1].get_float();
    this->wavelength = args[2].get_float();

    this->start = -1;
}

void WaveEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);
}

vec4 WaveEffect::draw(float index, const ChromaState& state) const {
    float t = state.get_time_diff(this->start);
    float phase = (index * state.pixel_length / this->wavelength - t / this->period) * 2 * M_PI;
    float val = (1 + sin(phase)) / 2;
    return this->effect->draw(val, state);
}

WheelEffect::WheelEffect(const std::vector<ChromaData> &args)
{
    this->effect = args[0].get_effect();
    this->period = args[1].get_float();

    this->start = -1;
}

void WheelEffect::tick(const ChromaState& state) {
    if (start < 0) this->start = state.time;
    this->effect->tick(state);
}

vec4 WheelEffect::draw(float index, const ChromaState& state) const {
    float t = state.get_time_diff(this->start);
    float phase = (t / this->period) * 2 * M_PI;
    float val = (1 + sin(phase)) / 2;
    return this->effect->draw(val, state);
}
