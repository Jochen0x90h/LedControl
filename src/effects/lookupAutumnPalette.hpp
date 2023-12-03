#pragma once

inline float3 lookupAutumnPalette(float x) {
    float r, g, b;
    if (x < 0.25f) {
        if (x < 0.125f) {
            // 0 - 0.125
            r = 0.65f - x * 3.6f;
            g = 0.16f + x * 5.12f;
            b = 0.16f + x * 0.32f;
        } else {
            // 0.125 - 0.25
            r = 0.2f + (x - 0.125f) * 5.6f;
            g = 0.8f + (x - 0.125f) * 0.8f;
            b = 0.2f - (x - 0.125f) * 1.6f;
        }
    } else {
        if (x < 0.5f) {
            // 0.25 - 0.5
            r = 0.9f;
            g = 0.9f - (x - 0.25f) * 1.88f;
            b = 0.0f;
        } else {
            if (x < 0.75f) {
                // 0.5 - 0.75
                r = 0.9f + (x - 0.5f) * 0.4f;
                g = 0.43f - (x - 0.5f) * 1.72f;
                b = 0.0f;
            } else {
                // 0.75 - 1
                r = 1.0f - (x - 0.75f) * 1.4f;
                g = 0.0f + (x - 0.75f) * 0.64f;
                b = 0.0f + (x - 0.75f) * 0.64f;
            }
        }
    }
    return {r, g, b};
}
