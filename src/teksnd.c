/***************************************************************************
 *   TEKSND.C  - A HMI replacement by Jonathon Fowler using JFAudioLib     *
 *                                                                         *
 ***************************************************************************/

#include "build.h"
#include "cache1d.h"
#include "pragmas.h"
#include "fx_man.h"
#include "music.h"
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

#define   BASESONG            0
#define   MAXBASESONGLENGTH   44136
#define   AVAILMODES          3
#define   SONGSPERLEVEL       3
#define   NUMLEVELS       7

int       totalsongsperlevel;
int       songplaying=-1;
char      songdata[MAXBASESONGLENGTH];
int       songlengths[SONGSPERLEVEL];
int       songoffsets[SONGSPERLEVEL];
int       songlist[1024];

static int fxstarted = 0, fhsounds = -1;
static int musicstarted = 0, fhsongs = -1;

static int transmutehmp(char *filedata);

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
     int       i;

     fhsongs = kopen4load("songs", 0);
     if( fhsongs < 0 ) {
          crashgame("setupmidi: cant open songs");
     }
     memset(songlist, 0, sizeof(songlist));
     klseek(fhsongs, -sizeof(songlist), SEEK_END);
     i = kread(fhsongs, songlist, sizeof(songlist));
     if( i != sizeof(songlist) ) {
          crashgame("setupmidi: bad read of songlist");
     }

     songplaying = -1;
     totalsongsperlevel=SONGSPERLEVEL*AVAILMODES;
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
     } else {
          fxstarted = 1;

          FX_SetCallBack(soundcallback);
          setupdigi();
     }

     err = MUSIC_Init(ASS_AutoDetect, NULL);
     if (err != MUSIC_Ok) {
          buildprintf("MUSIC_Init error: %s\n", MUSIC_ErrorString(err));
     } else {
          musicstarted = 1;

          setupmidi();
     }
}

void
uninitsb(void)
{
     int i;

     if( musicstarted )  {
          MUSIC_StopSong();
          MUSIC_Shutdown();
          musicstarted = 0;
     }

     if( fxstarted ) {
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsound[i].handle == NULL_HANDLE )
                    continue;
               FX_StopSound(dsound[i].handle);
          }
          FX_Shutdown();
          fxstarted = 0;
     }

     if (fhsounds >= 0) {
          kclose(fhsounds);
          fhsounds = -1;
     }
     if (fhsongs >= 0) {
          kclose(fhsongs);
          fhsongs = -1;
     }
}

void
musicoff(void)
{
     int  i;

     if( musicstarted )  {
          MUSIC_StopSong();
    }
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
stopsong(int sn)
{
     if( songplaying != sn ) {
          return;
     }

     MUSIC_StopSong();
     songplaying=-1;
}

void
removesong(int sn)
{
     if(songplaying == sn ) {
          MUSIC_StopSong();
          songplaying=-1;
     }
}

int
playsong(int sn)
{
     int       rv;
     int       midilen;

     if( !musicstarted || (toggles[TOGGLE_MUSIC] == 0) ) {
          return(0);
     }
     if( (sn < 0) || (sn >= SONGSPERLEVEL) || (songplaying == sn) ) {
          return(0);
     }

     removesong(sn);
     if( songlengths[sn] == 0 )
         return(0);

     klseek(fhsongs, songoffsets[sn], SEEK_SET);
     rv=kread(fhsongs, songdata, songlengths[sn]);
     if( rv != songlengths[sn] ) {
         crashgame("playsong: bad read");
     }

     midilen = transmutehmp(songdata);
     if (midilen <= 0) {
          buildprintf("playsong: could not convert song to MIDI\n");
          return 0;
     }

     rv = MUSIC_PlaySong(songdata, midilen, 1);
     if( rv != MUSIC_Ok ) {
          buildprintf("playsong: could not play song: %s\n", MUSIC_ErrorString(MUSIC_ErrorCode));
          return 0;
     }

    #ifdef MUSICDEBUG
     showmessage("PLAYING SONG %2d", sn);
    #endif
     songplaying = sn;

     return(1);
}

void
musicfade()
{
     MUSIC_StopSong();
     songplaying=-1;
}

void
menusong(int insubway)
{
     int i,index;

     if( !musicstarted )
          return;

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          removesong(i);
     }

     if(insubway)
          index=(NUMLEVELS*(AVAILMODES*SONGSPERLEVEL)+3);
     else
          index=NUMLEVELS*(AVAILMODES*SONGSPERLEVEL);

     index+=2;

     songoffsets[BASESONG] = songlist[index*3]*4096;
     songlengths[BASESONG] = songlist[(index*3)+1];
     if( songlengths[BASESONG] >= MAXBASESONGLENGTH ) {
          crashgame("prepsongs: basesong exceeded max length");
     }

     playsong(BASESONG);
}

void
startmusic(int level)
{
     int       i,index;

     if( !musicstarted ) {
          return;
     }

     if( level > 6 ) {
          return;
     }

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          removesong(i);
     }

     index=totalsongsperlevel*(level);

     index+=SONGSPERLEVEL;    // Skip FM.
     index+=SONGSPERLEVEL;    // Skip AWE32.

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          songoffsets[i]=songlist[(index*3)+(i*3)]*4096;
          songlengths[i]=songlist[((index*3)+(i*3))+1];
          if( songlengths[i] >= MAXBASESONGLENGTH ) {
               crashgame("prepsongs: basesong exceeded max length");
          }
     }

     playsong(BASESONG);
}


/*
Brutal in-place transformation of a SOS HMP file (a.k.a. NDMF according to sosm.h)
into Standard MIDI, mutating global looping controllers into Apogee EMIDI equivalents.

By Jonathon Fowler, 2023
Provided to the public domain given how dubiously licensed the sources of information
going into this were. Realised through a combination of:
  - SOS.H in the Witchaven code dump exposing the file structure
  - http://www.r-t-c-m.com/knowledge-base/downloads-rtcm/tekwar-tools/sos40.zip
    providing the SOS special MIDI controller descriptions
  - A crucial hint about variable length encoding byte order at
    https://github.com/Mindwerks/wildmidi/blob/master/docs/formats/HmpFileFormat.txt#L84-L96

Overall:
    struct ndmfheader header;
    struct ndmftracks tracks[header.numtracks];
    uint8_t branchtable[];

    NDMF is little-endian, MIDI is big-endian.
    NDMF variable-length encoding: 0aaaaaaa 0bbbbbbb 1ccccccc
    MIDI variable-length encoding: 1ccccccc 1bbbbbbb 0aaaaaaa

Transformation can happen in-place because NDMF has a massive header compared to MIDI,
so every write will be happening onto ground already trodden. Strict aliasing be damned.
*/

#if defined(__GNUC__) || defined(__clang__)
#define PACKED_STRUCT struct __attribute__ ((packed))
#elif defined(_MSC_VER)
#define PACKED_STRUCT struct
#pragma pack(1)
#else
#define PACKED_STRUCT struct
#endif

PACKED_STRUCT ndmfheader {
     uint8_t ident[32];       // "HMIMIDIP013195" \0...
     uint32_t branchofs;      // File offset to the branch table at the end
     uint32_t pad[3];
     uint32_t numtracks;
     uint32_t ticksperqunote;
     uint32_t tempo;          // Game clock dependent
     uint32_t playtime;       // Song length in seconds
     uint32_t channelprio[16];
     uint32_t trackmap[32][5];
     uint8_t  ctrlrestore[128];
     uint32_t pad2[2];
};

PACKED_STRUCT ndmftrackheader {
     uint32_t tracknum;
     uint32_t tracklen;       // Header length inclusive
     uint32_t channel;
     uint8_t data[];          // [tracklen-12]
};

PACKED_STRUCT smfheader {
     uint8_t ident[4];        // "MThd"
     uint32_t headsize;       // 6
     uint16_t format;
     uint16_t numtracks;
     uint16_t ticksperqunote;
};
PACKED_STRUCT smftrackheader {
     uint8_t ident[4];        // "MTrk"
     uint32_t tracklen;       // Header length exclusive
     uint8_t data[];          // [tracklen]
};

static int transmutehmp(char *filedata)
{
     const char ndmfident[] = "HMIMIDIP013195";
     const int commandlengths[8] = { 2, 2, 2, 2, 1, 1, 2, -1 };
     const int syscomlengths[16] =  { -1, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 };

     if (memcmp(ndmfident, filedata, sizeof ndmfident)) return -1;

     // Extract the important values from the NDMF header.
     struct ndmfheader *ndmfhead = (struct ndmfheader *)filedata;
     int numtracks = B_LITTLE32(ndmfhead->numtracks);
     int ticksperqunote = B_LITTLE32(ndmfhead->ticksperqunote);
     int tempo = 120000000 / B_LITTLE32(ndmfhead->tempo);
     ndmfhead = NULL;

     // Construct a new MIDI header.
     struct smfheader *smfhead = (struct smfheader *)filedata;
     memcpy(smfhead->ident, "MThd", 4);
     smfhead->headsize = B_BIG32(6);
     smfhead->format = B_BIG16(1);
     smfhead->numtracks = B_BIG16(numtracks);
     smfhead->ticksperqunote = B_BIG16(ticksperqunote);

     // Transcribe tracks.
     int ndmffileofs = sizeof(struct ndmfheader);
     int smffileofs = sizeof(struct smfheader);
     for (int trk = 0; trk < numtracks; trk++) {
          struct ndmftrackheader *ndmftrack = (struct ndmftrackheader *)&filedata[ndmffileofs];
          struct smftrackheader *smftrack = (struct smftrackheader *)&filedata[smffileofs];

          int ndmfdatalen = B_LITTLE32(ndmftrack->tracklen) - 12;
          int smfdatalen = ndmfdatalen;
          if (trk == 0) {
               // We need to add a tempo event to the first MIDI track.
               smfdatalen += 7;
          }

          memcpy(smftrack->ident, "MTrk", 4);
          smftrack->tracklen = B_BIG32(smfdatalen);

          uint8_t *ndmfdata = (uint8_t *)&ndmftrack->data[0];
          uint8_t *smfdata = (uint8_t *)&smftrack->data[0];

          if (trk == 0) {
               // Insert a tempo event.
               *(smfdata++) = 0;
               *(smfdata++) = 0xff;
               *(smfdata++) = 0x51;
               *(smfdata++) = 3;
               *(smfdata++) = (tempo>>16)&0xff;
               *(smfdata++) = (tempo>>8)&0xff;
               *(smfdata++) = tempo&0xff;
          }

          // Process events.
          uint8_t status = 0;
          for (int i = 0; i < ndmfdatalen; ) {
               uint8_t b;
               int copylen = 0;

               // Re-encode the offset.
               uint8_t vlenbytes[4], vlencnt = 0;
               do {
                    b = ndmfdata[i++];
                    vlenbytes[vlencnt++] = b & 0x7f;
               } while (!(b & 0x80));
               do {
                    b = vlenbytes[--vlencnt];
                    if (vlencnt) b |= 0x80;
                    *(smfdata++) = b;
               } while (vlencnt > 0);

               b = ndmfdata[i];
               if (b&0x80) {
                    // A new status byte.
                    *(smfdata++) = b;
                    i++;

                    status = b;    // Keep for running status.
                    copylen = commandlengths[(status & 0x7f)>>4];

                    if ((b&0xf0) == 0xf0) {
                         switch (b&0x0f) {
                              case 0x0: // Sysex.
                                   do *(smfdata++) = (b = ndmfdata[i++]);
                                   while (!(b&0x80) && b != 0xf7);
                                   break;
                              case 0xf: // Meta.
                                   *(smfdata++) = ndmfdata[i++];  // Type.
                                   copylen = (*(smfdata++) = ndmfdata[i++]);     // Length.
                                   break;
                              default:  // Sys common.
                                   copylen = syscomlengths[b&0x0f];
                                   break;
                         }
                    } else if ((b&0xf0) == 0xb0) {     // Controller change.
                         // SOS/EMIDI custom controller range. For whatever reason SOS
                         // controller values have their high bit set.
                         if (ndmfdata[i] >= 102 && ndmfdata[i] <= 119) {
                              if (trk == 1 && ndmfdata[i] == 110) {        // Global loop start
                                   *(smfdata++) = 118;
                                   *(smfdata++) = (ndmfdata[i+1] & 0x7f);
                              } else if (trk == 1 && ndmfdata[i] == 111) { // Global loop end.
                                   *(smfdata++) = 119;
                                   *(smfdata++) = 127;
                              } else {
                                   *(smfdata++) = 102; // Neuter all other controllers.
                                   *(smfdata++) = 0;
                              }
                              i += 2;
                              copylen = 0;
                         }
                    }
               } else {
                    copylen = commandlengths[(status & 0x7f)>>4];
               }

               for (; copylen>0; copylen--) {     // Copy data bytes.
                    *(smfdata++) = ndmfdata[i++];
               }
          }

          ndmffileofs += ndmfdatalen + 12;
          smffileofs += smfdatalen + 8;
     }

     return smffileofs;
}

#undef PACKED_STRUCT
#ifdef _MSC_VER
#pragma pack()
#endif
