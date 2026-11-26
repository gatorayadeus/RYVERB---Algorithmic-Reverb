// Pruebas offline de RYVRB (sin JUCE). g++ -O2 -std=c++17 -I../Source -I../Source/DSP dsp_test.cpp
#include <cstdio>
#include <random>
#include <functional>
#include "ReverbEngine.h"
#include "Presets.h"

static std::vector<float> run (double sr, const ReverbParams& p, const std::vector<float>& in)
{
    ReverbEngine e; e.prepare (sr); e.setParams (p);
    std::vector<float> L = in, R = in;
    for (size_t i = 0; i < in.size(); i += 256)
    {
        int n = (int) std::min<size_t> (256, in.size() - i);
        float* b[2] = { L.data() + i, R.data() + i };
        e.process (b, 2, n);
    }
    std::vector<float> m (in.size());
    for (size_t i = 0; i < m.size(); ++i) m[i] = 0.5f * (L[i] + R[i]);
    return m;
}

// RT60 por integracion inversa de Schroeder, ajuste entre -5 y -25 dB (T20 x 3)
static double rt60 (const std::vector<float>& ir, double sr)
{
    std::vector<double> e (ir.size());
    double acc = 0; for (size_t i = ir.size(); i-- > 0;) { acc += (double) ir[i] * ir[i]; e[i] = acc; }
    double e0 = e[0]; double t5 = -1, t25 = -1;
    for (size_t i = 0; i < e.size(); ++i)
    {
        double db = 10 * std::log10 (e[i] / e0 + 1e-30);
        if (t5 < 0 && db <= -5) t5 = (double) i / sr;
        if (t25 < 0 && db <= -25) { t25 = (double) i / sr; break; }
    }
    return (t5 < 0 || t25 < 0) ? -1 : (t25 - t5) * 3.0;
}

static std::vector<float> lowpassBand (std::vector<float> x, double sr, double f)
{ ryvrb::Biquad a, b; a.lowPass (sr, f); b.lowPass (sr, f); for (auto& v : x) v = b.process (a.process (v)); return x; }
static std::vector<float> highpassBand (std::vector<float> x, double sr, double f)
{ ryvrb::Biquad a, b; a.highPass (sr, f); b.highPass (sr, f); for (auto& v : x) v = b.process (a.process (v)); return x; }

static ReverbParams fromPreset (const RyPreset& r)
{
    ReverbParams p; const float* v = r.v;
    p.predelayMs = v[0]; p.decay = v[1]; p.lowRatio = v[2]; p.lowFreq = v[3]; p.highRatio = v[4]; p.highFreq = v[5];
    p.size = v[6]; p.diffusion = v[7]; p.modulation = v[8]; p.width = v[9];
    p.duckAmount = v[10]; p.duckThresholdDb = v[11]; p.duckReleaseMs = v[12];
    p.lowCutHz = v[13]; p.highCutHz = v[14]; p.lowGainDb = v[15]; p.highGainDb = v[16]; p.mix = v[17];
    return p;
}

int main()
{
    int fails = 0;
    auto check = [&] (bool ok, const char* fmt, auto... args) { std::printf (ok ? "  OK     " : "  FALLA  "); std::printf (fmt, args...); std::printf ("\n"); fails += !ok; };

    std::printf ("[1] RT60 vs Decay (ratios 1.0, mix 100%%)\n");
    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (float dec : { 0.8f, 2.0f, 5.0f })
        {
            ReverbParams p; p.mix = 100; p.decay = dec; p.lowRatio = p.highRatio = 1.f; p.predelayMs = 0; p.lowCutHz = 20; p.highCutHz = 20000;
            std::vector<float> x ((size_t) (sr * (dec * 2.2 + 1.0)), 0.f); x[0] = 1.f;
            double rt = rt60 (run (sr, p, x), sr);
            check (rt > dec * 0.9 && rt < dec * 1.1, "sr=%6.0f decay=%.1f s -> RT60 medido %.2f s", sr, dec, rt);
        }

    std::printf ("[2] Low/High ratio por banda (decay 2 s, lowRatio 2.0, highRatio 0.4)\n");
    {
        double sr = 48000; ReverbParams p; p.mix = 100; p.decay = 2.f; p.lowRatio = 2.f; p.highRatio = 0.4f; p.lowFreq = 300; p.highFreq = 3000;
        p.predelayMs = 0; p.lowCutHz = 20; p.highCutHz = 20000;
        std::vector<float> x ((size_t) (sr * 9), 0.f); x[0] = 1.f;
        auto ir = run (sr, p, x);
        double rtLow = rt60 (lowpassBand (ir, sr, 150), sr), rtHigh = rt60 (highpassBand (ir, sr, 8000), sr);
        check (rtLow > 3.0 && rtLow < 5.5, "banda grave (<150 Hz): RT60 %.2f s (esperado ~4)", rtLow);
        check (rtHigh > 0.4 && rtHigh < 1.2, "banda aguda (>8 kHz): RT60 %.2f s (esperado ~0.8)", rtHigh);
    }

    std::printf ("[3] Estabilidad en extremos (ruido a fondo de escala, 25 s)\n");
    {
        double sr = 48000; ReverbParams p; p.mix = 100; p.decay = 20.f; p.lowRatio = 3.f; p.highRatio = 2.f; p.size = 200; p.modulation = 100;
        p.diffusion = 100; p.width = 200; p.lowGainDb = 12; p.highGainDb = 12; p.lowCutHz = 20; p.highCutHz = 20000;
        std::mt19937 g (1); std::uniform_real_distribution<float> d (-1, 1);
        std::vector<float> x ((size_t) (sr * 25)); for (auto& v : x) v = d (g);
        auto y = run (sr, p, x); double pk = 0; bool fin = true;
        for (float v : y) { fin = fin && std::isfinite (v); pk = std::max (pk, (double) std::abs (v)); }
        check (fin && pk < 60.0, "pico de salida %.2f (finito: %s)", pk, fin ? "si" : "NO");
        std::vector<float> x2 ((size_t) (sr * 60), 0.f); for (size_t i = 0; i < (size_t) sr; ++i) x2[i] = d (g);
        p.decay = 5.f; p.lowRatio = p.highRatio = 1.f; p.size = 100; p.width = 100; p.lowGainDb = p.highGainDb = 0;
        auto y2 = run (sr, p, x2); double tailEnd = 0; for (size_t i = y2.size() - 48000; i < y2.size(); ++i) tailEnd = std::max (tailEnd, (double) std::abs (y2[i]));
        check (tailEnd < 1e-3, "cola a los 59 s con decay 5 s: pico %.2e", tailEnd);
    }

    std::printf ("[4] Nivel (ruido RMS 0.1, decay 2 s, mix 100%%)\n");
    {
        double sr = 48000; ReverbParams p; p.mix = 100; p.decay = 2.f; p.lowRatio = p.highRatio = 1.f;
        std::mt19937 g (2); std::normal_distribution<float> d (0, 0.1f);
        std::vector<float> x ((size_t) (sr * 6)); for (auto& v : x) v = d (g);
        auto y = run (sr, p, x); double ei = 0, eo = 0; size_t n0 = (size_t) (sr * 3);
        for (size_t i = n0; i < x.size(); ++i) { ei += x[i] * x[i]; eo += y[i] * y[i]; }
        double dB = 10 * std::log10 (eo / ei);
        check (dB > -7 && dB < 4, "ganancia humeda/seca %.1f dB", dB);
    }

    std::printf ("[5] Mix 0%% y bypass dejan pasar la senal seca\n");
    {
        double sr = 48000; std::vector<float> x ((size_t) sr); for (size_t i = 0; i < x.size(); ++i) x[i] = 0.3f * std::sin (i * 0.05f);
        ReverbParams p; p.mix = 0; auto y = run (sr, p, x); double err = 0; for (size_t i = 0; i < x.size(); ++i) err = std::max (err, (double) std::abs (y[i] - x[i]));
        check (err < 1e-6, "mix 0: error maximo %.2e", err);
        p.mix = 100; p.bypass = true; y = run (sr, p, x); err = 0; for (size_t i = 0; i < x.size(); ++i) err = std::max (err, (double) std::abs (y[i] - x[i]));
        check (err < 1e-6, "bypass: error maximo %.2e", err);
    }

    std::printf ("[6] Ducker reduce la reverb con senal fuerte\n");
    {
        double sr = 48000; std::vector<float> x ((size_t) (sr * 3)); for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * std::sin (i * 0.06f);
        ReverbParams p; p.mix = 100; p.decay = 3; p.duckAmount = 0; auto a = run (sr, p, x);
        p.duckAmount = 100; p.duckThresholdDb = -30; auto b = run (sr, p, x);
        double ea = 0, eb = 0; for (size_t i = (size_t) (sr * 2); i < x.size(); ++i) { ea += a[i] * a[i]; eb += b[i] * b[i]; }
        double dB = 10 * std::log10 (eb / ea);
        check (dB < -10, "reduccion con ducker 100%%: %.1f dB", dB);
    }

    std::printf ("[7] Mono (1 canal) sin NaN\n");
    {
        double sr = 44100; ReverbEngine e; e.prepare (sr); ReverbParams p; e.setParams (p);
        std::vector<float> m (44100, 0.f); m[0] = 1.f; bool fin = true;
        for (size_t i = 0; i + 512 <= m.size(); i += 512) { float* b[1] = { m.data() + i }; e.process (b, 1, 512); }
        for (float v : m) fin = fin && std::isfinite (v);
        check (fin, "salida mono finita");
    }

    std::printf ("[8] Los %d presets de fabrica: dentro de rango, estables y con cola finita\n", kNumPresets);
    {
        double sr = 48000; int bad = 0;
        std::mt19937 g (3); std::uniform_real_distribution<float> d (-0.5f, 0.5f);
        std::vector<float> x ((size_t) (sr * 30), 0.f); for (size_t i = 0; i < (size_t) (sr * 0.5); ++i) x[i] = d (g);
        const float lo[18] = { 0, 0.1f, 0.1f, 50, 0.1f, 1000, 25, 0, 0, 0, 0, -60, 20, 20, 1000, -12, -12, 0 };
        const float hi[18] = { 250, 20, 3, 2000, 2, 16000, 200, 100, 100, 200, 100, 0, 1000, 1000, 20000, 12, 12, 100 };
        for (int i = 0; i < kNumPresets; ++i)
        {
            bool inRange = true; for (int k = 0; k < 18; ++k) inRange = inRange && kPresets[i].v[k] >= lo[k] && kPresets[i].v[k] <= hi[k];
            auto y = run (sr, fromPreset (kPresets[i]), x); bool fin = true; double pk = 0;
            for (float v : y) { fin = fin && std::isfinite (v); pk = std::max (pk, (double) std::abs (v)); }
            if (!inRange || !fin || pk > 4.0) { ++bad; std::printf ("    preset %d (%s) problema: rango=%d finito=%d pico=%.2f\n", i, kPresets[i].name, inRange, fin, pk); }
        }
        check (bad == 0, "%d presets revisados, %d con problemas", kNumPresets, bad);
        check (ryNumCategories() == 9, "categorias: %d", ryNumCategories());
    }

    std::printf (fails ? "\n%d PRUEBAS FALLARON\n" : "\nTODAS LAS PRUEBAS OK\n", fails);
    return fails != 0;
}
