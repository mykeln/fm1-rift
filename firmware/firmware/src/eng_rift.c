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
    {"PAIR",    {0, 32, 74, 64, 45, 74, 28, 100}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 64)},
    {"FOLD",    {1, 44, 76, 68, 45, 104, 92, 103}, {10, 0, 127, 45}, 0, 1, FX(46, 36, 64, 76)},
    {"SINES",   {2, 54, 89, 64, 43, 86, 78, 104}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 75)},
    {"WAVES",   {3, 82, 84, 72, 40, 68, 68, 102}, {10, 0, 127, 45}, 0, 1, FX(32, 36, 64, 70)},
    {"CHORD",   {4, 45, 96, 64, 47, 90, 88, 104}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 72)},
    {"CLUSTER", {5, 68, 102, 70, 40, 64, 60, 103}, {10, 0, 127, 45}, 0, 1, FX(30, 36, 66, 75)},
    {"PARTIAL", {6, 45, 89, 64, 45, 80, 75, 102}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 64, 72)},
    {"CROSS",   {7, 85, 89, 70, 47, 94, 88, 106}, {10, 0, 127, 45}, 0, 1, FX(42, 36, 64, 80)},
    {"CAVERN",  {8, 48, 91, 66, 49, 116, 112, 110}, {10, 0, 127, 55}, 0, 1, FX(22, 36, 64, 64)},
    /* mode + 9 * variation: stable indices, complementary voicings. Large
     * spaces use near-neutral CELL/WARP so the pitched source remains clear.
     * Keep a dry anchor; high FEED supplies the tail rather than more drive. */
    {"PAIR HALO",{0, 18, 80, 62, 49, 116, 104, 107}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 65, 52)},
    {"FOLD GLOW",{1, 22, 78, 62, 46, 114, 104, 106}, {10, 0, 127, 45}, 0, 1, FX(64, 36, 64, 52)},
    {"SINE VEIL",{2, 68, 76, 64, 49, 120, 108, 106}, {10, 0, 127, 45}, 0, 1, FX(14, 36, 65, 80)},
    {"WAVE HAZE",{3, 16, 90, 58, 50, 114, 106, 104}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 64, 78)},
    {"CHORD AIR",{4, 28, 88, 64, 47, 118, 110, 111}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 64, 86)},
    {"CLUST VEIL",{5, 34, 93, 64, 48, 118, 108, 100}, {10, 0, 127, 45}, 0, 1, FX(18, 36, 63, 64)},
    {"PART HALO",{6, 76, 96, 64, 48, 116, 106, 111}, {10, 0, 127, 45}, 0, 1, FX(16, 36, 64, 84)},
    {"CROSS VEIL",{7, 24, 96, 64, 47, 116, 110, 110}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 86)},
    {"CAVE CHOIR",{8, 26, 102, 64, 50, 122, 116, 116}, {10, 0, 127, 55}, 0, 1, FX(18, 36, 64, 84)},
    {"PAIR EMBER",{0, 64, 100, 72, 43, 110, 94, 105}, {10, 0, 127, 45}, 0, 1, FX(32, 36, 65, 76)},
    {"FOLD AMBER",{1, 58, 92, 74, 43, 98, 90, 107}, {10, 0, 127, 45}, 0, 1, FX(78, 36, 65, 78)},
    {"SINE ORBIT",{2, 32, 96, 70, 46, 114, 100, 105}, {10, 0, 127, 45}, 0, 1, FX(20, 36, 63, 56)},
    {"WAVE TIDE",{3, 54, 92, 68, 48, 112, 108, 110}, {10, 0, 127, 45}, 0, 1, FX(36, 36, 64, 68)},
    {"CHORD DUSK",{4, 70, 96, 68, 44, 110, 96, 109}, {10, 0, 127, 45}, 0, 1, FX(26, 36, 64, 80)},
    {"CLUST AURA",{5, 92, 115, 66, 45, 106, 98, 109}, {10, 0, 127, 45}, 0, 1, FX(24, 36, 64, 80)},
    {"PART PRISM",{6, 82, 99, 68, 45, 108, 94, 100}, {10, 0, 127, 45}, 0, 1, FX(22, 36, 64, 64)},
    {"CROSS OPAL",{7, 56, 84, 76, 44, 104, 94, 108}, {10, 0, 127, 45}, 0, 1, FX(40, 36, 65, 78)},
    {"CAVE VAULT",{8, 78, 112, 70, 46, 108, 104, 113}, {10, 0, 127, 55}, 0, 1, FX(28, 36, 64, 80)},
};
static const engine_t ENG_RIFT = {
    .name = "RIFT", .page_title = {"GENERATOR", "PROCESSOR"},
    .edit = {
        {"ALGO", F_ENUM, 0, 8, 0, RIFT_MODES, 0},
        {"MORPH", F_PCT, 0, 127, 32, 0, 0},
        {"SPAN", F_PCT, 0, 127, 74, 0, 0},
        {"WARP", F_PCT, 0, 127, 64, 0, 0},
        {"FIELD", F_PCT, 0, 127, 45, 0, 0},
        {"TIME", F_PCT, 0, 127, 74, 0, 0},
        {"FORM", F_PCT, 0, 127, 28, 0, 0},
        {"FEED", F_PCT, 0, 127, 100, 0, 0},
    },
    .presets = RIFT_PRESETS, .npresets = NELEM(RIFT_PRESETS),
    .knob = {P_E1, P_E2, P_E3, P_E4}, .poly = 1,
    .note_on = rift_on, .render = rift_voice,
};
