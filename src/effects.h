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

class AlphaEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float alpha;
    public:
        AlphaEffect(const std::vector<ChromaData>& args) : effect(args[0].get_effect()), alpha(args[1].get_float()) {}
        void tick(const ChromaState& state) { this->effect->tick(state); }
        vec4 draw(float index, const ChromaState& state) const { return this->effect->draw(index, state) * alpha; }
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

class WipeEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
    public:
        WipeEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};


class BlinkEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
        bool on;
    public:
        BlinkEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class BlinkFadeEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
        float transition;
    public:
        BlinkFadeEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class WormEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
        float cutoff;
    public:
        WormEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class FadeInEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
        float transition;
    public:
        FadeInEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class FadeOutEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float time;
        float start;
        float transition;
    public:
        FadeOutEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class WaveEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float period;
        float wavelength;
        float start;
    public:
        WaveEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class WheelEffect : public ChromaEffect {
    private:
        std::shared_ptr<ChromaEffect> effect;
        float period;
        float start;
    public:
        WheelEffect(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

#endif