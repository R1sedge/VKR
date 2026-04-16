#pragma once
#include "data/particleData.h"
#include "sim/structs.h"

void findPairsNaive(const Particles3D& p, float radius, std::vector<CollisionPair>& out);