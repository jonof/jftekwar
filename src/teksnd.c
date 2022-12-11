/***************************************************************************
 *   TEKSND.C  - A HMI replacement by Jonathon Fowler using JFAudioLib     *
 *                                                                         *
 ***************************************************************************/

#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "fx_man.h"
#include "tekwar.h"

#ifdef _WIN32
#include "winlayer.h"
#endif

//defines for looping sound variables
extern    int       loopinsound;
extern    int       baydoorloop;
extern    int       ambsubloop;

// a 1-1 map from sounds in sound file to this array
struct    soundbuffertype {
     int       users;
     int       offset;
     void *    cache_ptr;
     int       cache_length;
     unsigned char cache_lock;
};

struct    soundtype {
     int       handle;
     int       sndnum;
     int       plevel;
     int       x,y;
     short     type;
};

static struct    soundbuffertype    sbuf[TOTALSOUNDS];
static struct    soundtype     dsound[MAXSOUNDS];

#define NULL_HANDLE (-1)
#define  AMBUPDATEDIST  4000L

#if 0

/***************************************************************************
 *   TEKSND.C  - HMI library replaces Kens sound stuff                     *
 *               Also timer routine and keytimerstuff is here              *
 *                                                                         *
 ***************************************************************************/

#define   BASESONG            0
#define   MAXBASESONGLENGTH   44136
#define   AVAILMODES          3
#define   SONGSPERLEVEL       3
#define   NUMLEVELS       7

int       totalsongsperlevel;
char      basesongdata[MAXBASESONGLENGTH];
char      secondsongdata[MAXBASESONGLENGTH];
char      thirdsongdata[MAXBASESONGLENGTH];

struct    songtype  {
     int       handle;
     int       offset;
     int       playing;
     int       pending;
     char     *buffer;
     long      length;
};
struct    songtype  song[SONGSPERLEVEL];
struct    songtype  *songptr[SONGSPERLEVEL];

int       songlist[4096];

stopsong(int sn)
{
     if( musicmode == MM_NOHARDWARE )
          return;

     if( songptr[sn]->playing == 0 ) {
          return;
     }
     if( songptr[sn]->pending != 0 ) {    // cant stop a pending song
          return;                         // since may be interrupted
     }                                    // by trigger function

     sosMIDIStopSong(songptr[sn]->handle);
     songptr[sn]->playing=0;
}

removesong(int sn)
{
     if( musicmode == MM_NOHARDWARE )
          return;

     if( songptr[sn]->handle != NULL_HANDLE ) {
          songptr[sn]->pending=0;
          sosMIDIStopSong(songptr[sn]->handle);
          sosMIDIUnInitSong(songptr[sn]->handle);
          songptr[sn]->handle=NULL_HANDLE;
          songptr[sn]->playing=0;
     }
}

int
playsong(int sn)
{
     int       rv;
     int       fpos;

     if( (musicmode == MM_NOHARDWARE) || (toggles[TOGGLE_MUSIC] == 0) ) {
          return(0);
     }
     if( (sn < 0) || (sn >= SONGSPERLEVEL) || (songptr[sn]->playing != 0) || (songptr[sn]->pending != 0) ) {
          return(0);
     }

     if( songptr[sn]->handle != NULL_HANDLE ) {
          removesong(sn);
     }
     if( songptr[sn]->length == 0 )
         return(0);

     songdataptr->lpSongData=( LPSTR)songptr[sn]->buffer;
     songdataptr->lpSongCallback=( VOID _far *)NULL; //songcallback;

     fpos=flushall();
     if( songptr[sn]->handle == NULL_HANDLE ) {
          lseek(fhsongs,0,SEEK_SET);
          fpos=filelength(fhsongs);
          lseek(fhsongs, songptr[sn]->offset, SEEK_SET);
          fpos=tell(fhsongs);
          rv=read(fhsongs, ( void *)songptr[sn]->buffer, songptr[sn]->length);
          if( rv != songptr[sn]->length ) {
              crashgame("playsong: bad read");
          }
          rv=sosMIDIInitSong(songdataptr, trackmapptr, ( WORD _far *)&(songptr[sn]->handle));
          if( rv != _ERR_NO_ERROR ) {
               songptr[sn]->handle=NULL_HANDLE;
               return(0);
          }
     }
     else {
          rv=sosMIDIResetSong(songptr[sn]->handle, songdataptr);
          if( rv != _ERR_NO_ERROR ) {
               songptr[sn]->handle=NULL_HANDLE;
              #ifdef MUSICDEBUG
               showmessage("CANT RESET SONG %2d", sn);
              #endif
          }
     }

     rv=sosMIDIStartSong(songptr[sn]->handle);
     if( rv != _ERR_NO_ERROR ) {
          songptr[sn]->handle=NULL_HANDLE;
          return(0);
     }

     if( (musicv<<3) > 0 ) {
          sosMIDIFadeSong(songptr[sn]->handle,_SOS_MIDI_FADE_IN,250,
                          0,(musicv<<3), 50);
     }

    #ifdef MUSICDEBUG
     showmessage("PLAYING SONG %2d", sn);
    #endif
     songptr[sn]->playing=1;
     songptr[sn]->pending=0;

     return(1);
}

#endif

static int fxstarted = 0, fhsounds = -1;

static void
setupdigi(void)
{
     int i;
     int32_t digilist[1024];

     fhsounds = kopen4load("sounds", 0);
     if( fhsounds < 0 ) {
          crashgame("setupdigi: cant open sounds");
     }
     memset(digilist, 0, sizeof(digilist));
     klseek(fhsounds, -sizeof(digilist), SEEK_END);
     i = kread(fhsounds, digilist, sizeof(digilist));
     if( i != sizeof(digilist) ) {
          crashgame("setupdigi: bad read of digilist");
     }

     memset(&sbuf, 0, sizeof(sbuf));
     for( i=0; i<TOTALSOUNDS; i++ ) {
          sbuf[i].offset=(digilist[i*3]*4096L);
          sbuf[i].cache_ptr=0L;
          sbuf[i].cache_length=digilist[i*3+1];
          sbuf[i].cache_lock=0x00;
     }
     for( i=0; i<MAXSOUNDS; i++ ) {
          dsound[i].handle=NULL_HANDLE;
          dsound[i].sndnum=0;
          dsound[i].plevel=0;
          dsound[i].x=0L;
          dsound[i].y=0L;
          dsound[i].type=ST_UPDATE;
          dsound[i].sndnum=-1;
     }
}

static void
setupmidi(void)
{
     /*
     int       i;

     if( musicmode == MM_NOHARDWARE ) {
          return;
     }

     if( musicmode != MM_NOHARDWARE ) {
          if( (fhsongs=open("SONGS",O_RDONLY | O_BINARY)) == -1 ) {
               crashgame("setupmidi: cant open songs");
          }
          lseek(fhsongs, 0, SEEK_SET);
          lseek(fhsongs, -4096, SEEK_END);
          read(fhsongs, ( void *)songlist, 4096);
     }

//jsa venom
     for( i=0; i<SONGSPERLEVEL; i++ ) {
          songptr[i]=&song[i];
          songptr[i]->handle=NULL_HANDLE;
          songptr[i]->offset=0;
          songptr[i]->playing= 0;
          songptr[i]->pending=0;
          songptr[i]->length=0L;
     }
     songptr[0]->buffer=&basesongdata;
     songptr[1]->buffer=&secondsongdata;
     songptr[2]->buffer=&thirdsongdata;

     totalsongsperlevel=SONGSPERLEVEL*AVAILMODES;
     */
}

static void
soundcallback(unsigned int i)
{
     if (i == (unsigned int)-1) return;

     sbuf[dsound[i].sndnum].users--;
     if( sbuf[dsound[i].sndnum].users < 0 )
          sbuf[dsound[i].sndnum].users=0;
     if( sbuf[dsound[i].sndnum].users == 0 ) {
          sbuf[dsound[i].sndnum].cache_lock=199;
     }
     dsound[i].handle=NULL_HANDLE;
     dsound[i].plevel=0;
     dsound[i].sndnum=-1;

     if (dsound[i].type&ST_VEHUPDATE) {
          vehiclesoundstopped(i);
     }
}


void
initsb(char digistat,char musistat,int mixrate,
    char numspeakers,char bytespersample,char intspersec,char quality)
{
     int err, channels = numspeakers, bitspersample = 4<<bytespersample;
     void *initdata = NULL;

     (void)digistat; (void)musistat; (void)intspersec; (void)quality;

     #ifdef _WIN32
     initdata = (void *) win_gethwnd();
     #endif

     err = FX_Init(ASS_AutoDetect, 16, &channels, &bitspersample, &mixrate, initdata);
     if (err != FX_Ok) {
          buildprintf("FX_Init error: %s\n", FX_ErrorString(err));
          return;
     }

     fxstarted = 1;

     FX_SetCallBack(soundcallback);
     setupdigi();

     //setupmidi();
}

void
uninitsb(void)
{
     int i;

     /*
     if( musicmode != MM_NOHARDWARE )  {
          for( i=0; i<SONGSPERLEVEL; i++ ) {
               if( songptr[i]->handle == NULL_HANDLE )
                    continue;
               sosMIDIStopSong(songptr[i]->handle);
               sosMIDIUnInitSong(songptr[i]->handle);
          }
          sosMIDIUnInitDriver(*fhmididriverptr, _TRUE );
          sosMIDIUnInitSystem();
     }
     */

     if( fxstarted ) {
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsound[i].handle == NULL_HANDLE )
                    continue;
               FX_StopSound(dsound[i].handle);
          }
          FX_Shutdown();
     }

     if (fhsounds >= 0) {
          kclose(fhsounds);
          fhsounds = -1;
     }
     /*
     if (fhsongs >= 0) {
          kclose(fhsongs);
          fhsongs = -1;
     }
     */
}

void
musicoff(void)
{
     /*
     int  i;

     if( musicmode != MM_NOHARDWARE )  {
          for( i=0; i<SONGSPERLEVEL; i++ ) {
               if( songptr[i]->handle == NULL_HANDLE )
                    continue;
               sosMIDIStopSong(songptr[i]->handle);
               sosMIDIUnInitSong(songptr[i]->handle);
          }
    }
    */
}

int
playsound(int sn, int sndx, int sndy, int loop, short type)
{
     int       i,nr=0;
     int      dist=0L,pan=0L,dx,dy;

     if( (toggles[TOGGLE_SOUND] == 0) || !fxstarted || (sn < 0) || (sn >= TOTALSOUNDS) )
          return(-1);

     if( type&(ST_UNIQUE|ST_AMBUPDATE|ST_TOGGLE) ) {
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsound[i].handle == NULL_HANDLE ) {
                    continue;
               }
               else if( dsound[i].sndnum == sn ) {
                    if( (type&ST_TOGGLE) != 0 ) {
                         stopsound(i);
                    }
                    return(-1);
               }
          }
     }

     for( i=0; i<MAXSOUNDS; i++ ) {
          if( dsound[i].handle == NULL_HANDLE )
               break;
     }
     if( i == MAXSOUNDS ) {
          // add plevel and multiple occurrence replacement
          return(-1);
     }

     dsound[i].type=type;
     dsound[i].x=sndx; dsound[i].y=sndy;

     sbuf[sn].cache_lock=200;

     if( sbuf[sn].cache_ptr == 0L ) {   // no longer in cache
          allocache(&sbuf[sn].cache_ptr, sbuf[sn].cache_length, &sbuf[sn].cache_lock);
          if( sbuf[sn].cache_ptr == 0L ) {
               sbuf[sn].cache_lock=0x00;
               return(-1);
          }
          klseek(fhsounds, sbuf[sn].offset, SEEK_SET);
          nr=kread(fhsounds, sbuf[sn].cache_ptr, sbuf[sn].cache_length);
          if( nr != sbuf[sn].cache_length ) {
               sbuf[sn].cache_ptr=0L;
               sbuf[sn].cache_lock=0x00;
               return(-1);
          }
     }

     if( (type&ST_IMMEDIATE) ) {
          dist=0;
          pan=0;
     }
     else {
          /*
          dist=klabs(posx[screenpeek]-sndx)+klabs(posy[screenpeek]-sndy);
          if( (type&ST_AMBUPDATE) || (type&ST_VEHUPDATE) ) {
               if( dist < AMBUPDATEDIST ) {
                   vol = (AMBUPDATEDIST<<3)-(dist<<3);
               }
               else {
                   vol=0;
               }
          }
          else {
               if(dist < 1500L)
                    vol = 0x7fff;
               else if(dist > 8500L) {
                    if(sn >= S_MALE_COMEONYOU)
                         vol = 0x0000;
                    else
                         vol = 0x1f00;
               }
               else
                    vol = 39000L-(dist<<2);
          }
          pan=((getangle(posx[screenpeek]-dsoundptr[i]->x,posy[screenpeek]-dsoundptr[i]->y)+(2047-ang[screenpeek]))&2047) >> 6;
          if( (pan < 0) || (pan > 35) )
              pan=13;
          */
          dx = klabs(posx[screenpeek]-sndx);
          dy = klabs(posy[screenpeek]-sndy);
          dist = ksqrt(dx*dx+dy*dy) >> 6;

          pan = 2048 + ang[screenpeek] - getangle(posx[screenpeek]-dsound[i].x,posy[screenpeek]-dsound[i].y);
          pan = (pan & 2047) >> 6;
     }

     dsound[i].handle = FX_PlayRaw3D(sbuf[sn].cache_ptr, sbuf[sn].cache_length, 11025, 0, pan, dist, 1, i );
     if (dsound[i].handle < 0) {
          dsound[i].handle=NULL_HANDLE;
          dsound[i].plevel=0;
          dsound[i].sndnum=-1;
          if( sbuf[sn].users == 0 ) {
               sbuf[sn].cache_lock=0;
          }
          return(-1);
     }
     else {
          sbuf[sn].users++;
         #ifdef SNDDEBUG
          showmessage("SND %03d ADDR %08ld USRS %02d", sn, sbuf[sn].cache_ptr, sbuf[sn].users);
         #endif
          dsound[i].sndnum=sn;
     }

     return(i);
}

void
updatevehiclesnds(int i, int sndx, int sndy)
{
     int dx, dy, dist, pan;
     if( !fxstarted ) {
          return;
     }
     if( (i < 0) || (i > MAXSOUNDS) ) {
          return;
     }

     dsound[i].x=sndx;
     dsound[i].y=sndy;

     /*
     dist=labs(posx[screenpeek]-sndx)+labs(posy[screenpeek]-sndy);
     if( dist < 1000L ) {
          vol = 0x7fff;
     }
     else if( dist > 9000L ) {
          vol = 0x0000;
     }
     else {
          vol = 36000L-(dist<<2);
     }
     if( (vol < 0) || (vol > 0x7FFF) ) {
          vol=0x7fff;
     }
     pan=((getangle(posx[screenpeek]-dsoundptr[i]->x,posy[screenpeek]-dsoundptr[i]->y)+(2047-ang[screenpeek]))&2047) >> 6;
     if( (pan < 0) || (pan > 35) )
          pan=13;
     */

     dx = klabs(posx[screenpeek]-sndx);
     dy = klabs(posy[screenpeek]-sndy);
     dist = ksqrt(dx*dx+dy*dy) >> 6;

     pan = 2048 + ang[screenpeek] - getangle(posx[screenpeek]-sndx,posy[screenpeek]-sndy);
     pan = (pan & 2047) >> 6;

     if( dsound[i].handle != NULL_HANDLE )
          FX_Pan3D(dsound[i].handle,pan,dist);
}

void
stopsound(int i)
{
     if( !fxstarted )
          return;
     if( (i < 0) || (i >= MAXSOUNDS) ) {
          return;
     }
     if( dsound[i].handle == NULL_HANDLE )
          return;

     FX_StopSound(dsound[i].handle);
     sbuf[dsound[i].sndnum].users--;
     if( sbuf[dsound[i].sndnum].users < 0 )
              sbuf[dsound[i].sndnum].users=0;
     if( sbuf[dsound[i].sndnum].users == 0 ) {
         sbuf[dsound[i].sndnum].cache_lock=199;
     }
     dsound[i].handle=NULL_HANDLE;
     dsound[i].plevel=0;
     dsound[i].sndnum=-1;
}

void
songmastervolume(int vol)
{
     (void)vol;
     /*
     if( musicmode == MM_NOHARDWARE )
          return;

     if( (vol < 0) || (vol > 127) )
          vol=127;
     sosMIDISetMasterVolume(vol);
     */
}

void
soundmastervolume(int vol)
{
     // SOS used a range of 0-0x7fff, AudioLib uses 0-255.
     if (!fxstarted) return;
     FX_SetVolume(vol >> 7);
}

void
updatesounds(int snum)
{
     int      dist=0L,pan=0L,dx,dy;
     int       i;

     if( (toggles[TOGGLE_SOUND] == 0) || !fxstarted )
          return;

     for( i=0; i<MAXSOUNDS; i++ ) {
          if( dsound[i].handle == NULL_HANDLE ) {
               continue;
          }
          if( (dsound[i].type&(ST_IMMEDIATE|ST_NOUPDATE|ST_VEHUPDATE)) != 0 ) {
               continue;
          }
          /*
          dist=klabs(posx[snum]-dsoundptr[i]->x)+labs(posy[snum]-dsoundptr[i]->y);
          if(dsoundptr[i]->type==ST_AMBUPDATE) {
               if( dist < AMBUPDATEDIST ) {
                    vol = (AMBUPDATEDIST<<3)-(dist<<3);
               }
               else {
                    vol=0;
               }
          }
          else {
               if(dist < 1500L)
                    vol = 0x7fff;
               else if(dist > 8500L)
                    vol = 0x1f00;
               else
                    vol = 39000L-(dist<<2);
          }
          */

          dx = klabs(posx[snum]-dsound[i].x);
          dy = klabs(posy[snum]-dsound[i].y);
          dist = ksqrt(dx*dx+dy*dy) >> 6;

          pan = 2048 + ang[snum] - getangle(posx[snum]-dsound[i].x,posy[snum]-dsound[i].y);
          pan = (pan & 2047) >> 6;

          if( dsound[i].handle != NULL_HANDLE )
               FX_Pan3D(dsound[i].handle,pan,dist);
     }
}

void
stopallsounds(void)
{
     int       i;

     if( !fxstarted )
          return;

     for( i=0; i< MAXSOUNDS; i++ ) {
          if( dsound[i].handle == NULL_HANDLE )
               continue;
          FX_StopSound(dsound[i].handle);
          sbuf[dsound[i].sndnum].users--;
          if( sbuf[dsound[i].sndnum].users < 0 )
              sbuf[dsound[i].sndnum].users=0;
          if( sbuf[dsound[i].sndnum].users == 0 ) {
               sbuf[dsound[i].sndnum].cache_lock=199;
          }
          dsound[i].handle=NULL_HANDLE;
          dsound[i].plevel=0;
          dsound[i].sndnum=-1;
     }

//clear variables that track looping sounds
     loopinsound=-1;
     baydoorloop=-1;
     ambsubloop=-1;
}

void
musicfade()
{
     /*
     int i;

     if( musicmode == MM_NOHARDWARE ) {
          return;
     }
     for( i=0; i<SONGSPERLEVEL; i++ ) {
          if( (songptr[i]->handle != NULL_HANDLE) ) {
               if( ((musicv<<3) > 0) && (sosMIDISongDone(songptr[i]->handle) == _FALSE) ) {
                    sosMIDIFadeSong(songptr[i]->handle,_SOS_MIDI_FADE_OUT_STOP, 700,
                                    (musicv<<3),0,50);
                    while( (sosMIDISongDone(songptr[i]->handle)==_FALSE) ) {
                    }
               }
               removesong(i);
          }
     }
     */
}

void
menusong(int insubway)
{
     (void)insubway;
     /*
     int i,index;

     if( musicmode == MM_NOHARDWARE )
          return;

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          removesong(i);
     }

     if(insubway)
          index=(NUMLEVELS*(AVAILMODES*SONGSPERLEVEL)+3);
     else
          index=NUMLEVELS*(AVAILMODES*SONGSPERLEVEL);

     index+=2;

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          songptr[0]->handle=NULL_HANDLE;
          songptr[0]->offset=songlist[index*3]*4096;
          songptr[0]->playing=0;
          songptr[0]->pending=0;
          songptr[0]->length=( WORD)songlist[(index*3)+1];
          if( songptr[0]->length >= MAXBASESONGLENGTH ) {
               crashgame("prepsongs: basesong exceeded max length");
          }
     }
     songptr[0]->buffer=&basesongdata;

     playsong(BASESONG);
     */
}

void
startmusic(int level)
{
     (void)level;
     /*
     int       i,index;

     if( musicmode == MM_NOHARDWARE ) {
          return;
     }

     if( level > 6 ) {
          return;
     }

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          removesong(i);
     }

     index=totalsongsperlevel*(level);

     switch( musicmode ) {
     case MM_MIDIFM:
          break;
     case MM_MIDIAWE32:
           index+=SONGSPERLEVEL;
          break;
     case MM_MIDIGEN:
           index+=SONGSPERLEVEL*2;
          break;
     }

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          songptr[i]->handle=NULL_HANDLE;
          songptr[i]->offset=songlist[(index*3)+(i*3)]*4096;
          songptr[i]->playing=0;
          songptr[i]->pending=0;
          songptr[i]->length=( WORD)songlist[((index*3)+(i*3))+1];
          if( songptr[i]->length >= MAXBASESONGLENGTH ) {
               crashgame("prepsongs: basesong exceeded max length");
          }
     }
     songptr[0]->buffer=&basesongdata;
     songptr[1]->buffer=&secondsongdata;
     songptr[2]->buffer=&thirdsongdata;

     playsong(BASESONG);
     */
}
