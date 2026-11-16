/* SPDX-License-Identifier: GPL-3.0-only
 * Controlled signals expose macro direction and cluster crossfades. */
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
static void sample(FILE *f, int32_t a, int32_t b)
{
    if (abs(a)>32767 || abs(b)>32767) exit(1);
    int16_t s[2]={(int16_t)a,(int16_t)b}; fwrite(s,2,2,f);
}
static void render(const char *dir, const char *name, uint32_t release)
{
    char path[512]; int32_t out[CTL*2];
    snprintf(path,sizeof path,"%s/%s.wav",dir,name);
    FILE *f=fopen(path,"wb"); if(!f) exit(2); wav(f,FS*8u);
    rift_reset(); rift.active=1;
    for(uint32_t b=0;b<FS*8u;b+=CTL) {
        uint32_t n=FS*8u-b<CTL?FS*8u-b:CTL;
        if(b>=release) trk[0].p[P_AHOLD]=0;
        rift_audio(&trk[0],out,n);
        for(uint32_t k=0;k<n;k++) sample(f,out[k*2],out[k*2+1]);
    }
    fclose(f);
}
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    host_tracks_init();
    /* Check the complete tone-shaping path too: a parallel sine or chorus
     * must not accidentally bypass FIELD's source closure. */
    for(uint32_t mode=0;mode<9;mode++) {
        int32_t out[CTL*2];
        host_preset(&trk[0],ENGI_RIFT,mode); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_E4]=0; trk[0].p[P_REV]=0; trk[0].p[P_DIST]=0;
        rift_reset(); rift.active=1;
        for(uint32_t b=0;b<FS/2;b+=CTL) rift_audio(&trk[0],out,CTL);
        for(uint32_t k=0;k<CTL*2;k++) if(abs(out[k])>8) {
            printf("FIELD source closure failed in %s\n",RIFT_MODES[mode]); return 1;
        }
    }
    puts("FIELD closes the complete dry source path in all nine modes: ok");
    /* A two-tone input distinguishes LP and HP from a gain change. */
    static const int field[]={0,32,64,96};
    for(uint32_t k=0;k<4;k++) {
        char path[512]; snprintf(path,sizeof path,"%s/field-%d.wav",argv[1],field[k]);
        FILE *f=fopen(path,"wb"); if(!f) return 2; wav(f,FS*2u);
        int32_t ic1=0,ic2=0,bp; tsvf_t c;
        int32_t cut=field[k]<64?field[k]*256*127/64:(field[k]-64)*256*127/63;
        tsvf_coef(&c,cut,0);
        for(uint32_t i=0;i<FS*2u;i++) {
            int32_t x=(int32_t)(4000*sin(2*M_PI*100*i/FS)+4000*sin(2*M_PI*4000*i/FS));
            int32_t y=rift_field_filter(&c,x,field[k]*256,8192,&ic1,&ic2,&bp);
            sample(f,y,y);
        }
        fclose(f);
    }
    for(uint32_t k=0;k<3;k++) {
        char name[64]; snprintf(name,sizeof name,"cluster-%u",k);
        host_preset(&trk[0],ENGI_RIFT,2); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_CHOR]=60; trk[0].p[P_E1]=32+k*8; trk[0].p[P_E2]=120;
        trk[0].p[P_E3]=64; trk[0].p[P_E4]=64; trk[0].p[P_DIST]=64;
        trk[0].p[P_LPHASE]=24; trk[0].p[P_LRATE]=0; trk[0].p[P_REV]=0;
        render(argv[1],name,FS*8u);
    }
    /* Move through other coordinates, restore, then reset phases/tail to
     * compare the landscape itself rather than its persistent audio state. */
    int32_t out[CTL*2];
    trk[0].p[P_LPHASE]=110; trk[0].p[P_LRATE]=90;
    for(uint32_t k=0;k<500;k++) rift_audio(&trk[0],out,CTL);
    trk[0].p[P_LPHASE]=24; trk[0].p[P_LRATE]=0;
    render(argv[1],"cluster-return",FS*8u);
    /* Large wet tank, center CELL, same source and gain on both FEED sides. */
    static const int feed[]={0,64,127};
    for(uint32_t k=0;k<3;k++) {
        char name[64]; snprintf(name,sizeof name,"feed-%d",feed[k]);
        host_preset(&trk[0],ENGI_RIFT,8); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_E7]=feed[k]; trk[0].p[P_DLY]=64; trk[0].p[P_REV]=127;
        render(argv[1],name,FS*2u);
    }
    /* The two routing sides must be identical at zero shift, and have
     * different spectra when CELL translates the circulating signal. */
    for(uint32_t k=0;k<2;k++) {
        char name[64]; snprintf(name,sizeof name,"route-%u",k);
        host_preset(&trk[0],ENGI_RIFT,8); trk[0].p[P_AHOLD]=1;
        trk[0].p[P_E7]=k?127:0; trk[0].p[P_DLY]=100; trk[0].p[P_REV]=127;
        render(argv[1],name,FS*8u);
    }
    return 0;
}
