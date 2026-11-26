#pragma once
#include <cstring>

// Orden de los valores de cada preset (mismo orden que kPresetParamIds):
// predelay, decay, lowRatio, lowFreq, highRatio, highFreq, size, diffusion, modulation, width,
// duckAmount, duckThreshold, duckRelease, lowCut, highCut, lowGain, highGain, mix
struct RyPreset { const char* category; const char* name; float v[18]; };

inline const char* const kPresetParamIds[18] = {
    "predelay", "decay", "lowratio", "lowfreq", "highratio", "highfreq", "size", "diffusion", "modulation", "width",
    "duckamt", "duckthr", "duckrel", "lowcut", "highcut", "lowgain", "highgain", "mix" };

inline const RyPreset kPresets[] = {
    // ---- Default
    { "Default", "Init Preset",   { 20, 2.5f, 1.15f, 400, 0.85f, 4000, 100, 70, 25, 100, 0, -24, 250, 20, 16000, 0, 0, 30 } },
    // ---- Halls
    { "Halls", "Concert Hall",    { 25, 2.8f, 1.10f, 500, 0.75f, 5000, 110, 75, 30, 110, 0, -24, 250, 40, 14000, 0, -1, 30 } },
    { "Halls", "Marble Hall",     { 30, 3.6f, 0.95f, 600, 0.90f, 7000, 125, 70, 20, 120, 0, -24, 250, 60, 16000, -1, 1.5f, 28 } },
    { "Halls", "Grand Cathedral", { 45, 7.5f, 1.25f, 350, 0.55f, 3500, 160, 80, 35, 130, 0, -24, 300, 80, 9000, 0, -2, 28 } },
    { "Halls", "Warm Hall",       { 20, 3.0f, 1.35f, 450, 0.60f, 3000, 105, 70, 28, 100, 0, -24, 250, 50, 8000, 1.5f, -2.5f, 30 } },
    { "Halls", "Bright Hall",     { 18, 2.4f, 0.90f, 700, 1.00f, 9000, 100, 75, 22, 110, 0, -24, 250, 70, 18000, -1, 2.5f, 26 } },
    // ---- Rooms
    { "Rooms", "Small Room",      { 4, 0.55f, 1.00f, 500, 0.70f, 5000, 55, 60, 8, 90, 0, -24, 150, 100, 12000, 0, -1, 22 } },
    { "Rooms", "Wood Studio",     { 8, 0.9f, 1.20f, 450, 0.65f, 4500, 70, 65, 10, 95, 0, -24, 180, 90, 10000, 1, -1.5f, 24 } },
    { "Rooms", "Live Room",       { 10, 1.3f, 1.05f, 500, 0.80f, 6000, 85, 70, 15, 105, 0, -24, 200, 80, 13000, 0, 0, 26 } },
    { "Rooms", "Tiled Bathroom",  { 2, 1.1f, 0.80f, 800, 1.10f, 9000, 45, 55, 6, 100, 0, -24, 150, 150, 18000, -2, 3, 24 } },
    // ---- Plates
    { "Plates", "Vintage Plate",  { 0, 2.0f, 0.85f, 600, 0.90f, 8000, 90, 95, 35, 120, 0, -24, 200, 120, 15000, -1.5f, 1, 28 } },
    { "Plates", "Bright Plate",   { 0, 1.8f, 0.80f, 800, 1.20f, 12000, 85, 95, 30, 125, 0, -24, 200, 140, 19000, -2, 3, 26 } },
    { "Plates", "Dark Plate",     { 0, 2.4f, 1.20f, 400, 0.50f, 3000, 95, 90, 30, 110, 0, -24, 200, 90, 7000, 1, -3, 28 } },
    { "Plates", "Short Plate",    { 0, 0.8f, 0.90f, 600, 0.85f, 7000, 70, 95, 25, 115, 0, -24, 150, 150, 15000, -1, 1, 25 } },
    // ---- Chambers
    { "Chambers", "Stone Chamber",{ 12, 2.2f, 1.00f, 500, 0.70f, 5000, 80, 65, 12, 100, 0, -24, 250, 70, 11000, 0, -1, 28 } },
    { "Chambers", "Echo Chamber", { 20, 1.6f, 1.10f, 450, 0.60f, 3800, 75, 40, 18, 90, 0, -24, 250, 100, 8000, 0.5f, -2, 30 } },
    { "Chambers", "Tight Chamber",{ 6, 1.0f, 1.00f, 550, 0.75f, 6000, 60, 60, 10, 95, 0, -24, 180, 110, 12000, 0, -0.5f, 24 } },
    // ---- Vocals
    { "Vocals", "Vocal Air",      { 30, 1.8f, 0.80f, 800, 1.10f, 10000, 90, 85, 40, 130, 0, -24, 250, 180, 18000, -3, 2.5f, 22 } },
    { "Vocals", "Lead Vocal Duck",{ 45, 2.2f, 0.90f, 700, 0.90f, 7500, 100, 80, 30, 115, 55, -28, 350, 150, 16000, -2, 1.5f, 28 } },
    { "Vocals", "Pop Vocal",      { 25, 1.5f, 0.85f, 700, 1.00f, 9000, 85, 85, 30, 120, 25, -26, 250, 160, 17000, -2, 2, 24 } },
    { "Vocals", "Ballad Vocal",   { 40, 3.2f, 1.10f, 500, 0.75f, 5500, 115, 80, 35, 125, 35, -28, 400, 120, 12000, -1, 0.5f, 30 } },
    { "Vocals", "Whisper Space",  { 55, 4.0f, 0.70f, 900, 0.90f, 8000, 120, 90, 45, 160, 70, -30, 500, 200, 14000, -3, 1, 20 } },
    // ---- Drums
    { "Drums", "Snare Room",      { 0, 0.7f, 0.90f, 600, 0.85f, 7000, 60, 70, 5, 100, 0, -24, 150, 150, 14000, -1, 1, 26 } },
    { "Drums", "Big Snare",       { 5, 1.6f, 0.85f, 700, 0.95f, 9000, 95, 80, 12, 115, 0, -24, 200, 180, 16000, -2, 2, 28 } },
    { "Drums", "Drum Glue",       { 0, 0.45f, 1.00f, 500, 0.80f, 6000, 45, 60, 5, 90, 0, -24, 120, 120, 12000, 0, 0, 18 } },
    // ---- Ambient
    { "Ambient", "Deep Space",    { 60, 12.0f, 1.40f, 300, 0.50f, 3500, 180, 90, 55, 180, 0, -24, 400, 100, 9000, 1, -2, 40 } },
    { "Ambient", "Shimmer Void",  { 80, 9.0f, 1.00f, 400, 1.20f, 12000, 150, 95, 75, 200, 0, -24, 400, 150, 19000, -2, 3, 38 } },
    { "Ambient", "Infinite Blue", { 70, 16.0f, 1.30f, 350, 0.60f, 4000, 170, 90, 60, 170, 0, -24, 500, 90, 10000, 1, -1, 45 } },
    { "Ambient", "Frozen Lake",   { 35, 6.0f, 0.70f, 700, 1.30f, 14000, 130, 92, 45, 160, 0, -24, 300, 200, 19500, -3, 4, 34 } },
    // ---- Stadium
    { "Stadium", "Steel Giant",   { 50, 5.5f, 1.10f, 400, 0.65f, 4500, 170, 60, 14, 140, 0, -24, 300, 90, 10000, 0, -1.5f, 30 } },
    { "Stadium", "Tunnel Walk",   { 10, 2.0f, 1.30f, 300, 0.55f, 3500, 60, 50, 20, 70, 0, -24, 200, 80, 8000, 2, -2, 34 } },
    { "Stadium", "Upper Stands",  { 70, 4.2f, 0.90f, 500, 0.80f, 6000, 150, 45, 12, 150, 0, -24, 300, 120, 11000, -1, 0, 28 } },
    { "Stadium", "Final Whistle", { 35, 3.4f, 1.00f, 450, 0.85f, 7000, 130, 55, 18, 135, 30, -26, 300, 100, 13000, 0, 1, 28 } },
};

inline constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

// ---- Utilidades de categorias ----
inline int ryNumCategories()
{
    int n = 0;
    for (int i = 0; i < kNumPresets; ++i)
        if (i == 0 || std::strcmp (kPresets[i].category, kPresets[i - 1].category) != 0) ++n;
    return n;
}
inline int ryCategoryOf (int preset)
{
    int c = -1;
    for (int i = 0; i <= preset && i < kNumPresets; ++i)
        if (i == 0 || std::strcmp (kPresets[i].category, kPresets[i - 1].category) != 0) ++c;
    return c;
}
inline int ryFirstOfCategory (int cat)
{
    int c = -1;
    for (int i = 0; i < kNumPresets; ++i)
    {
        if (i == 0 || std::strcmp (kPresets[i].category, kPresets[i - 1].category) != 0) ++c;
        if (c == cat) return i;
    }
    return 0;
}
inline int ryCountInCategory (int cat)
{
    const int first = ryFirstOfCategory (cat);
    int n = 0;
    while (first + n < kNumPresets && std::strcmp (kPresets[first + n].category, kPresets[first].category) == 0) ++n;
    return n;
}
