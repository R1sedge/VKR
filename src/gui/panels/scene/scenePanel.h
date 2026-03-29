#pragma once
#include "imgui.h"

struct AppState;
struct AppCommands;

class ScenePanel 
{
public:
    void draw(const AppState& state, AppCommands& commands);
    void setSceneIndex(int idx) { m_sceneIndex = idx; }

private:
    float m_anim = 0.0f;
    int   m_sceneIndex = 0;

    static constexpr float kPanelW = 220.0f;
    static constexpr float kTriggerX = 96.0f;   // px от ПРАВОГО края
    static constexpr float kAnimSpeed = 10.0f;

    void drawEdgeHint(const ImGuiIO& io);
    void drawSceneList(const AppState& state, AppCommands& commands);
};