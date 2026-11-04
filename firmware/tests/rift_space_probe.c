/* SPDX-License-Identifier: GPL-3.0-only
 * Long CAVERN drone, decay and algorithm-handover recordings. The final
 * take holds one unchanging note for 48 seconds to expose internal motion. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#define FELUCCA_RIFT 1
#include "../platforms/host/firmware.c"
#include "../platforms/host/tracks.c"
static void wav(FILE *f, uint32_t frames)
{
    uint32_t v; uint16_t s;
    fwrite("RIFF",1,4,f); v=36+frames*4; fwrite(&v,4,1,f); fwrite("WAVEfmt ",1,8,f);
    v=16; fwrite(&v,4,1,f); s=1; fwrite(&s,2,1,f); s=2; fwrite(&s,2,1,f);
    v=FS; fwrite(&v,4,1,f); v=FS*4; fwrite(&v,4,1,f); s=4; fwrite(&s,2,1,f); s=16; fwrite(&s,2,1,f);
    fwrite("data",1,4,f); v=frames*4; fwrite(&v,4,1,f);
}
int main(int argc, char **argv)
{
    static const char *tag[3] = {"base", "switch", "evolving"};
    int32_t out[CTL*2]; char path[512];
    if(argc != 2) return 2;
    host_tracks_init();
    for(uint32_t mode=8;mode<9;mode++) for(uint32_t test=0;test<3;test++) {
        host_preset(&trk[0],ENGI_RIFT,mode); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_LPHASE]=24; trk[0].p[P_LRATE]=48;
        rift_reset(); rift.active=1;
        snprintf(path,sizeof path,"%s/%s-%s.wav",argv[1],RIFT_MODES[mode],tag[test]);
        uint32_t frames = FS * (test == 2u ? 60u : 36u);
        uint32_t release = FS * (test == 2u ? 48u : 6u);
        FILE *f=fopen(path,"wb"); if(!f) return 2; wav(f,frames);
        for(uint32_t b=0;b<frames;b+=CTL) {
            uint32_t n=frames-b<CTL?frames-b:CTL;
            if(b>=release) { trk[0].p[P_AHOLD]=0; if(test == 1u) trk[0].p[P_E0]=7; }
            rift_audio(&trk[0],out,n);
            for(uint32_t k=0;k<2*n;k++){int16_t s=(int16_t)out[k]; fwrite(&s,2,1,f);}
        }
        fclose(f);
    }
    return 0;
}
