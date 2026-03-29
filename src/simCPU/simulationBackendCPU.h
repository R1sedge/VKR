#pragma once

#include <vector>

#include "common/Config.h"
#include "sim/simulationBackend.h"
#include "sim/structs.h"
#include "simCPU/neighborSearch/pairsGrid.h"
#include "simCPU/constraints/boxBounds.h"
#include "simCPU/constraints/circleCollision.h"
#include "scene/SceneFiller.h"

class SimulationBackendCPU final : public ISimulationBackendImpl
{
public:
    SimulationBackendCPU();

    void reset() override;
    void update(float dt) override;
    void setWorldBounds(float left, float right, float bottom, float top) override;
    void loadScene(const SceneDescription& desc) override;

    const Particles2D& getParticles() const override { return particles; }

    void setIterations(int iter) { iterations = iter; }
    void configureGrid(float left, float right, float bottom, float top, float cellSize);
    void setVelocityDamping(float d) { velocityDamping = d; }

    const std::vector<int>& getNeighborOffsets() const { return neighborOffsets; }
    const std::vector<int>& getNeighborIds() const { return neighborIds; }

private:
    int iterations = Config::iterations;
    float velocityDamping = 0.003f;

    Particles2D particles;
    BoxBoundsConstraint2D boxConstraint;
    CircleCollisionConstraint2D circleCollision;

    std::vector<CollisionPair> collisionPairs;
    UniformGrid2D grid;

    // CSR Список соседей для PBF:
    // Соседи частицы i в промежутке [neighborOffsets[i], neighborOffsets[i + 1])
    std::vector<int> neighborOffsets;
    std::vector<int> neighborIds;

private:
    // общие PBD/PBF стадии 
    void beginStep();
    void predictPositions(float dt);

    void buildBroadphase();
    void buildCollisionPairs();
    void buildNeighbors();

    void finalizeVelocities(float dt);

    // PBF стадии
    void computeDensity();
    void computeLambda();
    void computeDeltaPositions();
    void applyDeltaPositions();
};
