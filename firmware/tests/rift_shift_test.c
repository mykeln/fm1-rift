/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#define FELUCCA_RIFT 1
#include "../platforms/host/firmware.c"
int main(void)
{
    int fail=0;
    for(int half=0;half<2;half++) for(int f=0;f<3;f++) for(int direction=-1;direction<=1;direction+=2){
        double rate=FS/(half+1.0);
        double hz=f==0?65:f==1?130:1000, shift=f<2?5:100;
        double ur=0,ui=0,lr=0,li=0,energy=0;
        uint32_t ph=0,sp=0,inc=(uint32_t)(hz/rate*4294967296.0);
        int32_t si=(int32_t)(shift/rate*4294967296.0)*direction;
        rift_reset();
        for(uint32_t b=0;b<(uint32_t)rate*2u;b++){
            int32_t x=sine_i(ph+0x40000000u)/4;
            int32_t y=rift_shift_sample(0,x,sine_i(sp+0x40000000u),sine_i(sp));
            if(b>=(uint32_t)rate){double t=(double)b/rate;
                ur+=y*cos(2*M_PI*(hz+shift)*t);ui+=y*sin(2*M_PI*(hz+shift)*t);
                lr+=y*cos(2*M_PI*(hz-shift)*t);li+=y*sin(2*M_PI*(hz-shift)*t);energy+=(double)y*y;
            }
            ph+=inc;sp+=(uint32_t)si;rift.hw=(rift.hw+1u)&31u;
        }
        double upper=ur*ur+ui*ui,lower=lr*lr+li*li;
        double reject=10*log10((direction>0?upper:lower)/(direction>0?lower:upper));
        double gain=sqrt(energy/rate)/(8192/sqrt(2));
        int ok=reject>20 && gain>.8 && gain<1.2;
        printf("shift @ %.0fHz: %.0fHz %+gHz: rejection=%.1fdB gain=%.3f %s\n",rate,hz,direction*shift,reject,gain,ok?"ok":"FAIL");
        fail+=!ok;
    }
    return !!fail;
}
