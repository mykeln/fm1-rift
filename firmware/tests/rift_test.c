/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../platforms/host/felucca_host.c"
static int failures;
static void check(const char *what, int ok)
{ printf("rift: %s: %s\n", what, ok ? "ok" : "FAIL"); failures += !ok; }
static void run_ms(uint32_t ms)
{ uint32_t k; for (k = 0; k < ms; k += 10) fh_render(441); }
static void tap(uint32_t b)
{ fh_buttons(1u << b); run_ms(30); fh_buttons(0); run_ms(30); }
static void screenshot(const char *dir, const char *name)
{
    char path[512]; uint32_t i; FILE *f;
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    f = fopen(path, "wb"); if (!f) exit(2);
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u*240u; i++) {
        uint16_t v = fh_screen()[i];
        unsigned char p[3];
        v = (uint16_t)((v >> 8) | (v << 8));
        p[0] = (unsigned char)(((v >> 11) & 31u) * 255u / 31u);
        p[1] = (unsigned char)(((v >> 5) & 63u) * 255u / 63u);
        p[2] = (unsigned char)((v & 31u) * 255u / 31u);
        fwrite(p, 1, 3, f);
    }
    fclose(f);
}
static void wav_header(FILE *f, uint32_t frames)
{
    uint32_t h[] = {36 + frames * 4, 16, 44100, 176400, frames * 4};
    uint16_t s[] = {1, 2, 4, 16};
    fwrite("RIFF",1,4,f); fwrite(h,4,1,f); fwrite("WAVEfmt ",1,8,f);
    fwrite(h+1,4,1,f); fwrite(s,2,2,f); fwrite(h+2,4,2,f); fwrite(s+2,2,2,f);
    fwrite("data",1,4,f); fwrite(h+4,4,1,f);
}
int main(int argc, char **argv)
{
    uint32_t mode, b, i, distinct = 0, hashes[9], random = 0x745134u;
    int32_t out[CTL * 2];
    clock_t started;
    fh_nor_erase(); fh_master(800); fh_boot(); run_ms(600);
    if (argc > 1) screenshot(argv[1], "generator");
    check("boots into RIFT drone on part 1", trk[0].engine == ENGI_RIFT && trk[0].p[P_AHOLD]);
    fh_enc(EN_K1, 7); run_ms(30);
    check("generator knob edits MORPH", trk[0].p[P_E1] == 39);
    tap(B_FX); fh_enc(EN_K1, 5); run_ms(30);
    if (argc > 1) screenshot(argv[1], "processor");
    check("FX page knob edits TIME", rift_page == 1 && trk[0].p[P_E5] == 79);
    tap(B_LFO); fh_enc(EN_K1, 3); run_ms(30);
    if (argc > 1) screenshot(argv[1], "variation");
    check("variation page edits SEED", rift_page == 2 && trk[0].p[P_LPHASE] == 27);
    fh_enc(EN_ALGO, 4); run_ms(30);
    check("algorithm knob selects a topology", trk[0].p[P_E0] == 4);
    fh_midi(0xB0, 24, 100); run_ms(30);
    check("MIDI CC24 controls FIELD", trk[0].p[P_E4] == 100);
    fh_midi(0x90, 48, 100); run_ms(30);
    check("MIDI note changes BASIS", trk[0].p[P_CHOR] == 48);
    fh_midi(0x90, 55, 100); run_ms(30);
    check("overlapping legato note changes BASIS", trk[0].p[P_CHOR] == 55);
    fh_midi(0x80, 55, 0); run_ms(30);
    check("releasing top note returns to held note", trk[0].p[P_CHOR] == 48);
    fh_midi(0x80, 48, 0); run_ms(30);
    check("released key leaves drone running", trk[0].p[P_AHOLD] && !trk[0].v[0].gate);
    tap(B_HOME);
    check("HOME opens menu and it stays open after release", ui.menu == 1);
    tap(B_HOME);
    check("HOME exits menu back to RIFT", !ui.menu && rift_home());
    tap(B_SAVE);
    check("SAVE opens storage UI after release", !ui.home && cur_page()->fam == FAM_SAVE);
    tap(B_HOME);
    check("HOME returns from storage UI", rift_home());
    tap(B_GLO);
    check("GLO opens standard editor", !ui.home && cur_page()->fam == FAM_EDIT);
    tap(B_HOME);
    check("HOME returns from standard editor", rift_home());
    fh_keys(1); run_ms(50);
    check("physical keyboard selects its mapped pitch", trk[0].p[P_CHOR] == 53);
    fh_keys(0); run_ms(50);
    {
        int16_t saved[P_COUNT];
        memcpy(saved, trk[0].p, sizeof saved);
        check("project saves RIFT macro state", project_save(0) == 0);
        trk[0].p[P_E1] = 1; trk[0].p[P_CHOR] = 1;
        project_load(0); run_ms(80);
        check("project restores all RIFT macros", !memcmp(saved, trk[0].p, sizeof saved));
    }
    set_engine_of(&trk[0], 0); run_ms(250);
    check("switching to standard engine releases RIFT delay ownership", trk[0].engine == 0 && !rift.active);
    set_engine_of(&trk[0], ENGI_RIFT); run_ms(250);
    check("switching back enters RIFT without a stalled fade", trk[0].engine == ENGI_RIFT && rift.active && !trk[0].xf_on);
    tap(B_ENV);
    for (mode = 0; mode < 9; mode++) {
        uint32_t h = 2166136261u, stereo = 0, peak = 0;
        double energy = 0, mean = 0;
        FILE *f = 0; char path[512];
        apply_preset_to(&trk[0], mode); rift_reset(); rift.active = 1;
        if (argc > 1) {
            snprintf(path, sizeof path, "%s/%02u-%s.wav", argv[1], mode + 1u, RIFT_MODES[mode]);
            f = fopen(path, "wb"); if (!f) return 2; wav_header(f, FS * 4u);
        }
        for (b = 0; b < FS * 4u; b += CTL) {
            uint32_t n = FS * 4u - b < CTL ? FS * 4u - b : CTL;
            rift_audio(&trk[0], out, n);
            for (i = 0; i < n * 2u; i++) {
                int32_t a = out[i], abs = a < 0 ? -a : a;
                int16_t sample = (int16_t)a;
                if ((uint32_t)abs > peak) peak = (uint32_t)abs;
                h = (h ^ (uint32_t)a) * 16777619u;
                energy += (double)a * a; mean += a;
                if (f) fwrite(&sample, 2, 1, f);
            }
            for (i = 0; i < n; i++) stereo += out[2*i] != out[2*i+1];
        }
        if (f) fclose(f);
        hashes[mode] = h;
        printf("rift: %s peak=%u rms=%.0f mean=%.1f stereo=%u\n", RIFT_MODES[mode], peak,
               sqrt(energy / (FS * 8.0)), mean / (FS * 8.0), stereo);
        check("mode has bounded, non-silent stereo audio", peak < 32768u && energy > 1e9 && stereo > FS);
        check("mode has small DC offset", fabs(mean / (FS * 8.0)) < 500);
    }
    for (mode = 0; mode < 9; mode++) for (b = mode + 1; b < 9; b++) distinct += hashes[mode] != hashes[b];
    check("all nine modes render different audio", distinct == 36);
    {
        int32_t a[CTL * 2];
        apply_preset_to(&trk[0], 8); rift_reset();
        for (b = 0; b < 1000; b++) rift_audio(&trk[0], out, CTL);
        memcpy(a, out, sizeof a); rift_reset();
        for (b = 0; b < 1000; b++) rift_audio(&trk[0], out, CTL);
        check("reset and identical patch render deterministically", !memcmp(a, out, sizeof a));
    }
    for (mode = 0; mode < 2; mode++) {
        double ur = 0, ui = 0, lr = 0, li = 0;
        uint32_t ph = 0, sp = 0, di = (uint32_t)(1000.0 / FS * 4294967296.0);
        int32_t si = (int32_t)(100.0 / FS * 4294967296.0) * (mode ? -1 : 1);
        memset(rift.hilbert, 0, sizeof rift.hilbert); rift.hw = 0;
        for (b = 0; b < FS * 2u; b++) {
            int32_t x = sine_i(ph + 0x40000000u) / 2;
            int32_t y = rift_shift_sample(0, x, sine_i(sp + 0x40000000u), sine_i(sp));
            if (b >= FS) {
                double t = (double)b / FS;
                ur += y * cos(2 * M_PI * 1100 * t); ui += y * sin(2 * M_PI * 1100 * t);
                lr += y * cos(2 * M_PI * 900 * t); li += y * sin(2 * M_PI * 900 * t);
            }
            ph += di; sp += (uint32_t)si; rift.hw = (rift.hw + 1u) & 31u;
        }
        check(mode ? "negative CELL shifts down, suppresses upper sideband" : "positive CELL shifts up, suppresses lower sideband",
              mode ? (lr*lr + li*li) > 20*(ur*ur + ui*ui) : (ur*ur + ui*ui) > 20*(lr*lr + li*li));
    }
    /* Exercise corners and moving parameters under ASan/UBSan: audio ranges stay finite. */
    for (mode = 0; mode < 9; mode++) {
        trk[0].p[P_E0] = (int16_t)mode;
        for (b = 0; b < 3000; b++) {
            for (i = 1; i < 8; i++) { random = rift_hash(random); trk[0].p[P_E0+i] = random & 127u; }
            trk[0].p[P_DIST] = 127; trk[0].p[P_DLY] = b & 127u;
            trk[0].p[P_CHOR] = b % 128u; trk[0].p[P_REV] = b & 127u;
            rift_audio(&trk[0], out, CTL);
            for (i = 0; i < CTL * 2; i++) if (out[i] < -32768 || out[i] > 32767) failures++;
        }
    }
    check("random macro sweeps remain bounded", !failures);
    trk[0].p[P_MUTE] = 1; trk[0].p[P_E7] = 0;
    for (b = 0; b < FS * 3u; b += CTL) rift_audio(&trk[0], out, CTL);
    { int peak = 0; for (i = 0; i < CTL*2; i++) if (abs(out[i]) > peak) peak = abs(out[i]);
      check("mute settles to silence with feedback zero", peak < 16); }
    started = clock(); trk[0].p[P_MUTE] = 0;
    for (b = 0; b < FS * 10u; b += CTL) rift_audio(&trk[0], out, CTL);
    printf("rift: 10s of audio rendered in %.3fs host CPU time (not a device CPU measurement)\n",
           (double)(clock() - started) / CLOCKS_PER_SEC);
    printf("rift: %d failures\n", failures);
    return failures ? 1 : 0;
}
