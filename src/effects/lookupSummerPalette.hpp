#pragma once

inline float3 lookupSummerPalette(float x) {
    float r, g, b;
    if (x < 0.5f) {
        if (x < 0.25f) {
            // 0 - 0.25
            r = 0.39f + x * 2.44f;
            g = 0.39f + x * 2.44f;
            b = 1.0f - x * 2.44f;
        } else {
            // 0.25 - 0.5
            r = 1.0f;
            g = 1.0f;
            b = 0.39f + (x - 0.25f) * 1.56f;
        }
    } else {
        if (x < 0.75f) {
            // 0.5 - 0.75
            r = 1.0f - (x - 0.5f) * 4.0f;
            g = 1.0f - (x - 0.5f) * 4.0f;
            b = 0.78f + (x - 0.5f) * 0.88f;
        } else {
            // 0.75 - 1
            r = 0.0f + (x - 0.75f) * 1.56f;
            g = 0.0f + (x - 0.75f) * 1.56f;
            b = 1.0f;
        }
    }
    return {r, g, b};
}
