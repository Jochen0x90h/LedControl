#pragma once

inline float3 lookupWinterPalette(float x) {
    float r, g, b;
    if (x < 0.5f) {
        if (x < 0.25f) {
            // 0 - 0.25
            r = 0.0f;
            g = 0.0f + x * 3.28f;
            b = 0.55f + x * 0.12f;
        } else {
            // 0.25 - 0.5
            r = 0.0f + (x - 0.25f) * 1.56f;
            g = 0.82f - (x - 0.25f) * 2.04f;
            b = 0.58f - (x - 0.25f) * 0.44f;
        }
    } else {
        if (x < 0.75f) {
            // 0.5 - 0.75
            r = 0.39f;
            g = 0.31f - (x - 0.5f) * 1.24f;
            b = 0.47f + (x - 0.5f) * 0.16f;
        } else {
            // 0.75 - 1
            r = 0.39f - (x - 0.75f) * 1.56f;
            g = 0.0f;
            b = 0.51f + (x - 0.75f) * 0.16f;
        }
    }
    return {r, g, b};
}
