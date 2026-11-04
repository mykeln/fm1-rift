/* SPDX-License-Identifier: GPL-3.0-only
 * Stationary drone and decay recordings: analyze spectrum and effect strength,
 * rather than treating a different sample hash as an audible difference. */
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
    static const int ids[9] = {-1,P_E5,P_E5,P_E6,P_E6,P_E7,P_E7,P_DLY,P_DLY};
    static const int values[9] = {0,0,127,0,127,0,127,0,127};
    static const char *tag[9] = {"base","time-0","time-127","form-0","form-127","feed-0","feed-127","cell-0","cell-127"};
    int32_t out[CTL*2]; char path[512];
    if(argc != 2) return 2;
    host_tracks_init();
    for(uint32_t mode=6;mode<9;mode++) for(uint32_t test=0;test<9;test++) {
        host_preset(&trk[0],ENGI_RIFT,mode); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_LPHASE]=24; trk[0].p[P_LRATE]=48;
        if(ids[test]>=0) trk[0].p[ids[test]]=(int16_t)values[test];
        rift_reset(); rift.active=1;
        snprintf(path,sizeof path,"%s/%s-%s.wav",argv[1],RIFT_MODES[mode],tag[test]);
        FILE *f=fopen(path,"wb"); if(!f) return 2; wav(f,FS*8u);
        for(uint32_t b=0;b<FS*8u;b+=CTL) {
            uint32_t n=FS*8u-b<CTL?FS*8u-b:CTL;
            if(b>=FS*4u) trk[0].p[P_AHOLD]=0;
            rift_audio(&trk[0],out,n);
            for(uint32_t k=0;k<2*n;k++){int16_t s=(int16_t)out[k]; fwrite(&s,2,1,f);}
        }
        fclose(f);
    }
    return 0;
}
