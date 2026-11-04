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
    {"FOLD",    {1, 44, 24, 68, 90, 104, 92, 78}, {10, 0, 127, 45}, 0, 1, FX(46, 36, 64, 76)},
    {"SINES",   {2, 54, 50, 64, 86, 86, 78, 80}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 75)},
    {"WAVES",   {3, 82, 40, 72, 80, 68, 68, 76}, {10, 0, 127, 45}, 0, 1, FX(32, 36, 64, 70)},
    {"CHORD",   {4, 45, 64, 64, 94, 90, 88, 80}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 80)},
    {"CLUSTER", {5, 68, 76, 70, 80, 64, 60, 78}, {10, 0, 127, 45}, 0, 1, FX(30, 36, 66, 75)},
    {"PARTIAL", {6, 45, 50, 64, 90, 80, 75, 76}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 72)},
    {"CROSS",   {7, 85, 50, 90, 74, 50, 52, 72}, {10, 0, 127, 45}, 0, 1, FX(42, 36, 68, 70)},
    {"CAVERN",  {8, 48, 54, 66, 98, 116, 112, 100}, {10, 0, 127, 55}, 0, 1, FX(22, 36, 65, 80)},
    /* mode + 9 * variation: stable indices, complementary voicings. Large
     * spaces use near-neutral CELL/WARP so the pitched source remains clear.
     * Keep a dry anchor; high FEED supplies the tail rather than more drive. */
    {"PAIR HALO",{0, 18, 32, 62, 98, 116, 104, 86}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 65, 52)},
    {"FOLD GLOW",{1, 22, 28, 62, 92, 114, 104, 84}, {10, 0, 127, 45}, 0, 1, FX(64, 36, 64, 52)},
    {"SINE VEIL",{2, 68, 24, 64, 98, 120, 108, 84}, {10, 0, 127, 45}, 0, 1, FX(14, 36, 65, 80)},
    {"WAVE HAZE",{3, 16, 90, 36, 100, 114, 106, 94}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 48, 90)},
    {"CHORD AIR",{4, 28, 48, 64, 94, 118, 110, 94}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 64, 86)},
    {"CLUST VEIL",{5, 34, 58, 64, 96, 118, 108, 84}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 63, 80)},
    {"PART HALO",{6, 76, 64, 64, 96, 116, 106, 94}, {10, 0, 127, 45}, 0, 1, FX(16, 36, 64, 84)},
    {"CROSS VEIL",{7, 24, 64, 64, 94, 116, 110, 92}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 86)},
    {"CAVE CHOIR",{8, 26, 76, 64, 100, 122, 116, 104}, {10, 0, 127, 55}, 0, 1, FX(18, 36, 63, 84)},
    {"PAIR EMBER",{0, 64, 72, 72, 86, 110, 94, 82}, {10, 0, 127, 45}, 0, 1, FX(32, 36, 65, 76)},
    {"FOLD AMBER",{1, 58, 56, 74, 86, 98, 90, 86}, {10, 0, 127, 45}, 0, 1, FX(78, 36, 65, 78)},
    {"SINE ORBIT",{2, 32, 64, 70, 92, 114, 100, 82}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 63, 56)},
    {"WAVE TIDE",{3, 54, 56, 68, 90, 112, 108, 92}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 84)},
    {"CHORD DUSK",{4, 70, 64, 68, 88, 110, 96, 90}, {10, 0, 127, 45}, 0, 1, FX(26, 36, 64, 80)},
    {"CLUST AURA",{5, 92, 102, 66, 90, 106, 98, 90}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 80)},
    {"PART PRISM",{6, 100, 70, 68, 90, 108, 94, 90}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 78)},
    {"CROSS OPAL",{7, 56, 40, 76, 88, 104, 94, 88}, {10, 0, 127, 45}, 0, 1, FX(40, 36, 65, 78)},
    {"CAVE VAULT",{8, 78, 96, 70, 92, 108, 104, 98}, {10, 0, 127, 55}, 0, 1, FX(28, 36, 65, 80)},
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
