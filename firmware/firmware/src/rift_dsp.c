/* SPDX-License-Identifier: GPL-3.0-only
 * Fixed-point stereo drone DSP, called only by the audio thread. All products
 * fit signed 32 bits. Eight modulated FDN lines run at half rate, reusing
 * the existing 65536-sample delay buffer. The generator stays at full rate.
 * Macro meanings and approximations: ../../docs/RIFT.md. */
#define RIFT_LINES 8u
#define RIFT_LINE 8192u
#define RIFT_MASK (RIFT_LINE - 1u)
static int16_t rift_diffusion[2][4][1024] __attribute__((section(".pool")));
static int16_t rift_chorus[2][256] __attribute__((section(".pool")));
static int16_t rift_tank_ap[RIFT_LINES][512] __attribute__((section(".pool")));
enum { RF_MODE, RF_MORPH, RF_SPAN, RF_WARP, RF_FIELD, RF_TIME, RF_FORM, RF_FEED,
       RF_FUSE, RF_BASIS, RF_CELL, RF_MIX, RF_SEED, RF_SCAN, RF_N };
static struct {
    int32_t sm[RF_N], lp[2], dc[2], gen_dc[2], out_dc[2], wet_lp[2], damp[RIFT_LINES], delay[RIFT_LINES], tank_dc[RIFT_LINES], gain;
    int32_t send_lp[2][2], space[2];
    int32_t hilbert[RIFT_LINES][2][4][4], shift_delay[RIFT_LINES], shift_reference[RIFT_LINES];
    uint32_t phase[4][2], shift, slow, drift, sample, w, hw, mode, ready, active, last_note;
    int32_t osc[4];
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
    memset(rift_chorus, 0, sizeof rift_chorus);
    rift.mode = 99;
}
/* RIFT has no conventional active drone voice. Flash erases must also wait
 * for its independent generator and long wet tail to become inaudible. */
static int rift_sounding(void)
{
    return rift.active && rift.quiet_frames < FS;
}
/* JieLi -Os emits hardware divide even for signed /2, /4, etc. Round
 * toward zero explicitly to retain the original samples without that cost. */
static inline __attribute__((always_inline)) int32_t rift_divpow2(int32_t x, uint32_t bits)
{
    return (x + ((x >> 31) & (int32_t)((1u << bits) - 1u))) >> bits;
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
/* Reference-style bipolar controls, using original curves and coefficient maps.
 * Public evidence and remaining differences: docs/vhikk-x-public-research.md. */
static int32_t rift_feedback_depth(int32_t feed)
{
    int32_t x = feed - 16384;
    /* Both endpoints reach the same depth despite MIDI's asymmetric halves. */
    return x < 0 ? -x * 2 : x * 128 / 63;
}
static int32_t rift_feedback_gain(int32_t depth)
{
    int32_t x = 32768 - depth;
    return mulq15(32768 - mulq15(mulq15(x, x), x), 32680);
}
static int32_t rift_field_filter(const tsvf_t *c, int32_t in, int32_t field,
                                 int32_t damping_k, int32_t *ic1, int32_t *ic2, int32_t *bp)
{
    int32_t v3 = in - *ic2;
    int32_t v1 = (c->a1 * *ic1 + c->a2 * v3) >> 13;
    int32_t lp = *ic2 + ((c->a2 * *ic1 + c->a3 * v3) >> 13);
    *ic1 = clamp(2 * v1 - *ic1, -150000, 150000);
    *ic2 = clamp(2 * lp - *ic2, -150000, 150000);
    int32_t hp = in - 2 * mulq16(v1, (uint32_t)damping_k * 8u) - lp;
    int32_t distance = field - 16384;
    int32_t amount = clamp(distance < 0 ? -distance * 8 : distance * 8, 0, 32768);
    *bp = v1;
    int32_t out = in + mulq16((distance < 0 ? lp : hp) - in, (uint32_t)amount * 2u);
    return field < 1024 ? mulq15(out, field * 32) : out;
}
static int32_t rift_chorus_sample(uint32_t side, int32_t in, int32_t amount, uint32_t delay)
{
    uint32_t at = (rift.sample - (delay >> 8)) & 255u;
    int32_t a = rift_chorus[side][at], b = rift_chorus[side][(at - 1u) & 255u];
    int32_t wet = a + ((b - a) * (int32_t)(delay & 255u) >> 8);
    rift_chorus[side][rift.sample & 255u] = (int16_t)clamp(in, -32768, 32767);
    return in + mulq15(wet - in, amount >> 1);
}
/* Exponential FM: audio-rate modulation multiplies frequency, never reverses
 * phase direction. Interpolate a one-octave ratio table; no per-sample float. */
static uint32_t rift_fm_inc(uint32_t inc, int32_t modulation)
{
    static const uint16_t ratio[17] = {16384,17109,17867,18658,19484,20347,21247,22188,
        23170,24196,25268,26387,27554,28774,30048,31379,32768};
    if (!modulation) return inc;
    int32_t x = clamp(modulation, -32768, 32767) + 32768;
    uint32_t octave = (uint32_t)x >> 14, at = ((uint32_t)x & 16383u) >> 10;
    int32_t r = ratio[at] + ((ratio[at+1u] - ratio[at]) * (x & 1023) >> 10);
    uint32_t v = (uint32_t)mulq16((int32_t)inc, (uint32_t)r) << octave;
    return v > 0x20000000u ? 0x20000000u : v;
}
static int32_t rift_source(uint32_t mode, uint32_t phase, uint32_t inc, int32_t wave)
{
    if (mode == 2u || mode == 6u) return sine_i(phase);
    if (mode == 5u || mode == 8u)
        return mulq15(sine_i(phase), 24576) + mulq15(sine_i(phase * 2u), (wave >> 2))
             + mulq15(sine_i(phase * 3u), (wave >> 3));
    return rift_wave(phase, inc, wave);
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
    int32_t x = rift_divpow2(in, 1), g = amount * 3 >> 2;
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
    rift.shift_reference[side] = (re * 32767 >> 15) * 2; /* phase-aligned unshifted analytic branch */
    return clamp(((re * cs >> 15) - (q * sn >> 15)) * 2, -30000, 30000);
}
/* Unity gain through the quiet tail; saturation only catches large tank peaks.
 * A tanh on every sample used to shorten the decay even at modest levels. */
static int32_t rift_tank_limit(int32_t x)
{
    int32_t a = x < 0 ? -x : x;
    if (a > 12000) a = 12000 + (softclip((a - 12000) * 2) >> 1);
    return x < 0 ? -a : a;
}
/* One small allpass per line adds density on every circuit of the tank. */
static int32_t rift_tank_diffuse(uint32_t lane, int32_t x, int32_t amount)
{
    static const uint16_t length[RIFT_LINES] = {149, 211, 263, 293, 337, 379, 421, 479};
    int16_t *line = rift_tank_ap[lane];
    int32_t g = amount * 5 >> 3;
    int32_t d = line[(rift.w - length[lane]) & 511u];
    x = rift_divpow2(x, 2); /* headroom for the orthogonal matrix's coherent peaks */
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
    int32_t p[RF_N], target[RIFT_LINES], morph[4][2], level[4], amp, fb, form, mix,
        warp, drive, shift_inc, scatter_c, scatter_s, damping, send_gain, feed_depth,
        raw_mix, field_cut, resonance, filter_k, span, fuse, cluster_mix, chorus;
    uint32_t node, cluster, slot;
    uint32_t inc[4][2], chorus_delay[2], mode, i, j, voices, voice_bits;
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
    voice_bits = mode == 0u ? 0u : mode == 1u ? 1u : 2u;
    warp = p[RF_WARP] - 16384;
    span = p[RF_SPAN] - 16384;
    fuse = p[RF_FUSE] - 16384;
    drive = fuse > 0 ? fuse * 2 : 0;
    cluster = mode == 2u || mode == 4u || mode == 5u || mode == 8u;
    node = (uint32_t)p[RF_MORPH] * 8u >> 15;
    slot = node & 1u;
    cluster_mix = (p[RF_MORPH] * 8) & 32767;
    for (j = 0; j < 4u; j++) {
        for (uint32_t k = 0; k < 2u; k++) {
            /* Each anchor has its OWN frequency and phase. MORPH mixes the
             * running voices; it does not glide one oscillator between notes.
             * Alternating phase slots retain the shared anchor across nodes. */
            uint32_t pos = (uint32_t)p[RF_SEED] * 2u + (cluster ? (node + k) * 4096u : 0u);
            int32_t spread = rift_field(pos, j + 1u);
            int32_t offset = spread * span / 1048576;
            int32_t pitch = p[RF_BASIS] / 16;
            if (mode < 2u) offset = j ? span / 32 : 0;
            if (mode == 4u) {
                static const int8_t scale[7] = {0, 2, 4, 5, 7, 9, 11};
                int32_t degree = offset / 24 + chord[j] * span / 16384;
                /* Floor division preserves the diatonic lattice below root. */
                int32_t oct = degree >= 0 ? degree / 7 : (degree - 6) / 7;
                offset = (12 * oct + scale[degree - oct * 7]) * 16;
            }
            if (mode == 6u) offset = (int32_t)j * span / 2048;
            inc[j][k] = pitch_inc(clamp(pitch + offset, 0, 127 * 16));
            if (mode == 6u) inc[j][k] = rift_ratio_inc(inc[j][k], partial_ratio[j]);
            if (mode == 7u) inc[j][k] = rift_ratio_inc(inc[j][k], cross_ratio[j]);
            if (mode == 8u) {
                if (j == 1u) inc[j][k] = inc[j][k] > 0x15555555u ? 0x20000000u : inc[j][k] + inc[j][k] / 2u;
                if (j >= 2u) inc[j][k] = rift_ratio_inc(inc[j][k], j);
            }
            if (inc[j][k] > 0x20000000u) inc[j][k] = 0x20000000u;
            int32_t wave = mode == 0u && j ? p[RF_SCAN] : p[RF_MORPH];
            if (cluster) wave = 16384 + rift_field(pos + (uint32_t)p[RF_SCAN], j + 9u);
            else if (mode != 0u) wave += rift_field((uint32_t)p[RF_SCAN], j + 9u) / 2;
            if (span < 0 && mode >= 3u && !cluster)
                wave += rift_field((uint32_t)p[RF_SEED] * 2u, j + 12u) * (-span) / 16384;
            morph[j][k] = clamp(wave, 0, 32511);
        }
        /* SEED also scans oscillator amplitudes. No hidden SCAN-rate LFO. */
        level[j] = 28672 + rift_field((uint32_t)p[RF_SEED] * 2u, j + 40u) / 4;
        if (mode == 5u || mode == 6u || mode == 8u)
            level[j] = 24576 + rift_field((uint32_t)p[RF_SEED], j + 40u) / 8
                     + rift_field((uint32_t)p[RF_SCAN], j + 60u) / 3;
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
    for (j = 0; j < 2u; j++)
        chorus_delay[j] = (uint32_t)(128 * 256 + rift_divpow2(sine_i(rift.slow + j * 0x40000000u), 1));
    /* FIELD: LP below noon, open at noon, HP above noon. A short blend
     * around the center avoids a discontinuity when changing filter type. */
    field_cut = p[RF_FIELD] < 16384 ? p[RF_FIELD] * 127 / 64
        : (p[RF_FIELD] - 16384) * 127 / 63;
    resonance = (mode == 0u || mode == 3u || mode == 4u) ? drive / 512 : 0;
    filter_k = 8192 - resonance * 7600 / 127;
    tsvf_coef_k(&filter, field_cut, filter_k);
    chorus = (mode == 0u || mode == 2u || mode == 4u || mode == 5u || mode == 6u || mode == 7u) && fuse < 0 ? -fuse * 2 : 0;
    if ((mode == 3u || mode == 4u) && fuse > 0) chorus = fuse;
    feed_depth = rift_feedback_depth(p[RF_FEED]);
    fb = rift_feedback_gain(feed_depth);
    /* CCW: translated signal. CW progressively adds unshifted taps, causing
     * moving interference notches. The same blend circulates in the loop. */
    raw_mix = p[RF_FEED] > 16384 ? (p[RF_FEED] - 16384) * 64 / 63 : 0;
    send_gain = 32768 - clamp(feed_depth - 96 * 256, 0, 32 * 256) * 3;
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
        int32_t previous[4] = {rift.osc[0], rift.osc[1], rift.osc[2], rift.osc[3]};
        for (j = 0; j < voices; j++) {
            int32_t prev = previous[(j + voices - 1u) & (voices - 1u)];
            int32_t self = previous[j], y[2];
            for (uint32_t k = 0; k < (cluster ? 2u : 1u); k++) {
                uint32_t bank = cluster ? (slot + k) & 1u : 0u;
                uint32_t step = inc[j][k], phase = rift.phase[j][bank];
                if (mode == 0u) {
                    if (warp < 0) step = rift_fm_inc(step, mulq15(prev, -warp * 2));
                } else if (mode == 1u) {
                    int32_t feedback = warp < 0 ? prev : self;
                    phase += (uint32_t)(mulq15(prev + rift_divpow2(feedback, 1), warp < 0 ? -warp : warp)) * 32768u;
                } else if (warp < 0) {
                    if (mode == 2u) phase += (uint32_t)(self * (-warp)) * 2u;
                    else if (mode != 3u) phase += (uint32_t)(prev * (-warp)) * 2u;
                } else if (warp > 0) step = rift_fm_inc(step, mulq15(prev, warp * 2));
                if (mode == 2u && drive) {
                    int32_t noise = (int32_t)(rift_hash(rift.sample + j * 101u) & 65535u) - 32768;
                    phase += (uint32_t)(noise * (drive >> 2)) * 2u;
                }
                if (mode == 7u) phase += (uint32_t)(prev * 4096) * 2u;
                y[k] = rift_source(mode, phase, step, morph[j][k]);
                rift.phase[j][bank] += step;
            }
            int32_t x = cluster ? y[0] + mulq15(y[1] - y[0], cluster_mix) : y[0];
            if (mode == 0u && !j && warp > 0) x += mulq15(mulq15(x, prev) - x, warp * 2);
            if (mode == 2u) {
                x += mulq15(mulq15(x, prev) - x, p[RF_SCAN]);
            }
            if (mode == 3u && warp < 0) {
                /* Continuous bit-depth morph: crossfade adjacent quantizers,
                 * so moving WARP does not jump between integer bit depths. */
                int32_t bits = (-warp) * 10, shift = bits >> 14, frac = (bits & 16383) * 2;
                int32_t a = (x >> shift) * (1 << shift);
                int32_t b = (x >> (shift + 1)) * (1 << (shift + 1));
                x = a + mulq15(b - a, frac);
            }
            if (mode == 1u || (mode == 5u && drive)) {
                int32_t folding = mode == 1u ? 2048 + (drive >> 2) : 2048 + (drive >> 1);
                x = sine_i((uint32_t)(x * folding) * 8u);
            } else if (drive && mode >= 4u)
                x += mulq15(softclip(x * 2) - x, drive);
            if (mode == 6u) x = mulq15(x, j ? 16384 + (p[RF_MORPH] >> 1) : 24576 - (p[RF_MORPH] >> 1));
            x = mulq15(x, level[j]);
            rift.osc[j] = x;
            if (mode == 0u && j) continue; /* PAIR modulator is not a second audible carrier. */
            /* SPAN CW increases stereo separation. Noon/CCW are centered. */
            int32_t pan = mode != 0u && span > 0 ? span : 0;
            dry[j & 1u] += rift_divpow2(mulq15(x, 16384 + pan), voice_bits);
            dry[(j & 1u) ^ 1u] += rift_divpow2(mulq15(x, 16384 - pan), voice_bits);
        }
        rift.gain += rift_divpow2(amp - rift.gain, 7);
        if (amp && rift.gain < amp && amp - rift.gain < 128) rift.gain++;
        if (!amp && rift.gain > 0 && rift.gain < 128) rift.gain--;
        for (j = 0; j < 2u; j++) {
            int32_t x = rift_chorus_sample(j, dry[j], chorus, chorus_delay[j]), bp;
            x = rift_field_filter(&filter, x, p[RF_FIELD], filter_k, &rift.lp[j], &rift.dc[j], &bp);
            if (mode == 3u && fuse < 0) {
                int32_t band = bp + sine_i(rift.phase[0][0]) / 8;
                if (p[RF_FIELD] < 1024) band = mulq15(band, p[RF_FIELD] * 32);
                x += mulq15(band - x, -fuse * 2);
            }
            rift.gen_dc[j] += rift_divpow2(x * 256 - rift.gen_dc[j], 10);
            x -= rift_divpow2(rift.gen_dc[j], 8);
            dry[j] = rift_divpow2(mulq15(softclip(x * 2), rift.gain), 1);
            /* The folder restores a near-full-scale sine from an already
             * rounded input. Match its body level to the other generators. */
            if (mode == 1u) dry[j] = rift_divpow2(dry[j] * 3, 2);
            if (mode == 2u || mode == 5u || mode == 6u) dry[j] = rift_divpow2(dry[j] * 3, 1);
            /* Two low-pass stages before decimation. This is a dark space
             * processor; the dry generator retains the full-rate spectrum. */
            rift.send_lp[j][0] += rift_divpow2(dry[j] - rift.send_lp[j][0], 1);
            rift.send_lp[j][1] += rift_divpow2(rift.send_lp[j][0] - rift.send_lp[j][1], 1);
        }
        if (!(rift.sample & 1u)) {
            int32_t d[RIFT_LINES], h[RIFT_LINES], rotated[RIFT_LINES];
            int32_t cs = sine_i(rift.shift + 0x40000000u), sn = sine_i(rift.shift);
            for (j = 0; j < 2u; j++) {
                int32_t x = mulq16(rift.send_lp[j][1], (uint32_t)send_gain * 2u);
                int32_t diffused = rift_diffuse(j, x, form);
                injection[j] = x + mulq15(diffused - x, form);
            }
            for (j = 0; j < RIFT_LINES; j++) {
                int32_t delta = target[j] - rift.delay[j];
                rift.delay[j] += clamp(delta, -8, 8);
                int32_t read_delay = rift.delay[j];
                /* Reflekta-like modes: FUSE CCW couples generator audio into
                 * the read position. This is audio-rate delay modulation. */
                if ((mode == 1u || mode == 8u) && fuse < 0)
                    read_delay += rift_divpow2(mulq15(rift.osc[j & 3u], -fuse), 3);
                read_delay = clamp(read_delay, 8 * 256, (int32_t)(RIFT_LINE - 4u) * 256);
                int32_t raw = rift_read(j, read_delay);
                int32_t shifted = rift_shift_sample(j, raw, cs, sn);
                d[j] = shifted + mulq15(rift.shift_reference[j] - shifted, raw_mix);
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
            int32_t next[2] = {rift_divpow2(d[0] + d[2] - d[4] + d[6], 1),
                               rift_divpow2(d[1] - d[3] + d[5] + d[7], 1)};
            for (j = 0; j < RIFT_LINES; j++) {
                int32_t x = h[j];
                rift.damp[j] += mulq16(x * 256 - rift.damp[j], (uint32_t)damping * 2u);
                x = rift_tank_diffuse(j, rift_divpow2(rift.damp[j], 8), form);
                /* A 7 Hz low cut prevents frequency-shifted subsonic energy
                 * monopolizing a long feedback tail. Preserve audible bass. */
                rift.tank_dc[j] += rift_divpow2(x * 256 - rift.tank_dc[j], 9);
                x -= rift_divpow2(rift.tank_dc[j], 8);
                x = mulq16(x, (uint32_t)fb * 2u) + rift_divpow2(injection[j & 1u], 1);
                dly_buf[j * RIFT_LINE + (rift.w & RIFT_MASK)] = (int16_t)rift_tank_limit(x);
            }
            for (j = 0; j < 2u; j++) {
                wet[j] = rift_divpow2(rift.space[j] + next[j], 1);
                rift.space[j] = next[j];
            }
            rift.shift += (uint32_t)shift_inc * 2u;
            rift.hw = (rift.hw + 1u) & 31u; rift.w++;
        } else {
            wet[0] = rift.space[0]; wet[1] = rift.space[1];
        }
        rift.sample++;
        for (j = 0; j < 2u; j++) {
            rift.wet_lp[j] += rift_divpow2(wet[j] - rift.wet_lp[j], 1);
            /* Compensate the diffuse tank's energy spread; high FORM brings
             * the surrounding wash forward as well as increasing density. */
            int32_t surrounding = rift_divpow2(rift.wet_lp[j], 1) + mulq16(rift.wet_lp[j], (uint32_t)form * 2u);
            int32_t x = dry[j] + mulq16(surrounding - dry[j], (uint32_t)mix * 2u);
            /* Folding and saturating feedback can each introduce DC after the generator's HPF. */
            rift.out_dc[j] += rift_divpow2(x * 256 - rift.out_dc[j], 11);
            out[2u * i + j] = mulq15(rift_tank_limit(x - rift_divpow2(rift.out_dc[j], 8)), rift.fade);
            if (out[2u * i + j] > 8 || out[2u * i + j] < -8) audible = 1;
        }
    }
    if (audible) rift.quiet_frames = 0;
    else if (rift.quiet_frames < FS) rift.quiet_frames += n;
}
