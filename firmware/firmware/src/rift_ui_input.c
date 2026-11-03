/* SPDX-License-Identifier: GPL-3.0-only
 * RIFT HOME owns the controls; SAVE and HOME enter the existing storage/menu UI.
 * Keyboard and MIDI continue through the audio thread's ordinary events_block. */
static void rift_recall(track_t *t, uint32_t mode, uint32_t variation)
{
    int16_t root = t->p[P_CHOR], drone = t->p[P_AHOLD], mute = t->p[P_MUTE];
    fm1_irq_off();
    apply_preset_to(t, mode + 9u * variation);
    t->p[P_CHOR] = root; t->p[P_AHOLD] = drone; t->p[P_MUTE] = mute;
    fm1_irq_on();
}
static int rift_input(uint32_t pressed)
{
    track_t *t = &trk[0];
    uint32_t k, b;
    int32_t s;
    static uint32_t random = 0x7357ba19u;
    static uint32_t navigation;
    if (!rift_home() || ui.uboot) return 0;
    song.sel = 0; song.seq_mode = 0; song.grid = 0;
    if ((s = panel_enc(EN_ALGO)) != 0) {
        uint32_t mode = (uint32_t)clamp(t->p[P_E0] + s, 0, 8);
        if (mode != (uint32_t)t->p[P_E0]) rift_recall(t, mode, (t->preset / 9u) % 3u);
    }
    if ((s = panel_enc(EN_SELECT)) != 0)
        t->p[P_CHOR] = (int16_t)clamp(t->p[P_CHOR] + s, 0, 108);
    if ((s = panel_enc(EN_PRESET)) != 0) {
        uint32_t variation = (uint32_t)(((int32_t)(t->preset / 9u) + s % 3 + 3) % 3);
        rift_recall(t, (uint32_t)clamp(t->p[P_E0], 0, 8), variation);
    }
    for (k = 0; k < 4u; k++) if ((s = panel_enc(EN_K1 + k)) != 0) {
        uint32_t id = RIFT_PAGE_IDS[rift_page][k];
        t->p[id] = (int16_t)clamp(t->p[id] + s, 0, 127);
    }
    for (k = 0; k < NB; k++) if ((pressed >> k) & 1u) {
        b = panel_btn_of(k);
        if (b == B_HOME || b == B_SAVE || b == B_GLO) navigation |= 1u << b;
        else if (b == B_ENV || b == B_EDIT) rift_page = 0;
        else if (b == B_FX) rift_page = 1;
        else if (b == B_LFO) rift_page = 2;
        else if (b == B_ARP) t->p[P_AHOLD] ^= 1;
        else if (b == B_PLAY) t->p[P_MUTE] ^= 1;
        else if (b == B_REC) {
            random = rift_hash(random + fm1_ms);
            t->p[P_LPHASE] = (int16_t)(random & 127u);
            t->p[P_LRATE] = (int16_t)((random >> 8) & 127u);
        } else if (b == B_OCTDN || b == B_OCTUP) {
            t->p[P_CHOR] = (int16_t)clamp(t->p[P_CHOR] + (b == B_OCTDN ? -12 : 12), 0, 108);
            song.octave = (int8_t)clamp(song.octave + (b == B_OCTDN ? -1 : 1), -3, 3);
        }
    }
    /* Enter another UI only after release, so its hold/tap handlers cannot
     * interpret the same physical press a second time. */
    for (b = 0; b < NB; b++) if ((navigation >> b) & 1u) {
        if ((fm1_in.buttons >> panel.btn[b]) & 1u) continue;
        navigation &= ~(1u << b);
        if (b == B_HOME) { ui.menu = 1; ui.force = 1; }
        else if (b == B_SAVE) page_tap(B_SAVE);
        else if (b == B_GLO) { ui.home = 0; open_family(FAM_EDIT); }
    }
    return 1;
}
