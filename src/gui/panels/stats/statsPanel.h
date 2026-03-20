#pragma once
#include "gui/timing/frameTimer.h"

// Детальная статистика тайминга.
// Появляется только при наведении мыши в правый верхний угол экрана.
class StatsPanel
{
public:
    void draw(const FrameTimer& timer, float simDt);

private:
    // Зона триггера: правые kTriggerW px, верхние kTriggerH px
    static constexpr float kTriggerW = 160.0f;
    static constexpr float kTriggerH = 90.0f;

    // Вертикальный отступ от верха, чтобы не перекрывать FPS строку
    static constexpr float kTopOffset = 36.0f;

    static void timingRow(const char* label, float ms, float colOffset = 120.0f);
};