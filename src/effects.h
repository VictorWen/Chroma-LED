#ifndef EFFECTS_H
#define EFFECTS_H

#include <math.h>

#include "chroma.h"


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

class RainbowEffect : public ChromaEffect {
    public:
        RainbowEffect(const std::vector<ChromaData>& args);
        vec4 draw(float index, const ChromaState& state) const;
};

#endif