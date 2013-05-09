/***************************************************************************
 *   TEKTXT.C  - text based & MiSCcellaneous routines for Tekwar game      *
 *   Uses BIOS modes for compatibility and scrolling.                      *
 *   Includes CD code.                                                     *
 *                                                     12/14/94 Les Bird   *
 ***************************************************************************/

#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "time.h"
#include "dos.h"

#include  "i86.h"
#include "build.h"

#include "tekwar.h"        
#include "tekver.c"

#define   CDRELEASE
#define   COPYRIGHTSTRING     "INTRACORP1995TW"
#define   GMIN           60
#define   GHRS           3600

struct    dostime_t dtime;
char      bypasscdcheck=0;
char      cddriveletter='D';
unsigned  criticalerror=0L;

struct    devhdrtype {
     long      nextdrvr;
     short     attrib;
     short     str_routine;
     short     int_routine;
     char      name[8];
     char      idunno[2];
     char      drive;
     char      numunits;
};

struct    devlsttype {
     char      subunit;
     struct    devhdrtype    *devhdr;
};

struct    dosreqhdrtype {
	char      length;  
	char      subunit;
	char		commandcode;
	short	status; 
	char		rsrvd[8];
};

struct    ioctlhdrtype {
     struct    dosreqhdrtype  doshdr;
     char      mediadescriptor;     // always 0
     char      *ctrlblkptr;
     short     blklength;
     short     startsector;         // always 0
     char      *volidptr;           // always 0
};

struct    rminfo    {
     long      edi,
               esi,
               ebp;
     long      reserved_by_system;
     long      ebx,
               edx,
               ecx,
               eax;
     short     flags;
     short     es,ds,fs,gs,ip,cs,sp,ss;
};
struct    rminfo    rmi;

union     REGS      regs;
struct    SREGS     sregs;

extern    void cduninit(void);

void      far  *reqptr;
char      gcdsubunit;
short     gcdhdraddr;
short     gcdstr;
short     gcdint;
short     gcderror;
short     gcdbusy;
short     gcddone;
short     gcderrcode;

enum {
     BLACK=0,
     BLUE,
     GREEN,
     CYAN,
     RED,
     MAGENTA,
     BROWN,
     LIGHTGRAY,
     DARKGRAY,
     LIGHTBLUE,
     LIGHTGREEN,
     LIGHTCYAN,
     LIGHTRED,
     LIGHTMAGENTA,
     YELLOW,
     WHITE
};

int  ovmode,
     vmodeflag=0;

static
int  cursbot,curstop,
     cursx,cursy,
     winx1,winy1,
     winx2,winy2,
     winatr;

#pragma aux setvmode =\
 	"int 0x10",\
	parm [eax]\


clock_t   crstart,crend;     
clock_t   gameseconds;

void
startgametime()
{
#ifdef TIMERESTORE
     _dos_gettime(&dtime);
     crstart=clock();
#endif
}

void
endgametime()
{
#ifdef TIMERESTORE     
     crend=clock();
     gameseconds=crend/CLOCKS_PER_SEC;

     printf("%lu seconds expired.\n", gameseconds);

     dtime.second=( unsigned char)(gameseconds%60L);
     dtime.minute=( unsigned char)(gameseconds/60L);
     dtime.hour  =( unsigned char)(gameseconds/3600L);;
     _dos_settime(&dtime);
#endif
}


int
getvmode(void)
{
     union REGS regs;

     regs.h.ah=0x0F;
     regs.h.al=0x00;
     int386(0x10,&regs,&regs);
     return(regs.h.al);
}

void
crash(char *s,...)
{
     va_list argptr;

     musicoff();
	sendlogoff();        
	uninitmultiplayers();
	uninitsb();
     cduninit();
	uninittimer();
	uninitkeys();
	uninitengine();
	uninitgroupfile();
     teksavesetup();
	setvmode(ovmode); 
     endgametime();

     va_start(argptr,s);
     vprintf(s,argptr);
     va_end(argptr);
     printf("\n");
     exit(0);
}

void
setatr(int fore,int back)
{
     winatr=(((back&0x0F)<<4)|(fore&0x0F));
}

void
setcsize(int t,int b)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x01;
     regs.h.ch=t;
     regs.h.cl=b;
     int386(0x10,&regs,&regs);
}

void
gotoxy(int x,int y)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x02;
     regs.h.bh=0;
     regs.h.dh=y;
     regs.h.dl=x;
     int386(0x10,&regs,&regs);
     cursx=x;
     cursy=y;
}

void
getcurs(void)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x03;
     regs.h.bh=0;
     int386(0x10,&regs,&regs);
     if (curstop == -1) {
          curstop=regs.h.ch;
     }
     if (cursbot == -1) {
          cursbot=regs.h.cl;
     }
     cursy=regs.h.dh;
     cursx=regs.h.dl;
}

void
setregion(int x1,int y1,int x2,int y2)
{
     winx1=x1;
     winy1=y1;
     winx2=x2;
     winy2=y2;
}

void
scrollup(void)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x06;
     regs.h.al=1;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
}

void
scrolldn(void)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x07;
     regs.h.al=1;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
}

void
clrregion(int fore,int back)
{
     union REGS regs;

     if (vmodeflag) {
          return;
     }
     setatr(fore,back);
     memset(&regs,0,sizeof(union REGS));
     regs.h.ah=0x06;
     regs.h.al=0;
     regs.h.bh=winatr;
     regs.h.ch=winy1;
     regs.h.cl=winx1;
     regs.h.dh=winy2;
     regs.h.dl=winx2;
     int386(0x10,&regs,&regs);
}

void
tprintf(char *fmt,...)
{
     char buf[80],*ptr;
     va_list arglist;
     union REGPACK regs;

     if (vmodeflag) {
          return;
     }
     va_start(arglist,fmt);
     vsprintf(buf,fmt,arglist);
     va_end(arglist);
     if ((ptr=strchr(buf,'\n')) != NULL) {
          getcurs();
          if (cursy == 24) {
               scrollup();
               gotoxy(0,cursy);
          }
          else {
               gotoxy(0,cursy+1);
          }
          *ptr=0;
     }
     memset(&regs,0,sizeof(union REGPACK));
     regs.h.ah=0x13;
     regs.h.al=1;
     regs.h.bh=0;
     regs.h.bl=winatr;
     regs.w.cx=strlen(buf);
     regs.h.dh=cursy;
     regs.h.dl=cursx;
     regs.w.es=FP_SEG(buf);
     regs.x.ebp=FP_OFF(buf);
     intr(0x10,&regs);
     getcurs();
     if (ptr != NULL) {
          gotoxy(0,cursy);
     }
}

void
printfat(int x,int y,char *fmt,...)
{
     char buf[80];
     va_list arglist;

     if (vmodeflag) {
          return;
     }
     va_start(arglist,fmt);
     vsprintf(buf,fmt,arglist);
     va_end(arglist);
     if (x == -1) {
          x=40-strlen(buf)/2;
     }
     else if (x == 0) {
          x=cursx;
     }
     if (y == 0) {
          y=cursy;
     }
     gotoxy(x,y);
     tprintf(buf);
}

int  _far
criterrhndlr(unsigned deverr, unsigned errcode, unsigned far *devhdr)
{
     criticalerror=errcode;
     _hardresume(_HARDERR_FAIL);
     return(_HARDERR_FAIL);
}

int
getDOSmem(int paragraphs, int *selector )
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0100;
	regs.w.bx = paragraphs;
	int386(0x31, &regs, &regs );

	if( regs.x.cflag ) {
		return 0;
	}
	else {
		*selector = regs.w.dx;
		return regs.w.ax;
	}
}

int
freeDOSmem(int selector  )
{
	union REGS regs;

	memset(&regs, 0, sizeof(regs));
	regs.w.ax = 0x0101;
	regs.w.dx = selector;
	int386(0x31, &regs, &regs );

	return regs.x.cflag;
}

int
cdinfo(void)
{
     int       rmsegment,rmselector;
     struct    devlsttype     *devlst;
     struct    devhdrtype     *devhdr;
     long      hdraddr;

	rmsegment=getDOSmem(12, &rmselector);
	if( !rmsegment ) {
		return(-1);
	}

     // CD-ROM 1501: get driver map
     memset(&rmi, 0, sizeof(rmi));
     memset(&regs,0,sizeof(union REGS));
     memset(&sregs,0,sizeof(struct SREGS));
     rmi.eax=0x00001501; 
     rmi.es=rmsegment;
     rmi.ebx=0;
     regs.w.ax=0x0300;
     regs.h.bl=0x2F;
     regs.h.bh=0;
     regs.w.cx=0;
     sregs.es=FP_SEG(&rmi);
     regs.x.edi=FP_OFF(&rmi);
     int386x(0x31, &regs,&regs,&sregs);

     devlst=( struct devlsttype *)(rmsegment<<4);
     hdraddr=( long)(devlst->devhdr);
     devhdr=( struct devhdrtype *)((hdraddr>>12)&0x000FFFF0);

    #define VERBOSE
    #ifdef  VERBOSE
     printf("CD Driver Strategy Routine %04x:%04x\n", ( short)(hdraddr>>16)&0x0000FFFF, devhdr->str_routine);
     printf("CD Driver InterruptRoutine %04x:%04x\n", ( short)(hdraddr>>16)&0x0000FFFF, devhdr->int_routine);
     printf("Driver Name %s\n", devhdr->name);
     printf("Drive Letter %c\n", 'A'+devhdr->drive-1);
    #endif

     gcdsubunit=devlst->subunit;
     gcdhdraddr=( short)((hdraddr>>16)&0x0000FFFF);
     gcdstr=devhdr->str_routine;
     gcdint=devhdr->int_routine;

     freeDOSmem(rmselector);
     return(0);
}

int
cdioctl(int action)
{
     int       rmsegment,rmselector,blksz;
     char      *lowp;
     struct    ioctlhdrtype   ioctlhdr;
     char      ccode;
     char      *writebuf;
     //va_list   optparm;

	rmsegment=getDOSmem(4, &rmselector);
	if( !rmsegment ) {
		return(-1);
	}

     // check for write buffer
     //va_start(optparm, action);
     //if( va_arg(optparm, char *) == ( char *)NULL ) {
     //     writebuf=( char *)NULL;
     //}
     //va_end(optparm);

     switch( action ) {
     case 0: ccode=0; blksz=1; break;   // eject
     case 2: ccode=2; blksz=1; break;   // reset drv
     case 5: ccode=5; blksz=1; break;   // close
     default: 
          return(-1);
     }

     ioctlhdr.doshdr.length=sizeof(struct ioctlhdrtype);
     ioctlhdr.doshdr.subunit=gcdsubunit;
     ioctlhdr.doshdr.commandcode=12;
     ioctlhdr.doshdr.status=0;
     ioctlhdr.mediadescriptor=0;
     ioctlhdr.ctrlblkptr=( char *)&ccode;
     ioctlhdr.blklength=blksz;
     ioctlhdr.startsector=0;
     ioctlhdr.volidptr=0;

	lowp=( char *)(rmsegment<<4);
     reqptr=( void far *)&ioctlhdr;
	_fmemcpy(( void far *)lowp, reqptr, sizeof(struct ioctlhdrtype));

     // les  bx, dword ptr [ioctlhdr]
     // call dword ptr [str_routine]
     memset(&rmi, 0, sizeof(struct rminfo));
     memset(&regs,0,sizeof(union REGS));
     memset(&sregs,0,sizeof(struct SREGS));
     rmi.cs=gcdhdraddr;
     rmi.ip=gcdstr;
     rmi.es=rmsegment;
     segread(&sregs);
     regs.x.eax=0x301;    
     regs.x.ebx=0;
     regs.x.ecx=0;
     sregs.es=FP_SEG(&rmi);
     regs.x.edi=FP_OFF(&rmi);
     int386x(0x31,&regs,&regs,&sregs);

     if( regs.w.cflag != 0 )
          printf("dpmi 301 error on str\n");

     // les  bx, dword ptr [ioctlhdr]
     // call dword ptr [int_routine]
     //memset(&rmi, 0, sizeof(struct rminfo));
     memset(&regs,0,sizeof(union REGS));
     memset(&sregs,0,sizeof(struct SREGS));
     rmi.cs=gcdhdraddr;
     rmi.ip=gcdint;
     rmi.es=rmsegment;
     segread(&sregs);
     regs.x.eax=0x301;
     regs.x.ebx=0;
     regs.x.ecx=0;
     sregs.es=FP_SEG(&rmi);
     regs.x.edi=FP_OFF(&rmi);
     int386x(0x31,&regs,&regs,&sregs);

     if( regs.w.cflag != 0 )
          printf("dpmi 301 error on int\n");

	_fmemcpy(reqptr, ( void far *)lowp, sizeof(struct ioctlhdrtype));

     gcderror=0x0000;
     gcdbusy =0x0000;
     gcddone =0x0000;
     gcderrcode=0x0000;
     gcderror  =(ioctlhdr.doshdr.status&0x8000);
     gcdbusy   =(ioctlhdr.doshdr.status&0x0200);
     gcddone   =(ioctlhdr.doshdr.status&0x0100);
     gcderrcode=(ioctlhdr.doshdr.status&0x00FF);

     freeDOSmem(rmselector);
     return(0);
}

struct    meminfotype {
     long      lab;
     long      mupa;
     long      mlpa;
     long      tlp;
     long      tup;
     long      fp;
     long      tpp;
     long      flp;
     long      spf;
     char      rsrvd[12];
};

void
meminfo(void)
{
     struct    meminfotype    meminfo;
     
     memset(&regs,0,sizeof(union REGS));
     memset(&sregs,0,sizeof(struct SREGS));
     regs.w.ax=0x0500;
     sregs.es=FP_SEG(&meminfo);
     regs.x.edi=FP_OFF(&meminfo);
     int386x(0x31, &regs,&regs,&sregs);
     printf("largest available block      : %ld bytes\n",   meminfo.lab);
     printf("max unlocked page allocation : %ld\n",         meminfo.mupa);
     printf("max   locked page allocation : %ld\n",         meminfo.mlpa);
     printf("total linear addr space      : %ld pages\n",   meminfo.tlp);
     printf("total unlocked pages         : %ld\n",         meminfo.tup);
     printf("free pages                   : %ld\n",         meminfo.fp);
     printf("total physical pages         : %ld\n",         meminfo.tpp);
     printf("free linear addr space       : %ld pages\n",   meminfo.flp);
     printf("size of file/partition       : %ld pages\n",   meminfo.spf);
}

void
dpmiinfo(void)
{
     memset(&regs,0,sizeof(union REGS));
     memset(&sregs,0,sizeof(struct SREGS));
     regs.w.ax=0x0400;
     int386x(0x31, &regs,&regs,&sregs);
     if( regs.w.cflag != 0 ) {
          printf("** DPMI host undetected **\n");
     }
     else {
          printf(     "DPMI host: supports spec version %2d : %2d\n", regs.w.ax&0x00FF, regs.w.ax&0xFF00);
          if( regs.w.bx&0x0001 ) 
               printf("           32 bit implementation\n");
          if( regs.w.bx&0x0002 ) 
               printf("           returns to real mode for reflected ints\n");
          if( regs.w.bx&0x0004 ) 
               printf("           virtual memory supported\n");
          switch( regs.h.cl ) {
          case 0x02: printf("           286 processor\n"); break;
          case 0x03: printf("           386 processor\n"); break;
          case 0x04: printf("           486 processor\n"); break;
          default: printf("           486 or better processor\n");
          }
          printf("           Virtual master IC base interrupt at 0x%0x\n", regs.h.dh);
          printf("           Virtual slave  IC base interrupt at 0x%0x\n", regs.h.dl);
     }
}

int
verifycd()
{
     int       i;
     union     REGS      regs;
     struct    SREGS     sregs;
     int       rmsegment,rmselector;
     char      buffer[64];
     char _far *bufptr=( char _far *)&buffer;

	rmsegment=getDOSmem(4, &rmselector);
    
     for( i=2; i<24; i++ ) {
          criticalerror=0;

          //asm  mov  ax, 0x1502
          //asm  les  bx, DWORD PTR buffer
          //asm  mov  cx, i 
          memset(&rmi, 0, sizeof(rmi));
          rmi.eax = 0x00001502;
          rmi.ebx = 0x00000000;
          rmi.es  = rmsegment;
          rmi.ecx = ( long)i;

          memset(&regs, 0, sizeof(regs));
          memset(&sregs, 0, sizeof(sregs));
          regs.w.ax=0x0300;
          regs.h.bl=0x2F;
          regs.h.bh=0;
          regs.w.cx=0;
          sregs.es=FP_SEG(&rmi);
          regs.x.edi=FP_OFF(&rmi);
          int386x(0x31,&regs,&regs,&sregs);

          if( (criticalerror) || (rmi.flags & 0x0001) ) {
               continue;
          }

          _fstrcpy(( void far *)bufptr, ( void far *)MK_FP(rmselector, 0));
          if( (strncmp(( void *)bufptr, COPYRIGHTSTRING, 15)) == 0 ) {
               freeDOSmem(rmselector);
               return(i);
          }
     }

     freeDOSmem(rmselector);
     return(0);
}

extern    int near asmcpuid(void);
#pragma aux asmcpuid  "*_"         \ 
    modify          [ebx ecx edx]; \    

extern    void videocheck(void);

void
tektextmode(void)
{
     int       i,dl;

     // check for game won file and delete it
     if( access("tekv.dat", F_OK) == 0 ) {
          remove("tekv.dat");
     }

     ovmode=getvmode();
     setvmode(0x03);
     setregion(0,0,79,24);
     clrregion(WHITE,BLACK);
     gotoxy(0,0);
     setatr(WHITE,MAGENTA);
     for (i=0 ; i < 80 ; i++) {
          tprintf(" ");
     }
     gotoxy(0,0);
     printfat(-1,0,TITLE,VERS);
     setregion(0,1,79,24);
     gotoxy(0,2);
     setatr(MAGENTA,WHITE);

     _harderr(criterrhndlr);          

    #ifdef VERIFYCPU
     switch( asmcpuid() ) {
     case 1:
          printf("TEKWAR requires 486 or better.\n");
          exit(-1);
     case 2:
          // is a 486
          break;
     default:
          break;
     }
    #endif

     installgphandler();

    #ifdef CDRELEASE
     dl=verifycd();
     if( dl != 0 ) {
          printf(" TEKWAR CD in drive %c.\n", 'A'+dl);
          cddriveletter=('A'+dl);
     }
     else if( bypasscdcheck == 0 ) {
          printf(" ERROR: cant find TEKWAR CD.\n\n");
          exit(-1);
     }
    #endif

     videocheck();
}



