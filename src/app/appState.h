#pragma once

struct AppState
{
    bool paused = true;
    bool artPressureEnabled = true;
    int activeSceneIndex = 0;

    // Mouse interaction
    float mouseForceRadius = 1.0f;
    int interactionMode = 0;  // 0=force, 1=container rotation
};