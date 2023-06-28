#ifndef EFFECTS_H
#define EFFECTS_H

#include "chroma.h"


class ColorEffect : public ChromaEffect {
    private:
        vec4 color;
    public:
        ColorEffect(const std::vector<ChromaData>& args);
        vec4 draw(float index, const ChromaState& state) const { return color; }
};


class RainbowEffect : public ChromaEffect {
    public:
        RainbowEffect(const std::vector<ChromaData>& args);
        vec4 draw(float index, const ChromaState& state) const;
};

class SplitEffect : public ChromaEffect {
    private:
        std::vector<std::shared_ptr<ChromaEffect>> effects;
    public:
        SplitEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state) { for (auto& effect : this->effects) effect->tick(state); }
        vec4 draw(float index, const ChromaState& state) const;
};

class GradientEffect : public ChromaEffect {
    private:
        std::vector<std::shared_ptr<ChromaEffect>> effects;
    public:
        GradientEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state) { for (auto& effect : this->effects) effect->tick(state); }
        vec4 draw(float index, const ChromaState& state) const;
};

class SlideEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
    public:
        SlideEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};


#endif