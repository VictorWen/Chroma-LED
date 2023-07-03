#ifndef PARTICLES_H
#define PARTICLES_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "chroma.h"
#include "chromatic.h"


class PhysicsBody : public ChromaObject {
    private:
        float _position;
        float _velocity;
        float _acceleration;
        float _mass;
        float _prev_position;
        float _prev_velocity;
    public:
        PhysicsBody(const std::vector<ChromaData>& args);
        PhysicsBody(float pos, float vel, float acc, float mass) :
            ChromaObject("PhysicsBody"), _position(pos), _velocity(vel), _acceleration(acc), _mass(mass) { }
        float pos() const { return this->_position; }
        float vel() const { return this->_velocity; }
        float acc() const { return this->_acceleration; }
        float mass() const { return this->_mass; }
        float prev_pos() const { return this->_prev_position; }
        float prev_vel() const { return this->_prev_velocity; }
        void tick(float time_delta) {
            this->_prev_position = this->_position;
            this->_prev_velocity = this->_prev_velocity;
            this->_position += (this->_velocity + this->_acceleration * time_delta / 2.0) * time_delta;
            this->_velocity += this->_acceleration * time_delta;
        }
        PhysicsBody clone() const {
            return PhysicsBody(*this);
        }
};

class CollisionEvent {

};

class ParticleSystem;

class ParticleEffect : public ChromaObject {
    private:
        float radius;
        float intensity;
        std::shared_ptr<ChromaEffect> effect;
        PhysicsBody body;
    public:
        ParticleEffect(const std::vector<ChromaData>& args);
        float get_radius() const { return this->radius; }
        float get_lower_bound() const { return this->body.pos() - this->radius; }
        float get_upper_bound() const { return this->body.pos() + this->radius; }
        const PhysicsBody& get_body() const { return this->body; }
        bool is_in_bounds(float pos) const { return (this->body.pos() - this->radius) <= pos && pos <= (this->body.pos() + this->radius); }
        void tick(ParticleSystem& system, const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
};

class ParticleSystem : public ChromaEffect {
    private:
        std::vector<vec4> screen;
        std::unordered_set<std::shared_ptr<ParticleEffect>> particles;
    public:
        ParticleSystem(const std::vector<ChromaData>& args);
        void tick(const ChromaState& state);
        vec4 draw(float index, const ChromaState& state) const;
        std::shared_ptr<ChromaObject> clone() const;
};

#endif