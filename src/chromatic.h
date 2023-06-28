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

	vec4 operator*(const float other) const {
		return vec4(x*other, y*other, z*other, w*other);
	}

	vec4 operator+(const vec4& other) const {
		return vec4(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	vec4 operator-() const {
		return vec4(-x, -y, -z, -w);
	}

	std::string to_string() const {
		return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ", " + std::to_string(w) + ")";
	}
};

vec4 abs(const vec4& vec);
vec4 max(const vec4& vec, const float other);

class Shader {
	public:
		virtual ~Shader() { return; }
		virtual void tick(const float time) { return; }
		virtual void draw(const float time, const float index, vec4& pixel) const { return; };
};

class ExampleShader : public Shader {
	public:
		virtual void draw(const float time, const float index, vec4& pixel) const;
};

#endif