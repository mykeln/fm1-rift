/* SPDX-License-Identifier: GPL-3.0-only
 * Fixed-point stereo drone DSP, called only by the audio thread. All products
 * fit signed 32 bits. Eight modulated FDN lines run at half rate, reusing
 * the existing 65536-sample delay buffer. The generator stays at full rate.
 * Macro meanings and approximations: ../../docs/RIFT.md. */
#define RIFT_LINES 8u
#define RIFT_LINE 8192u
#define RIFT_MASK (RIFT_LINE - 1u)
static int16_t rift_diffusion[2][4][1024] __attribute__((section(".pool")));
static int16_t rift_tank_ap[RIFT_LINES][512] __attribute__((section(".pool")));
enum { RF_MODE, RF_MORPH, RF_SPAN, RF_WARP, RF_FIELD, RF_TIME, RF_FORM, RF_FEED,
       RF_FUSE, RF_BASIS, RF_CELL, RF_MIX, RF_SEED, RF_SCAN, RF_N };
static struct {
    int32_t sm[RF_N], lp[2], dc[2], gen_dc[2], out_dc[2], wet_lp[2], damp[RIFT_LINES], delay[RIFT_LINES], tank_dc[RIFT_LINES], gain;
    int32_t send_lp[2][2], space[2];
    int32_t hilbert[RIFT_LINES][2][4][4], shift_delay[RIFT_LINES];
    uint32_t phase[4], shift, slow, drift, motion, sample, w, hw, mode, ready, active, last_note;
    int32_t fade;
    volatile uint32_t quiet_frames; /* published by audio; main-loop flash-idle guard */
} rift;
_Static_assert(DLY_LEN >= RIFT_LINES * RIFT_LINE, "RIFT delay allocation");

static void rift_reset(void)
{
    memset(&rift, 0, sizeof rift);
    memset(dly_buf, 0, sizeof dly_buf);
    memset(rift_diffusion, 0, sizeof rift_diffusion);
    memset(rift_tank_ap, 0, sizeof rift_tank_ap);
    rift.mode = 99;
}
/* RIFT has no conventional active drone voice. Flash erases must also wait
 * for its independent generator and long wet tail to become inaudible. */
static int rift_sounding(void)
{
    return rift.active && rift.quiet_frames < FS;
}
static uint32_t rift_hash(uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
    x *= 0x846ca68bu; return x ^ (x >> 16);
}
static uint32_t rift_ratio_inc(uint32_t inc, uint32_t ratio)
{
    return inc > 0x20000000u / ratio ? 0x20000000u : inc * ratio;
}
/* Continuous deterministic random field; crossing an anchor never reseeds audio. */
static int32_t rift_field(uint32_t pos, uint32_t lane)
{
    uint32_t cell = (pos >> 12) & 15u; /* periodic: the moving Q16 coordinate can wrap smoothly */
    int32_t a = (int32_t)(rift_hash(cell + lane * 101u) & 32767u) - 16384;
    int32_t b = (int32_t)(rift_hash(((cell + 1u) & 15u) + lane * 101u) & 32767u) - 16384;
    int32_t f = (int32_t)(pos & 4095u);
    f = (f * f >> 12) * (12288 - 2 * f) >> 12; /* smoothstep Q12 */
    return a + ((b - a) * f >> 12);
}
static int32_t rift_wave(uint32_t ph, uint32_t inc, int32_t morph)
{
    int32_t a, b, pos = clamp(morph, 0, 32511) * 3;
    uint32_t zone = (uint32_t)pos >> 15;
    int32_t frac = pos & 32767;
    if (!zone) { a = sine_i(ph); b = osc_tri(ph); }
    else if (zone == 1u) { a = osc_tri(ph); b = osc_saw(ph, inc); }
    else { a = osc_saw(ph, inc); b = osc_pulse(ph, inc, 0x80000000u); }
    return a + ((b - a) * frac >> 15);
}
static int32_t rift_read(uint32_t lane, int32_t delay)
{
    uint32_t d = (uint32_t)delay >> 8, f = (uint32_t)delay & 255u;
    uint32_t at = (rift.w - d) & RIFT_MASK;
    int32_t a = dly_buf[lane * RIFT_LINE + at];
    int32_t b = dly_buf[lane * RIFT_LINE + ((at - 1u) & RIFT_MASK)];
    return a + ((b - a) * (int32_t)f >> 8);
}
static int32_t rift_diffuse(uint32_t side, int32_t in, int32_t amount)
{
    static const uint16_t length[2][4] = {{113, 283, 467, 797}, {163, 331, 587, 887}};
    int32_t x = in / 2, g = amount * 3 / 4;
    for (uint32_t stage = 0; stage < 4u; stage++) {
        int16_t *line = rift_diffusion[side][stage];
        int32_t d = line[(rift.w - length[side][stage]) & 1023u];
        int32_t y = d - mulq15(x, g);
        line[rift.w & 1023u] = (int16_t)clamp(x + mulq15(y, g), -32768, 32767);
        x = y;
    }
    return softclip(x * 2);
}
static void rift_rotate(int32_t a, int32_t b, int32_t c, int32_t s, int32_t *x, int32_t *y)
{
    *x = mulq16(a, (uint32_t)c * 2u) - mulq16(b, (uint32_t)s * 2u);
    *y = mulq16(a, (uint32_t)s * 2u) + mulq16(b, (uint32_t)c * 2u);
}
/* Olli Niemitalo's 90-degree allpass pair (2003), squared coefficients Q15.
 * Each section H(z)=(a-z^-2)/(1-a*z^-2); branch 0 has one extra delay.
 * Unlike the old 31-tap FIR, this pair resolves the instrument's bass notes.
 * Mathematical reference and attribution: docs/RIFT.md. */
static const int16_t RIFT_HILB[2][4] = {
    {15709, 28712, 32001, 32686},
    {5301, 24020, 30977, 32460},
};
static int32_t rift_shift_sample(uint32_t side, int32_t in, int32_t cs, int32_t sn)
{
    uint32_t branch, k, parity = rift.hw & 1u;
    int32_t y[2];
    for (branch = 0; branch < 2u; branch++) {
        int32_t x = in * 128; /* half amplitude, Q8 state prevents fixed-point limit-cycle hiss */
        for (k = 0; k < 4u; k++) {
            int32_t *s = rift.hilbert[side][branch][k];
            int32_t a = RIFT_HILB[branch][k];
            int32_t v = clamp(mulq16(x + s[parity + 2u], (uint32_t)a * 2u) - s[parity], -30000 * 256, 30000 * 256);
            s[parity] = x; s[parity + 2u] = v; x = v;
        }
        y[branch] = x;
    }
    int32_t q = rift.shift_delay[side] >> 8, re = y[1] >> 8;
    rift.shift_delay[side] = y[0];
    return clamp(((re * cs >> 15) - (q * sn >> 15)) * 2, -30000, 30000);
}
/* Unity gain through the quiet tail; saturation only catches large tank peaks.
 * A tanh on every sample used to shorten the decay even at modest levels. */
static int32_t rift_tank_limit(int32_t x)
{
    int32_t a = x < 0 ? -x : x;
    if (a > 12000) a = 12000 + softclip((a - 12000) * 2) / 2;
    return x < 0 ? -a : a;
}
/* One small allpass per line adds density on every circuit of the tank. */
static int32_t rift_tank_diffuse(uint32_t lane, int32_t x, int32_t amount)
{
    static const uint16_t length[RIFT_LINES] = {149, 211, 263, 293, 337, 379, 421, 479};
    int16_t *line = rift_tank_ap[lane];
    int32_t g = amount * 5 / 8;
    int32_t d = line[(rift.w - length[lane]) & 511u];
    x /= 4; /* headroom for the orthogonal matrix's coherent peaks */
    int32_t y = d - mulq15(x, g);
    line[rift.w & 511u] = (int16_t)clamp(x + mulq15(y, g), -30000, 30000);
    return y * 4;
}
static void rift_audio(track_t *t, int32_t *out, uint32_t n)
{
    static const uint8_t ids[RF_N] = {P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
        P_DIST, P_CHOR, P_DLY, P_REV, P_LPHASE, P_LRATE};
    static const int8_t chord[4] = {0, 7, 12, 16};
    static const uint8_t partial_ratio[4] = {1, 2, 3, 5}, cross_ratio[4] = {1, 2, 3, 7};
    static const uint16_t taps[RIFT_LINES] = {8171, 7481, 6749, 5987, 5279, 4513, 3767, 2999};
    int32_t p[RF_N], target[RIFT_LINES], morph[4], amp, fb, form, mix, warp, shift_inc, scatter_c, scatter_s, damping;
    uint32_t inc[4], mode, i, j, voices;
    tsvf_t filter;
    uint32_t audible = 0;
    if (t->v[0].gate && t->v[0].note != rift.last_note) {
        rift.last_note = t->v[0].note; t->p[P_CHOR] = t->v[0].note;
    }
    if (!rift.ready) {
        for (j = 0; j < RF_N; j++) rift.sm[j] = (int32_t)t->p[ids[j]] * 256;
        rift.ready = 1;
    }
    for (j = 0; j < RF_N; j++) {
        int32_t goal = clamp(t->p[ids[j]], 0, j == RF_MODE ? 8 : 127) * 256;
        int32_t diff = goal - rift.sm[j];
        rift.sm[j] += diff / 16 + (diff > 0 ? 1 : diff < 0 ? -1 : 0);
        p[j] = rift.sm[j];
    }
    mode = (uint32_t)clamp(t->p[P_E0], 0, 8);
    /* All modes now share one tank: leave its tail alive when changing the
     * generator. Fade just the source down before a discontinuous topology
     * change, then let the existing source envelope fade the new one up. */
    uint32_t changing = 0;
    if (rift.mode == 99u) rift.mode = mode;
    if (rift.mode != mode) {
        if (rift.gain > 32) { changing = 1; mode = rift.mode; }
        else rift.mode = mode;
    }
    if (rift.fade < 32767) rift.fade = clamp(rift.fade + 512, 0, 32767);
    voices = mode < 2u ? 2u : 4u;
    warp = p[RF_WARP] - 16384;
    for (j = 0; j < 4u; j++) {
        int32_t spread = rift_field((uint32_t)(p[RF_SEED] + (mode == 2u || mode >= 4u ? p[RF_MORPH] * 2 : 0)), j + 1u);
        int32_t offset = (spread * (p[RF_SPAN] >> 8)) / 8192; /* +/- 254 sixteenths of a semitone */
        int32_t pitch = p[RF_BASIS] / 16;
        if (!j) offset = 0;
        if (mode == 4u) offset = ((int32_t)chord[j] * (p[RF_SPAN] >> 8) / 64) * 16;
        if (mode == 6u) offset = (int32_t)j * (p[RF_SPAN] - 16384) / 2048;
        inc[j] = pitch_inc(clamp(pitch + offset, 0, 127 * 16));
        if (mode == 6u) inc[j] = rift_ratio_inc(inc[j], partial_ratio[j]);
        if (mode == 7u) inc[j] = rift_ratio_inc(inc[j], cross_ratio[j]);
        if (mode == 8u) {
            /* A fifth/octave choir with slow detuning, distinct from the
             * harmonic partial bank and the metallic cross-modulation mode. */
            if (j == 1u) inc[j] = inc[j] > 0x15555555u ? 0x20000000u : inc[j] + inc[j] / 2u;
            if (j >= 2u) inc[j] = rift_ratio_inc(inc[j], j);
            if (inc[j] > 0x20000000u) inc[j] = 0x20000000u;
            inc[j] = (uint32_t)((int32_t)inc[j] + (int32_t)(inc[j] >> 14) * (sine_i(rift.slow + j * 0x40000000u) >> 10));
        }
        /* Keep the oscillators out of the top octave where heavy modulation aliases most. */
        if (inc[j] > 0x20000000u) inc[j] = 0x20000000u;
        morph[j] = clamp(p[RF_MORPH] + rift_field((uint32_t)p[RF_SCAN] + (rift.motion >> 16), j + 9u) / 2, 0, 32511);
    }
    for (j = 0; j < RIFT_LINES; j++) {
        target[j] = 16 * 256 + ((int32_t)taps[j] * p[RF_TIME] >> 7);
        target[j] = mulq16(target[j], (uint32_t)(8192 + p[RF_FORM] * 3 / 4) * 2u);
        target[j] += rift_field((uint32_t)p[RF_SEED], j + 20u);
        /* Independent slow orbits keep a held note moving. The small tape
         * excursion detunes the reflections without an obvious chorus cycle. */
        uint32_t orbit = rift.drift * (1u + j % 3u) + j * 0x23670000u;
        target[j] += sine_i(orbit) * (p[RF_FORM] >> 7) >> 12;
        target[j] = clamp(target[j], 16 * 256, (int32_t)(RIFT_LINE - 8u) * 256);
        if (!rift.delay[j]) rift.delay[j] = target[j];
    }
    rift.slow += n * 10713u;
    rift.drift += n * 3211u;
    rift.motion += n * (64u + (uint32_t)(p[RF_SCAN] >> 5));
    tsvf_coef(&filter, p[RF_FIELD], p[RF_FUSE] >> 9);
    fb = 32768 - p[RF_FEED];
    fb = mulq15(32768 - mulq15(mulq15(fb, fb), fb), 32680); /* cubic decay curve; below unity */
    form = p[RF_FORM];
    scatter_s = sine_i((uint32_t)form << 14);
    scatter_c = sine_i(((uint32_t)form << 14) + 0x40000000u);
    damping = 13000 + (32768 - form) / 2; /* bright discrete repeats -> darker diffuse tank */
    mix = p[RF_MIX];
    {
        int32_t cell = p[RF_CELL] - 16384, magnitude = cell < 0 ? -cell : cell;
        shift_inc = (cell * magnitude / 32) * 3; /* fine at center, approximately +/- 258 Hz at the ends */
    }
    amp = t->p[P_AHOLD] ? 32767 : 0;
    for (j = 0; j < NVOICE; j++) if (t->v[j].gate && t->v[j].active) amp = 32767;
    if (t->p[P_MUTE] || changing) amp = 0;
    for (i = 0; i < n; i++) {
        int32_t dry[2] = {0, 0}, injection[2], wet[2];
        int32_t prev = sine_i(rift.phase[voices - 1u]);
        for (j = 0; j < voices; j++) {
            uint32_t phase = rift.phase[j];
            int32_t y;
            if (warp >= 0) phase += (uint32_t)(prev * warp) * 2u;
            /* CROSS has its own coupled-oscillator route, including at neutral WARP. */
            if (mode == 7u) phase += (uint32_t)(prev * (4096 + (p[RF_FUSE] >> 1))) * 2u;
            if (mode == 2u || mode == 6u) {
                y = sine_i(phase);
                y += (sine_i(phase * 2u) - y) * (p[RF_SCAN] >> 2) >> 15;
                if (mode == 6u) y = mulq15(y, j ? 16384 + p[RF_MORPH] / 2 : 24576 - p[RF_MORPH] / 2);
            }
            else y = rift_wave(phase, inc[j], morph[j]);
            if (mode == 1u) y = sine_i((uint32_t)(y * (1024 + (p[RF_FUSE] >> 2))) * 8u);
            if (warp < 0) y += ((mulq15(y, prev) - y) * (-warp) >> 14);
            prev = y;
            dry[j & 1u] += y / (int32_t)voices;
            rift.phase[j] += inc[j];
        }
        rift.gain += (amp - rift.gain) / 128;
        if (amp && rift.gain < amp && amp - rift.gain < 128) rift.gain++;
        if (!amp && rift.gain > 0 && rift.gain < 128) rift.gain--;
        for (j = 0; j < 2u; j++) {
            int32_t x = tsvf_lp(&filter, dry[j], &rift.lp[j], &rift.dc[j]);
            rift.gen_dc[j] += (x * 256 - rift.gen_dc[j]) / 1024;
            x -= rift.gen_dc[j] / 256;
            dry[j] = mulq15(softclip(x * 2), rift.gain) / 2;
            if (mode == 6u) dry[j] = dry[j] * 3 / 2;
            /* Two low-pass stages before decimation. This is a dark space
             * processor; the dry generator retains the full-rate spectrum. */
            rift.send_lp[j][0] += (dry[j] - rift.send_lp[j][0]) / 2;
            rift.send_lp[j][1] += (rift.send_lp[j][0] - rift.send_lp[j][1]) / 2;
        }
        if (!(rift.sample & 1u)) {
            int32_t d[RIFT_LINES], h[RIFT_LINES], rotated[RIFT_LINES];
            int32_t cs = sine_i(rift.shift + 0x40000000u), sn = sine_i(rift.shift);
            for (j = 0; j < 2u; j++) {
                int32_t x = rift.send_lp[j][1];
                int32_t diffused = rift_diffuse(j, x, form);
                injection[j] = x + mulq15(diffused - x, form);
            }
            for (j = 0; j < RIFT_LINES; j++) {
                int32_t delta = target[j] - rift.delay[j];
                rift.delay[j] += clamp(delta, -8, 8);
                d[j] = rift_shift_sample(j, rift_read(j, rift.delay[j]), cs, sn);
            }
            /* Three orthogonal rotation layers connect every line at high
             * FORM, while low FORM keeps discrete, pitched delay structures. */
            for (j = 0; j < RIFT_LINES; j += 2u)
                rift_rotate(d[j], d[j+1u], scatter_c, scatter_s, &h[j], &h[j+1u]);
            for (j = 0; j < RIFT_LINES; j += 4u) {
                rift_rotate(h[j], h[j+2u], scatter_c, scatter_s, &rotated[j], &rotated[j+2u]);
                rift_rotate(h[j+1u], h[j+3u], scatter_c, scatter_s, &rotated[j+1u], &rotated[j+3u]);
            }
            for (j = 0; j < 4u; j++)
                rift_rotate(rotated[j], rotated[j+4u], scatter_c, scatter_s, &h[j], &h[j+4u]);
            int32_t next[2] = {(d[0] + d[2] - d[4] + d[6]) / 2,
                               (d[1] - d[3] + d[5] + d[7]) / 2};
            for (j = 0; j < RIFT_LINES; j++) {
                int32_t x = h[j];
                rift.damp[j] += mulq16(x * 256 - rift.damp[j], (uint32_t)damping * 2u);
                x = rift_tank_diffuse(j, rift.damp[j] / 256, form);
                /* A 7 Hz low cut prevents frequency-shifted subsonic energy
                 * monopolizing a long feedback tail. Preserve audible bass. */
                rift.tank_dc[j] += (x * 256 - rift.tank_dc[j]) / 512;
                x -= rift.tank_dc[j] / 256;
                x = mulq16(x, (uint32_t)fb * 2u) + injection[j & 1u] / 2;
                dly_buf[j * RIFT_LINE + (rift.w & RIFT_MASK)] = (int16_t)rift_tank_limit(x);
            }
            for (j = 0; j < 2u; j++) {
                wet[j] = (rift.space[j] + next[j]) / 2;
                rift.space[j] = next[j];
            }
            rift.shift += (uint32_t)shift_inc * 2u;
            rift.hw = (rift.hw + 1u) & 31u; rift.w++;
        } else {
            wet[0] = rift.space[0]; wet[1] = rift.space[1];
        }
        rift.sample++;
        for (j = 0; j < 2u; j++) {
            rift.wet_lp[j] += (wet[j] - rift.wet_lp[j]) / 2;
            /* Compensate the diffuse tank's energy spread; high FORM brings
             * the surrounding wash forward as well as increasing density. */
            int32_t surrounding = rift.wet_lp[j] / 2 + mulq16(rift.wet_lp[j], (uint32_t)form * 2u);
            int32_t x = dry[j] + mulq16(surrounding - dry[j], (uint32_t)mix * 2u);
            /* Folding and saturating feedback can each introduce DC after the generator's HPF. */
            rift.out_dc[j] += (x * 256 - rift.out_dc[j]) / 2048;
            out[2u * i + j] = mulq15(rift_tank_limit(x - rift.out_dc[j] / 256), rift.fade);
            if (out[2u * i + j] > 8 || out[2u * i + j] < -8) audible = 1;
        }
    }
    if (audible) rift.quiet_frames = 0;
    else if (rift.quiet_frames < FS) rift.quiet_frames += n;
}
