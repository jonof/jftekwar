/***************************************************************************
 *   CTM.C  - CyberMAXX support routines                                   *
 *            uses: SERIAL1.LIB functions                                  *
 *                                                                         *
 ***************************************************************************/

#include "string.h"
#include "build.h"
#include "names.h"
#include "stdarg.h"
#include "stdlib.h"
#include "time.h"
#include "io.h"
#include "fcntl.h"       
#include "sys\types.h"   
#include "sys\stat.h"
#include "serial.h"

#include "tekwar.h"

#ifdef __386__
#define   LES_MKFP(s,o)  (void far *)(s<<4)
#else
#define   LES_MKFP(s,o)  MK_FP(s,o)
#endif

struct serialData sd;

int  cyberenabled,
     iglassenabled,
     vfx1enabled;

void
ctm_deinit(void)
{
     if (cyberenabled) {
          deInitSerial(&sd);
          cyberenabled=0;
     }
}

void
ctm_init(int port)
{
     long cybcode=0,delayclock;

     if (initSerial(&sd,port-1,USE_16550,1024,1024) == SER_OK) {
          cyberenabled=1;
          setBPS(&sd,9600);
          writeSer(&sd,'X');
          delayclock=totalclock+CLKIPS;
          while (totalclock < delayclock && cybcode < 3) {
               if (rxBuffEmpty(&sd)) {
                    continue;
               }
               switch (cybcode) {
               case 0:
                    if (readSer(&sd) == 'C') {
                         cybcode++;
                    }
                    break;
               case 1:
                    if (readSer(&sd) == 'T') {
                         cybcode++;
                    }
                    break;
               case 2:
                    if (readSer(&sd) == 'M') {
                         cybcode++;
                    }
                    break;
               }
          }
          if (cybcode != 3) {
               showmessage("CyberMAXX not detected");
               ctm_deinit();
               return;
          }
          showmessage("CyberMAXX detected");
          writeSer(&sd,'C');       // CyberMAXX absolute coordinate mode
          writeSer(&sd,'F');       // CyberMAXX binary transfer mode
          writeSer(&sd,'N');       // CyberMAXX no checksum byte
          writeSer(&sd,'H');       // CyberMAXX continuous sample mode
          writeSer(&sd,128);       // 5 packets/sec sample rate
     }
}

short
getserch(struct serialData *s)
{
//     while (rxBuffEmpty(s));
     return((short)readSer(s));
}

void
ctm_read(short *yaw,short *pitch,short *roll)
{
     struct serialData *s;

     s=&sd;
     if (rxBuffEmpty(s)) {
          return;
     }
     if (s->bih->rxq.count < 8) {
          return;
     }
     if (getserch(s) == 0xFF) {
          if (getserch(s) == 0xFF) {
               *yaw=((getserch(s)<<8)|getserch(s));
               *pitch=((getserch(s)<<8)|getserch(s));
               *roll=((getserch(s)<<8)|getserch(s));
          }
     }
}

void
vio_deinit(void)
{
     deInitSerial(&sd);
     iglassenabled=0;
}

void
vio_reset(void)
{
     writeSer(&sd,'!');
     writeSer(&sd,'R');
     writeSer(&sd,0x0D);
}

void
vio_setup(void)
{
     writeSer(&sd,'!');
     writeSer(&sd,'M');
     writeSer(&sd,'2');  // cooked data mode
     writeSer(&sd,',');
     writeSer(&sd,'P');  // polled
     writeSer(&sd,',');
     writeSer(&sd,'B');  // binary
     writeSer(&sd,0x0D);
}

int
vio_init(int port)
{
     long delayclock,tries=0;

     if (initSerial(&sd,port-1,USE_16550,1024,1024) == SER_OK) {
          iglassenabled=0;
          setBPS(&sd,9600);
          vio_reset();
          delayclock=totalclock+(CLKIPS>>1);
          while (totalclock < delayclock && iglassenabled == 0 && tries < 10) {
               if (rxBuffEmpty(&sd)) {
                    continue;
               }
               if (readSer(&sd) == 'O') {
                    iglassenabled=port+1;
               }
               else {
                    vio_reset();
                    delayclock=totalclock+(CLKIPS>>1);
                    tries++;
               }
          }
          if (iglassenabled == 0) {
               vio_deinit();
               return(0);
          }
          vio_setup();
          return(1);
     }
     return(0);
}

void
vio_read(short *yaw,short *pitch,short *roll)
{
     short c,chk,chk2,yaw2,pitch2,roll2;
     long delayclock;
     struct serialData *s;

     s=&sd;
     flushRead(&sd);
     writeSer(&sd,'S');
     delayclock=totalclock+(CLKIPS>>3);
     while (totalclock < delayclock) {
          if (s->rx.q->count >= 8) {
               break;
          }
     }
     chk=0;
     pitch2=yaw2=roll2=0;
     if ((c=getserch(s)) == 0xFF) {
          chk+=c;
          yaw2=((c=getserch(s))<<8);    // yaw high byte
          chk+=c;
          yaw2+=(c=getserch(s));        // yaw low byte
          chk+=c;
          pitch2=((c=getserch(s))<<8);  // pitch high byte
          chk+=c;
          pitch2+=(c=getserch(s));      // pitch low byte
          chk+=c;
          roll2=((c=getserch(s))<<8);   // roll high byte
          chk+=c;
          roll2+=(c=getserch(s));       // roll low byte
          chk+=c;
          chk2=getserch(s);             // checksum
          if ((chk&0xFF) == chk2) {
               *yaw=yaw2;
               *pitch=pitch2;
               *roll=roll2;
          }
     }
}

extern
short pitch,
     roll,
     yaw;

void
whvrmon(void)
{
#if 0
     char tmpbuf[80];

     sprintf(tmpbuf,"Y: %05d P: %05d R: %05d C: %05d",yaw,pitch,roll,
          sd.rx.q->count);
     printext256(0L,8L,31,-1,tmpbuf,1);
#endif
}
