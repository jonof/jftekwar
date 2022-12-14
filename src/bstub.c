#include "build.h"
#include "editor.h"
#include "pragmas.h"
#include "baselayer.h"
#include "cache1d.h"
#include "names.h"

#define AVERAGEFRAMES 16
static int averagefps;
static unsigned int frameval[AVERAGEFRAMES], framecnt = 0;

#define NUMOPTIONS 8
#define NUMKEYS 19

unsigned char option[NUMOPTIONS] = {0,0,0,0,0,0,98,0};
int keys[NUMBUILDKEYS] =
{
	0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
	0x1e,0x2c,0xd1,0xc9,0x33,0x34,
	0x9c,0x1c,0xd,0xc,0xf,0x45
};

char *defsfilename = "tekwar.def";
int nextvoxid = 0;

#define   PULSELIGHT     0
#define   FLICKERLIGHT   1
#define   DELAYEFFECT    2
#define   WPANNING       3
#define   FPANNING       4
#define   CPANNING       5
#define   FLICKERDELAY   6
#define   BLINKDELAY     7
#define   STEADYLIGHT    8

#define   NO_PIC    0

#define   INANIMATE      0       
#define   INACTIVE       100     
#define   STANDING       201     
#define   AMBUSH         202     
#define   GUARD          203     
#define   STALK          204     
#define   FLEE           205     
#define   CHASE          206     
#define   PATROL         207     
#define   CRAWL          208     
#define   STROLL         209     
#define   PINBALL        403
#define   FLY            500     
#define   RODENT         502

#define   GUN2FLAG            2
#define   GUN3FLAG            3
#define   GUN4FLAG            4
#define   GUN5FLAG            5
#define   GUN6FLAG            6
#define   GUN7FLAG            7

#define   CLASS_NULL               0
#define   CLASS_FCIAGENT           1
#define   CLASS_CIVILLIAN          2
#define   CLASS_SPIDERDROID        3
#define   CLASS_COP                4
#define   CLASS_MECHCOP            5
#define   CLASS_TEKBURNOUT         6
#define   CLASS_TEKGOON            7
#define   CLASS_ASSASSINDROID      8
#define   CLASS_SECURITYDROID      9
#define   CLASS_TEKBOSS            10
#define   CLASS_TEKLORD            11

#define   PTS_CIVILLIAN            10    
#define   PTS_SPIDERDROID          15
#define   PTS_COP                  50
#define   PTS_MECHCOP              100
#define   PTS_TEKBURNOUT           25
#define   PTS_TEKGOON              50
#define   PTS_ASSASSINDROID        100
#define   PTS_SECURITYDROID        200
#define   PTS_TEKBOSS              100
#define   PTS_TEKLORD              200

static   char tempbuf[256];
static   char lo[32];
static   char hi[32];
static   const char *levelname;
static   short curwall=0,wallpicnum=0,curwallnum=0;
static   short cursprite=0,curspritenum=0;
static   char wallsprite=0;

struct    picattribtype  {
     unsigned  char      numframes;
     unsigned  char      animtype;
     signed    char      ycenteroffset,xcenteroffset;
     unsigned  char      animspeed;
     };

#ifdef __GNUC__
#  if __GNUC__ == 4 && __GNUC_MINOR__ >= 7
#    define TPACK __attribute__ ((packed, gcc_struct))
#  else
#    define TPACK __attribute__ ((packed))
#  endif
#else
#define TPACK
#endif

#ifdef _MSC_VER
#pragma pack(1)
#endif

#ifdef __WATCOMC__
#pragma pack(push,1);
#endif

struct TPACK spriteextension {
     unsigned  char      class;
     signed    char      hitpoints;
     unsigned  short     target;
     unsigned  short     fxmask;  
     unsigned  short     aimask;
     short     basestat;
     unsigned  short     basepic;
     unsigned  short     standpic;
     unsigned  short     walkpic;
     unsigned  short     runpic;
     unsigned  short     attackpic;
     unsigned  short     deathpic;
     unsigned  short     painpic;
     unsigned  short     squatpic;
     unsigned  short     morphpic;
     unsigned  short     specialpic;
     unsigned  char      lock;
     unsigned  char      weapon;
     short     ext2;
     };

struct TPACK XTsavetype {
     short     XTnum;
     struct    spriteextension      sprXT;
};

struct TPACK XTtrailertype {
     int      numXTs;          
     int      start;
     char      mapname[13];
     char      ID[13];
};

#ifdef _MSC_VER
#pragma pack()
#endif

#ifdef __WATCOMC__
#pragma pack(pop)
#endif

#undef TPACK

struct    spriteextension  spriteXT[MAXSPRITES];
struct XTsavetype XTsave;
struct    XTtrailertype  XTtrailer;

#define   TRAILERID "**MAP_EXTS**"

short     recordXT = -1;
struct    spriteextension     recXT;

#define   MAXCLASSES     15
short     numinclass[MAXCLASSES];

char editvelocity=0;
char elapsedtime=0;
char copyext=0;
char recordext=0;
char liststates=0;
char matchcstats=0;
char docleanup=0;
char spriteextmodified=0;


int  JS_CheckExtras(void);
void JS_DeleteExt(short sn);
int  JS_FirstFreeExt(void);
int  JS_InitExtension(short sn, short en);
int  JS_LockExt(short j);
int  JS_ShowSpriteExts(short spritenum);
void checkextras(void);


// overrides of engine insert/delete sprite
void ExtDeleteSprite(short spritenum)
{
	if( (sprite[spritenum].extra!=-1) ) {
          JS_DeleteExt(spritenum);
     }
}

void
PrintStatus(char *string,int num,char x,char y,char color)
{
     sprintf(tempbuf,"%s %d",string,num);
     printext16(x*8,ydim16+y*8,color,-1,tempbuf,0);
}

void
PrintStatusStr(char *string,char *s,char x,char y,char color)
{
     sprintf(tempbuf,"%s %s",string,s);
     printext16(x*8,ydim16+y*8,color,-1,tempbuf,0);
}

void
SpriteName(short spritenum, char *lo2)
{
     strcpy(lo2,names[sprite[spritenum].picnum]);
}

void
JS_MatchCstats(void)
{
     char      what[50];
     int       i,ans;

     sprintf(what, "1=YES,MATCH SPR CSTATS. 0 TO CANCEL. ");
     ans=getnumber16(what, 0, 2L, 0);

     if( ans != 1 ) {
          printmessage16("Cancelled...");
          return;
     }

     for( i=0; i<MAXSPRITES; i++ ) {
          if( sprite[i].cstat & 0x0001 ) {
               sprite[i].cstat|=0x0100;
          }
     }

     printmessage16("All sprite hitscans match blocking...");

 return;
}

void
JS_CleanupExtensions(void)
{
     char      what[50];

     sprintf(what, "Checking extras...");
     printmessage16(what);
     checkextras();
}

void
JS_RecordExt(int sn)
{
     char      what[50];

     clearmidstatbar16();

     if( sprite[sn].extra < 0 ) {
          sprintf(what, "Sprite has no EXT to record !");
          printmessage16(what);
          return;
     }

     recordXT=sprite[sn].extra;

     JS_ShowSpriteExts(sn);

     sprintf(what, "Recording Ext %d for copying...", sn);
     printmessage16(what);

     memcpy(&recXT, &spriteXT[recordXT], sizeof(struct spriteextension));     
}

void
JS_CopyExt(int sn)
{
     char      what[50];
     short     j;
     int       ans;

     clearmidstatbar16();

     if( (recordXT < 0) || (recordXT > MAXSPRITES) ) {
          sprintf(what, "NO current valid copy EXT to copy !");
          printmessage16(what);
          return;
     }

     j=sprite[sn].extra;

     if( j < 0 ) {
          sprintf(what, "Mapping EXT for sprite %d...", sn);
          printmessage16(what);
          j=JS_FirstFreeExt();
          if( j == -1 ) {
               printmessage16("ERROR! Not able to map to an extension.");
               return;
          }
          printmessage16("Extension Mapped.");
          sprite[sn].extra=j;
          JS_InitExtension(sn, j);
          JS_LockExt(j);
          JS_ShowSpriteExts(sn);
     }
     else {
          sprintf(tempbuf, "Overwrite existing EXT for sprite %d ?  1=YES 0=NO ", sn);
          ans=getnumber16(tempbuf, 0, 2L, 0);
          if( ans != 1 ) {
               printmessage16("Not Copied...");
               return;
          }
     }

     memcpy(&spriteXT[j], &recXT, sizeof(struct spriteextension));

     sprintf(what, "Copying Ext %d to sprite %d...", recordXT, sn);
     printmessage16(what);

     JS_ShowSpriteExts(sn);
}

void
JS_DeleteExt(short sn)
{
     short     x=sprite[sn].extra;

     memset(&spriteXT[x], 0, sizeof(struct spriteextension));

     if( qsetmode != 200 ) {
          printmessage16("Deleting Extension...");
     }

     // reset recordXT
     if( x == recordXT ) {
          recordXT=-1;
     }

 return;
}

void
JS_ClearAllExts(void)
{
     char      what[50];
     int       i,ans;

     sprintf(what, "1=YES,DELETE ALL EXTS, 0 TO CANCEL. ");
     ans=getnumber16(what, 0, 2L, 0);

     if( ans != 1 ) {
          printmessage16("Cancelled...");
          return;
     }

     sprintf(what, "REALLY DELETE EXTS ? 1=YES  0=NO ");
     ans=getnumber16(what, 0, 2L, 0);

     if( ans != 1 ) {
          printmessage16("Cancelled...");
          return;
     }

     for( i=0; i<MAXSPRITES; i++ ) {
          memset(&spriteXT[i], 0, sizeof(struct spriteextension));
     }
     for( i=0; i<MAXSPRITES; i++ ) {
          sprite[i].extra=-1;
     }

     printmessage16("All sprite EXTS cleared...");

 return;
}

void
JS_ClearAllSprites(void)
{
     char      what[50];
     int       i,ans;

     sprintf(what, "1=YES,DELETE ALL SPRITES, 0 TO CANCEL. ");
     ans=getnumber16(what, 0, 2L, 0);

     if( ans != 1 ) {
          printmessage16("Cancelled...");
          return;
     }

     sprintf(what, "REALLY DELETE SPRITES ? 1=YES  0=NO ");
     ans=getnumber16(what, 0, 2L, 0);

     if( ans != 1 ) {
          printmessage16("Cancelled...");
          return;
     }

     for( i=0; i<MAXSPRITES; i++ ) {
          deletesprite(i);
          memset(&spriteXT[i], 0, sizeof(struct spriteextension));
          sprite[i].extra=-1;
     }

     printmessage16("All sprites gone...");

 return;
}

// reads ext info into sprXT array then strips .map file of ext info
int
JS_LoadSpriteExts(const char *mapname)
{
     int       fh,i,nr,nex;

     for (i=0; i<MAXSPRITES; i++) {
          memset(&spriteXT[i], 0, sizeof(struct spriteextension));
          spriteXT[i].lock=0x00;
     }

     fh = kopen4load(mapname, 0);
     if ( fh == -1 ) {
          return(-1);
     }

     // read in XTtrailer 
     klseek(fh, -((int)sizeof(struct XTtrailertype)), SEEK_END);
     memset(&XTtrailer, 0, sizeof(struct XTtrailertype));
     kread(fh, &XTtrailer, sizeof(struct XTtrailertype));

     // if no previous extension info then continue
     if( strcmp(XTtrailer.ID, TRAILERID) != 0 ) {
          buildputs("New File - No extension information...\n");
          goto noext;
     }

     buildputs("Reading in extensions...\n");

     // load and intialize spriteXT array members
     klseek(fh, XTtrailer.start, SEEK_SET);
     for( i=0; i<XTtrailer.numXTs; i++ ) {
          nr=kread(fh, &XTsave, sizeof(struct XTsavetype));
          if( nr != sizeof(struct XTsavetype) )
               break;
          spriteXT[XTsave.XTnum]=XTsave.sprXT;  // struct assign
     }

     nex=JS_CheckExtras();
     if( nex != i ) {
          buildprintf("ERROR: %d Sprite Extras != %d Extensions.\n", nex,i);
     }
     else {
          buildprintf("Loaded %d extensions.\n", i);
     }

noext:

     kclose(fh);

 return(0);
}

void checkextras(void)
{
     int       i,j,ext;
     FILE      *fp;

     fp=fopen("exts", "wt");
     if( fp == NULL ) {
          return;
     }

     fprintf(fp,"\n\t SPR\t\t EXT\n\n");
     for( i=0; i<MAXSPRITES; i++ ) {
          if( sprite[i].extra != -1 ) {
               fprintf(fp,"\t%4d\t\t%4d\n", i,sprite[i].extra);
          }
     }

     fprintf(fp, "\n");
     // check for maps to unlocked XT
     for( i=0; i<MAXSPRITES; i++ ) {
          ext=sprite[i].extra;
          if( ext != -1 ) {
               if( spriteXT[ext].lock == 0x00 ) {
                    sprite[i].extra=-1;
                    buildprintf("bad extra for %d\n", i);
                    fprintf(fp, "Sprite %d at %d,%d mapped to unlocked XT %4d\n", i,sprite[i].x,sprite[i].y,ext);
               }
          }
     }

     fprintf(fp, "\n");
     // check for locked XT with no mapping
     for( i=0; i<MAXSPRITES; i++ ) {
          if( spriteXT[i].lock == 0xFF ) {
               for( j=0; j<MAXSPRITES; j++ ) {
                    if( sprite[j].extra == i ) {
                         break;
                    }
               }
               if( j == MAXSPRITES ) {
                    spriteXT[i].lock=0x00;
                    buildprintf("extension %d had no map\n", i);
                    fprintf(fp, "No mapping for locked XT %4d\n", i);
               }
          }
     }

     fprintf(fp, "\n");
     // check for non-unique mapping
     for( i=0; i<MAXSPRITES; i++ ) {
          ext=sprite[i].extra;
          if( ext != -1 ) {
               for( j=0; j<MAXSPRITES; j++ ) {
                    if( (j != i) && (sprite[j].extra == ext) ) {
                         buildprintf("non unique map for %d\n",i);
                         fprintf(fp, "Non unique mapping for %4d at %d,%d\n", i,sprite[i].x,sprite[i].y);
                         break;
                    }
               }
          }
     }

     fclose(fp);
}

// tags on information in sprXT array to end of .map file
int
JS_SaveSpriteExts(const char *mapname)
{
     int       fh,i;

     buildputs("Appending extensions...\n");

     fh=open(mapname, O_BINARY | O_WRONLY, 0);
     if ( fh == -1 ) {
          return(-1);
     }

     //checkextras();

     // prepare trailer
     memset(&XTtrailer, 0, sizeof(struct XTtrailertype));
     strncpy(XTtrailer.mapname, mapname, 12);
     XTtrailer.mapname[12]='\0';
     strcpy(XTtrailer.ID, TRAILERID);
     XTtrailer.ID[12]='\0';
     XTtrailer.numXTs=0L;

     // go to end of map = beginning of eXTensions
     XTtrailer.start=(int)lseek(fh, 0, SEEK_END);

     // verify extension then
     // write extensions to end of map file
     XTtrailer.numXTs=0L;
     for (i=0; i<MAXSPRITES; i++) {
          if( spriteXT[i].lock == 0xFF ) {
               memset(&XTsave, 0, sizeof(struct XTsavetype));
               XTtrailer.numXTs++;
               XTsave.XTnum=i;
               XTsave.sprXT=spriteXT[i];
               write(fh, &XTsave, sizeof(struct XTsavetype));
          }
     }

     // write trailer 
     write(fh, &XTtrailer, sizeof(struct XTtrailertype));
     close(fh);

 return(0);
}

int
JS_CheckExtras(void)
{
     int       i,nse;

     nse=0;
     for( i=0; i<MAXSPRITES; i++ ) {
          if( sprite[i].extra != -1 )
               nse++;
     }

 return(nse);
}

int
JS_InitExtension(short sn, short en)
{
 memset(&spriteXT[en], 0, sizeof(struct spriteextension));     
 spriteXT[en].basestat=sprite[sn].statnum;
 spriteXT[en].basepic=sprite[sn].picnum;

 return(1);
}

void
JS_GetPicInfo(struct picattribtype *picattr, short picnum)
{
     long      amask=picanm[picnum];

     memset(picattr, 0, sizeof(struct picattribtype));

     picattr->numframes=amask&0x0000003F;     
     picattr->animtype =( amask&0x000000C0 )>>6;
     picattr->xcenteroffset=( amask&0x0000FF00 )>>8;
     picattr->ycenteroffset=( amask&0x00FF0000 )>>16;
     picattr->animspeed=( amask&0x0F000000 )>>24;
}

int 
JS_ShowSpriteExts(short spritenum)
{
     short     i=sprite[spritenum].extra;
     char      quip[128];
     struct    picattribtype  picinfo;

	if (qsetmode == 200)    //In 3D mode
          return(0);

     clearmidstatbar16();
     printmessage16("                                        ");

     if ( i < 0 ) {
          printmessage16("Sprite has NO Mapped Extension !!");
          return(-1);
     }
     if( spriteXT[i].lock != 0xFF ) {
          sprite[spritenum].extra=-1;
          printmessage16("Sprite Mapped to unlocked EXT - repaired.");
          return(-1);
     }

     memset(quip, '\0', 128);

     PrintStatus("Sprite ", ( int)spritenum,1,4,11);
     PrintStatus("Picnum ", sprite[spritenum].picnum,1,5,11);
     sprintf(quip, "%s", names[sprite[spritenum].picnum]);
     PrintStatusStr(quip,"", 1,6,3);

     PrintStatus("Status ", sprite[spritenum].statnum,1,7,11);
     switch( sprite[spritenum].statnum ) {
     case INANIMATE:
          PrintStatusStr("","INANIMATE",1,8,3);
          break;
     case INACTIVE:       
          PrintStatusStr("","INACTIVE",1,8,3);
          break;                        
     case STANDING:
          PrintStatusStr("","STANDING",1,8,3);
          break;
     case AMBUSH:         
          PrintStatusStr("","AMBUSH",1,8,3);
          break;
     case GUARD:         
          PrintStatusStr("","GUARD",1,8,3);
          break;
     case CHASE:          
          PrintStatusStr("","CHASE",1,8,3);
          break;
     case FLEE:        
          PrintStatusStr("","FLEE",1,8,3);
          break;
     case STALK:
          PrintStatusStr("","STALK",1,8,3);
          break;
     case PATROL:         
          PrintStatusStr("","PATROL",1,8,3);
          break;
     case CRAWL:         
          PrintStatusStr("","CRAWL",1,8,3);
          break;
     case STROLL:         
          PrintStatusStr("","STROLL",1,8,3);
          break;
     case FLY:
          PrintStatusStr("","FLY",1,8,3);
          break;
     case RODENT:
          PrintStatusStr("","RODENT",1,8,3);
          break;
     case PINBALL:
          PrintStatusStr("","PINBALL",1,8,3);
          break;
     default:
          sprite[spritenum].statnum=INACTIVE;          
          PrintStatusStr("","BAD STATNUM",1,8,4);
          break;
     }

     PrintStatus(        "Extension ", i,1,9,11);

     switch ( ( int)spriteXT[i].lock ) {
     case 0x00:
          PrintStatusStr("ERROR Lock", " OFF" ,1,10,2);
          break;
     case 0xFF:
          PrintStatusStr("Lock      ", " ON" ,1,10,3);
          break;
     default:
          PrintStatusStr("BAD LOCK NUM", "" ,1,10,3);
          break;
     }

     if( spriteXT[i].class != CLASS_NULL ) {
          sprintf(quip, "1 of %d ", numinclass[spriteXT[i].class]);
          PrintStatusStr(quip,"",1,12,2);
          }
     switch ( spriteXT[i].class ) {
     case CLASS_NULL:
          break;
     case CLASS_FCIAGENT:
          PrintStatusStr("Cosmos Employees","",1,13,2);
          break;
     case CLASS_CIVILLIAN:     
          PrintStatusStr("Civillians","",1,13,2);
          break;
     case CLASS_SPIDERDROID:     
          PrintStatusStr("Spider Droids","",1,13,2);
          break;
     case CLASS_COP:     
          PrintStatusStr("Cops","",1,13,2);
          break;
     case CLASS_MECHCOP:     
          PrintStatusStr("Mech Cops","",1,13,2);
          break;            
     case CLASS_TEKBURNOUT:     
          PrintStatusStr("Tek Burnouts","",1,13,2);
          break;
     case CLASS_TEKGOON:     
          PrintStatusStr("Tek Goons","",1,13,2);
          break;
     case CLASS_ASSASSINDROID:     
          PrintStatusStr("Assasin Droids","",1,13,2);
          break;
     case CLASS_SECURITYDROID:     
          PrintStatusStr("Security Droids","",1,13,2);
          break;
     case CLASS_TEKBOSS:     
          PrintStatusStr("Tek Bosses","",1,13,2);
          break;
     case CLASS_TEKLORD:                
          PrintStatusStr("Tek Lords","",1,13,2);
          break;
     default:
          PrintStatusStr("UNCLASSIFIED","",1,13,2);
          break;
     }

     PrintStatus("Class     ", ( int)spriteXT[i].class    ,18,4,11);
     PrintStatus("Hitpoints ", ( int)spriteXT[i].hitpoints,18,5,11);
     PrintStatus("Target    ", ( int)spriteXT[i].target   ,18,6,11);
     PrintStatus("BaseStat  ", ( int)spriteXT[i].basestat ,18,7,11);
     switch( spriteXT[i].basestat ) {
     case INANIMATE:
          PrintStatusStr("","INANIMATE",18,8,3);
          break;
     case INACTIVE:       
          PrintStatusStr("","INACTIVE",18,8,3);
          break;
     case STANDING:
          PrintStatusStr("","STANDING",18,8,3);
          break;                           
     case AMBUSH:         
          PrintStatusStr("","AMBUSH",18,8,3);
          break;
     case GUARD:         
          PrintStatusStr("","GUARD",18,8,3);
          break;
     case CHASE:          
          PrintStatusStr("","CHASE",18,8,3);
          break;
     case FLEE:        
          PrintStatusStr("","FLEE",18,8,3);
          break;
     case STALK:
          PrintStatusStr("","STALK",18,8,3);
          break;
     case PATROL:         
          PrintStatusStr("","PATROL",18,8,3);
          break;
     case CRAWL:         
          PrintStatusStr("","CRAWL",18,8,3);
          break;
     case STROLL:         
          PrintStatusStr("","STROLL",18,8,3);
          break;
     case FLY:
          PrintStatusStr("","FLY",18,8,3);
          break;
     case RODENT:
          PrintStatusStr("","RODENT",18,8,3);
          break;
     case PINBALL:
          PrintStatusStr("","PINBALL",18,8,3);
          break;
     default:
          PrintStatusStr("","BAD BASE STAT",18,8,4);
          spriteXT[i].basestat=INACTIVE;
          break;
     }

     PrintStatus("FXMask    ", ( int)spriteXT[i].fxmask ,18,9,11);
     PrintStatus("AIMask    ", ( int)spriteXT[i].aimask ,18,10,11);

     /*
#define   GUN2FLAG            2
#define   GUN3FLAG            3
#define   GUN4FLAG            4
#define   GUN5FLAG            5
#define   GUN6FLAG            6
#define   GUN7FLAG            7
     case 0:                  // bare hands
     case 1:                  // baton
     case 2:                  // stunner
     case 3:                  // shrike DBK pistol
     case 4:                  // kralov CEG machine pistol
     case 5:                  // orlow 34S force gun
     case 6:                  // energy weapon
     case 7:                  // explosives
     case 8:                  // sleeping gas
     case 9:                  // flame thrower
     case 10:                 // rocket launcher
      */
     PrintStatus("Weapon    ", ( int)spriteXT[i].weapon ,18,11,11);
     switch( spriteXT[i].weapon ) {
     case GUN3FLAG:
          PrintStatusStr("","STUNNER",18,12,3);
          break;                           
     case GUN4FLAG:
          PrintStatusStr("","SHRIKE",18,12,3);
          break;
     case GUN5FLAG:
          PrintStatusStr("","KRALOV",18,12,3);
          break;
     case GUN6FLAG:
          PrintStatusStr("","PULSAR",18,12,3);
          break;
     default:
          PrintStatusStr("","BARE HANDS",18,12,4);
          spriteXT[i].weapon=0;
          break;
     }

     PrintStatus("Ext2      ", ( int)spriteXT[i].ext2   ,18,13,11);

     PrintStatus("BasePic   ", ( int)spriteXT[i].basepic   ,34,4,11);
     PrintStatus("StandPic  ", ( int)spriteXT[i].standpic  ,34,5,11);
     PrintStatus("WalkPic   ", ( int)spriteXT[i].walkpic   ,34,6,11);
     PrintStatus("RunPic    ", ( int)spriteXT[i].runpic    ,34,7,11);
     PrintStatus("AttackPic ", ( int)spriteXT[i].attackpic ,34,8,11);

     PrintStatus("DeathPic  ", ( int)spriteXT[i].deathpic  ,34,9,11);
     PrintStatus("PainPic   ", ( int)spriteXT[i].painpic   ,34,10,11);
     PrintStatus("SquatPic  ", ( int)spriteXT[i].squatpic  ,34,11,11);
     PrintStatus("MorphPic  ", ( int)spriteXT[i].morphpic  ,34,12,11);
     PrintStatus("SpecialPic", ( int)spriteXT[i].specialpic,34,13,11);

     PrintStatusStr("\x1b", names[spriteXT[i].basepic]   ,50,4,3);
     PrintStatusStr("\x1b", names[spriteXT[i].standpic]  ,50,5,3);
     PrintStatusStr("\x1b", names[spriteXT[i].walkpic]   ,50,6,3);
     PrintStatusStr("\x1b", names[spriteXT[i].runpic]    ,50,7,3);
     PrintStatusStr("\x1b", names[spriteXT[i].attackpic] ,50,8,3);

     PrintStatusStr("\x1b", names[spriteXT[i].deathpic]  ,50,9,3);
     PrintStatusStr("\x1b", names[spriteXT[i].painpic]   ,50,10,3);
     PrintStatusStr("\x1b", names[spriteXT[i].squatpic]  ,50,11,3);
     PrintStatusStr("\x1b", names[spriteXT[i].morphpic]  ,50,12,3);
     PrintStatusStr("\x1b", names[spriteXT[i].specialpic],50,13,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].basepic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM %2d  %2d", picinfo.numframes, picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil %2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD %2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK %2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,4,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].standpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,5,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].walkpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,6,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].runpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,7,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].attackpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,8,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].deathpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,9,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].painpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,10,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].squatpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,11,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].morphpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,12,3);

     JS_GetPicInfo(&picinfo, spriteXT[i].specialpic);
     switch( picinfo.animtype ) {
     case 0: sprintf(quip, "NoANM:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 1: sprintf(quip, "Oscil:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 2: sprintf(quip, "AnmFD:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     case 3: sprintf(quip, "AnmBK:%2d  %2d", picinfo.numframes,picinfo.animspeed); break;
     default:sprintf(quip, "  ???    "); break;
     }
     PrintStatusStr("", quip,64,13,3);

 return(0);
}

int
JS_SpriteXTManual(void)
{
     clearmidstatbar16();
     printmessage16("                                        ");

          liststates=0;

          printmessage16(" Manual Page: Sprites...");

          PrintStatusStr("      Valid State Types","",1,4,11);

          PrintStatusStr("  0","INANIMATE",1,6,0);
          PrintStatusStr("100","INACTIVE" ,1,7,0);
          PrintStatusStr("500","FLY"      ,1,8,0);
          PrintStatusStr("500","RODENT"   ,1,9,0);
          PrintStatusStr("201","STANDING" ,1,10,0);
          PrintStatusStr("202","AMBUSH"   ,1,11,0);

          PrintStatusStr("203","GUARD"    ,16,6,0);
          PrintStatusStr("204","STALK"    ,16,7,0);
          PrintStatusStr("205","FLEE"     ,16,8,0);
          PrintStatusStr("206","CHASE"    ,16,9,0);
          PrintStatusStr("207","PATROL"   ,16,10,0);
          PrintStatusStr("209","STROLL"   ,16,11,0);
     /*
     case 0:                  // bare hands
     case 1:                  // baton
     case 2:                  // stunner
     case 3:                  // shrike DBK pistol
     case 4:                  // kralov CEG machine pistol
     case 5:                  // orlow 34S force gun
     case 6:                  // energy weapon
     case 7:                  // explosives
     case 8:                  // sleeping gas
     case 9:                  // flame thrower
     case 10:                 // rocket launcher
     */
          PrintStatusStr(" Valid Weapons ","",30,4,11);
                            
          PrintStatusStr(" 3 STUNNER",""     ,30,6,0);
          PrintStatusStr(" 4 SHRIKE",""    ,30,7,0);
          PrintStatusStr(" 5 KRALOW",""    ,30,8,0);
          PrintStatusStr(" 6 PULSAR",""    ,30,9,0);

          PrintStatusStr(" Valid Class Types ","",50,4,11);

          PrintStatusStr("1 FCIAGENT", ""     ,48,6,0);
          PrintStatusStr("2 CIVILLIAN", ""    ,48,7,0);
          PrintStatusStr("3 SPIDERDROID", ""  ,48,8,0);
          PrintStatusStr("4 COP", ""          ,48,9,0);
          PrintStatusStr("5 MECHCOP", ""      ,48,10,0);
          PrintStatusStr("6 TEKBURNOUT", ""   ,48,11,0);

          PrintStatusStr("7  TEKGOON", ""      ,62,6,0);
          PrintStatusStr("8  ASSASSINDROID", "",62,7,0);
          PrintStatusStr("9  SECURITYDROID", "",62,8,0);
          PrintStatusStr("10 TEKBOSS",""       ,62,9,0);
          PrintStatusStr("11 TEKLORD",""       ,62,10,0);
                                                       
 return(0);
}

int
JS_FirstFreeExt(void)
{
 short    i;

 for ( i=0; i<MAXSPRITES; i++ ) {
     if ( spriteXT[i].lock == 0x00 )
          return(i);
 }

 return(-1);   // no free spot found
}

int
JS_LockExt(short j)                  
{
     if ( spriteXT[j].lock == 0xFF ) {
          return(-1);
     }
     spriteXT[j].lock=0xFF;

 return(0);
}

void
JS_GetNumInClass(void)
{
     short i;

     for( i=0; i<MAXCLASSES; i++ ) {
          numinclass[i]=0;
     }
     for( i=0; i<MAXSPRITES; i++ ) {
          if( spriteXT[i].lock == 0xFF ) {
               numinclass[spriteXT[i].class]++;
          }
     }

 return;
}

int
JS_EditSpriteExts(short spritenum)
{
     char      what[50];
     short     j;

     spriteextmodified=1;

     clearmidstatbar16();

     j=sprite[spritenum].extra;

     if ( j < 0 ) {        // .extra is initialized to -1 in build.obj
          printmessage16("No Previous Extension.");
          j=JS_FirstFreeExt();
          if ( j == -1 ) {
               printmessage16("ERROR! Not able to map to an extension.");
               return(-1);
          }
          printmessage16("Extension Mapped.");
          sprite[spritenum].extra=j;
          JS_InitExtension(spritenum, j);
          JS_LockExt(j);
     }
     else {
          printmessage16("Editing Existing Extension.");
     }

     JS_ShowSpriteExts(spritenum);

     sprintf(what, "spriteextra[%d].class     = ", j);
     spriteXT[j].class=getnumber16(what, spriteXT[j].class, 12L, 0);

     sprintf(what, "spriteextra[%d].hitpoints = ", j);
     spriteXT[j].hitpoints=getnumber16(what, spriteXT[j].hitpoints, 256L, 0);

     sprintf(what, "spriteextra[%d].target    = ", j);
     spriteXT[j].target=getnumber16(what, spriteXT[j].target, 4096L, 0);
                                                   
     sprintf(what, "spriteextra[%d].fxmask    = ", j);
     spriteXT[j].fxmask=getnumber16(what, spriteXT[j].fxmask, 256L, 0);

     sprintf(what, "spriteextra[%d].aimask    = ", j);
     spriteXT[j].aimask=getnumber16(what, spriteXT[j].aimask, 256L, 0);

     sprintf(what, "spriteextra[%d].basestat  = ", j);
     spriteXT[j].basestat=getnumber16(what, spriteXT[j].basestat, 4096L, 0);

     sprintf(what, "spriteextra[%d].basepic   = ", j);
     spriteXT[j].basepic=getnumber16(what, spriteXT[j].basepic, 4096L, 0);

     sprintf(what, "spriteextra[%d].standpic = ", j);
     spriteXT[j].standpic=getnumber16(what, spriteXT[j].standpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].walkpic = ", j);
     spriteXT[j].walkpic=getnumber16(what, spriteXT[j].walkpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].runpic = ", j);
     spriteXT[j].runpic=getnumber16(what, spriteXT[j].runpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].attackpic = ", j);
     spriteXT[j].attackpic=getnumber16(what, spriteXT[j].attackpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].deathpic  = ", j);
     spriteXT[j].deathpic=getnumber16(what, spriteXT[j].deathpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].painpic = ", j);
     spriteXT[j].painpic=getnumber16(what, spriteXT[j].painpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].squatpic = ", j);
     spriteXT[j].squatpic=getnumber16(what, spriteXT[j].squatpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].morphpic  = ", j);
     spriteXT[j].morphpic=getnumber16(what, spriteXT[j].morphpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].specialpic  = ", j);
     spriteXT[j].specialpic=getnumber16(what, spriteXT[j].specialpic, 4096L, 0);

     sprintf(what, "spriteextra[%d].weapon  = ", j);
     spriteXT[j].weapon=getnumber16(what, spriteXT[j].weapon, 7L, 0);

     sprintf(what, "spriteextra[%d].ext2  = ", j);
     spriteXT[j].ext2=getnumber16(what, spriteXT[j].ext2, 4096L, 0);

     printmessage16("Sprite Extensions Saved.");

     JS_ShowSpriteExts(spritenum);

 return(0);
}

int
JS_ChangeVelocity(short sn)
{
     char      what[50];

     sprintf(what, "sprite[%d].xvel  = ", sn);
     sprite[sn].xvel=getnumber16(what, sprite[sn].xvel, 10L, 0);

     sprintf(what, "sprite[%d].yvel  = ", sn);          
     sprite[sn].yvel=getnumber16(what, sprite[sn].yvel, 10L, 0);

     sprintf(what, "sprite[%d].zvel  = ", sn);
     sprite[sn].zvel=getnumber16(what, sprite[sn].zvel, 10L, 0);

     printmessage16("Velocities adjusted");

     keystatus[56]=1;
     keystatus[15]=1;

 return(0);
}



//Detecting 2D / 3D mode:
//   qsetmode is 200 in 3D mode
//   qsetmode is 350/480 in 2D mode
//
//You can read these variables when F5-F8 is pressed in 3D mode only:
//
//   If (searchstat == 0)  WALL        searchsector=sector, searchwall=wall
//   If (searchstat == 1)  CEILING     searchsector=sector
//   If (searchstat == 2)  FLOOR       searchsector=sector
//   If (searchstat == 3)  SPRITE      searchsector=sector, searchwall=sprite
//   If (searchstat == 4)  MASKED WALL searchsector=sector, searchwall=wall
//
//   searchsector is the sector of the selected item for all 5 searchstat's
//
//   searchwall is undefined if searchstat is 1 or 2
//   searchwall is the wall if searchstat = 0 or 4
//   searchwall is the sprite if searchstat = 3 (Yeah, I know - it says wall,
//                                      but trust me, it's the sprite number)

int ExtInit(void)
{
	int i, rv = 0;
	unsigned char remapbuf[256];

	buildputs("Tekwar BUILD V1.2\n\n");

#if defined(DATADIR)
    {
        const char *datadir = DATADIR;
        if (datadir && datadir[0]) {
            addsearchpath(datadir);
        }
    }
#endif

     {
          char *supportdir = Bgetsupportdir(1);
          char *appdir = Bgetappdir();
          char dirpath[BMAX_PATH+1];

          // the OSX app bundle, or on Windows the directory where the EXE was launched
          if (appdir) {
               addsearchpath(appdir);
               free(appdir);
          }

          // the global support files directory
          if (supportdir) {
               Bsnprintf(dirpath, sizeof(dirpath), "%s/JFTekWar", supportdir);
               addsearchpath(dirpath);
               free(supportdir);
          }
     }

     // default behaviour is to write to the user profile directory, but
     // creating a 'user_profiles_disabled' file in the current working
     // directory where the game was launched makes the installation
     // "portable" by writing into the working directory
     if (access("user_profiles_disabled", F_OK) == 0) {
          char cwd[BMAX_PATH+1];
          if (getcwd(cwd, sizeof(cwd))) {
               addsearchpath(cwd);
          }
     } else {
          char *supportdir;
          char dirpath[BMAX_PATH+1];
          int asperr;

          if ((supportdir = Bgetsupportdir(0))) {
#if defined(_WIN32) || defined(__APPLE__)
               const char *confdir = "JFTekWar";
#else
               const char *confdir = ".jftekwar";
#endif
               Bsnprintf(dirpath, sizeof(dirpath), "%s/%s", supportdir, confdir);
               asperr = addsearchpath(dirpath);
               if (asperr == -2) {
                    if (Bmkdir(dirpath, S_IRWXU) == 0) {
                         asperr = addsearchpath(dirpath);
                    } else {
                         asperr = -1;
                    }
               }
               if (asperr == 0) {
                    chdir(dirpath);
               }
               free(supportdir);
          }
     }

	initgroupfile("stuff.dat");
	bpp = 8;
	if (loadsetup("build.cfg") < 0) buildputs("Configuration file not found, using defaults.\n"), rv = 1;
	memcpy((void *)buildkeys,(void *)keys,sizeof(buildkeys));   //Trick to make build use setup.dat keys
	if (option[4] > 0) option[4] = 0;
	if (initengine()) {
		wm_msgbox("Build Engine Initialisation Error",
				"There was a problem initialising the Build engine: %s", engineerrstr);
		return -1;
	}
	initinput();
	initmouse();

		//You can load your own palette lookup tables here if you just
		//copy the right code!
	for(i=0;i<256;i++)
		remapbuf[i] = ((i+32)&255);  //remap colors for screwy palette sectors
	makepalookup(16,remapbuf,0,0,0,1);

	kensplayerheight = 34;
	zmode = 0;
	defaultspritecstat = 0;

    parallaxtype=2;
    parallaxyoffs=112;
    pskyoff[0]=0;   
    pskyoff[1]=1;   
    pskyoff[2]=2; 
    pskyoff[3]=3;   
    pskybits=2;     // 4 tiles
    memset(&recXT, 0, sizeof(struct spriteextension));
    recordXT=-1;

	return rv;
}

void ExtUnInit(void)
{
	uninitgroupfile();
	writesetup("build.cfg");
}

void ExtPreCheckKeys(void)
{
}

void ExtCheckKeys(void)
{
	int i, j;

	if (qsetmode == 200)    //In 3D mode
	{
         if( somethingintab == 3 )   // prevent extra value copy
              somethingintab = 254;

		i = frameval[framecnt&(AVERAGEFRAMES-1)];
		j = frameval[framecnt&(AVERAGEFRAMES-1)] = getticks(); framecnt++;
		if (i != j) averagefps = ((mul3(averagefps)+((AVERAGEFRAMES*1000)/(j-i)) )>>2);
		Bsprintf(tempbuf,"%d",averagefps);
		printext256(0L,0L,31,-1,tempbuf,1);
		
		editinput();
	}
	else
	{
         if( keystatus[39] == 1 ) {  // ';' to clear all exts 
              keystatus[39]=0;
              JS_ClearAllExts();
         }
         if( keystatus[26] == 1 ) {  // '[' to clear all exts 
              keystatus[26]=0;
              JS_ClearAllSprites();
         }
         if( keystatus[47] == 1 ) {  // 'V' to edit velocity
              keystatus[47]=0;
              editvelocity=1;
              keystatus[66]=1;
         }
         if( keystatus[40] == 1 ) {  // ''' to cleanup from old versions
              keystatus[40]=0;
              docleanup=1;
              keystatus[66]=1;
         }
         if( keystatus[27] == 1 ) {  // ']' match cstats
              keystatus[27]=0;
              JS_MatchCstats();
         }
         if( keystatus[45] == 1 ) {  // XT Copy
              keystatus[45]=0;
              copyext=1;
              keystatus[64]=1;
         }
         if( keystatus[19] == 1 ) {  // XT Record
              keystatus[19]=0;
              recordext=1;
              keystatus[64]=1;
         }
         if( keystatus[50] == 1 ) {  // 'M' for Manual on sprites
              keystatus[50]=0;
              liststates=1;
              JS_SpriteXTManual();
         }
	}
}

void ExtCleanUp(void)
{
}

void ExtPreLoadMap(void)
{
}

void ExtLoadMap(const char *mapname)
{
    JS_LoadSpriteExts(mapname);   
    wm_setwindowtitle(mapname);
}

void ExtPreSaveMap(void)
{
}

void ExtSaveMap(const char *mapname)
{
    JS_SaveSpriteExts(mapname);  
}

const char *ExtGetSectorCaption(short sectnum)
{
	if ((sector[sectnum].lotag|sector[sectnum].hitag) == 0)
	{
		tempbuf[0] = 0;
	}
	else
	{
          switch((unsigned short)sector[sectnum].lotag) {
          case 1:
               sprintf(lo,"ACTIVATE SECTOR");
               break;
          case 6:
               sprintf(lo,"RISING DOOR");
               break;
          case 7:
               sprintf(lo,"DROP DOOR");
               break;
          case 8:
               sprintf(lo,"HOR SPLIT DOOR");
               break;
          case 9:
               sprintf(lo,"VER SPLIT DOOR");
               break;
          case 1000:
               sprintf(lo,"PLATFORM ELEVATOR");
               break;
          case 1003:
               sprintf(lo,"BOX ELEVATOR");
               break;
          case 1004:
               sprintf(lo,"PLATFORM W/DELAY");
               break;
          case 1005:
               sprintf(lo,"BOX W/DELAY");
               break;
          default:
               sprintf(lo,"%hu",(unsigned short)sector[sectnum].lotag);
               break;
          }
          sprintf(tempbuf,"%hu,%s",(unsigned short)sector[sectnum].hitag,lo);
	}
	return(tempbuf);
}

const char *ExtGetWallCaption(short wallnum)
{
    long i=0;

     if (keystatus[0x57] > 0) {    // f11   Grab pic 0x4e +
          wallpicnum=wall[curwall].picnum;
          sprintf(tempbuf,"Grabed Wall Picnum %d",wallpicnum);
          printmessage16(tempbuf);
     }
     if (keystatus[0x2b] > 0) {    // |
          if (wallsprite == 1) {
               for (i=curwallnum ; i < MAXWALLS ; i++) {
                    if (wall[i].picnum == wall[curwall].picnum) {
                         posx=(wall[i].x)-(((wall[i].x)-(wall[wall[i].point2].x))/2);
                         posy=(wall[i].y)-(( (wall[i].y)-(wall[wall[i].point2].y))/2);
                         printmessage16("Wall Found");
                         curwallnum++;
                         keystatus[0x2b]=0;
                         return(tempbuf);
                    }
                    curwallnum++;
               }
          }
          if (wallsprite == 2) {
               for (i=curspritenum ; i < MAXSPRITES ; i++) {
                    if (sprite[i].picnum == sprite[cursprite].picnum && sprite[i].statnum == 0) {
                         posx=sprite[i].x;
                         posy=sprite[i].y;
                         ang=sprite[i].ang;
                         printmessage16("Sprite Found");
                         curspritenum++;
                         keystatus[0x2b]=0;
                         return(tempbuf);
                    }
                    curspritenum++;
               }
          }
     }
     if ((wall[wallnum].lotag|wall[wallnum].hitag) == 0) {
          tempbuf[0]=0;
     }
     else {
          sprintf(tempbuf,"%hu,%hu",(unsigned short)wall[wallnum].hitag,
										  (unsigned short)wall[wallnum].lotag);
     }
	return(tempbuf);
}

const char *ExtGetSpriteCaption(short spritenum)
{
     tempbuf[0]=0;
     if ((sprite[spritenum].lotag|sprite[spritenum].hitag) == 0) {
          SpriteName(spritenum,lo);
          if (lo[0] != 0) {
               sprintf(tempbuf,"%s",lo);
          }
     }
     else if ((unsigned short)sprite[spritenum].picnum == 104) {
          sprintf(hi,"%hu",(unsigned short)sprite[spritenum].hitag);
          switch((unsigned short)sprite[spritenum].lotag) {
          case 1000:
               sprintf(lo,"PULSING");
               break;
          case 1001:
               sprintf(lo,"FLICKERING");
               break;
          case 1002:
               sprintf(lo,"TIC DELAY");
               break;
          case 1003:
               sprintf(hi,"A=%d",sprite[spritenum].ang);
               sprintf(lo,"PANNING WALLS");
               break;
          case 1004:
               sprintf(hi,"A=%d",sprite[spritenum].ang);
               sprintf(lo,"PANNING FLOORS");
               break;
          case 1005:
               sprintf(hi,"A=%d",sprite[spritenum].ang);
               sprintf(lo,"PANNING CEILINGS");
               break;
          case 1006:
               sprintf(lo,"FLICKER W/DELAY");
               break;
          case 1007:
               sprintf(lo,"BLINK W/DELAY");
               break;
          case 1008:
               sprintf(lo,"STEADY LIGHT");
               break;
          case 1009:
               sprintf(lo,"WARP");
               break;
          default:
               sprintf(lo,"%hu",(unsigned short)sprite[spritenum].lotag);
               break;
          }
          sprintf(tempbuf,"%s,%s",hi,lo);
     }
     else {
          SpriteName(spritenum,lo);
          sprintf(tempbuf,"%hu,%hu %s",(unsigned short)sprite[spritenum].hitag,
                                   (unsigned short)sprite[spritenum].lotag,lo);
     }
	return(tempbuf);
}

//printext16 parameters:
//printext16(long xpos, long ypos, short col, short backcol,
//           char name[82], char fontsize)
//  xpos 0-639   (top left)
//  ypos 0-479   (top left)
//  col 0-15
//  backcol 0-15, -1 is transparent background
//  name
//  fontsize 0=8*8, 1=3*5

//drawline16 parameters:
// drawline16(long x1, long y1, long x2, long y2, char col)
//  x1, x2  0-639
//  y1, y2  0-143  (status bar is 144 high, origin is top-left of STATUS BAR)
//  col     0-15

void ExtShowSectorData(short sectnum)   //F5
{
    int  i,effect;
    int  delay=0,rotating=0,switches=0,totsprites=0;
    char lighting[16],flooreffect[16],walleffect[16],ceileffect[16];
    char *secttype;

     switch (sector[sectnum].lotag) {
     case 0:
          secttype="NORMAL";
          break;
     case 1:
          secttype="ACTIVATE SECTOR";
          break;
     case 6:
          secttype="RISING DOOR";
          break;
     case 7:
          secttype="DROP DOOR";
          break;
     case 8:
          secttype="HOR SPLIT DOOR";
          break;
     case 9:
          secttype="VER SPLIT DOOR";
          break;
     case 14:
          secttype="ROTATING";
          break;
     case 1000:
          secttype="PLATFORM ELEVATOR";
          break;
     case 1003:
          secttype="BOX ELEVATOR";
          break;
     default:
          secttype="UNDEFINED";
          break;
     }
     memset(lighting,0,sizeof(lighting));
     memset(flooreffect,0,sizeof(flooreffect));
     memset(ceileffect,0,sizeof(ceileffect));
     memset(walleffect,0,sizeof(walleffect));
     i=headspritesect[sectnum];
     while (i != -1) {
          if (sprite[i].picnum == 104) {
               effect=sprite[i].lotag-1000;
               switch (effect) {
               case PULSELIGHT:
                    strcat(lighting,"Pulsing ");
                    break;
               case FLICKERLIGHT:
                    strcat(lighting,"Flickering ");
                    break;
               case DELAYEFFECT:
                    delay=sprite[i].hitag;
                    break;
               case WPANNING:
                    sprintf(walleffect,"Panning (A=%d)",sprite[i].ang);
                    break;
               case FPANNING:
                    sprintf(flooreffect,"Panning (A=%d)",sprite[i].ang);
                    break;
               case CPANNING:
                    sprintf(ceileffect,"Panning (A=%d)",sprite[i].ang);
                    break;
               }
          }
          else if (sprite[i].lotag == 3) {
               rotating++;
          }
          else if (sprite[i].lotag == 4) {
               switches++;
          }
          else {
               totsprites++;
          }
          i=nextspritesect[i];
     }
     clearmidstatbar16();
     sprintf(tempbuf,"Level %s",levelname);
     printmessage16(tempbuf);
     sprintf(tempbuf,"Effects for Sector %d (type=%s)",sectnum,secttype);
     printext16(1*8,ydim16+4*8,11,-1,tempbuf,0);
     PrintStatusStr("Lighting Effect =",lighting,2,6,11);
     PrintStatus("Tic Delay       =",delay,2,7,11);
     PrintStatusStr("Wall Effects    =",walleffect,2,8,11);
     PrintStatusStr("Floor Effects   =",flooreffect,2,9,11);
     PrintStatusStr("Ceiling Effects =",ceileffect,2,10,11);
     PrintStatus("Rotating Sprites=",rotating,2,11,11);
     PrintStatus("Switches        =",switches,2,12,11);
     PrintStatus("Total Sprites   =",totsprites,2,13,11);
}

void ExtShowWallData(short wallnum)       //F6
{
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		clearmidstatbar16();             //Clear middle of status bar

		sprintf(tempbuf,"Wall %d",wallnum);
		printext16(8,ydim16+32,11,-1,tempbuf,0);
	}
}

void ExtShowSpriteData(short spritenum)   //F6
{
    if( elapsedtime == 1) {
         elapsedtime=0;
    }
    else if( copyext ) {
         copyext=0;
         JS_CopyExt(spritenum);
    }
    else if( recordext ) {
         recordext=0;
         JS_RecordExt(spritenum);
    }
    else {
         JS_ShowSpriteExts(spritenum);
    }
}

void ExtEditSectorData(short sectnum)    //F7
{
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else                    //In 2D mode
	{
		printmessage16("");              //Clear message box (top right of status bar)
		ExtShowSectorData(sectnum);
	}
}

void ExtEditWallData(short wallnum)       //F8
{
	if (qsetmode == 200)    //In 3D mode
	{
	}
	else
	{
		printmessage16("");              //Clear message box (top right of status bar)
		ExtShowWallData(wallnum);
	}
}

void ExtEditSpriteData(short spritenum)   //F8
{
     cursprite=spritenum;
     curspritenum=0;
     if ( editvelocity == 1 ) {
          JS_ChangeVelocity(spritenum);
          editvelocity=0;
          keystatus[0x42]=0;
          return;
     }
     if ( docleanup == 1 ) {
          JS_CleanupExtensions();
          docleanup=0;
          keystatus[0x42]=0;
          return;
     }
     sprintf(tempbuf,"Current Sprite %d",cursprite);
     printmessage16(tempbuf);
     JS_EditSpriteExts(spritenum);
     keystatus[0x42]=0;
}

void faketimerhandler(void)
{
	sampletimer();
}


void ExtAnalyzeSprites(void)
{
}

