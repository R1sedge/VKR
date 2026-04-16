#include "boxBounds.h"

void BoxBoundsConstraint2D :: project(Particles3D& particles)
    {
        for (int i = 0; i < particles.count; ++i)
        {
            float& x = particles.x[i];
            float& y = particles.y[i];
            float& vy = particles.vy[i];

            if (x < left + radius)
                x = left + radius;
            if (x > right - radius) 
                x = right - radius;
            if (y < bottom + radius)
                y = bottom + radius;
            if (y > top - radius) 
                y = top - radius;
        }
    }