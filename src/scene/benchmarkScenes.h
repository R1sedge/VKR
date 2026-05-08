#pragma once
#include "sceneDescription.h"

namespace BenchmarkScenes 
{
// Кубический сосуд, кубический блок воды на дне по центру.
SceneDescription makeBox(int targetN);

// Вытянутый сосуд 3:1, кубический блок воды у левой стенки (dam break).
SceneDescription makeDamBreak(int targetN);
}