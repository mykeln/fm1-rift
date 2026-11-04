/* SPDX-License-Identifier: GPL-3.0-only
 * Factory sounds held long enough to expose feedback accumulation, followed
 * by a release. Uses the same preset loader and DSP as the instrument. */
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
static int render(const char *path, uint32_t seconds, uint32_t skip, uint32_t release)
{
    int32_t out[CTL*2];
    rift_reset(); rift.active=1;
    FILE *f=fopen(path,"wb"); if(!f) return 2; wav(f,FS*(seconds-skip));
    for(uint32_t b=0;b<FS*seconds;b+=CTL) {
        uint32_t n=FS*seconds-b<CTL?FS*seconds-b:CTL;
        if(b>=FS*release) trk[0].p[P_AHOLD]=0;
        rift_audio(&trk[0],out,n);
        for(uint32_t k=0;k<2*n;k++){
            if(out[k] < -32768 || out[k] > 32767) { fclose(f); return 1; }
            /* CTL need not divide the skip boundary. */
            if(b+k/2 >= FS*skip) { int16_t s=(int16_t)out[k]; fwrite(&s,2,1,f); }
        }
    }
    fclose(f);
    return 0;
}
static void preset(uint32_t pi)
{
    host_preset(&trk[0],ENGI_RIFT,pi); trk[0].p[P_AHOLD]=1;
    trk[0].p[P_LPHASE]=24; trk[0].p[P_LRATE]=48;
}
int main(int argc, char **argv)
{
    char path[512]; int result;
    if(argc != 2) return 2;
    host_tracks_init();
    for(uint32_t pi=0;pi<27;pi++) {
        preset(pi);
        snprintf(path,sizeof path,"%s/%s-%u.wav",argv[1],RIFT_MODES[pi%9],1+pi/9);
        if((result=render(path,20,0,16))) return result;
    }
    /* Identical macros isolate CLUSTER's identity from preset differences. */
    for(uint32_t mode=3;mode<=5;mode+=2) {
        preset(3); trk[0].p[P_E0]=mode;
        snprintf(path,sizeof path,"%s/controls/matched-%s.wav",argv[1],RIFT_MODES[mode]);
        if((result=render(path,8,4,8))) return result;
    }
    static const int id[] = {P_E1,P_E2,P_E3,P_E4,P_E5,P_E6,P_E7,P_DIST,P_CHOR,P_DLY,P_REV,P_LPHASE,P_LRATE};
    static const char *name[] = {"MORPH","SPAN","WARP","FIELD","TIME","FORM","FEED","FUSE","BASIS","CELL","MIX","SEED","SCAN"};
    for(uint32_t mode=0;mode<9;mode++) for(uint32_t control=0;control<13;control++) for(uint32_t hi=0;hi<2;hi++) {
        preset(mode); trk[0].p[id[control]]=hi?127:0;
        /* BASIS endpoints use two audible octaves, not a subsonic root. */
        if(id[control]==P_CHOR) trk[0].p[P_CHOR]=hi?48:36;
        snprintf(path,sizeof path,"%s/controls/%s-%s-%u.wav",argv[1],RIFT_MODES[mode],name[control],hi);
        if((result=render(path,8,6,8))) return result;
    }
    return 0;
}
