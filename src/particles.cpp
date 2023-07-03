#include <math.h>

#include "particles.h"


ParticleEffect::ParticleEffect(const std::vector<ChromaData> &args) : ChromaObject("Particle"), body(10., 0., 0., 1.)
{
    this->effect = args[0].get_effect();
    this->radius = args[1].get_float();
}

void ParticleEffect::tick(ParticleSystem &system, const ChromaState &state)
{

}

vec4 ParticleEffect::draw(float index, const ChromaState &state) const
{
    return this->effect->draw(index, state);
}

ParticleSystem::ParticleSystem(const std::vector<ChromaData> &args)
{
    std::vector<ChromaData> list = args[0].get_list();
    for (auto& data : list) { //TODO: insert type check (somewhere)
        this->particles.insert(std::dynamic_pointer_cast<ParticleEffect>(data.get_obj()));
    }
}

void ParticleSystem::tick(const ChromaState &state)
{
    this->screen = std::vector<vec4>(state.pixel_length);

    for (auto& particle : this->particles) {
        particle->tick(*this, state);
    }

    for (auto& particle : this->particles) {
        //* NOTE: this can be parallelized
        for (int i = floor(particle->get_lower_bound()); i < floor(particle->get_upper_bound()) && i < state.pixel_length; i++) {
            float index = (i - particle->get_lower_bound()) / (particle->get_upper_bound() - particle->get_lower_bound());
            this->screen[i] += particle->draw(index, state); // TODO: alpha channels?
        }
    }
}

vec4 ParticleSystem::draw(float index, const ChromaState &state) const
{
    const int gaussian_radius = 4;
    float kernel[] = {0.0002, 0.0060, 0.0606, 0.2417, 0.3829, 0.2417, 0.0606, 0.0060, 0.0002};
    
    size_t position = floor(state.pixel_length * index);
    vec4 color(0, 0, 0, 0);

    for (int i = - gaussian_radius; i < gaussian_radius; i++) {
        int j = position + i;
        if (j >= 0 && j < state.pixel_length) {
            color += this->screen[j] * kernel[i];
        }
    }

    return color;
}