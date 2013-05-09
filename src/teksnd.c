
/***************************************************************************
 *   TEKSND.C  - HMI library replaces Kens sound stuff                     *
 *               Also timer routine and keytimerstuff is here              *
 *                                                                         *
 ***************************************************************************/

#include <sys\types.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <dos.h>
#include "io.h"
#include "fcntl.h"
#include "string.h"
#include "malloc.h"
#include "build.h"
#include "names.h"

#include "tekwar.h"

//#define   SNDDEBUG
//#define     MUSICDEBUG


#pragma pack(4)               // HMI structures must be packed !!

#define  AMBUPDATEDIST  4000L

#define  _NULL          0
#define  VOID           void

typedef  int            BOOL;
typedef  unsigned int   UINT;
typedef  unsigned char  BYTE;
typedef  unsigned       WORD;
typedef  signed long    LONG;
typedef  unsigned long  DWORD;

typedef  BYTE  *        PBYTE;
typedef  char near *    PSTR;
typedef  WORD  *        PWORD;
typedef  LONG  *        PLONG;
typedef  VOID  *        PVOID;

typedef  BYTE  far   *  LPBYTE;
typedef  BYTE  far   *  LPSTR;
typedef  WORD  far   *  LPWORD;
typedef  LONG  far   *  LPLONG;
typedef  VOID  far   *  LPVOID;

typedef  BYTE  huge  *  HPBYTE;
typedef  BYTE  huge  *  HPSTR;
typedef  WORD  huge  *  HPWORD;
typedef  LONG  huge  *  HPLONG;
typedef  VOID  huge  *  HPVOID;

#define  _TIMER_DOS_RATE   0xff00

#define  _SOS_DEBUG_NORMAL       0x0000
#define  _SOS_DEBUG_NO_TIMER     0x0001
#define  _SOS_TIMER_DPMI         0x0002

#define  _ACTIVE           0x8000
#define  _LOOPING          0x4000
#define  _FIRST_TIME       0x2000
#define  _PENDING_RELEASE  0x1000
#define  _CONTINUE_BLOCK   0x0800
#define  _PITCH_SHIFT      0x0400
#define  _PANNING          0x0200
#define  _VOLUME           0x0100
#define  _TRANSLATE16TO8   0x0080
#define  _STAGE_LOOP       0x0040
#define  _TRANSLATE8TO16   0x0020
#define  _STEREOTOMONO     0x0010

#define  _HMI_INI_MANAGER

#define   SM_NOHARDWARE       0
#define   SM_STANDARD         1

#define   MM_NOHARDWARE       0
#define   MM_MIDIFM           1
#define   MM_MIDIDIGI         2
#define   MM_MIDIGEN          3
#define   MM_MIDIAWE32        4

char      soundmode=SM_NOHARDWARE;
char      musicmode=MM_NOHARDWARE;

//JSA_NEW
#define	 MAX_LOOP_LENGTH		60000L
#define   MAX_SND_LOOPS        14

typedef  struct   _tagINIInstance {
            WORD  wFlags;           
            BYTE  szName[ 128 ];    
            PSTR  pData;            
            WORD  wSize;            
            WORD  wMaxSize;         
            PSTR  pCurrent;         
            WORD  wCurrent;         
            PSTR  pSection;         
            PSTR  pItemPtr;         
            PSTR  pItem;            
            PSTR  pList;            
            PSTR  pListPtr;         
} _INI_INSTANCE;

#define  _INI_SECTION_START   '['
#define  _INI_SECTION_END     ']'
#define  _INI_EQUATE          '='
#define  _INI_SPACE           ' '
#define  _INI_TAB             0x9
#define  _INI_STRING_START    '"'
#define  _INI_STRING_END      '"'
#define  _INI_EOL             0x0d
#define  _INI_CR              0x0d
#define  _INI_LF              0x0a
#define  _INI_HEX_INDICATOR   'x'
#define  _INI_LIST_SEPERATOR  ','
#define  _INI_EXTRA_MEMORY    1024
#define  _INI_MODIFIED        0x8000

#define  _ERR_NO_SLOTS ( WORD )-1
enum {
   _ERR_NO_ERROR,
   _ERR_DRIVER_NOT_LOADED,
   _ERR_INVALID_POINTER,
   _ERR_DETECT_INITIALIZED,
   _ERR_FAIL_ON_FILE_OPEN,
   _ERR_MEMORY_FAIL,
   _ERR_INVALID_DRIVER_ID,
   _ERR_NO_DRIVER_FOUND,
   _ERR_DETECTION_FAILURE,
   _ERR_DRIVER_LOADED,
   _ERR_INVALID_HANDLE,
   _ERR_NO_HANDLES,
   _ERR_PAUSED,   
   _ERR_NOT_PAUSED,
   _ERR_INVALID_DATA,
   _ERR_DRV_FILE_FAIL,
   _ERR_INVALID_PORT,
   _ERR_INVALID_IRQ,
   _ERR_INVALID_DMA,
   _ERR_INVALID_DMA_IRQ
};   
enum {
         _FALSE, 
         _TRUE 
};

typedef struct {
   BYTE  szDeviceName[ 32 ];  
   WORD  wDeviceVersion;      
   WORD  wBitsPerSample;      
   WORD  wChannels;           
   WORD  wMinRate;            
   WORD  wMaxRate;            
   WORD  wMixerOnBoard;       
   WORD  wMixerFlags;         
   WORD  wFlags;              
   short far * lpPortList;    
   short far * lpDMAList;     
   short far * lpIRQList;     
   short far * lpRateList;    
   WORD  fBackground;         
   WORD  wDeviceID;           
   WORD  wTimerID;            
} _SOS_CAPABILITIES; 
_SOS_CAPABILITIES digicap;
_SOS_CAPABILITIES _far *digicapptr=( _SOS_CAPABILITIES _far *)&digicap;

typedef struct {
   WORD  wPort;
   WORD  wIRQ;
   WORD  wDMA; 
   WORD  wParam;
} _SOS_HARDWARE;
_SOS_HARDWARE  digihardware;
_SOS_HARDWARE  _far *digihardwareptr=( _SOS_HARDWARE _far *)&digihardware;

#define  _SOS_DMA_BUFFERSIZE     0x2000
#define  _SOS_SAMPLE_RATE        11025
#define  _SOS_FILL_TIMER_RATE    40
#define  _SOS_NORMAL_TIMER       0x00
#define  _CALLBACK_TIMER_RATE    40

typedef struct {
   WORD  wBufferSize;
   LPSTR lpBuffer;
   BOOL  wAllocateBuffer;
   WORD  wSampleRate;
   WORD  wParam;
   LONG  dwParam;
   VOID ( far *lpFillHandler )( VOID );
   LPSTR lpDriverMemory;
   LPSTR lpDriverMemoryCS;
   LPSTR lpTimerMemory;
   LPSTR lpTimerMemoryCS;
   WORD  wTimerID;
   WORD  wPhysical;
} _SOS_INIT_DRIVER;
_SOS_INIT_DRIVER    digiinitdriver = {
     _SOS_DMA_BUFFERSIZE,
	_NULL,
	_TRUE,
	_SOS_SAMPLE_RATE,
	19,
	0L,
	_NULL,
	_NULL,
	_NULL,
	_SOS_NORMAL_TIMER
};
_SOS_INIT_DRIVER  _far *digiinitdriverptr=( _SOS_INIT_DRIVER _far *)&digiinitdriver;

enum {
   _SAMPLE_PROCESSED,
   _SAMPLE_LOOPING,
   _SAMPLE_DONE
};

enum
{
   _LEFT_CHANNEL,
   _RIGHT_CHANNEL,
   _CENTER_CHANNEL,
   _INTERLEAVED
};

typedef struct {
   LPSTR lpSamplePtr;
   WORD  dwSampleSize;
   WORD  wLoopCount;
   WORD  wChannel;
   WORD  wVolume;
   WORD  wSampleID;
   VOID  ( far cdecl *lpCallback )( WORD, WORD, WORD );
   WORD  wSamplePort; 
   WORD  wSampleFlags;
   WORD  dwSampleByteLength;
   WORD  dwSampleLoopPoint;
   WORD  dwSampleLoopLength;
   WORD  dwSamplePitchAdd;
   WORD  wSamplePitchFraction;
   WORD  wSamplePanLocation;
   WORD  wSamplePanSpeed;
   WORD  wSamplePanDirection;
   WORD  wSamplePanStart;
   WORD  wSamplePanEnd;
   WORD  wSampleDelayBytes;
   WORD  wSampleDelayRepeat;
   WORD  dwSampleADPCMPredicted;
   WORD  wSampleADPCMIndex;
   WORD  wSampleRootNoteMIDI;   
   WORD  dwSampleTemp1;   
   WORD  dwSampleTemp2;   
   WORD  dwSampleTemp3;   
} _SOS_START_SAMPLE;
_SOS_START_SAMPLE   sample[MAXSOUNDS];
_SOS_START_SAMPLE   _far *sampleptr[MAXSOUNDS];

#define   NULL_HANDLE    -1
#define   MAXLOOPS        2

//defines for looping sound variables
extern    int       loopinsound;
extern    int       baydoorloop;
extern    int       ambsubloop;


// a 1-1 map from sounds in sound file to this array
struct    soundbuffertype {
     int       users;
     long      offset;
     long      cache_ptr;
     long      cache_length;
     char      cache_lock;
};
struct    soundbuffertype    sbuf[TOTALSOUNDS];
struct    soundbuffertype    *sbufptr[TOTALSOUNDS];
struct    soundbuffertype    loopbuf[MAXLOOPS];
struct    soundbuffertype    *loopbufptr[MAXLOOPS];

struct    soundtype {
     int       handle;
     int       sndnum;
     int       plevel;
     long      x,y;
     short     type;
};
struct    soundtype     dsound[MAXSOUNDS];
struct    soundtype     *dsoundptr[MAXSOUNDS];
struct    soundtype     lsound[MAXLOOPS];
struct    soundtype     *lsoundptr[MAXLOOPS];
DWORD    *LoopList;

VOID      _far cdecl digiloopcallback(  WORD, WORD, WORD  );

_SOS_START_SAMPLE loopsampledata[MAXLOOPS] =
	{  
	   {_NULL, 0L, -1, _CENTER_CHANNEL, 0x3fff,
		 0x1000, digiloopcallback, 0, _LOOPING | _VOLUME, 
		 0L, 0L, 0L, 0L, 0, 0, 0, 0x8000, 0, 0, 0, 0, 
		 0L, 0, 0, 0L, 0L, 0L},

      {_NULL, 0L, -1, _CENTER_CHANNEL, 0x3fff,
		 0x2000, digiloopcallback, 0, _LOOPING | _VOLUME, 
		 0L, 0L, 0L, 0L, 0, 0, 0, 0x8000, 0, 0, 0, 0, 
		 0L, 0, 0, 0L, 0L, 0L},
	};
_SOS_START_SAMPLE   _far *loopsampleptr[MAXLOOPS];

WORD     LoopIndex=0,looptoggle=0,loopmusepauseflag=0;
WORD     hLoopFile	=  -1;       // Handle for Loop file


volatile	int LoopPending=0;

WORD      digiport;
WORD _far *digiportptr=( WORD _far *)&digiport;
WORD      fhdigifill;
WORD _far *fhdigifillptr=( WORD _far *)&fhdigifill;
WORD      fhdigidriver;
WORD _far *fhdigidriverptr=( WORD _far *)&fhdigidriver;

int       fhsounds;
int       fhsongs;



#define   BASESONG            0
#define   MAXBASESONGLENGTH   44136
#define   AVAILMODES          3
#define   SONGSPERLEVEL       3 
#define	NUMLEVELS		 7

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

#define  _SOS_MIDI_MAX_DRIVERS  5
#define  _SOS_MIDI_MAX_CHANNELS    0x10

#define  _SOS_MIDI_FADE_IN          0x01  
#define  _SOS_MIDI_FADE_OUT         0x02  
#define  _SOS_MIDI_FADE_OUT_STOP    0x04  


#define  _MIDI_SOUND_MASTER_II      0xa000
#define  _MIDI_MPU_401              0xa001
#define  _MIDI_FM                   0xa002
#define  _MIDI_OPL2                 0xa002
#define  _MIDI_CALLBACK             0xa003
#define  _MIDI_MT_32                0xa004
#define  _MIDI_DIGI                 0xa005  
#define  _MIDI_INTERNAL_SPEAKER     0xa006
#define  _MIDI_WAVE_TABLE_SYNTH     0xa007  
#define  _MIDI_AWE32                0xa008  
#define  _MIDI_OPL3                 0xa009  
#define  _MIDI_GUS                  0xa00a  

typedef struct {
   BYTE     szDeviceName[ 32 ];  
   WORD     wDeviceVersion;      
   WORD     wFlags;              
   WORD     wProcessData;        
   short far *   lpPortList;     
   short far *   lpIRQList;      
   WORD     wDeviceID;           
} _SOS_MIDI_CAPABILITIES; 
_SOS_MIDI_CAPABILITIES midicap;
_SOS_MIDI_CAPABILITIES _far *midicapptr=( _SOS_MIDI_CAPABILITIES _far *)&midicap;

typedef struct {
   WORD  wDriverID;
   WORD  wTimerRate;
   WORD  wTimerCallbackRate;
   WORD  wMaxVoices;
   WORD  wVelocitySensing;
   _SOS_INIT_DRIVER far * sDIGIDriverInfo;
   _SOS_HARDWARE far *  sDIGIHardwareInfo;    
} _SOS_MIDI_DIGI_INIT_DRIVER;
_SOS_MIDI_DIGI_INIT_DRIVER    mididigiinitdriver;
_SOS_MIDI_DIGI_INIT_DRIVER    _far *mididigiinitdriverptr=(_SOS_MIDI_DIGI_INIT_DRIVER _far *)&mididigiinitdriver;

typedef struct {
   WORD        wDIGIDriverID;
   VOID far * lpDriverMemory;
   VOID far * lpDriverMemoryCS;
   _SOS_MIDI_DIGI_INIT_DRIVER far * sDIGIInitInfo;
   WORD  wParam;
   DWORD dwParam;
} _SOS_MIDI_INIT_DRIVER;
_SOS_MIDI_INIT_DRIVER    midiinitdriver;
_SOS_MIDI_INIT_DRIVER    _far *midiinitdriverptr=( _SOS_MIDI_INIT_DRIVER _far *)&midiinitdriver;

typedef struct {
   BYTE _huge * lpSongData;
   VOID ( far * lpSongCallback )( WORD );
} _SOS_MIDI_INIT_SONG;
_SOS_MIDI_INIT_SONG songdatastatic;
_SOS_MIDI_INIT_SONG _far *songdataptr=(_SOS_MIDI_INIT_SONG _far *)&songdatastatic;

enum
{ 
   _DRV_MIDI_GET_CAPS,
   _DRV_MIDI_GET_CALL_TABLE,
   _DRV_MIDI_SPECIAL1
};

typedef struct {
   BYTE  szName[ 32 ];
   WORD  wDrivers;
   WORD  lOffset;
   WORD  lFileSize;
} _MIDIFILEHEADER;

typedef struct {
   BYTE  szName[ 32 ];
   WORD  lNextDriver;
   WORD  wSize;
   WORD  wDeviceID;
   WORD  wExtenderType;
} _MIDIDRIVERHEADER;

typedef struct {
   WORD  wPort;
   WORD  wIRQ;
   WORD  wParam;
} _SOS_MIDI_HARDWARE;
_SOS_MIDI_HARDWARE  midihardware;
_SOS_MIDI_HARDWARE  _far *midihardwareptr=( _SOS_MIDI_HARDWARE _far *)&midihardware;

WORD      fhmididriver;
WORD _far *fhmididriverptr=( WORD _far *)&fhmididriver;
WORD      fhmididigidriver;
WORD _far *fhmididigidriverptr=( WORD _far *)&fhmididigidriver;

int       songlist[4096];

#define   MELODICBANKLENGTH   0x152C
#define   DRUMBANKLENGTH      0x152C
LPSTR     melodicbankptr;
LPSTR     drumbankptr;
LPSTR     digitalbankptr;

#define  _MIDI_MAP_TRACK   0xff

typedef struct {
   WORD wTrackDevice[ 32 ];
} _SOS_MIDI_TRACK_DEVICE;

_SOS_MIDI_TRACK_DEVICE   songtrackmap = { 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, 
   _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK, _MIDI_MAP_TRACK 
};
_SOS_MIDI_TRACK_DEVICE   _far *trackmapptr=(_SOS_MIDI_TRACK_DEVICE _far *)&songtrackmap;

UINT  PanArray[] = {
		//REAR to HARD LEFT (angle = 0->512)   
	0x8000,0x7000,0x6000,0x5000,0x4000,0x3000,0x2000,0x1000,0x0000,
		//HARD LEFT to CENTER (angle = 513-1024)
	0x1000,0x20f0,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000, 
		//CENTER to HARD RIGHT (angle = 1025-1536)
	0x70f0,0x8000,0x9000,0xa000,0xb000,0xc000,0xd000,0xe000,0xf000,   
		//HARD RIGHT to REAR (angle = 1537-2047)
	0xffff,0xf000,0xe000,0xd000,0xc000,0xb000,0xa000,0x9000,0x8000
};

extern    void smkinit(WORD fhptr);
extern    void smkuninit(WORD fhptr);
extern    int  cyberenabled;
extern    int  musicv,soundv;

VOID      _far cdecl soundcallback(  WORD, WORD, WORD  );
void      _far      timerhandler(void);
unsigned  _far      timerhandle1;
volatile  int  quarterseconds=0;
volatile  int  seconds=0;
volatile  int  minutes=0;
volatile  int  hours=0;
volatile  int  messageon=0;
int	     digiloopflag=0;
long	     SeekIndex,SampleSize;

inittimer()
{
     sosTIMERInitSystem(_TIMER_DOS_RATE, _SOS_DEBUG_NORMAL);   
	if( _ERR_NO_ERROR != sosTIMERRegisterEvent(CLKIPS, timerhandler, &timerhandle1) ) {
          crash("inittimer: no handles");
     }
}

uninittimer()
{
     if (cyberenabled) {
          ctm_deinit();
     }

	sosTIMERRemoveEvent(timerhandle1);
     sosTIMERUnInitSystem(0);
}

void _far 
timerhandler()
{
	totalclock++;
	outp(0x20,0x20);
}

initaudioptrs()
{
     int         i;

     for( i=0; i<TOTALSOUNDS; i++ ) {
          sbufptr[i]=&sbuf[i];
          sbufptr[i]->users=0;
          sbufptr[i]->offset=0;
          sbufptr[i]->cache_ptr=0L;
          sbufptr[i]->cache_length=0;
          sbufptr[i]->cache_lock=0x00;
     }
     for( i=0; i<MAXSOUNDS; i++ ) {
          dsoundptr[i]=&dsound[i];
          dsoundptr[i]->handle=NULL_HANDLE;
          dsoundptr[i]->sndnum=0;
          dsoundptr[i]->plevel=0;
          dsoundptr[i]->x=0L;
          dsoundptr[i]->y=0L;
          dsoundptr[i]->type=ST_UPDATE;
          dsoundptr[i]->sndnum=-1;
          sampleptr[i]=(_SOS_START_SAMPLE   _far *)&sample[i];
          sampleptr[i]->wSamplePitchFraction=0;
          sampleptr[i]->wSamplePanDirection=0;
          sampleptr[i]->wSamplePanStart=0;
          sampleptr[i]->wSamplePanEnd=0;
          sampleptr[i]->wSampleDelayBytes=0;
          sampleptr[i]->wSampleDelayRepeat=0;
          sampleptr[i]->dwSampleADPCMPredicted=0;
          sampleptr[i]->wSampleADPCMIndex=0;
          sampleptr[i]->wSampleRootNoteMIDI=0;
          sampleptr[i]->dwSampleTemp1=0;
          sampleptr[i]->dwSampleTemp2=0;
          sampleptr[i]->dwSampleTemp3=0;
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


     for( i=0; i<MAXLOOPS; i++ ) {
          loopbufptr[i]=&loopbuf[i];
          loopbufptr[i]->users=0;
          loopbufptr[i]->offset=0;
          loopbufptr[i]->cache_ptr=0L;
          loopbufptr[i]->cache_length=0;
          loopbufptr[i]->cache_lock=0x00;
          lsoundptr[i]=&lsound[i];
          lsoundptr[i]->handle=NULL_HANDLE;
          lsoundptr[i]->sndnum=0;
          lsoundptr[i]->plevel=0;
          lsoundptr[i]->x=0L;
          lsoundptr[i]->y=0L;
          lsoundptr[i]->type=ST_UPDATE;
          lsoundptr[i]->sndnum=-1;
          loopsampleptr[i]=(_SOS_START_SAMPLE   _far *)&loopsampledata[i];
     }
}

setupdigi()
{
     int       i;
     DWORD     *digilist;

     if( soundmode == SM_NOHARDWARE ) {
          return;
     }

     digilist=( DWORD *)malloc(( size_t)4096);
     if( digilist == ( DWORD *)NULL ) {
          crash("setupdigi: digilist malloc failed");
     }

	fhsounds=open("sounds",O_RDONLY|O_BINARY);
	if( fhsounds == -1 ) {
	     crash("setupdigi: cant open sounds");
	}
     memset(digilist,0, 4096);
	lseek(fhsounds,-4096L,SEEK_END);
	i=read(fhsounds,( void *)digilist, 4096);
     if( i != 4096 ) {
          crash("setupdigi: bad read of digilist");
     }

     for( i=0; i<TOTALSOUNDS; i++ ) {
          sbufptr[i]=&sbuf[i];
          sbufptr[i]->users=0;
          sbufptr[i]->offset=(digilist[i*3]*4096L);
          sbufptr[i]->cache_ptr=0L;
          sbufptr[i]->cache_length=( WORD)(digilist[i*3+1]);
          sbufptr[i]->cache_lock=0x00;
     }
     for( i=0; i<MAXSOUNDS; i++ ) {
          dsoundptr[i]=&dsound[i];
          dsoundptr[i]->handle=NULL_HANDLE;
          dsoundptr[i]->sndnum=0;
          dsoundptr[i]->plevel=0;
          dsoundptr[i]->x=0L;
          dsoundptr[i]->y=0L;
          dsoundptr[i]->type=ST_UPDATE;
          dsoundptr[i]->sndnum=-1;
          sampleptr[i]=(_SOS_START_SAMPLE   _far *)&sample[i];
          sampleptr[i]->wSamplePitchFraction=0;
          sampleptr[i]->wSamplePanDirection=0;
          sampleptr[i]->wSamplePanStart=0;
          sampleptr[i]->wSamplePanEnd=0;
          sampleptr[i]->wSampleDelayBytes=0;
          sampleptr[i]->wSampleDelayRepeat=0;
          sampleptr[i]->dwSampleADPCMPredicted=0;
          sampleptr[i]->wSampleADPCMIndex=0;
          sampleptr[i]->wSampleRootNoteMIDI=0;
          sampleptr[i]->dwSampleTemp1=0;
          sampleptr[i]->dwSampleTemp2=0;
          sampleptr[i]->dwSampleTemp3=0;
     }

     free(digilist);
}

setupmidi()
{
     int       fh,dl,rv,i;

     if( musicmode == MM_NOHARDWARE ) {
          return;
     }

     melodicbankptr=( LPSTR)0;
     drumbankptr=( LPSTR)0;
	digitalbankptr=( LPSTR)0;

     if( (musicmode != MM_MIDIFM) && (musicmode != MM_MIDIDIGI) )
          goto nobanks;

     melodicbankptr=( LPSTR)malloc(( size_t)MELODICBANKLENGTH);
     drumbankptr=( LPSTR)malloc(( size_t)DRUMBANKLENGTH);
     if( (melodicbankptr == ( LPSTR)NULL) || (drumbankptr == ( LPSTR)NULL) ) {
          crash("setupmidi: failed malloc");
     }
	if( (fh=open("melodic.bnk",O_RDONLY)) == -1 ) {
          crash("setupmidi: cant open melodic.bnk");
     }
	read(fh, ( void * )melodicbankptr, MELODICBANKLENGTH);
	close(fh);
	rv=sosMIDISetInsData(*fhmididriverptr, melodicbankptr, 1);
     if( rv != _ERR_NO_ERROR ) {
          crash("setupmidi: bad SetInsData");
     }
	if( (fh=open("drum.bnk",O_RDONLY)) == -1 ) {
          crash("setupmidi: cant open drum.bnk");
     }
	read(fh, ( void * )drumbankptr, DRUMBANKLENGTH);
	close(fh);
	rv=sosMIDISetInsData(*fhmididriverptr, drumbankptr, 1);
     if( rv != _ERR_NO_ERROR ) {
          crash("setupmidi: bad SetInsData");
     }

	if( (musicmode == MM_MIDIDIGI) && (midihardwareptr->wPort == 0x388) ) {
		if( (fh=open("test.dig",O_BINARY|O_RDWR)) == -1 ) {
               crash("setupmidi: cant open test.dig");
          }
   	     dl=lseek(fh, 0L, SEEK_END);
   	     lseek(fh, 0L, SEEK_SET);
		digitalbankptr=( LPSTR)malloc(( size_t)dl);
          if( digitalbankptr == ( LPSTR)NULL ) {
               crash("setupmidi: failed malloc digbnkptr");
          }
		rv=read(fh, ( void * )digitalbankptr, dl);
          if( rv != dl ) {
               crash("setupmidi: bad .dig read");
          }
		close(fh);
		rv=sosMIDISetInsData(*fhmididigidriverptr, digitalbankptr, 1);
          if( rv != _ERR_NO_ERROR ) {
               crash("setupmidi: bad SetInsData");
          }
	}	

nobanks:

     if( musicmode != MM_NOHARDWARE ) {
		if( (fhsongs=open("SONGS",O_RDONLY | O_BINARY)) == -1 ) {
               crash("setupmidi: cant open songs");
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


}

#define   lm(_str_) printf(" %s...\n", _str_);

initsb(char option1,char option2,long digihz,char option7a,char option7b,int val,char option7c)
{
     int  fh;
     WORD rv;
     int  vol;

     initaudioptrs();

     if( readhmicfg() == _FALSE ) {
          musicmode=MM_NOHARDWARE;
          soundmode=SM_NOHARDWARE;
          return;
     }
 
     switch( digicapptr->wDeviceID ) {
     case 0x00000000:
     case 0xFFFFFFFF:
          soundmode=SM_NOHARDWARE;
          break;
     default:
          soundmode=SM_STANDARD;
          break;
     }

     switch( soundmode ) {
     case SM_NOHARDWARE:
          lm("no sound fx");
          break;
     default:
          lm("standard sound fx");
          break;
     }

     switch( midicapptr->wDeviceID ) {
     case _MIDI_SOUND_MASTER_II    :    //   0xa000
     case _MIDI_MPU_401            :    //   0xa001
          musicmode=MM_MIDIGEN;
          break;
     case _MIDI_OPL2               :    //   0xa002
          musicmode=MM_MIDIFM;
          break;
     case _MIDI_MT_32              :    //   0xa004
          musicmode=MM_NOHARDWARE;
          break;
     case _MIDI_INTERNAL_SPEAKER   :    //   0xa006
          musicmode=MM_NOHARDWARE;
          break;
     case _MIDI_WAVE_TABLE_SYNTH   :    //   0xa007  
          musicmode=MM_NOHARDWARE;
          break;
     case _MIDI_AWE32              :    //   0xa008  
          musicmode=MM_MIDIAWE32;
          break;
     case _MIDI_OPL3               :    //   0xa009  
          musicmode=MM_MIDIFM;
          break;
     case _MIDI_GUS                :    //   0xa00a  
          musicmode=MM_NOHARDWARE;
          break;
     case 0x00000000:
     case 0xFFFFFFFF:
          musicmode=MM_NOHARDWARE;
          break;
     default:
          musicmode=MM_MIDIFM;
          break;
     }

    #ifdef DIGILOOPSACTIVE
     if( (digiloopflag != 0) && (soundmode != SM_NOHARDWARE) ) {
		 musicmode=MM_NOHARDWARE;	
     }
     else {
           digiloopflag=0;
     }
    #else
     digiloopflag=0;
    #endif

     switch( musicmode ) {
     default:
     case MM_NOHARDWARE:
           if( digiloopflag == 0 ) {                  
               lm("no music");               
           }
           else {
               lm("music is looped samples");
           }
           break;
     case MM_MIDIFM:
          lm("music is midi fm");
           break;
     case MM_MIDIDIGI:
          lm("music is midi digi");
           break;
     case MM_MIDIGEN:
          lm("music is general midi");
           break;
     case MM_MIDIAWE32:
          lm("music is awe32");
           break;
     }

     if( soundmode != SM_NOHARDWARE ) {
 	     if( sosDIGIInitSystem(_NULL,_SOS_DEBUG_NORMAL) != _ERR_NO_ERROR ) {
		     crash("initsb: DIGIInit failed");
		}
	     rv=sosDIGIInitDriver(digicapptr->wDeviceID,digihardwareptr,digiinitdriverptr,fhdigidriverptr);  
          if( rv != _ERR_NO_ERROR ) {
               crash("initsb: failed DIGIInitDriver");
		}
          rv=sosTIMERRegisterEvent(_SOS_FILL_TIMER_RATE,digiinitdriverptr->lpFillHandler,fhdigifillptr);
          if( rv != _ERR_NO_ERROR ) {
     		crash("intsb: failed register lpFillHandler" );
		}
          vol=(soundv<<11);
          if( (vol < 0) || (vol > 0x7fff) )
               vol=0x7fff;
          sosDIGISetMasterVolume(*fhdigidriverptr,vol);
	     setupdigi();

          smkinit(*fhdigidriverptr);
	}
     else {
          smkinit(-1);
     }


     if( digiloopflag != 0 ) {    
           initlooptable();
     }

     if( musicmode != MM_NOHARDWARE ) {
          if( sosMIDIInitSystem(_NULL,_SOS_DEBUG_NORMAL) != _ERR_NO_ERROR ) {
               crash("initsb: MIDIInit failed");
          }
          midiinitdriverptr->lpDriverMemory=( VOID _far *)_NULL;
          midiinitdriverptr->sDIGIInitInfo=( _SOS_MIDI_DIGI_INIT_DRIVER _far *)NULL;
          rv=sosMIDIInitDriver(midicapptr->wDeviceID,midihardwareptr,midiinitdriverptr,fhmididriverptr);
          if( rv != _ERR_NO_ERROR ) {
               crash("initsb: MIDIInitDriver failed");
          }
          if( (musicmode == MM_MIDIDIGI) && (soundmode != SM_NOHARDWARE) ) {
               mididigiinitdriverptr->wDriverID=digicapptr->wDeviceID;
               mididigiinitdriverptr->wTimerRate=_SOS_FILL_TIMER_RATE; 
               mididigiinitdriverptr->wMaxVoices=0x08;
               mididigiinitdriverptr->wVelocitySensing=_FALSE;
               mididigiinitdriverptr->sDIGIDriverInfo=digiinitdriverptr;
               mididigiinitdriverptr->sDIGIHardwareInfo=digihardwareptr;
               midiinitdriverptr->lpDriverMemory=( VOID _far *)_NULL;
               midiinitdriverptr->sDIGIInitInfo=mididigiinitdriverptr;
               rv=sosMIDIInitDriver(_MIDI_DIGI,midihardwareptr,midiinitdriverptr,fhmididigidriverptr);
               if( rv != _ERR_NO_ERROR ) {
                    crash("initsb: MIDIInitDriver failed");
               }
          }
          vol=(musicv<<3);
          if( (vol < 0) || (vol > 127) )
               vol=127;
          sosMIDISetMasterVolume(vol);
	     sosMIDIEnableChannelStealing(_FALSE);
          setupmidi();
     }
}

VOID _far cdecl 
soundcallback(WORD fhdriver, WORD action, WORD fhsample)
{
     int  i;

     switch( action ) {
     case _SAMPLE_PROCESSED:
          return;
     case _SAMPLE_LOOPING:
          return;
     case _SAMPLE_DONE:
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsoundptr[i]->handle == fhsample ) {
                    sbufptr[dsoundptr[i]->sndnum]->users--;
                    if( sbufptr[dsoundptr[i]->sndnum]->users == 0 ) {
                         sbufptr[dsoundptr[i]->sndnum]->cache_lock=0x00;                              
                    }
                    dsoundptr[i]->handle=NULL_HANDLE;
                    dsoundptr[i]->plevel=0;
                    dsoundptr[i]->sndnum=-1;
                    break;
               }
          }
          break;
     }
     return;
}

VOID _far cdecl 
digiloopcallback(WORD fhdriver, WORD action, WORD fhsample)
{
     if ( action == _SAMPLE_LOOPING ) {
			if(LoopPending) {
				SND_SwapLoops();
				LoopPending = 0;
			}
     }
} 

int
playsound(int sn, long sndx,long sndy, int loop, short type)
{
     int       i,nr=0;
     long      dist=0L,vol=0L,pan=0L;

     if( (toggles[TOGGLE_SOUND] == 0) || (soundmode == SM_NOHARDWARE) || (sn < 0) || (sn >= TOTALSOUNDS) )
          return(-1);

     if( type&(ST_UNIQUE|ST_AMBUPDATE|ST_TOGGLE) ) {
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsoundptr[i]->handle == NULL_HANDLE ) {
                    continue;
               }
               else if( dsoundptr[i]->sndnum == sn ) {
                    if( (type&ST_TOGGLE) != 0 ) {
                         stopsound(i);
                    }
                    return(-1);
               }
          }
     }

     for( i=0; i<MAXSOUNDS; i++ ) {
          if( dsoundptr[i]->handle == NULL_HANDLE ) 
               break;
     }
     if( i == MAXSOUNDS ) {
          // add plevel and multiple occurrence replacement
          return(-1);
     }

     dsoundptr[i]->type=type;
     dsoundptr[i]->x=sndx; dsoundptr[i]->y=sndy;

     sbufptr[sn]->cache_lock=1;

     if( sbufptr[sn]->cache_ptr == 0L ) {   // no longer in cache
          allocache(&(sbufptr[sn]->cache_ptr), sbufptr[sn]->cache_length, &(sbufptr[sn]->cache_lock));
          if( sbufptr[sn]->cache_ptr == 0L ) {
               sbufptr[sn]->cache_lock=0x00;
               return(-1);
          }
	     lseek(fhsounds, sbufptr[sn]->offset, SEEK_SET);
	     nr=read(fhsounds,( void *)(sbufptr[sn]->cache_ptr),sbufptr[sn]->cache_length);
          if( nr != sbufptr[sn]->cache_length ) {
               sbufptr[sn]->cache_ptr=0L;
               sbufptr[sn]->cache_lock=0x00;
               return(-1);
          }
     }
     else {
     }

     if( (type&ST_IMMEDIATE) ) {
          vol=0x7fff;
          pan=13;
     }
     else {
          dist=labs(posx[screenpeek]-sndx)+labs(posy[screenpeek]-sndy);
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
     }
     if( (vol < 0) )
         vol=0;
     if( (vol > 0x7fff) )
         vol=0x7fff;

     sampleptr[i]->lpSamplePtr=( LPSTR)sbufptr[sn]->cache_ptr;
     sampleptr[i]->dwSampleSize=sbufptr[sn]->cache_length;
     sampleptr[i]->wLoopCount=loop;
     sampleptr[i]->wChannel=_CENTER_CHANNEL;
     sampleptr[i]->wVolume=vol;
     sampleptr[i]->wSampleID=sn;
     sampleptr[i]->lpCallback=soundcallback;
     sampleptr[i]->wSamplePort=0;
     sampleptr[i]->wSampleFlags=_LOOPING|_VOLUME|_PANNING;
     sampleptr[i]->dwSampleLoopPoint=0;
     sampleptr[i]->dwSampleLoopLength=0;
     sampleptr[i]->dwSamplePitchAdd=0;
     sampleptr[i]->dwSampleByteLength=sbufptr[sn]->cache_length;
     sampleptr[i]->wSamplePanLocation=PanArray[pan];
     if( sampleptr[i]->wSamplePanLocation > 0xffff ) 
          sampleptr[i]->wSamplePanLocation=0x8000;
     sampleptr[i]->wSamplePanSpeed=0;

	dsoundptr[i]->handle=sosDIGIStartSample(*fhdigidriverptr,sampleptr[i]);
     if( dsoundptr[i]->handle == _ERR_NO_SLOTS ) {
          dsoundptr[i]->handle=NULL_HANDLE;
          dsoundptr[i]->plevel=0;
          dsoundptr[i]->sndnum=-1;
          if( sbufptr[sn]->users == 0 ) {
               sbufptr[sn]->cache_lock=0;
          }
          return(-1);
     }
     else {
	     sbufptr[sn]->users++;
         #ifdef SNDDEBUG
          showmessage("SND %03d ADDR %08ld USRS %02d", sn, sbufptr[sn]->cache_ptr, sbufptr[sn]->users);
         #endif
         dsoundptr[i]->sndnum=sn;
     }

     return(i);    
}

stopsound(int i)
{
     if( soundmode == SM_NOHARDWARE )
          return;
     if( (i < 0) || (i >= MAXSOUNDS) ) {
          return;
     }
     if( dsoundptr[i]->handle == NULL_HANDLE )
          return;
     
     sosDIGIStopSample(*fhdigidriverptr, dsoundptr[i]->handle);
     sbufptr[dsoundptr[i]->sndnum]->users--;
     if( sbufptr[dsoundptr[i]->sndnum]->users < 0 )
              sbufptr[dsoundptr[i]->sndnum]->users=0;
     if( sbufptr[dsoundptr[i]->sndnum]->users == 0 ) {
         sbufptr[dsoundptr[i]->sndnum]->cache_lock=0x00;                              
     }
     dsoundptr[i]->handle=NULL_HANDLE;
     dsoundptr[i]->plevel=0;
     dsoundptr[i]->sndnum=-1;
}

void
updatesounds(int    snum)
{
     long      dist=0L,vol=0L,pan=0L;
     int       i,bufnum,panindx;

     if( (toggles[TOGGLE_SOUND] == 0) || (soundmode == SM_NOHARDWARE) ) 
          return;

     for( i=0; i<MAXSOUNDS; i++ ) {
          if( dsoundptr[i]->handle == NULL_HANDLE ) {
               continue;
          }
          if( (dsoundptr[i]->type&(ST_IMMEDIATE|ST_NOUPDATE|ST_VEHUPDATE)) != 0 ) {
               continue;
          }
          dist=labs(posx[snum]-dsoundptr[i]->x)+labs(posy[snum]-dsoundptr[i]->y);

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

          if( (vol < 0) )
              vol=0;
          if( (vol > 0x7fff) )
              vol=0x7fff;

          if( dsoundptr[i]->handle != NULL_HANDLE ) {   // safeguard on int level
               sosDIGISetSampleVolume(*fhdigidriverptr, dsoundptr[i]->handle, vol);
          }
         #ifdef DYNAMICPANPERFECT
          panindx=((getangle(posx[snum]-dsoundptr[i]->x,posy[snum]-dsoundptr[i]->y)+(2047-ang[snum]))&2047) >> 6;
          if( (panindx < 0) || (panindx > 35) ) 
               panindx=13;
          pan=PanArray[panindx];
          if( pan > 0xffff )
               pan=0xffff;
          if( dsoundptr[i]->handle != NULL_HANDLE ) {   // safeguard on int level
               sosDIGISetPanLocation(*fhdigidriverptr, dsoundptr[i]->handle, pan);
          }
         #endif
     }
}

void
updatevehiclesnds(int i, long sndx, long sndy)
{
     long      dist=0L,vol=0L,pan=0L;

	if( soundmode == SM_NOHARDWARE ) {
   	     return;
     }
     if( (i < 0) || (i > MAXSOUNDS) ) {
          return;
     }

	dsoundptr[i]->x=sndx;
	dsoundptr[i]->y=sndy;

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

     if( dsoundptr[i]->handle != NULL_HANDLE ) 
	     sosDIGISetSampleVolume(*fhdigidriverptr, dsoundptr[i]->handle, vol);

     pan=((getangle(posx[screenpeek]-dsoundptr[i]->x,posy[screenpeek]-dsoundptr[i]->y)+(2047-ang[screenpeek]))&2047) >> 6;
     if( (pan < 0) || (pan > 35) ) 
          pan=13;

     if( dsoundptr[i]->handle != NULL_HANDLE ) 
	     sosDIGISetPanLocation(*fhdigidriverptr, dsoundptr[i]->handle, PanArray[pan]);
}


VOID _far cdecl 
songcallback(WORD shandle)
{
}

VOID _far cdecl 
triggercallback(WORD shandle, BYTE track, BYTE id)
{
}

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
              crash("playsong: bad read");
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


void menusong(int insubway)
{
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

     switch( musicmode ) {
     case MM_MIDIFM:
          break;
     case MM_MIDIAWE32:
		 index++;        
          break;
     case MM_MIDIGEN:
		 index+=2;
          break;
     }

     for( i=0; i<SONGSPERLEVEL; i++ ) {
          songptr[0]->handle=NULL_HANDLE;
          songptr[0]->offset=songlist[index*3]*4096;
          songptr[0]->playing=0;
          songptr[0]->pending=0;
          songptr[0]->length=( WORD)songlist[(index*3)+1];
          if( songptr[0]->length >= MAXBASESONGLENGTH ) {
               crash("prepsongs: basesong exceeded max length");
          }
     }
     songptr[0]->buffer=&basesongdata;

     playsong(BASESONG);
	

}

startmusic(int level)
{
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
               crash("prepsongs: basesong exceeded max length");
          }
     }
     songptr[0]->buffer=&basesongdata;
     songptr[1]->buffer=&secondsongdata;
     songptr[2]->buffer=&thirdsongdata;

     playsong(BASESONG);
}

songmastervolume(int vol)
{
     if( musicmode == MM_NOHARDWARE )
          return;

     if( (vol < 0) || (vol > 127) )
          vol=127;
     sosMIDISetMasterVolume(vol);
}

soundmastervolume(int vol)
{
     if( soundmode == SM_NOHARDWARE )
          return;

     if( (vol < 0) || (vol > 0x7FFF) )
          vol=0x7fff;
     sosDIGISetMasterVolume(*fhdigidriverptr, vol);
}

musicfade(int  dir)
{
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
}

musicoff(void)
{
     int  i;

     if( musicmode != MM_NOHARDWARE )  {
          for( i=0; i<SONGSPERLEVEL; i++ ) {
               if( songptr[i]->handle == NULL_HANDLE )
                    continue;
               sosMIDIStopSong(songptr[i]->handle);
               sosMIDIUnInitSong(songptr[i]->handle);
          }
    }
}

stopallsounds()
{
     int       i;

     if( soundmode == SM_NOHARDWARE )
          return;

     for( i=0; i< MAXSOUNDS; i++ ) {
          if( dsoundptr[i]->handle == NULL_HANDLE )
               continue;
          sosDIGIStopSample(*fhdigidriverptr, dsoundptr[i]->handle);
          sbufptr[dsoundptr[i]->sndnum]->users--;
          if( sbufptr[dsoundptr[i]->sndnum]->users < 0 )
              sbufptr[dsoundptr[i]->sndnum]->users=0;
          if( sbufptr[dsoundptr[i]->sndnum]->users == 0 ) {
               sbufptr[dsoundptr[i]->sndnum]->cache_lock=0x00;                              
          }
          dsoundptr[i]->handle=NULL_HANDLE;
          dsoundptr[i]->plevel=0;
          dsoundptr[i]->sndnum=-1;
     }

//clear variables that track looping sounds
     loopinsound=-1;
     baydoorloop=-1;
     ambsubloop=-1;

}

uninitsb(void)
{
     int       i;

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

    if( digiloopflag != 0 ) {
          for( i=0; i<MAXLOOPS; i++ ) {
               if( lsoundptr[i]->handle == NULL_HANDLE )
                    continue;
               sosDIGIStopSample(*fhdigidriverptr, lsoundptr[i]->handle);
          }
    }

	if( soundmode != SM_NOHARDWARE ) {
          for( i=0; i<MAXSOUNDS; i++ ) {
               if( dsoundptr[i]->handle == NULL_HANDLE )
                    continue;
               sosDIGIStopSample(*fhdigidriverptr, dsoundptr[i]->handle);
          }
	     if( soundmode != SM_NOHARDWARE ) {
	          sosTIMERRemoveEvent(*fhdigifillptr);
		     sosDIGIUnInitDriver(*fhdigidriverptr, _TRUE,_TRUE);
		     sosDIGIUnInitSystem();
          }
     }

     if( fhsounds >= 0 )
          close(fhsounds);
     if( fhsongs >= 0 )
          close(fhsongs);

     if( hLoopFile != -1 )
	      close( hLoopFile );
     if( LoopList != ( DWORD *)NULL )
	      free( LoopList );

     if( melodicbankptr )
          free( ( void *)melodicbankptr);
     if( drumbankptr )
          free( ( void *)drumbankptr);
     if( digitalbankptr )
          free( ( void *)digitalbankptr);

     smkuninit(*fhdigidriverptr);
}

void
initlooptable(void)
{
	if(!digiloopflag) 
		return;
	
	hLoopFile = open("LOOPS",O_RDONLY | O_BINARY);
	if( hLoopFile == -1 ) {
		crash("initlooptable: cant open loops");
	}
	LoopList = ( DWORD    *)malloc(0x1000); 
    if( LoopList == ( DWORD *)NULL )
         crash("initlooptable: cant get mem for LoopList");
	lseek(hLoopFile,-4096L,SEEK_END);
	read(hLoopFile,(void *)FP_OFF(LoopList),4096);
}

void
tekprepdigiloops(void)
{
	if( !digiloopflag )
		return;

	loopbufptr[0]->cache_lock=1;
	allocache(&(loopbufptr[0]->cache_ptr), MAX_LOOP_LENGTH, &(loopbufptr[0]->cache_lock));
	if( loopbufptr[0]->cache_ptr == 0L ) {
		loopbufptr[0]->cache_lock=0x00;
		digiloopflag=0;
 	}

	loopbufptr[1]->cache_lock=1;
	allocache(&(loopbufptr[1]->cache_ptr), MAX_LOOP_LENGTH, &(loopbufptr[1]->cache_lock));
	if( loopbufptr[1]->cache_ptr == 0L ) {
		loopbufptr[1]->cache_lock=0x00;
		digiloopflag=0;
	}
}

void 
SND_LoadLoop(int load_start)
{
     int	nr=0;
	SeekIndex = ( LoopList[(LoopIndex * 3)+0] * 4096 );
	SampleSize= (WORD)LoopList[(LoopIndex * 3) + 1];
     lseek(hLoopFile, SeekIndex, SEEK_SET);

     if(!load_start) {
	     nr=read(hLoopFile,( void *)(loopbufptr[looptoggle]->cache_ptr),SampleSize);
   	     if( nr != SampleSize ) {
   		     loopbufptr[looptoggle]->cache_ptr=0L;
   		     loopbufptr[looptoggle]->cache_lock=0x00;
   		     crash("read problem with loops");
		}
          loopsampleptr[looptoggle]->lpSamplePtr=( LPSTR)loopbufptr[looptoggle]->cache_ptr;
	     loopsampleptr[looptoggle]->dwSampleSize= SampleSize;
	     loopsampleptr[looptoggle]->dwSampleByteLength= SampleSize;
	}
	else {
	     nr=read(hLoopFile,( void *)(loopbufptr[looptoggle]->cache_ptr),SampleSize);
   	     if( nr != SampleSize ) {
   		     loopbufptr[looptoggle]->cache_ptr=0L;
   		     loopbufptr[looptoggle]->cache_lock=0x00;
   		     crash("read problem with loops");
		}
         loopsampleptr[looptoggle]->lpSamplePtr=( LPSTR)loopbufptr[looptoggle]->cache_ptr;
	     loopsampleptr[looptoggle]->dwSampleSize= SampleSize;
	     loopsampleptr[looptoggle]->dwSampleByteLength= SampleSize;
	     lsoundptr[looptoggle]->handle=sosDIGIStartSample(*fhdigidriverptr,loopsampleptr[looptoggle]);
	     looptoggle^=1;
	}

	LoopIndex++;
	if(LoopIndex>MAX_SND_LOOPS-1)
	     LoopIndex=0;
}

VOID 
SND_SwapLoops(VOID)
{
     int temp,i;

	temp=looptoggle^1;

	if( !sosDIGISampleDone(*fhdigidriverptr,lsoundptr[temp]->handle) )
	{
		sosDIGIStopSample(*fhdigidriverptr,lsoundptr[temp]->handle);
		lsoundptr[looptoggle]->handle = sosDIGIStartSample(*fhdigidriverptr,loopsampleptr[looptoggle] );
	}
	  
	looptoggle^=1;

}

BOOL	cdecl 
hmiINIOpen( _INI_INSTANCE * sInstance, PSTR szName )
{
     WORD  hFile;

     strcpy( sInstance->szName, szName );

     if( ( hFile = open(szName, O_RDONLY | O_BINARY )) == -1 )
          return( _FALSE );

     sInstance->wSize=lseek( hFile, 0, SEEK_END );
     sInstance->wMaxSize=sInstance->wSize + _INI_EXTRA_MEMORY;
     lseek( hFile, 0, SEEK_SET );
     if( (sInstance->pData=( PSTR)malloc( sInstance->wMaxSize) ) == _NULL ) {
          close( hFile );
          return( _FALSE );
     }

     if( read( hFile, sInstance->pData, sInstance->wSize ) != sInstance->wSize ) {
          close( hFile );
          free( sInstance->pData );
          return( _FALSE );
     }

     close( hFile );

     sInstance->pCurrent  =  sInstance->pData;
     sInstance->wCurrent  =  0;

     sInstance->pItem     =  _NULL;
     sInstance->pList     =  _NULL;
     sInstance->pItemPtr  =  _NULL;
     sInstance->pListPtr  =  _NULL;

     sInstance->wFlags    &= ~_INI_MODIFIED;

     return( _TRUE );
}

BOOL	cdecl 
hmiINIClose( _INI_INSTANCE * sInstance )
{
     WORD  hFile;

     if( sInstance->wFlags & _INI_MODIFIED ) {
          if ( ( hFile =  open( (const char * )sInstance->szName, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0 ) ) == -1 ) {
               free( sInstance->pData );
               return( _FALSE );
          }
          write( hFile, sInstance->pData, sInstance->wSize );
          close( hFile );
     }

     free( sInstance->pData );

     return( _TRUE );
}

static   PSTR  szHexNumbers="0123456789ABCDEF";

WORD	
hmiINIGetHexIndex( BYTE bValue )
{
     WORD	wIndex;

     for( wIndex = 0; wIndex < 16; wIndex++ )
          if( szHexNumbers[ wIndex ] == toupper( bValue ) )
               return( wIndex );

     return( -1 );
}

static   WORD  wMultiplier[]={ 1, 16, 256, 4096, 65536, 1048576, 16777216, 268435456 };

WORD	
hmiINIHex2Decimal( PSTR szHexValue )
{
     WORD  wDecimal=0;
     WORD	 wPlaces=strlen( szHexValue );
     WORD	 wMultIndex;
     WORD	 wIndex=0;

     do {
          wDecimal+=wMultiplier[ wPlaces - 1 ] * hmiINIGetHexIndex( (BYTE)szHexValue[ wIndex++ ] );
          wPlaces--;
     } while( wPlaces > 0 );

     return( wDecimal );
}

BOOL  cdecl 
hmiINILocateSection( _INI_INSTANCE * sInstance, PSTR szName )
{
     PSTR  pDataPtr;
     PSTR	 pSectionPtr;
     PSTR  szSection;
     WORD  wIndex;
     WORD  wFoundFlag  =  _FALSE;

     pDataPtr    =  sInstance->pData;
     wIndex      =  0;

     do {
          if( *pDataPtr == _INI_SECTION_START ) {
               pSectionPtr =  pDataPtr;
               pDataPtr++;
               szSection   =  szName;
               while( *pDataPtr == *szSection && wIndex < sInstance->wSize ) {
                  szSection++;
                  pDataPtr++;
                  wIndex++;
               }
               if( *pDataPtr == _INI_SECTION_END && *szSection == _NULL ) {
                  wFoundFlag  =  _TRUE;
                  while( *pDataPtr != _INI_LF )
                    pDataPtr++;
                  pDataPtr++;
                  sInstance->pListPtr  =  pDataPtr;
                  sInstance->pCurrent  =  pDataPtr;
                  sInstance->wCurrent  =  wIndex;
                  sInstance->pSection  =  pSectionPtr;
               }
          }
          pDataPtr++;
          wIndex++;
     } while( !wFoundFlag && wIndex < sInstance->wSize );

     return( ( BOOL )wFoundFlag );
}

BOOL	cdecl 
hmiINIGetDecimal( _INI_INSTANCE * sInstance, WORD * wValue )
{
     PSTR  pDataPtr;
	WORD  wDValue;
     BYTE  bBuffer[ 32 ];
     WORD  wIndex;

     if( sInstance->pList )
          pDataPtr=sInstance->pList;
     else
          pDataPtr=sInstance->pItem;
     if( pDataPtr == _NULL )
          return( _FALSE );

     while( *pDataPtr == _INI_SPACE )
          pDataPtr++;

     if( *pDataPtr == _INI_EOL )
          return( _FALSE );

     wIndex   =  0;
     while( *pDataPtr != _INI_EOL && *pDataPtr != _INI_LIST_SEPERATOR && *pDataPtr != _INI_SPACE ) {
          bBuffer[ wIndex++ ]  =  *pDataPtr++;
     }
     bBuffer[ wIndex ]='\0';
     if( wIndex == 0 )
          return( _FALSE );

     while( *pDataPtr == _INI_SPACE )
          pDataPtr++;
     if( *pDataPtr == _INI_LIST_SEPERATOR ) {
          sInstance->pList  =  ++pDataPtr;
     }
     else
          sInstance->pList  =  pDataPtr;

     if( bBuffer[ 1 ] == _INI_HEX_INDICATOR ) {
          wDValue  =  hmiINIHex2Decimal( &bBuffer[ 2 ] );
     }
     else {
          wDValue  =  (WORD)atoi( bBuffer );
     }

     *wValue  =  wDValue;

     return( _TRUE );
}

BOOL	cdecl 
hmiINILocateItem( _INI_INSTANCE * sInstance, PSTR szItem )
{
     PSTR  pDataPtr;
     PSTR  pItemPtr;
     PSTR  szSearch;
     WORD  wIndex;
     WORD  wFoundFlag=_FALSE;

     pDataPtr =  sInstance->pCurrent;
     wIndex   =  sInstance->wCurrent;

     do {

          szSearch =  szItem;
          if( *pDataPtr == *szSearch ) {
               pItemPtr    =  pDataPtr;
               pDataPtr++;
               szSearch++;
               wIndex++;
               while( *pDataPtr == *szSearch && wIndex < sInstance->wSize ) {
                    pDataPtr++;
                    szSearch++;
                    wIndex++;
               }
               if( *szSearch == _NULL ) {
                    while( *pDataPtr != _INI_EQUATE && *pDataPtr != _INI_EOL ) {
                         pDataPtr++;
                         wIndex++;
                    }
                    if( *pDataPtr == _INI_EQUATE ) {
                         pDataPtr++;
                         wIndex++;
                         sInstance->pItem     =  pDataPtr;
                    }
                    else {
                         sInstance->pItem     =  _NULL;
                    }
                    sInstance->pItemPtr  =  pItemPtr;
                    sInstance->pList  =  _NULL;
                    wFoundFlag  =  _TRUE;
               }
          }
          pDataPtr++;
          wIndex++;

     } while( !wFoundFlag && wIndex < sInstance->wSize && *pDataPtr != _INI_SECTION_START );

     return( ( BOOL )wFoundFlag );
}

BOOL	cdecl 
hmiINIGetItemDecimal( _INI_INSTANCE * sInstance, PSTR szItem, WORD * wValue )
{
     if( !hmiINILocateItem( sInstance, szItem ) )
          return( _FALSE );

     if( !hmiINIGetDecimal( sInstance, wValue ) )
          return( _FALSE );

     return( _TRUE );
}

readhmicfg()
{
     _INI_INSTANCE  sInstance;
     BOOL           wError;

      if( !hmiINIOpen(&sInstance, "hmiset.cfg") )
          return( _FALSE );

      if( !hmiINILocateSection(&sInstance, "DIGITAL") ) {
          hmiINIClose( &sInstance );
          return( _FALSE );
      }
      wError=hmiINIGetItemDecimal( &sInstance, "DeviceID",  ( WORD *)&(digicapptr->wDeviceID)  );
      wError=hmiINIGetItemDecimal( &sInstance, "DevicePort",( WORD *)&(digihardwareptr->wPort) );
      wError=hmiINIGetItemDecimal( &sInstance, "DeviceDMA", ( WORD *)&(digihardwareptr->wDMA)  );
      wError=hmiINIGetItemDecimal( &sInstance, "DeviceIRQ", ( WORD *)&(digihardwareptr->wIRQ)  );
      if( !wError ) {
          hmiINIClose( &sInstance );
          return( _FALSE );
      }

      if( !hmiINILocateSection(&sInstance, "MIDI") ) {
          hmiINIClose( &sInstance );
          return( _FALSE );
      }
      wError=hmiINIGetItemDecimal( &sInstance, "DeviceID",   ( WORD *)&(midicapptr->wDeviceID)  );
      wError=hmiINIGetItemDecimal( &sInstance, "DevicePort", ( WORD *)&(midihardwareptr->wPort) );
      if( !wError ) {
          hmiINIClose( &sInstance );
          return( _FALSE );
      }

      hmiINIClose( &sInstance );
      return( _TRUE );
}

