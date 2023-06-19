#include "chromatic.h"
#include <math.h>
#include <algorithm>

#include <iostream>

vec4 abs(const vec4& vec) {
    return vec4(std::abs(vec.x), std::abs(vec.y), std::abs(vec.z), std::abs(vec.w));
}

vec4 max(const vec4& vec, const float other) {
    return vec4(std::max(vec.x, other), std::max(vec.y, other), std::max(vec.z, other), std::max(vec.w, other));
}

void ExampleShader::tick(const float time) { return; }

void ExampleShader::draw(const float time, const float index, vec4& pixel) const {
    float i = fmod((index + time) * 3, 3);
	pixel = vec4(std::fmod(i + 1, 3), i, std::fmod(i - 1, 3), 1);
	pixel = max(-abs(pixel - 1) + 1, 0);
}