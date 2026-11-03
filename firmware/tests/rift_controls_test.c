/* SPDX-License-Identifier: GPL-3.0-only
 * Reproduce dial behavior through the full host input/render path. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
static uint32_t blits, pixels;
#define GFX_HOOK_TEXT(x0,y0,x1,y1,s,flags) ((void)0)
#define GFX_HOOK_BLIT(x,y,r0) (blits++, pixels += cv_w * (cv_h - (r0)))
#define GFX_HOOK_BEGIN() ((void)0)
#define GFX_HOOK_PIXELS(n) ((void)0)
#include "../platforms/host/felucca_host.c"
static int fail;
static void check(const char *name, int ok)
{ printf("controls: %s: %s\n", name, ok ? "ok" : "FAIL"); fail += !ok; }
static void run_ms(uint32_t ms)
{ for (uint32_t i = 0; i < ms; i += 10) fh_render(441); }
static uint32_t sound(void)
{
    int32_t out[CTL * 2]; uint32_t h = 2166136261u;
    rift_reset(); rift.active = 1;
    for (uint32_t b = 0; b < FS * 2u; b += CTL) {
        rift_audio(&trk[0], out, CTL);
        if (b > FS) for (uint32_t i = 0; i < CTL * 2u; i++) h = (h ^ (uint32_t)out[i]) * 16777619u;
    }
    return h;
}
int main(void)
{
    uint32_t before, after;
    fh_nor_erase(); fh_master(800); fh_boot(); run_ms(600);
    blits = pixels = 0;
    before = fh_screen_draws(); fh_enc(EN_K1, 1); run_ms(30);
    after = fh_screen_draws();
    printf("controls: one-detent update: %u LCD writes, %u text blits, %u text pixels\n", after-before, blits, pixels);
    check("changing one value redraws only that value", after-before == 1 && blits == 1 && pixels <= 76u * 19u);
    apply_preset_to(&trk[0], 0); run_ms(30);
    before = sound();
    fh_enc(EN_PRESET, 1); run_ms(30); after = sound();
    printf("controls: PRESETS: preset=%u mode=%d page=%u before=%08x after=%08x\n",
           trk[0].preset, trk[0].p[P_E0], rift_page, before, after);
    check("PRESETS recalls a different sound within current algorithm",
          trk[0].preset != 0 && trk[0].p[P_E0] == 0 && before != after);
    check("PRESETS keeps the active control page", rift_page == 0);
    {
        uint32_t root = (uint32_t)trk[0].p[P_CHOR];
        fh_enc(EN_ALGO, 4); run_ms(40);
        check("ALGORITHM recalls current sound variation for the new algorithm",
              trk[0].p[P_E0] == 4 && trk[0].preset == 13);
        check("sound recall preserves root and drone state", trk[0].p[P_CHOR] == root && trk[0].p[P_AHOLD]);
        uint32_t names = 0;
        for (uint32_t k = 0; k < ENG_RIFT.npresets; k++) names += preset_cat(ENGI_RIFT, k) == CAT_PAD;
        check("all 27 sounds are registered as pads", ENG_RIFT.npresets == 27 && names == 27);
        for (uint32_t mode = 0; mode < 9; mode++) {
            uint32_t h[3];
            for (uint32_t variation = 0; variation < 3; variation++) {
                apply_preset_to(&trk[0], mode + 9u * variation); h[variation] = sound();
            }
            check("each algorithm has three distinct rendered sounds", h[0] != h[1] && h[1] != h[2] && h[0] != h[2]);
        }
    }
    {
        uint32_t sounds[9], pairs = 0;
        apply_preset_to(&trk[0], 0);
        for (uint32_t mode = 0; mode < 9; mode++) {
            trk[0].p[P_E0] = (int16_t)mode;
            sounds[mode] = sound();
        }
        for (uint32_t a = 0; a < 9; a++) for (uint32_t b = a+1; b < 9; b++)
            pairs += sounds[a] != sounds[b];
        printf("controls: same settings across algorithms: %u/36 different pairs\n", pairs);
        check("algorithm modes differ even with identical macro settings", pairs == 36);
    }
    /* Separate UI-only changes from a disconnected DSP control. */
    static const uint32_t id[] = {P_E0, P_E5, P_E6, P_E7, P_DLY};
    static const char *label[] = {"ALGORITHM", "TIME", "FORM", "FEED", "CELL"};
    for (uint32_t k = 0; k < 5; k++) {
        apply_preset_to(&trk[0], 0);
        before = sound();
        trk[0].p[id[k]] = k == 0 ? 1 : k == 1 ? 100 : k == 2 ? 120 : k == 3 ? 110 : 90;
        after = sound();
        printf("controls: %s audio %08x -> %08x\n", label[k], before, after);
        check("parameter reaches DSP and changes audio", before != after);
    }
    printf("controls: %d failures\n", fail);
    return fail ? 1 : 0;
}
