#pragma once

#include <glm/vec4.hpp>

#include "common/Config.h"

struct AppCommands
{
    // Управление
    bool togglePause = false;

    bool hasSetPaused = false;
    bool setPausedValue = false;

    bool stepOnce = false;
    bool reset = false;

    // PBF
    bool hasSetRestDensity = false;
    float restDensityValue = Config::restDensity;

    bool hasSetArtPressure  = false;
    bool artPressureEnabled = true;

    bool hasSetArtificialPressureK = false;
    float artificialPressureK = Config::artificialPressureK;

    bool hasSetVorticity = false;
    float vorticityEpsilon = Config::vorticityEpsilon;

    bool hasSetXSPH = false;
    float xsphViscosity = Config::xsphViscosity;

    // Сцена
    bool hasSetScene = false;
    int sceneIndex = 0;

    // Отрисовка
    bool resetCamera = false;

    bool hasSetParticleColorMode = false;
    int particleColorMode = Config::particleColorMode;

    bool hasSetMaxGradSpeed = false;
    float maxGradSpeed = Config::maxGradSpeed;

    bool hasSetPhaseColors = false;
    glm::vec4 phase0Color = Config::phase0Color;
    glm::vec4 phase1Color = Config::phase1Color;

    // Параметры симуляции
    bool hasSetDt = false;
    float dt = Config::dt;

    bool hasSetGravity = false;
    float gravityX = Config::gravityX;
    float gravityY = Config::gravityY;
    float gravityZ = Config::gravityZ;

    bool hasSetMaxSpeed = false;
    float maxSpeed = Config::maxSpeed;

    bool hasSetWallResponse = false;
    float wallRestitution = Config::wallRestitution;
    float wallFriction = Config::wallFriction;

    bool hasSetBaffleFiltering = false;
    bool baffleFilteringEnabled = Config::enableBafflePairFiltering;

    void clear() {*this = AppCommands();}
};