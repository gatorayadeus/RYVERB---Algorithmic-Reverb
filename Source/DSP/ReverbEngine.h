#pragma once
// Motor de reverb algoritmica RYVRB. C++ puro, sin dependencias de JUCE.
//
//  entrada -> pre-delay -> 4 allpass de difusion -> FDN de 8 lineas (matriz Hadamard)
//          -> EQ (low cut / high cut / shelves) -> ancho estereo -> ducker -> mezcla
//
// Cada linea del FDN lleva un filtro de decaimiento de 3 bandas controlado por
// Decay, Low Ratio / Low Freq y High Ratio / High Freq (RT60 por banda).
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct ReverbParams
{
    float predelayMs = 20.f, decay = 2.5f;
    float lowRatio = 1.15f, lowFreq = 400.f, highRatio = 0.85f, highFreq = 4000.f;
    float size = 100.f, diffusion = 70.f, modulation = 25.f, width = 100.f;   // %
    float duckAmount = 0.f, duckThresholdDb = -24.f, duckReleaseMs = 250.f, duckAttackMs = 8.f;
    float lowCutHz = 20.f, highCutHz = 16000.f, lowGainDb = 0.f, highGainDb = 0.f;
    float mix = 30.f, outputDb = 0.f;                                          // mix en %
    bool  bypass = false;
};

namespace ryvrb
{
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void reset() { z1 = z2 = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }

    static double clampF (double fs, double f) { return std::min (std::max (f, 10.0), fs * 0.45); }
    void highPass (double fs, double f, double q = 0.7071)
    {
        const double w = 2 * M_PI * clampF (fs, f) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowPass (double fs, double f, double q = 0.7071)
    {
        const double w = 2 * M_PI * clampF (fs, f) / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowShelf (double fs, double f, double dB)
    {
        const double A = std::pow (10.0, dB / 40), w = 2 * M_PI * clampF (fs, f) / fs, c = std::cos (w);
        const double al = std::sin (w) / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
             (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highShelf (double fs, double f, double dB)
    {
        const double A = std::pow (10.0, dB / 40), w = 2 * M_PI * clampF (fs, f) / fs, c = std::cos (w);
        const double al = std::sin (w) / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
};

struct DelayLine
{
    std::vector<float> buf; int mask = 0, w = 0;
    void init (int minSize) { int n = 1; while (n < minSize) n <<= 1; buf.assign ((size_t) n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.f); w = 0; }
    // Llamar ANTES de write(): devuelve x[n-d], con d >= 1 (interpolacion lineal)
    inline float read (float d) const
    {
        const float p = (float) w - d;
        const int i = (int) std::floor (p);
        const float f = p - (float) i;
        return buf[(size_t) (i & mask)] * (1.f - f) + buf[(size_t) ((i + 1) & mask)] * f;
    }
    inline void write (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
};

struct Allpass
{
    std::vector<float> buf; int mask = 0, w = 0, D = 100;
    void init (int delay) { D = std::max (1, delay); int n = 1; while (n < D + 2) n <<= 1; buf.assign ((size_t) n, 0.f); mask = n - 1; w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.f); w = 0; }
    inline float process (float x, float g)
    {
        const float wd = buf[(size_t) ((w - D) & mask)];
        const float wn = x + g * wd;
        buf[(size_t) w] = wn; w = (w + 1) & mask;
        return -g * wn + wd;
    }
};
}

class ReverbEngine
{
public:
    static constexpr int kLines = 8;

    std::atomic<float> inPeak { 0.f }, outPeak { 0.f };   // picos (lineal) para los medidores

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        const double k = sr / 44100.0;
        for (int c = 0; c < 2; ++c)
        {
            pre[c].init ((int) (0.30 * sr) + 8);
            static const int apD[4] = { 113, 163, 211, 277 };
            for (int s = 0; s < 4; ++s) ap[c][s].init ((int) std::lround (apD[s] * k * (c == 0 ? 1.0 : 1.07)));
        }
        static const int base[kLines] = { 1117, 1327, 1559, 1777, 1993, 2237, 2459, 2731 };
        for (int j = 0; j < kLines; ++j)
        {
            baseDelay[j] = (float) (base[j] * k);
            fdn[j].init ((int) (baseDelay[j] * 2.2f) + 64);
            lfoInc[j] = (float) (2 * M_PI * (0.17 + 0.063 * j) / sr);
            lfoPhase[j] = (float) j * 0.8f;
        }
        envCoefA = (float) (1.0 - std::exp (-1.0 / (0.004 * sr)));
        smoothK  = (float) (1.0 - std::exp (-1.0 / (0.02 * sr)));
        slowK    = (float) (1.0 - std::exp (-1.0 / (0.30 * sr)));
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            pre[c].clear();
            for (auto& a : ap[c]) a.clear();
            hp[c].reset(); lp[c].reset(); ls[c].reset(); hs[c].reset();
        }
        for (int j = 0; j < kLines; ++j) { fdn[j].clear(); lpl[j] = lph[j] = 0.f; }
        env = 0.f; duckGain = 1.f; snap = true;
        inPeak = 0.f; outPeak = 0.f;
    }

    void setParams (const ReverbParams& p) { P = p; }

    void process (float* const* io, int channels, int numSamples)
    {
        const int nch = std::min (channels, 2);
        if (nch <= 0 || numSamples <= 0) return;

        updateTargets();
        if (snap) { snapState(); snap = false; }

        float pkIn = 0.f, pkOut = 0.f;
        const float duckRelK = (float) (1.0 - std::exp (-1.0 / (std::max (5.f, P.duckReleaseMs) * 0.001 * sr)));
        const float duckAtkK = (float) (1.0 - std::exp (-1.0 / (std::max (0.5f, P.duckAttackMs) * 0.001 * sr)));
        const float envRelK  = duckRelK;
        const float modDepth = (P.modulation * 0.01f) * 9.f * (float) (sr / 44100.0);
        const float apG = 0.72f * P.diffusion * 0.01f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = io[0][i];
            const float dryR = nch > 1 ? io[1][i] : dryL;
            pkIn = std::max (pkIn, std::max (std::abs (dryL), std::abs (dryR)));

            // suavizados
            sizeSm  += slowK   * (tSize - sizeSm);
            preSm   += smoothK * (tPre  - preSm);
            mixSm   += smoothK * (tMix  - mixSm);
            outSm   += smoothK * (tOut  - outSm);

            // pre-delay
            const float pd = std::max (1.f, preSm * 0.001f * (float) sr);
            float xl = pre[0].read (pd); pre[0].write (dryL);
            float xr = pre[1].read (pd); pre[1].write (dryR);

            // difusion de entrada
            for (int s = 0; s < 4; ++s) { xl = ap[0][s].process (xl, apG); xr = ap[1][s].process (xr, apG); }

            // lineas del FDN
            float y[kLines], v[kLines];
            for (int j = 0; j < kLines; ++j)
            {
                lfoPhase[j] += lfoInc[j]; if (lfoPhase[j] > 6.2831853f) lfoPhase[j] -= 6.2831853f;
                const float d = baseDelay[j] * sizeSm + modDepth * std::sin (lfoPhase[j]);
                y[j] = fdn[j].read (std::max (2.f, d));

                gL[j] += smoothK * (tgL[j] - gL[j]);
                gM[j] += smoothK * (tgM[j] - gM[j]);
                gH[j] += smoothK * (tgH[j] - gH[j]);
                lpl[j] += aLow  * (y[j] - lpl[j]);
                lph[j] += aHigh * (y[j] - lph[j]);
                v[j] = gM[j] * y[j] + (gL[j] - gM[j]) * lpl[j] + (gH[j] - gM[j]) * (y[j] - lph[j]);
            }

            float wl = (y[0] - y[2] + y[4] - y[6]) * kOutScale;
            float wr = (y[1] - y[3] + y[5] - y[7]) * kOutScale;

            // matriz Hadamard 8x8 (transformada rapida), normalizada
            for (int h = 1; h < kLines; h <<= 1)
                for (int a = 0; a < kLines; a += h * 2)
                    for (int b = a; b < a + h; ++b) { const float s = v[b], t = v[b + h]; v[b] = s + t; v[b + h] = s - t; }
            static const float inSign[kLines] = { 1, 1, -1, 1, 1, -1, -1, 1 };
            for (int j = 0; j < kLines; ++j)
                fdn[j].write (v[j] * 0.35355339f + ((j & 1) ? xr : xl) * inSign[j] * kInjGain);

            // EQ del canal humedo
            wl = hs[0].process (ls[0].process (lp[0].process (hp[0].process (wl))));
            wr = hs[1].process (ls[1].process (lp[1].process (hp[1].process (wr))));

            // ancho estereo
            { const float m = 0.5f * (wl + wr), s = 0.5f * (wl - wr) * P.width * 0.01f; wl = m + s; wr = m - s; }

            // ducker
            {
                const float a = std::max (std::abs (dryL), std::abs (dryR));
                env += (a > env ? envCoefA : envRelK) * (a - env);
                const float envDb = 20.f * std::log10 (env + 1e-9f);
                const float red = P.duckAmount * 0.01f * 24.f * std::min (1.f, std::max (0.f, (envDb - P.duckThresholdDb) / 12.f));
                const float tg = std::pow (10.f, -red / 20.f);
                duckGain += (tg < duckGain ? duckAtkK : duckRelK) * (tg - duckGain);
                wl *= duckGain; wr *= duckGain;
            }

            const float oL = (dryL * (1.f - mixSm) + wl * mixSm) * outSm;
            const float oR = (dryR * (1.f - mixSm) + wr * mixSm) * outSm;
            if (nch > 1) { io[0][i] = oL; io[1][i] = oR; }
            else          io[0][i] = 0.5f * (oL + oR);
            pkOut = std::max (pkOut, std::max (std::abs (oL), std::abs (oR)));
        }

        float cur = inPeak.load (std::memory_order_relaxed);
        while (pkIn > cur && !inPeak.compare_exchange_weak (cur, pkIn)) {}
        cur = outPeak.load (std::memory_order_relaxed);
        while (pkOut > cur && !outPeak.compare_exchange_weak (cur, pkOut)) {}
    }

private:
    static constexpr float kInjGain = 0.5f;
    static constexpr float kOutScale = 0.85f;
    static constexpr float kDecayComp = 1.14f;   // compensa el tramo inicial rapido de la curva

    void updateTargets()
    {
        tSize = std::clamp (P.size, 25.f, 200.f) * 0.01f;
        tPre  = std::clamp (P.predelayMs, 0.f, 250.f);
        tMix  = P.bypass ? 0.f : std::clamp (P.mix, 0.f, 100.f) * 0.01f;
        tOut  = std::pow (10.f, P.outputDb / 20.f);

        float lf = P.lowFreq, hf = P.highFreq;
        if (lf >= hf) lf = hf * 0.5f;
        aLow  = 1.f - std::exp (-2.f * 3.14159265f * lf / (float) sr);
        aHigh = 1.f - std::exp (-2.f * 3.14159265f * hf / (float) sr);

        const float rtM = std::clamp (P.decay * kDecayComp, 0.05f, 60.f);
        const float rtL = std::clamp (P.decay * kDecayComp * P.lowRatio,  0.05f, 90.f);
        const float rtH = std::clamp (P.decay * kDecayComp * P.highRatio, 0.05f, 90.f);
        for (int j = 0; j < kLines; ++j)
        {
            const float dSec = baseDelay[j] * tSize / (float) sr;
            auto g = [&] (float rt) { return std::min (0.9995f, std::pow (10.f, -3.f * dSec / rt)); };
            tgM[j] = g (rtM); tgL[j] = g (rtL); tgH[j] = g (rtH);
        }

        for (int c = 0; c < 2; ++c)
        {
            hp[c].highPass (sr, P.lowCutHz);
            lp[c].lowPass  (sr, P.highCutHz);
            ls[c].lowShelf (sr, 200.0, P.lowGainDb);
            hs[c].highShelf (sr, 4000.0, P.highGainDb);
        }
    }

    void snapState()
    {
        sizeSm = tSize; preSm = tPre; mixSm = tMix; outSm = tOut;
        for (int j = 0; j < kLines; ++j) { gL[j] = tgL[j]; gM[j] = tgM[j]; gH[j] = tgH[j]; }
    }

    ReverbParams P;
    double sr = 44100.0;
    ryvrb::DelayLine pre[2], fdn[kLines];
    ryvrb::Allpass ap[2][4];
    ryvrb::Biquad hp[2], lp[2], ls[2], hs[2];
    float baseDelay[kLines] {}, lfoPhase[kLines] {}, lfoInc[kLines] {};
    float lpl[kLines] {}, lph[kLines] {};
    float gL[kLines] {}, gM[kLines] {}, gH[kLines] {}, tgL[kLines] {}, tgM[kLines] {}, tgH[kLines] {};
    float aLow = 0.1f, aHigh = 0.3f;
    float tSize = 1.f, tPre = 20.f, tMix = 0.3f, tOut = 1.f;
    float sizeSm = 1.f, preSm = 20.f, mixSm = 0.3f, outSm = 1.f;
    float env = 0.f, duckGain = 1.f, envCoefA = 0.01f, smoothK = 0.001f, slowK = 0.0001f;
    bool snap = true;
};
