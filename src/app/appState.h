#pragma once

enum InteractionMode
{
    InteractionModeCameraControl = 0,
    InteractionModeVesselRotation = 1,
    InteractionModeForceApplication = 2,
};

struct AppState
{
    bool paused = true;
    bool artPressureEnabled = true;
    int activeSceneIndex = 0;

    // Mouse interaction
    float mouseForceRadius = 1.0f;
    int interactionMode = InteractionModeCameraControl;
};