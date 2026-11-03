/* SPDX-License-Identifier: GPL-3.0-only */
static uint32_t rift_page;
static const uint8_t RIFT_PAGE_IDS[3][4] = {
    {P_E1, P_E2, P_E3, P_E4}, {P_E5, P_E6, P_E7, P_DLY}, {P_LPHASE, P_LRATE, P_DIST, P_REV}
};
static const char *const RIFT_PAGE_LABELS[3][4] = {
    {"MORPH", "SPAN", "WARP", "FIELD"}, {"TIME", "FORM", "FEED", "CELL"}, {"SEED", "SCAN", "FUSE", "MIX"}
};
static int rift_home(void)
{
    return trk[0].eng_req == ENGI_RIFT && ui.home && !ui.menu && !ui.confirm && !ui.ly;
}
static void rift_draw(void)
{
    static struct {
        uint32_t ready, page, palette;
        int16_t mode, preset, basis, drone, mute, value[4];
    } shown;
    uint32_t k, full, page_changed, mode;
    track_t *t = &trk[0];
    char value[16], line[48];
    full = !shown.ready || ui.force || shown.palette != ux.gen;
    page_changed = full || shown.page != rift_page;
    mode = (uint32_t)clamp(t->p[P_E0], 0, 8);
    if (full) {
        lcd_fill(0, 0, 240, 240, T_BG);
        draw_text_line(10, 202, 220, &AF_S, "ENV: GEN  FX: FX  LFO: VAR", T_MID, T_BG, 0);
        draw_text_line(10, 220, 220, &AF_S, "PRESETS: SOUND  SELECT: BASIS", T_MID, T_BG, 0);
    }
    if (full || shown.preset != (int16_t)t->preset) {
        str_cpy(line, "RIFT 0.2 / FM1   SOUND ", sizeof line);
        fmt_int(line + str_len(line), (t->preset / 9u) % 3u + 1u);
        str_cpy(line + str_len(line), "/3", sizeof line - str_len(line));
        draw_text_line(10, 5, 220, &AF_S, line, T_MID, T_BG, 0);
    }
    if (full || shown.mode != (int16_t)mode)
        draw_text_line(10, 24, 220, &AF_L, RIFT_MODES[mode], T_THEME, T_BG, 0);
    if (page_changed || shown.basis != t->p[P_CHOR] || shown.mute != t->p[P_MUTE] || shown.drone != t->p[P_AHOLD]) {
        str_cpy(line, t->p[P_MUTE] ? "GEN OFF " : t->p[P_AHOLD] ? "DRONE  " : "KEYS   ", sizeof line);
        fmt_int(line + str_len(line), t->p[P_CHOR]);
        str_cpy(line + str_len(line), " MIDI / ", sizeof line - str_len(line));
        str_cpy(line + str_len(line), rift_page == 0 ? "GEN" : rift_page == 1 ? "FX" : "VAR", sizeof line - str_len(line));
        draw_text_line(10, 60, 220, &AF_S, line, T_TEXT, T_BG, 0);
    }
    for (k = 0; k < 4u; k++) {
        int32_t v = t->p[RIFT_PAGE_IDS[rift_page][k]];
        uint32_t y = 84u + k * 28u;
        if (!page_changed && shown.value[k] == v) continue;
        shown.value[k] = (int16_t)v;
        if ((rift_page == 0u && k == 2u) || (rift_page == 1u && k == 3u)) v -= 64;
        fmt_int(value, v);
        if (page_changed)
            draw_text_line(10, y, 140, &AF_M, RIFT_PAGE_LABELS[rift_page][k], T_TEXT, T_BG, 0);
        draw_text_line(154, y, 76, &AF_M, value, T_THEME, T_BG, 1);
    }
    shown.ready = 1; shown.page = rift_page; shown.palette = ux.gen;
    shown.mode = (int16_t)mode; shown.preset = (int16_t)t->preset; shown.basis = t->p[P_CHOR];
    shown.drone = t->p[P_AHOLD]; shown.mute = t->p[P_MUTE];
    ui.force = 0;
}
