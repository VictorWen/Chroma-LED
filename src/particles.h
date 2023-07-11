#ifndef PARTICLES_H
#define PARTICLES_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "chroma.h"
#include "chromatic.h"


class CollisionEvent;

struct PhysicsBody : public ChromaObject {
    float position;
    float velocity;
    float acceleration;
    float mass;
    float prev_position;
    float prev_velocity;
    std::vector<CollisionEvent> collisions;
    PhysicsBody() : PhysicsBody(0, 0, 0, 1) { };
    PhysicsBody(const std::vector<ChromaData>& args);
    PhysicsBody(float pos, float vel, float acc, float mass) :
        ChromaObject("PhysicsBody"), position(pos), velocity(vel), acceleration(acc), mass(mass) { }
    void tick(float time_delta) {
        this->prev_position = this->position;
        this->prev_velocity = this->prev_velocity;
        this->position += (this->velocity + this->acceleration * time_delta / 2.0) * time_delta;
        this->velocity += this->acceleration * time_delta;
    }
    PhysicsBody clone() const {
        return PhysicsBody(*this);
    }
};

class CollisionEvent {
    private:
        PhysicsBody* _body1;
        PhysicsBody* _body2;
        float _collision_time;
    public:
        CollisionEvent(PhysicsBody& body1, PhysicsBody& body2, float collision_time) :
            _body1(&body1), _body2(&body2), _collision_time(collision_time) { }
        CollisionEvent(const CollisionEvent& other) :
            _body1(other._body1), _body2(other._body2), _collision_time(other._collision_time) { }
        PhysicsBody& body1() { return *this->_body1; }
        PhysicsBody& body2() { return *this->_body2; }
        float collision_time() { return this->_collision_time; }
};

class ParticleSystem;
class ParticleBehavior;

class ParticleEffect : public ChromaObject {
    private:
        float radius;
        float intensity;
        std::shared_ptr<ChromaEffect> effect;
        std::vector<std::shared_ptr<ParticleBehavior>> behaviors;
        PhysicsBody body;
        bool alive;
    public:
        ParticleEffect(const std::vector<ChromaData>& args);
        ParticleEffect(const ParticleEffect& other);
        float get_radius() const { return this->radius; }
        float get_lower_bound() const { return this->body.position - this->radius; }
        float get_upper_bound() const { return this->body.position + this->radius; }
        PhysicsBody& get_body() { return this->body; }
        bool is_in_bounds(float pos) const { return (this->body.position - this->radius) <= pos && pos <= (this->body.position + this->radius); }
        void tick(ParticleSystem& system, const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
        std::shared_ptr<ParticleEffect> clone(const PhysicsBody& body);
        bool is_alive() { return this->alive; }
        void kill() { this->alive = false; }
};

class ParticleSystem : public ChromaEffect {
    private:
        std::vector<vec4> screen;
        std::unordered_set<std::shared_ptr<ParticleEffect>> particles;
        std::unordered_set<std::shared_ptr<ParticleEffect>> pending_particles;
    public:
        ParticleSystem(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
        void add_particle(std::shared_ptr<ParticleEffect>& particle) { this->pending_particles.insert(particle); }
        //std::shared_ptr<ChromaObject> clone() const;
};

class ParticleBehavior : public ChromaObject {
    protected:
        bool is_alive;
    public:
        ParticleBehavior() : ChromaObject("ParticleBehavior"), is_alive(true) { }
        virtual void tick(ParticleSystem& system, ParticleEffect& particle, const ChromaState& state) = 0;
        virtual std::shared_ptr<ParticleBehavior> clone() const = 0;
};

class EmitterBehavior : public ParticleBehavior {
    private:
        std::shared_ptr<ParticleEffect> particle;
        float density;
        float time_start;
        int particles_emitted;
        void emit(ParticleSystem& system, ParticleEffect& effect);
    public:
        EmitterBehavior(const std::vector<ChromaData>& args); 
        void tick(ParticleSystem& system, ParticleEffect& particle, const ChromaState& state);
        std::shared_ptr<ParticleBehavior> clone() const;
};

class LifetimeBehavior : public ParticleBehavior { 
    private:
        float lifetime;
        float time_start;
    public:
        LifetimeBehavior(const std::vector<ChromaData>& args);
        void tick(ParticleSystem& system, ParticleEffect& particle, const ChromaState& state);
        std::shared_ptr<ParticleBehavior> clone() const;
};

#endif