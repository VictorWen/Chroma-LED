#include <math.h>
#include <algorithm>

#include "particles.h"



PhysicsBody::PhysicsBody(const std::vector<ChromaData> &args) : ChromaObject("PhysicsBody"), 
    position(0), velocity(0), acceleration(0), mass(1)
{
    this->position = args[0].get_float();
    if (args.size() >= 2)
        this->velocity = args[1].get_float();
    if (args.size() >= 3)
        this->acceleration = args[2].get_float();
    if (args.size() >= 4)
        this->mass = args[3].get_float();
}

ParticleEffect::ParticleEffect(const std::vector<ChromaData> &args) : ChromaObject("Particle"), body(0, 0, 0, 1)
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

ParticleEffect::ParticleEffect(const ParticleEffect &other) : 
    ChromaObject("Particle"), radius(other.radius), intensity(other.intensity), body(other.body), alive(true)
{
    this->effect = other.effect; // TODO: implement effect.clone() instead
    for (auto& behavior : other.behaviors) {
        this->behaviors.push_back(behavior->clone());
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
    auto output = std::make_shared<ParticleEffect>(*this);
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

float calculate_collision_time(const PhysicsBody& body1, const PhysicsBody& body2) {
    float delta_pos = body2.prev_position - body1.prev_position;
    float delta_vel = body2.prev_velocity - body1.prev_velocity;
    float delta_acc = body2.acceleration - body1.acceleration;
    float delta_t = -1;

    if (delta_acc != 0) { // Solve a quadratic equation
        // solving for t in 0.5 * at^2 + vt + p
        float discrim = delta_vel * delta_vel - 4 * 0.5 * delta_acc * delta_pos;
        if (discrim < 0)
            return -1;
        float sqrt_discrim = sqrt(discrim);
        delta_t = (-delta_vel - sqrt_discrim) / delta_acc;
        if (delta_t < 0)
            delta_t = (-delta_vel + sqrt_discrim) / delta_acc;
    }
    else if (delta_vel != 0) // Solve a linear equation
        delta_t = -delta_pos / delta_vel;
    else  // No relative movement -> No collision
        return -1;

    if (delta_vel < 0)
        return -1;
    return delta_vel;
}

void calculate_collision(float time_delta, PhysicsBody& body1, PhysicsBody& body2) {
    float collision_time = calculate_collision_time(body1, body2);
    if (collision_time < 0 || collision_time > time_delta) // Collision not possible in this tick
        return;
    CollisionEvent event(body1, body2, collision_time); 
    body1.collisions.push_back(event);
    body2.collisions.push_back(event);
}

void ParticleSystem::tick(const ChromaState &state)
{
    this->screen = std::vector<vec4>(state.pixel_length);

    // Physics tick
    // 1. Calculate bounding intervals
    std::vector<std::tuple<float, bool, PhysicsBody*>> interval_points;
    for (const std::shared_ptr<ParticleEffect>& particle : this->particles) {
        PhysicsBody& body = particle->get_body();
        body.tick(state.delta_time);
        
        float left, right;
        if (body.position < body.prev_position) {
            left = body.position;
            right = body.prev_position;
        } else {
            left = body.prev_position;
            right = body.position;
        }
        interval_points.push_back({left, false, &body});
        interval_points.push_back({right, true, &body});
    }
    
    // 2. Sort intervals by start time
    std::sort(interval_points.begin(), interval_points.end());

    // 3. Find overlaps
    std::unordered_set<PhysicsBody*> active_intervals;
    for (size_t i = 0; i < interval_points.size(); i++) {
        std::tuple<float, bool, PhysicsBody*> point = interval_points[i];
        if (std::get<1>(point)) {
            active_intervals.erase(std::get<2>(point));
        }
        else {
            for (PhysicsBody* body: active_intervals) {
                calculate_collision(state.delta_time, *body, *std::get<2>(point));
            }
        }
    }

    std::vector<std::shared_ptr<ParticleEffect>> dead_particles;
    for (auto& particle : this->particles) {
        particle->tick(*this, state);
        if (!particle->is_alive())
            dead_particles.push_back(particle);
    }
    for (auto& particle : dead_particles) {
        this->particles.erase(particle);
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
        this->emit(system, particle);
        this->particles_emitted++;
    }
}

void EmitterBehavior::emit(ParticleSystem &system, ParticleEffect &particle)
{
    PhysicsBody body(
        particle.get_body().position + this->particle->get_body().position, 
        particle.get_body().velocity + this->particle->get_body().velocity, 
        particle.get_body().acceleration, 
        particle.get_body().mass
    );
    auto emission = this->particle->clone(body);
    fprintf(stderr, "EMISSION %d\n", emission->is_alive());
    system.add_particle(emission);
}

std::shared_ptr<ParticleBehavior> EmitterBehavior::clone() const {
    auto clone = std::make_shared<EmitterBehavior>(*this);
    clone->time_start = -1;
    return clone;
}

LifetimeBehavior::LifetimeBehavior(const std::vector<ChromaData>& args) : time_start(-1)
{
    this->lifetime = args[0].get_float();
}

void LifetimeBehavior::tick(ParticleSystem &system, ParticleEffect &particle, const ChromaState &state)
{
    if (this->time_start < 0)
        this->time_start = state.time;
    if (state.get_time_diff(this->time_start) >= this->lifetime)
        particle.kill();
}

std::shared_ptr<ParticleBehavior> LifetimeBehavior::clone() const {
    auto clone = std::make_shared<LifetimeBehavior>(*this);
    clone->time_start = -1;
    return clone;
}