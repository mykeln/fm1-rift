/* SPDX-License-Identifier: GPL-3.0-only
 * RIFT: original VHIKK-X-inspired drone instrument. No manufacturer code or tables.
 * The engine descriptor supplies storage and note handling; fx.c runs its stereo
 * processor once, using the existing delay allocation rather than extra SRAM.
 * This is an exclusive instrument on part 1, not a four-part Felucca engine. */
static const char *const RIFT_MODES[] = {
    "PAIR", "FOLD", "SINES", "WAVES", "CHORD", "CLUSTER", "PARTIAL", "CROSS", "CAVERN"
};
static void rift_on(track_t *t, voice_t *v) { t->p[P_CHOR] = v->note; }
static void rift_voice(track_t *t, voice_t *v, int32_t *o, uint32_t n, const vmod_t *m)
{ (void)t; (void)v; (void)o; (void)n; (void)m; }
static const preset_t RIFT_PRESETS[] = {
    {"PAIR",    {0, 32, 20, 64, 90, 74, 28, 72}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 64)},
    {"FOLD",    {1, 70, 32, 80, 76, 48, 40, 74}, {10, 0, 127, 45}, 0, 1, FX(56, 36, 64, 70)},
    {"SINES",   {2, 54, 50, 64, 86, 86, 78, 80}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 75)},
    {"WAVES",   {3, 82, 40, 72, 80, 68, 68, 76}, {10, 0, 127, 45}, 0, 1, FX(32, 36, 64, 70)},
    {"CHORD",   {4, 45, 64, 64, 94, 90, 88, 80}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 80)},
    {"CLUSTER", {5, 68, 76, 70, 80, 64, 60, 78}, {10, 0, 127, 45}, 0, 1, FX(30, 36, 66, 75)},
    {"PARTIAL", {6, 45, 50, 64, 90, 80, 75, 76}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 72)},
    {"CROSS",   {7, 85, 50, 90, 74, 50, 52, 72}, {10, 0, 127, 45}, 0, 1, FX(42, 36, 68, 70)},
    {"CAVERN",  {8, 78, 46, 78, 102, 110, 112, 98}, {10, 0, 127, 55}, 0, 1, FX(36, 36, 64, 104)},
    /* Two more sound banks, indexed mode + 9 * variation. Original indices 0..8 stay valid. */
    {"PAIR AIR", {0, 12, 60, 56, 106, 106, 94, 96}, {10, 0, 127, 45}, 0, 1, FX(35, 36, 75, 90)},
    {"FOLD DUST",{1, 108, 84, 35, 62, 32, 100, 94}, {10, 0, 127, 45}, 0, 1, FX(100, 36, 88, 90)},
    {"SINE MIST",{2, 100, 105, 88, 108, 110, 114, 106}, {10, 0, 127, 45}, 0, 1, FX(42, 36, 78, 100)},
    {"WAVE HAZE",{3, 16, 90, 36, 100, 114, 106, 94}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 48, 90)},
    {"CHORD AIR",{4, 108, 100, 42, 104, 106, 112, 104}, {10, 0, 127, 45}, 0, 1, FX(38, 36, 76, 100)},
    {"CLUST FOG",{5, 18, 110, 42, 96, 110, 116, 100}, {10, 0, 127, 45}, 0, 1, FX(16, 36, 45, 100)},
    {"PART AIR", {6, 100, 96, 42, 110, 106, 112, 106}, {10, 0, 127, 45}, 0, 1, FX(50, 36, 78, 100)},
    {"CROSS AIR",{7, 20, 110, 32, 100, 108, 112, 100}, {10, 0, 127, 45}, 0, 1, FX(72, 36, 48, 96)},
    {"CAVE MIST",{8, 10, 105, 40, 104, 120, 122, 110}, {10, 0, 127, 55}, 0, 1, FX(46, 36, 76, 112)},
    {"PAIR IRON",{0, 118, 115, 112, 72, 18, 24, 114}, {10, 0, 127, 45}, 0, 1, FX(92, 36, 100, 104)},
    {"FOLD IRON",{1, 30, 115, 118, 108, 18, 18, 114}, {10, 0, 127, 45}, 0, 1, FX(120, 36, 20, 108)},
    {"SINE IRON",{2, 20, 118, 118, 80, 20, 12, 116}, {10, 0, 127, 45}, 0, 1, FX(108, 36, 112, 104)},
    {"WAVE IRON",{3, 120, 118, 114, 70, 18, 14, 112}, {10, 0, 127, 45}, 0, 1, FX(100, 36, 16, 104)},
    {"CHORD IRON",{4, 118, 120, 114, 74, 20, 20, 116}, {10, 0, 127, 45}, 0, 1, FX(96, 36, 110, 104)},
    {"CLUST IRON",{5, 120, 120, 116, 70, 20, 18, 114}, {10, 0, 127, 45}, 0, 1, FX(108, 36, 18, 104)},
    {"PART IRON",{6, 18, 118, 116, 76, 18, 16, 118}, {10, 0, 127, 45}, 0, 1, FX(114, 36, 112, 108)},
    {"CROSS IRON",{7, 122, 118, 120, 68, 20, 20, 114}, {10, 0, 127, 45}, 0, 1, FX(112, 36, 18, 108)},
    {"CAVE IRON",{8, 118, 120, 112, 72, 20, 28, 118}, {10, 0, 127, 55}, 0, 1, FX(100, 36, 108, 112)},
};
static const engine_t ENG_RIFT = {
    .name = "RIFT", .page_title = {"GENERATOR", "PROCESSOR"},
    .edit = {
        {"ALGO", F_ENUM, 0, 8, 0, RIFT_MODES, 0},
        {"MORPH", F_PCT, 0, 127, 32, 0, 0},
        {"SPAN", F_PCT, 0, 127, 20, 0, 0},
        {"WARP", F_PCT, 0, 127, 64, 0, 0},
        {"FIELD", F_CUTOFF, 0, 127, 90, 0, 0},
        {"TIME", F_PCT, 0, 127, 74, 0, 0},
        {"FORM", F_PCT, 0, 127, 28, 0, 0},
        {"FEED", F_PCT, 0, 127, 72, 0, 0},
    },
    .presets = RIFT_PRESETS, .npresets = NELEM(RIFT_PRESETS),
    .knob = {P_E1, P_E2, P_E3, P_E4}, .poly = 1,
    .note_on = rift_on, .render = rift_voice,
};
