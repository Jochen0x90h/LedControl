#pragma once

inline float3 lookupSpringPalette(float x) {
    float r, g, b;
    if (x < 0.5f) {
        if (x < 0.25f) {
            // 0 - 0.25
            r = 0.0f + x * 2.24f;
            g = 0.82f + x * 0.44f;
            b = 0.58f - x * 0.0799999f;
        } else {
            // 0.25 - 0.5
            r = 0.56f + (x - 0.25f) * 1.36f;
            g = 0.93f - (x - 0.25f) * 0.12f;
            b = 0.56f;
        }
    } else {
        if (x < 0.75f) {
            // 0.5 - 0.75
            r = 0.9f;
            g = 0.9f;
            b = 0.56f - (x - 0.5f) * 2.24f;
        } else {
            // 0.75 - 1
            r = 0.9f - (x - 0.75f) * 3.6f;
            g = 0.9f - (x - 0.75f) * 0.32f;
            b = 0.0f + (x - 0.75f) * 2.32f;
        }
    }
    return {r, g, b};
}
