#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "pragmas.h"

#include "tekwar.h"

#if 0

/***************************************************************************
 *   TEKSMK.C  -   smack flic stuff etc. for Tekwar                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "conio.h"
#include "dos.h"
#include "io.h"
#include "fcntl.h"
#include "sys\stat.h"
#include "sys\types.h"
#include "build.h"
#include "names.h"

#include "tekwar.h"
#include "smack.h"

//#define   SMKDEBUG

typedef  unsigned   WORD;

#define   MAXFRAMES           300
#define   MAXRADBUFFS         16

#pragma aux setvmode =\
	"int 0x10",\
	parm [eax]\

extern    char      cddriveletter;
extern    char      generalplay;
extern    int       nobriefflag;
extern    char      novideoid;

struct    radbuftype     {
     long      cache_ptr;
     long      cache_length;
     char      cache_lock;
};
struct    radbuftype     radbuf[MAXRADBUFFS];

long      smacktrak;


RCFUNC 
void PTR4* RADLINK 
radmalloc(u32 numbytes)
{
     int       i;
     
     for( i=0; i<MAXRADBUFFS; i++ ) {
          if( radbuf[i].cache_ptr == 0L ) {
               break;
          }
     }
     if( i == MAXRADBUFFS ) {
          crash("radmalloc ran out of buffptrs");
          //return( ( void PTR4*)NULL);
          smacktrak=0L;
     }

     radbuf[i].cache_lock=1;
     radbuf[i].cache_length=numbytes;
     allocache(&(radbuf[i].cache_ptr), radbuf[i].cache_length, &(radbuf[i].cache_lock));
     if( radbuf[i].cache_ptr == 0L ) {
          crash("radmalloc failed on %d", i);
     }

     return( ( void PTR4*)radbuf[i].cache_ptr);
}

RCFUNC 
void RADLINK 
radfree(void PTR4* ptr)
{
     int       i;
     
     for( i=0; i<MAXRADBUFFS; i++ ) { 
          if( radbuf[i].cache_ptr == ( long)ptr ) {
               radbuf[i].cache_lock=0;
               break;
          }
     }
     uninitcache();
}

void
videocheck()
{
     char      vdriver[64];

     SVGADetect(1);
     memset(vdriver,0,64);
     SVGADetected(vdriver);
     if( strcmp(vdriver,"Vanilla VGA") == 0 ) {
          if( novideoid == 0 ) {
               printf(" ERROR: VESA not found - please load a VESA driver.\n"); 
               exit(-1);
          }
     }
}

void
smkinit(WORD digifh)
{
     if( digifh != -1 ) {
          SetSmackSOSHandle(digifh, CLKIPS);     
          smacktrak=SMACKTRACK1;
     }
     else {
          SetSmackSOSHandle(0, CLKIPS);     
          smacktrak=0;
     }
}

void
smkuninit(WORD fhptr) 
{
     if( fhptr == ( WORD)9999 ) {
          LowSoundDriverUnload();
     }
}

void
smkplayseq(char *name)
{
     u32            i;
     int            fh,s;
     Smack          *smk;
    #ifdef SMKDEBUG
     SmackSum       smksum;
    #endif
     char           smkname[32];

     if( (generalplay != 0) || (nobriefflag != 0) ) {
          return;
     }

     uninitcache(0);

     memset(smkname, 0, sizeof(smkname));
     smkname[0]=cddriveletter;
     strcat(smkname, ":\\TEKWAR\\SMK\\");
     strcat(smkname, name);
     strcat(smkname, ".SMK");
     smk=SmackOpen(smkname,smacktrak,0);

     if( smk ) {
          SVGADetect(1);
          if( SVGASetup(smk->Width,smk->Height) ) {
               SmackToScreen(smk,0,0,SVGABytesPS(),SVGAWinTbl(),SVGASetBank());
               SVGASetGraph();
               for( i=1; i<=smk->Frames; i++ ) {
                    if( smk->NewPalette ) {
                         PaletteSet((smk->CurPalette == 1) ? smk->Col1:smk->Col2);
                    }
                    SmackDoFrame(smk);
                    if( i != smk->Frames ) {
                         SmackNextFrame(smk);
                    }
                    do {
                         if( keystatus[1] != 0 ) {
                              keystatus[1]==0;
                              goto done;
                         }
                    } while( SmackWait(smk) );
               }
          }  
done:
    #ifdef SMKDEBUG 
     SmackSummary(smk, &smksum);
    #endif
     SmackClose(smk);
     }

     for( i=0; i<MAXRADBUFFS; i++ ) { 
          radbuf[i].cache_lock=0;
     }

     uninitcache(0);
}

#endif

#include "smacker.h"

static smk smkmenu;
#define SMKPICNUM (MAXTILES-1)

void
smkplayseq(char *name)
{
    debugprintf("smkplayseq(\"%s\")\n", name);
}

void
smkopenmenu(char *name)
{
    unsigned long xsiz, ysiz;
    
    smkmenu = smk_open_file(name, SMK_MODE_DISK);
    if (!smkmenu) {
        debugprintf("smk_open_file(\"%s\") returned null\n", name);
        return;
    }

    // Smacker frames are decoded linewise, but BUILD expects its
    // tiles columnwise, so we will treat the tile as though it's
    // rotated 90 degrees and flipped horizontally.
    
    smk_enable_all(smkmenu, SMK_VIDEO_TRACK);
    smk_info_video(smkmenu, &xsiz, &ysiz, NULL);
    tilesizx[SMKPICNUM] = ysiz;
    tilesizy[SMKPICNUM] = xsiz;
}

void
smkmenuframe(int fn)
{
    unsigned char spal[768], *palptr;
    int i;
    
    if (!smkmenu) {
        return;
    }
    
    // Render the first frame as the menu background.
    smk_first(smkmenu);
    
    // Next, render the particular frame requested.
    smk_render_frame(smkmenu, fn-1);

    // Convert the palette to the VGA (0-63) range BUILD uses.
    palptr = smk_get_palette(smkmenu);
    for (i=0; i<768; i++, palptr++) {
        spal[i] = (*palptr) >> 2;
    }
    
    waloff[SMKPICNUM] = smk_get_video(smkmenu);
    invalidatetile(SMKPICNUM, 0, -1);

    setbrightness(brightness, spal, 0);
}

void
smkshowmenu()
{
    if (!smkmenu) {
        return;
    }

    clearview(0);
    rotatesprite(160<<16, 100<<16, divscale16(200, tilesizx[SMKPICNUM]),
                 512, SMKPICNUM, 0, 0, 2+4, 0, 0, xdim-1, ydim-1);
    nextpage();
}

void
smkclosemenu()
{
    if (!smkmenu) {
        return;
    }
    
    waloff[SMKPICNUM] = 0;
    
    smk_close(smkmenu);
}