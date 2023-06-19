#ifndef CHROMATIC_H
#define CHROMATIC_H

struct vec3 {
	float x, y, z;
};

struct vec4 {
	float x, y, z, w;

	vec4(const float x=0, const float y=0, const float z=0, const float w=0) :
		x(x), y(y), z(z), w(w) { }

	vec4 operator-(const float other) const {
		return vec4(x-other, y-other, z-other, w-other);
	}
	
	vec4 operator+(const float other) const {
		return vec4(x+other, y+other, z+other, w+other);
	}

	vec4 operator-() const {
		return vec4(-x, -y, -z, -w);
	}
};

class Shader {
	public:
		virtual ~Shader() { return; }
		virtual void tick(const float time) { return; }
		virtual void draw(const float time, const float index, vec4& pixel) const { return; };
};

class ExampleShader : public Shader {
	public:
		void tick(const float time);
		virtual void draw(const float time, const float index, vec4& pixel) const;
};

#endif