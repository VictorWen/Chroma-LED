#include <math.h>

#include "particles.h"


PhysicsBody::PhysicsBody(const std::vector<ChromaData> &args) : ChromaObject("PhysicsBody"), 
    _position(0), _velocity(0), _acceleration(0), _mass(1)
{
    this->_position = args[0].get_float();
    if (args.size() >= 2)
        this->_velocity = args[1].get_float();
    if (args.size() >= 3)
        this->_acceleration = args[2].get_float();
    if (args.size() >= 4)
        this->_mass = args[3].get_float();
}

ParticleEffect::ParticleEffect(const std::vector<ChromaData> &args) : ChromaObject("Particle"), body(10., 0., 0., 1.)
{
    this->effect = args[0].get_effect();
    this->body = *(args[1].get_obj<PhysicsBody>());
    this->radius = args[2].get_float();
    if (args.size() > 3) {
        for (auto& behavior : args[3].get_list()) {
            this->behaviors.push_back(behavior.get_obj<ParticleBehavior>());
        }
    }
}

void ParticleEffect::tick(ParticleSystem &system, const ChromaState &state)
{
    this->body.tick(state.delta_time);
    for (auto& behavior : this->behaviors) {
        behavior->tick(system, *this, state);
    }
}

vec4 ParticleEffect::draw(float index, const ChromaState &state) const
{
    return this->effect->draw(index, state);
}

std::shared_ptr<ParticleEffect> ParticleEffect::clone(const PhysicsBody &body)
{
    auto output = std::make_shared<ParticleEffect>(*this); // TODO: clone color effect properly
    output->body = body;
    return output;
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
    // fprintf(stderr, "SYSTEM TICK START\n");
    for (auto& particle : this->particles) {
        particle->tick(*this, state);
    }

    for (auto& particle : this->particles) {
        //* NOTE: this can be parallelized
        for (int i = floor(particle->get_lower_bound()); i <= floor(particle->get_upper_bound()) && i < state.pixel_length; i++) {
            if (i >= 0) {
                float index = (i - particle->get_lower_bound()) / (particle->get_upper_bound() - particle->get_lower_bound());
                this->screen[i] += particle->draw(index, state); // TODO: alpha channels?
            }
        }
    }

    for (auto& new_particle : this->pending_particles) {
        this->particles.insert(new_particle);
    }
    this->pending_particles.clear();
    // fprintf(stderr, "SYSTEM TICK END\n");
}

vec4 ParticleSystem::draw(float index, const ChromaState &state) const
{
    const int gaussian_radius = 4;
    float kernel[] = {0.0002, 0.0060, 0.0606, 0.2417, 0.3829, 0.2417, 0.0606, 0.0060, 0.0002};
    
    int position = floor(state.pixel_length * index);
    vec4 color(0, 0, 0, 0);

    for (int i = -gaussian_radius; i <= gaussian_radius; i++) {
        int j = position + i;
        if (j >= 0 && j < state.pixel_length) {
            color += this->screen[j] * kernel[i + gaussian_radius];
        }
    }

    return color;
}

EmitterBehavior::EmitterBehavior(const std::vector<ChromaData> &args) :
    time_start(-1), particles_emitted(0) 
{
    this->particle = args[0].get_obj<ParticleEffect>();
    this->density = args[1].get_float();
}

void EmitterBehavior::tick(ParticleSystem &system, ParticleEffect &particle, const ChromaState &state)
{
    if (this->time_start < 0)
        this->time_start = state.time;
    if (state.get_time_diff(this->time_start) * this->density > this->particles_emitted) {
        fprintf(stderr, "EMISSION\n");
        this->emit(system, particle);
        this->particles_emitted++;
    }
}

void EmitterBehavior::emit(ParticleSystem &system, ParticleEffect &particle)
{
    PhysicsBody body(
        particle.get_body().pos() + this->particle->get_body().pos(), 
        particle.get_body().vel() + this->particle->get_body().vel(), 
        particle.get_body().acc(), 
        particle.get_body().mass()
    );
    auto emission = this->particle->clone(body);
    system.add_particle(emission);
}
