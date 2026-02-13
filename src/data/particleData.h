#pragma once
#include <vector>
#include <cstddef>

namespace Config {
    constexpr size_t MAX_PARTICLES = 5000;
    constexpr float PARTICLE_RADIUS = 5.0f;
}

struct ParticleData {
    // SoA: данные разделены по типам для cache locality
    std::vector<float> pos_x;
    std::vector<float> pos_y;
    std::vector<float> old_pos_x;
    std::vector<float> old_pos_y;
    std::vector<float> acc_x;
    std::vector<float> acc_y;
    std::vector<float> radii;

    size_t count = 0;

    // Резервируем память заранее (важно!)
    ParticleData() {
        reserve(Config::MAX_PARTICLES);
    }

    void reserve(size_t capacity) {
        pos_x.reserve(capacity);
        pos_y.reserve(capacity);
        old_pos_x.reserve(capacity);
        old_pos_y.reserve(capacity);
        acc_x.reserve(capacity);
        acc_y.reserve(capacity);
        radii.reserve(capacity);
    }

    void addParticle(float x, float y, float radius = Config::PARTICLE_RADIUS) {
        pos_x.push_back(x);
        pos_y.push_back(y);
        old_pos_x.push_back(x);
        old_pos_y.push_back(y);
        acc_x.push_back(0.0f);
        acc_y.push_back(0.0f);
        radii.push_back(radius);
        count++;
    }

    void clear() {
        pos_x.clear();
        pos_y.clear();
        old_pos_x.clear();
        old_pos_y.clear();
        acc_x.clear();
        acc_y.clear();
        radii.clear();
        count = 0;
    }
};
