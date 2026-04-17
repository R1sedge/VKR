#include "PhysicsOverrides.h"
#include <common/Config.h>

void PhysicsOverrides::applyToConfig() const 
{
    if (gravity) 
    {
        Config::gravityX = gravity->x;
        Config::gravityY = gravity->y;
        Config::gravityZ = gravity->z;
    }
}