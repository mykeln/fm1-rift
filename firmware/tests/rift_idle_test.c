/* SPDX-License-Identifier: GPL-3.0-only
 * The real host main/audio/storage loop: a drone has no conventional active
 * voice, but must still prevent an automatic flash erase during its sound. */
#include <stdio.h>
#include "../platforms/host/felucca_host.c"
static void run_ms(uint32_t ms)
{ for (uint32_t k=0; k<ms; k+=10) fh_render(441); }
int main(void)
{
    int fail=0;
    fh_nor_erase(); fh_master(800); fh_boot(); run_ms(600);
    apply_preset_to(&trk[0], 8); run_ms(12000);
    int ok=!autosave_quiet() && !as.writes;
    printf("rift idle: sounding drone defers autosave: %s (writes=%u)\n",ok?"ok":"FAIL",as.writes); fail+=!ok;
    settings.palette = settings.palette == 2u ? 0u : 2u;
    settings_save();
    ok=persist_pending != 0;
    printf("rift idle: settings write stays queued during drone: %s\n",ok?"ok":"FAIL"); fail+=!ok;
    trk[0].p[P_MUTE]=1; run_ms(3000);
    ok=!autosave_quiet();
    printf("rift idle: audible released tail defers autosave: %s\n",ok?"ok":"FAIL"); fail+=!ok;
    trk[0].p[P_E7]=0; run_ms(3000);
    ok=autosave_quiet();
    printf("rift idle: quiet muted tank permits autosave: %s\n",ok?"ok":"FAIL"); fail+=!ok;
    ok=!persist_pending && persist_saved.palette == palette_to_stored(settings.palette);
    printf("rift idle: queued settings save after silence: %s\n",ok?"ok":"FAIL"); fail+=!ok;
    run_ms(12000);
    ok=as.writes==1;
    printf("rift idle: pending state saves once after silence: %s (writes=%u)\n",ok?"ok":"FAIL",as.writes); fail+=!ok;
    return !!fail;
}
