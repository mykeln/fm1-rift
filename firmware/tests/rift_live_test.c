/* SPDX-License-Identifier: GPL-3.0-only
 * Live control path through UI, mixer, audio ISR and simulated DAC. */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "../platforms/host/felucca_host.c"
static void run_ms(unsigned ms) { for(unsigned k=0;k<ms;k+=10) fh_render(441); }
static void wav(FILE *f, unsigned frames)
{
    uint32_t v; uint16_t w;
    fwrite("RIFF",1,4,f); v=36+frames*4; fwrite(&v,4,1,f); fwrite("WAVEfmt ",1,8,f);
    v=16; fwrite(&v,4,1,f); w=1; fwrite(&w,2,1,f); w=2; fwrite(&w,2,1,f);
    v=FS; fwrite(&v,4,1,f); v=FS*4; fwrite(&v,4,1,f); w=4; fwrite(&w,2,1,f); w=16; fwrite(&w,2,1,f);
    fwrite("data",1,4,f); v=frames*4; fwrite(&v,4,1,f);
}
static double energy(unsigned ms, FILE *f)
{
    double e=0;unsigned n=0;
    for(unsigned k=0;k<ms;k+=10) {
        fh_render(441);float *l=fh_out_l(),*r=fh_out_r();
        for(unsigned i=0;i<441;i++) {
            e+=l[i]*l[i]+r[i]*r[i];n+=2;
            if(f) { int16_t v[2]={(int16_t)(l[i]*32767),(int16_t)(r[i]*32767)};fwrite(v,2,2,f); }
        }
    }
    return e/n;
}
int main(int argc, char **argv)
{
    int fail=0;fh_nor_erase();fh_master(800);fh_boot();run_ms(600);
    fh_enc(EN_ALGO,4);run_ms(300);
    for(unsigned k=0;k<3;k++) {
        if(k) {fh_enc(EN_PRESET,1);run_ms(100);}
        int ok=trk[0].p[P_E0]==4 && trk[0].preset==4+k*9;
        printf("live: CHORD variation %u reaches audio state: %s\n",k+1,ok?"ok":"FAIL");fail+=!ok;
        for(unsigned fixed=0;fixed<2;fixed++) {
            fx_usb_fixed=fixed;
            FILE *f=0;
            if(argc>1 && !fixed) {
                char path[512];snprintf(path,sizeof path,"%s/CHORD-%u.wav",argv[1],k+1);
                f=fopen(path,"wb");if(!f)return 2;wav(f,FS);
            }
            double loud=energy(1000,f);if(f)fclose(f);
            fail+=loud<1e-8;
            fh_master(0);run_ms(1200);double quiet=energy(300,0);
            printf("live: physical master silences CHORD %u, USB fixed=%u: %s (%.1f dB attenuation)\n",
                   k+1,fixed,quiet<1e-12?"ok":"FAIL",10*log10((quiet+1e-30)/(loud+1e-30)));
            fail+=quiet>=1e-12;fh_master(800);run_ms(500);
        }

    }
    printf("live: %d failures\n",fail);return !!fail;
}
