#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <juce_core/juce_core.h>

// Pitch shifter de dominio temporal: dos lecturas con ventana Hann sobre una línea de retardo.
// Si la ventana es un múltiplo de 2*periodo, las dos lecturas quedan en fase y suena limpio.
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        bufSize = 1 << 14;
        mask = bufSize - 1;
        buf.assign ((size_t) bufSize, 0.0f);
        writePos = 0;
        phase = 0.0f;
        window = targetWindow = 1024.0f * (float) (sampleRate / 44100.0);
    }

    void setWindow (float w) { targetWindow = juce::jlimit (256.0f, 8000.0f, w); }

    float process (float in, float ratio)
    {
        buf[(size_t) writePos] = in;

        window += 0.0002f * (targetWindow - window);

        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);

        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float w1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        const float w2 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p2);

        const float out = w1 * read (phase * window) + w2 * read (p2 * window);

        writePos = (writePos + 1) & mask;
        return out;
    }

private:
    float read (float delay) const
    {
        float pos = (float) writePos - delay;
        if (pos < 0.0f) pos += (float) bufSize;
        const int   i0   = (int) pos;
        const float frac = pos - (float) i0;
        const int   a    = i0 & mask;
        const int   b    = (i0 + 1) & mask;
        return buf[(size_t) a] + frac * (buf[(size_t) b] - buf[(size_t) a]);
    }

    std::vector<float> buf;
    int   bufSize = 1 << 14, mask = (1 << 14) - 1, writePos = 0;
    float phase = 0.0f, window = 1024.0f, targetWindow = 1024.0f;
};
