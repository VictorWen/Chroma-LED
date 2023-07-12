#ifndef DISCO_H
#define DISCO_H

#include <cinttypes>
#include <memory>

#include "chroma.h"

struct DiscoPacket { 
    uint32_t start;
    uint32_t end;
    uint32_t n_frames;
    vec4 data[4000];
};

class DiscoController {
    public:
        virtual int write(const std::vector<vec4>& pixels) = 0;
};

class UDPDisco : public DiscoController {
    public:
        int write(const std::vector<vec4>& pixels);
};

#endif