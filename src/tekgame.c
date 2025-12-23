#include "build.h"
#include "names.h"
#include "pragmas.h"
#include "cache1d.h"
#include "baselayer.h"
#include "mmulti.h"
#include "osd.h"
#include "startwin.h"
#include "version.h"

#include "tekwar.h"
#include "tekver.c"

#define   SECT_LOTAG_CLIMB                    5060

#ifdef    TEKWAR
FILE      *dbgfp;
int       dbgfilen;
char      dbgfname[16];
int       dbgflag;
int       dbgcolumn;
short     mousebias=1;
short     biasthreshhold=72;
char      biasthreshholdon=0;
short     lastmousy;
char      keyedhorizon;
//** Les START - 09/26/95
char      jstickenabled=0;
int       jlowx,jlowy,
          jhighx,jhighy;
char      oldjoyb;
//** Les  END  - 09/26/95
short     yaw,pitch,roll,vrangle,vrpitch;
int       mouselookmode,mouselook;

#endif

int vel, svel, angvel, horizvel;
int vel2, svel2, angvel2, horizvel2;

int recording = -2;

#ifdef    TEKWAR
int xdimgame = 640, ydimgame = 480, bppgame = 8;
int forcesetup = 1;

#define   MAXMOREOPTIONS      21
#define   MAXTOGGLES          16
#define   MAXGAMESTUFF        16
unsigned char option[NUMOPTIONS] = {
      1,       // 0  VIDEO MODE CHAINED OR NO
      0,       // 1  SOUND CHOICE
      0,       // 2  MUSIC CHOICE
      1,       // 3  MOUSE ON/OFF, +2 JOYSTICK ON/OFF
      0,       // 4  MULTIPLAYER COUNT
      0,       // 5  MULTIPLYER SETTING
      2,       // 6  VIDEO RES CHOICE
      64+4+2   // 7  SOUND FREQ (22kHz 16bit stereo)
};
int keys[NUMKEYS];
const int defaultkeys[NUMKEYS] = {
     200,         // 0  FWD
     208,         // 1  BKWD
     203,         // 2  RIGHT
     205,         // 3  LEFT
      42,         // 4  RUN / AMPLIFY
      56,         // 5  STRAFE
      29,         // 6  SHOOT
      57,         // 7  USE
      45,         // 8  JUMP
      46,         // 9  CROUCH
     201,         // 10 LOOK UP
     209,         // 11 LOOK DOWN
      51,         // 12 SLIDE LEFT
      52,         // 13 SLIDE RIGHT
      15,         // 14 MAP MODE
     156,         // 15 SWITCH PLAYER
      13,         // 16 EXPAND VIEW
      12,         // 17 SHRINK VIEW
      50,         // 18 MESSAGE MODE
     199,         // 19 AUTOCENTER
      19,         // 20 TOGGLE REARVIEW
      18,         // 21 TOGGLE PREPARED ITEM
      35,         // 22 TOGGLE HEALTH METER
      34,         // 23 TOGGLE CROSSHAIRS
      20,         // 24 TOGGLE ELAPSED TIME
      31,         // 25 TOGGLE SCORE
      23,         // 26 TOGGLE INVENTORY
      53,         // 27 CONCEAL WEAPON
      58,         // 28 MOUSE LOOKUP/DOWN
      69,         // 29 CONSOLE
};
const int modernkeys[NUMKEYS] = {
      17,         // 0  FWD (W)
      31,         // 1  BKWD (S)
     203,         // 2  RIGHT
     205,         // 3  LEFT
      42,         // 4  RUN / AMPLIFY
     184,         // 5  STRAFE (RAlt)
      29,         // 6  SHOOT
      18,         // 7  USE (E)
      57,         // 8  JUMP (Space)
      56,         // 9  CROUCH (LAlt)
     201,         // 10 LOOK UP
     209,         // 11 LOOK DOWN
      30,         // 12 SLIDE LEFT (E)
      32,         // 13 SLIDE RIGHT (D)
      15,         // 14 MAP MODE
     156,         // 15 SWITCH PLAYER
      13,         // 16 EXPAND VIEW
      12,         // 17 SHRINK VIEW
      50,         // 18 MESSAGE MODE
     199,         // 19 AUTOCENTER
      19,         // 20 TOGGLE REARVIEW
      16,         // 21 TOGGLE PREPARED ITEM (Q)
      35,         // 22 TOGGLE HEALTH METER
      34,         // 23 TOGGLE CROSSHAIRS
      20,         // 24 TOGGLE ELAPSED TIME
      46,         // 25 TOGGLE SCORE (C)
      23,         // 26 TOGGLE INVENTORY
      53,         // 27 CONCEAL WEAPON
      22,         // 28 MOUSE LOOKUP/DOWN (U)
      69,         // 29 CONSOLE
};
int moreoptions[MAXMOREOPTIONS] = {
        0,     // 0  N/U
        6,     // 1  MOUSE BUTTON 1 MAP
        7,     // 2  MOUSE BUTTON 2 MAP
        0,     // 3  N/U
        4,     // 4  JOYSTICK BUTTON 1 MAP
        6,     // 5  JOYSTICK BUTTON 2 MAP
       10,     // 6  JOYSTICK BUTTON 3 MAP
       11,     // 7  JOYSTICK BUTTON 4 MAP
        1,     // 8  DIFFICULTY LEVEL
       16,     // 9  SOUND VOLUME
       16,     // 10 MUSIC VOLUME
        8,     // 11 MOUSE SENSITIVITY
        1,     // 12 HEAD BOB
        0,     // 13 N/U
        0,     // 14 N/U
        0      // 15 N/U
};
char toggles[MAXTOGGLES] = { 1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0 };
int  gamestuff[MAXGAMESTUFF] = {
     -1,       // 0  joyxcenter
     -1,       // 1  joyycenter
      0,       // 2  screensize
      0,       // 3  brightness
    100,       // 4  biasthreshhold
      0,       // 5  warpretang
      0,       // 6  warpretsect
      0,       // 7
      0,       // 8
      0,       // 9
      0,       // 10
      0,       // 11
      0,       // 12
      0,       // 13
      0,       // 14
      0        // 15
};
int digihz[8] = {6000,8000,11025,16000,22050,32000,44100,48000};
#endif

char frame2draw[MAXPLAYERS];
int frameskipcnt[MAXPLAYERS];

char gundmost[320];

     //Shared player variables
int posx[MAXPLAYERS], posy[MAXPLAYERS], posz[MAXPLAYERS];
int horiz[MAXPLAYERS], zoom[MAXPLAYERS], hvel[MAXPLAYERS];
short ang[MAXPLAYERS], cursectnum[MAXPLAYERS], ocursectnum[MAXPLAYERS];
short playersprite[MAXPLAYERS], deaths[MAXPLAYERS];
int lastchaingun[MAXPLAYERS];
int health[MAXPLAYERS], score[MAXPLAYERS], saywatchit[MAXPLAYERS];
short numbombs[MAXPLAYERS], oflags[MAXPLAYERS];
char dimensionmode[MAXPLAYERS];
char revolvedoorstat[MAXPLAYERS];
short revolvedoorang[MAXPLAYERS], revolvedoorrotang[MAXPLAYERS];
int revolvedoorx[MAXPLAYERS], revolvedoory[MAXPLAYERS];

     //Local multiplayer variables
int locselectedgun;
signed char locvel, olocvel;
short locsvel, olocsvel;                          // Les 09/27/95
short locangvel, olocangvel;                      // Les 09/27/95
signed char lochorizvel, olochorizvel;
short locbits, olocbits;

     //Local multiplayer variables for second player
int locselectedgun2;
signed char locvel2, olocvel2;
short locsvel2, olocsvel2;                        // Les 09/27/95
short locangvel2, olocangvel2;                    // Les 09/27/95
signed char lochorizvel2, olochorizvel2;
short locbits2, olocbits2;

  //Multiplayer syncing variables
signed char fsyncvel[MAXPLAYERS], osyncvel[MAXPLAYERS], syncvel[MAXPLAYERS];
short fsyncsvel[MAXPLAYERS], osyncsvel[MAXPLAYERS], syncsvel[MAXPLAYERS];       // Les 09/27/95
short fsyncangvel[MAXPLAYERS], osyncangvel[MAXPLAYERS], syncangvel[MAXPLAYERS]; // Les 09/27/95
signed char fsynchorizvel[MAXPLAYERS], osynchorizvel[MAXPLAYERS], synchorizvel[MAXPLAYERS];
unsigned short fsyncbits[MAXPLAYERS], osyncbits[MAXPLAYERS], syncbits[MAXPLAYERS];

char frameinterpolate = 1, ready2send = 0;
int ototalclock = 0, gotlastpacketclock = 0, smoothratio;
int oposx[MAXPLAYERS], oposy[MAXPLAYERS], oposz[MAXPLAYERS];
int ohoriz[MAXPLAYERS], ozoom[MAXPLAYERS];
short oang[MAXPLAYERS];

point3d osprite[MAXSPRITESONSCREEN];

int movefifoplc, movefifoend;
signed char baksyncvel[MOVEFIFOSIZ][MAXPLAYERS];
short baksyncsvel[MOVEFIFOSIZ][MAXPLAYERS];       // Les 09/27/95
short baksyncangvel[MOVEFIFOSIZ][MAXPLAYERS];     // Les 09/27/95
signed char baksynchorizvel[MOVEFIFOSIZ][MAXPLAYERS];
short baksyncbits[MOVEFIFOSIZ][MAXPLAYERS];

     //GAME.C sync state variables
short syncstat = 0;
int syncvalplc, othersyncvalplc;
int syncvalend, othersyncvalend;
int syncvalcnt, othersyncvalcnt;
short syncval[MOVEFIFOSIZ], othersyncval[MOVEFIFOSIZ];

extern int crctable[256];
char playerreadyflag[MAXPLAYERS];

     //Game recording variables
int reccnt, recstat = 1;
signed char recsyncvel[16384][2];
short recsyncsvel[16384][2];                      // Les 09/27/95
short recsyncangvel[16384][2];                    // Les 09/27/95
signed char recsynchorizvel[16384][2];
short recsyncbits[16384][2];

     //Miscellaneous variables
unsigned char tempbuf[max(576,MAXXDIM)];
char boardfilename[80];
short screenpeek = 0, oldmousebstatus = 0, brightness = 0;
short screensize, screensizeflag = 0;
short neartagsector, neartagwall, neartagsprite;
int lockclock, neartagdist, neartaghitdist;
int masterslavetexttime;
int globhiz, globloz, globhihit, globlohit;

     //Board animation variables
short rotatespritelist[16], rotatespritecnt;
short warpsectorlist[64], warpsectorcnt;
short xpanningsectorlist[16], xpanningsectorcnt;
short ypanningwalllist[64], ypanningwallcnt;
short floorpanninglist[64], floorpanningcnt;
short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
int dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];
short swingcnt, swingwall[32][5], swingsector[32];
short swingangopen[32], swingangclosed[32], swingangopendir[32];
short swingang[32], swinganginc[32];
int swingx[32][8], swingy[32][8];
short revolvesector[4], revolveang[4], revolvecnt;
int revolvex[4][16], revolvey[4][16];
int revolvepivotx[4], revolvepivoty[4];
short subwaytracksector[4][128], subwaynumsectors[4], subwaytrackcnt;
int subwaystop[4][8], subwaystopcnt[4];
int subwaytrackx1[4], subwaytracky1[4];
int subwaytrackx2[4], subwaytracky2[4];
int subwayx[4], subwaygoalstop[4], subwayvel[4], subwaypausetime[4];
short waterfountainwall[MAXPLAYERS], waterfountaincnt[MAXPLAYERS];
short slimesoundcnt[MAXPLAYERS];

     //Variables that let you type messages to other player
char getmessage[162], getmessageleng;
int getmessagetimeoff;
char typemessage[162];
int typemessageleng = 0, typemode = 0;
char scantoasc[128] =
{
     0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
     'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
     'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
     'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
     0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
     '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
char scantoascwithshift[128] =
{
     0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
     'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
     'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
     'B','N','M','<','>','?',0,'*',0,32,0,0,0,0,0,0,
     0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
     '2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

     //These variables are for animating x, y, or z-coordinates of sectors,
     //walls, or sprites (They are NOT to be used for changing the [].picnum's)
     //See the setanimation(), and getanimategoal() functions for more details.
int *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
int animatevel[MAXANIMATES], animateacc[MAXANIMATES], animatecnt = 0;

#define AVERAGEFRAMES 32
static unsigned int showfps, averagefps, framecnt, frameval[AVERAGEFRAMES];

static int osdcmd_showfps(const osdfuncparm_t *parm);
static int osdcmd_vidmode(const osdfuncparm_t *parm);
static int osdcmd_setkeys(const osdfuncparm_t *parm);
static int osdcmd_setting(const osdfuncparm_t *parm);

void
debugout(short p)
{
     static int dbglines;

     if (dbgcolumn != 0) {
          fprintf(dbgfp,"\n");
     }
     fprintf(dbgfp,"%2d %6d %3d %04X %04d %06d %06d %06d %06d %d\n",
                    p,lockclock,movefifoplc,syncbits[p],ang[p],posx[p],posy[p],posz[p],
                    health[p],randomseed);
     dbglines++;
     dbgcolumn=0;
     if (dbglines > 2000) {
          dbglines=0;
          fclose(dbgfp);
          sprintf(dbgfname,"DEBUG.%03d",dbgfilen++);
          dbgfp=fopen(dbgfname,"wt");
          fprintf(dbgfp," P  CLOCK PLC BITS  ANG   X     Y     Z   HEALTH RSEED\n");
          fprintf(dbgfp,"== ====== === ==== ==== ===== ===== ===== ====== =========\n");
     }
}


char      localname[MAXNAMESIZE];
char      netnames[MAXPLAYERS][MAXNAMESIZE];

#if defined RENDERTYPEWIN || (defined RENDERTYPESDL && (defined __APPLE__ || defined HAVE_GTK))
# define HAVE_STARTWIN
#endif

int
app_main(int argc, char const * const argv[])
{
     int      i, waitplayers;
     int     other;

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

     tekargv(argc, argv);

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

     buildsetlogfile("tekwar.log");

     wm_setapptitle("JFTekWar");
     buildprintf("\nJFTekWar\n"
          "Based on " TITLE "\n"
          "Version %s.\nBuilt %s %s.\n",
        VERS, game_version, game_date, game_time);

     if (preinitengine()) {
          wm_msgbox("Build Engine Initialisation Error",
               "There was a problem initialising the Build engine: %s", engineerrstr);
          exit(1);
     }

     lm("tekloadsetup");
     tekloadsetup();

#ifdef HAVE_STARTWIN
    {
        struct startwin_settings settings;

        memset(&settings, 0, sizeof(settings));
        settings.fullscreen = fullscreen;
        settings.xdim3d = xdimgame;
        settings.ydim3d = ydimgame;
        settings.bpp3d = bppgame;
        settings.forcesetup = forcesetup;
        settings.usemouse = !!(option[3]&1);
        settings.usejoy = !!(option[3]&2);
        settings.samplerate = digihz[option[7]>>4];
        settings.bitspersample = 1<<(((option[7]&2)>0)+3);
        settings.channels = ((option[7]&4)>0)+1;

        if (forcesetup) {
            if (startwin_run(&settings) == STARTWIN_CANCEL) {
                uninitengine();
                exit(0);
            }
        }

        fullscreen = settings.fullscreen;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
        option[3] = (option[3]&~1)|(settings.usemouse);
        option[3] = (option[3]&~2)|(settings.usejoy<<1);
        option[7] = 0;
        for (i=0;i<8;i++) if (digihz[i] <= settings.samplerate) option[7] = i<<4;
        option[7] |= (settings.bitspersample == 16)<<1;
        option[7] |= (settings.channels == 2)<<2;
    }
#endif

     lm("initgroupfile");
     initgroupfile("stuff.dat");

     if (initengine()) {
          wm_msgbox("Build Engine Initialisation Error",
               "There was a problem initialising the Build engine: %s", engineerrstr);
          exit(1);
     }

     lm("inittimer");
     inittimer(CLKIPS, NULL);
     lm("tekinitmultiplayers");
     tekinitmultiplayers(0, NULL);
     lm("loadpics");
     loadpics("tiles000.art", 8*1048576);
     lm("tekpreinit");
     tekpreinit();
     lm("tekgamestarted");
     tekgamestarted();
     lm("initinput");
     initinput();
     lm("initmouse");
     if( option[3] != 0 ) initmouse();

     if (dbgflag) {
          lm("debug mode: ON");
          sprintf(dbgfname,"DEBUG.%03d",dbgfilen++);
          dbgfp=fopen(dbgfname,"wt");
          fprintf(dbgfp," P  CLOCK PLC BITS  ANG   X     Y     Z   HEALTH RSEED\n");
          fprintf(dbgfp,"== ====== === ==== ==== ===== ===== ===== ====== =========\n");
          dbgcolumn=0;
     }

     setgamemode(fullscreen, xdimgame, ydimgame, bppgame);

     lm("initsb");
     initsb(option[1],option[2],digihz[option[7]>>4],((option[7]&4)>0)+1,((option[7]&2)>0)+1,0,0);

     OSD_RegisterFunction("showfps","showfps [state]: show frame rate", osdcmd_showfps);
     OSD_RegisterFunction("restartvid","restartvid: reinitialise the video mode",osdcmd_vidmode);
     OSD_RegisterFunction("vidmode","vidmode [xdim ydim] [bpp] [fullscreen]: immediately change the video mode",osdcmd_vidmode);
     OSD_RegisterFunction("setkeys","setkeys [modern|default]: set keys to modern (WASD) or default (arrows)", osdcmd_setkeys);
     OSD_RegisterFunction("mouselookmode","mouselookmode [onf]: set mouse look mode (0 momentary, 1 toggle)", osdcmd_setting);
     OSD_RegisterFunction("mousespeed","mousespeed [x] [y]: set mouse sensitivity (range 1 to 16)", osdcmd_setting);
     OSD_RegisterFunction("mousebutton","mousebutton [button] [action]: set mouse button actions (use 'mousebutton list' to show actions)", osdcmd_setting);
     OSD_RegisterFunction("joystickbutton","joystickbutton [button] [action]: set joystick button actions (use 'joystickbutton list' to show actions)", osdcmd_setting);
     OSD_RegisterFunction("keybutton","keybutton [scancode] [action]: set an action to a key scancode (use 'keybutton list' to show actions)", osdcmd_setting);

     if( option[4] > 0 ) {
        lm("multiplayer init");
        teknetpickmap();
          sendlogon();
          if( option[4] < 5 ) {
               waitplayers=2;
          }
          else {
               waitplayers=option[4]-3;
          }
          while( numplayers < waitplayers ) {
               clearview(0);
               overwritesprite((xdim>>1)-160,0,408,0,0,0);
               sprintf((char *)tempbuf,"  MULTIPLAYER MODE  ");
               printext((xdim>>1)-80,(ydim>>1)-24,(char *)tempbuf,ALPHABET2,0);
               sprintf((char *)tempbuf,"%2d OF %2d PLAYERS IN",numplayers,waitplayers);
               printext((xdim>>1)-80,ydim>>1,(char *)tempbuf,ALPHABET2,0);
               nextpage();
               if( getpacket(&other,tempbuf) > 0 ) {
                    if( tempbuf[0] == 255 ) {
                         keystatus[1] = 1;
                    }
               }
               if( keystatus[1] > 0 ) {
                    goto gameends;
               }
          }
          screenpeek = myconnectindex;
          clearview(0);
     }
     for( i=connecthead ; i >= 0 ; i=connectpoint2[i] ) {
          initplayersprite((short)i);
     }

     if( option[4] == 0 ) {
          i = smkplayseq("tvopen");
          if (i < 1) i = smkplayseq("tekndie");
          if (i < 1) i = smkplayseq("tekintro");
          smkplayseq("intro");
     }

missionselection:
     if( option[4] == 0 ) {
          if( choosemission() == 0 ) {
               goto gameends;
          }
     }

     reccnt=0;
     movefifoplc = 0; movefifoend = 0;
     syncvalplc = 0; othersyncvalplc = 0;
     syncvalend = 0; othersyncvalend = 0;
     syncvalcnt = 0L; othersyncvalcnt = 0L;
     olocvel = 0; olocvel2 = 0;
     olocsvel = 0; olocsvel2 = 0;
     olocangvel = 0; olocangvel2 = 0;
     olochorizvel = 0; olochorizvel2 = 0;
     olocbits = 0; olocbits2 = 0;
     lockclock = 0;
     ototalclock = 0;
     gotlastpacketclock = 0;
     masterslavetexttime = 0;
     for( i=0; i<MAXPLAYERS; i++ ) {
          fsyncvel[i] = syncvel[i] = osyncvel[i] = 0;
          fsyncsvel[i] = syncsvel[i] = osyncsvel[i] = 0;
          fsyncangvel[i] = syncangvel[i] = osyncangvel[i] = 0;
          fsynchorizvel[i] = synchorizvel[i] = osynchorizvel[i] = 0;
          fsyncbits[i] = syncbits[i] = osyncbits[i] = 0;
     }
     resettiming();

     ready2send = 1;
    #ifdef NETNAMES
     if( option[4] != 0 ) {
          tempbuf[0]=8;
          tempbuf[1]=myconnectindex;
          memcpy(&tempbuf[2],localname,10);
          tempbuf[12]=0;
          for( i=connecthead; i>=0; i=connectpoint2[i] ) {
               if( i != myconnectindex ) {
                    sendpacket(i,tempbuf,12);
               }
          }
          memcpy(netnames[myconnectindex],localname,10);
          netnames[myconnectindex][10]=0;
     }
    #endif
     screenpeek=myconnectindex;
     while( !gameover ) {
        handleevents();
        OSD_DispatchQueued();
          while( movefifoplc != movefifoend ) {
               domovethings();
          }
          if( activemenu != 0 ) {
               domenuinput();
          }
          drawscreen(screenpeek,(totalclock-gotlastpacketclock)*(65536/TICSPERFRAME));
     }
     ready2send = 0;

     if( option[4] == 0 ) {
          debrief=1;
          goto missionselection;
     }

gameends:

     copyrightscreen();

     sendlogoff();
     uninitmultiplayers();
     uninitsb();
     cduninit();
     uninittimer();
     uninitinput();
     uninitengine();
     uninitgroupfile();

     teksavesetup();

     if (dbgflag) {
          fclose(dbgfp);
     }

     exit(0);
}

void
processinput(short snum)
{
     int      nexti;
     int      i,j, doubvel, xvect, yvect, goalz;

     // move player snum
     if (snum < 0 || snum >= MAXPLAYERS) {
          crashgame("game712: Invalid player number (%d)",snum);
     }

     if (cursectnum[snum] < 0 || cursectnum[snum] >= numsectors) {
          crashgame("game718: Invalid sector for player %d @ %ld,%ld (%d)",snum,
               posx[snum],posy[snum],cursectnum[snum]);
     }

     if( (syncvel[snum]|syncsvel[snum]) != 0 ) {
          // no run while crouching
          if( ((syncbits[snum]&2) != 0) && (mission != 7) ) {
               doubvel = 1+((syncbits[snum]&256) == 0);
          }
          else {
               doubvel = (TICSPERFRAME<<((syncbits[snum]&256)>0));
               doubvel<<=1;
          }
          xvect = 0, yvect = 0;
          if( syncvel[snum] != 0 ) {
               xvect += ((((int)syncvel[snum])*doubvel*(int)sintable[(ang[snum]+2560)&2047])>>3);
               yvect += ((((int)syncvel[snum])*doubvel*(int)sintable[(ang[snum]+2048)&2047])>>3);
          }
          if( syncsvel[snum] != 0 ) {
               xvect += ((((int)syncsvel[snum])*doubvel*(int)sintable[(ang[snum]+2048)&2047])>>3);
               yvect += ((((int)syncsvel[snum])*doubvel*(int)sintable[(ang[snum]+1536)&2047])>>3);
          }
          clipmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],xvect,yvect,128L,4<<8,4<<8,CLIPMASK0);
          frameinterpolate = 1;
          revolvedoorstat[snum] = 1;
          if( option[4] == 0 ) {
               tekheadbob();
          }
     }
     else {
          revolvedoorstat[snum] = 0;
          headbob=0;
     }

     // push player away from walls if clipmove doesn't work
     if( pushmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],128L,4<<8,4<<8,CLIPMASK0) < 0 ) {
          changehealth(snum,-1000);  // if this fails then instant death
          changescore(snum,-5);
     }

     if (cursectnum[snum] < 0 || cursectnum[snum] >= numsectors) {
          crashgame("game748: Invalid sector for player %d @ %ld,%ld (%d)",snum,
               posx[snum],posy[snum],cursectnum[snum]);
     }
     if (playersprite[snum] < 0 || playersprite[snum] >= MAXSPRITES) {
          crashgame("game751: Invalid sprite for player %d (%d)",snum,playersprite[snum]);
     }

     // getzrange returns the highest and lowest z's for an entire box,
     // NOT just a point.  This prevents you from falling off cliffs
     // when you step only slightly over the cliff.
     sprite[playersprite[snum]].cstat ^= 1;
     getzrange(posx[snum],posy[snum],posz[snum],cursectnum[snum],&globhiz,&globhihit,&globloz,&globlohit,128L,CLIPMASK0);
     sprite[playersprite[snum]].cstat ^= 1;

     if( cursectnum[snum] != ocursectnum[snum] ) {
          teknewsector(snum);
     }

     // ang += angvel*constant, engine calculates angvel
     if( syncangvel[snum] != 0 ) {
          doubvel = TICSPERFRAME;
          // if run key then turn faster
          if( (syncbits[snum]&256) > 0 ) {
               doubvel += (TICSPERFRAME>>1);
          }
          ang[snum] += ((((int)syncangvel[snum])*doubvel)>>4);
          ang[snum] = (ang[snum]+2048)&2047;
     }

     if( health[snum] < 0 ) {
          health[snum] -= (TICSPERFRAME<<1);
          if( health[snum] <= -160 ) {
               hvel[snum] = 0;
               if( snum == myconnectindex ) {
                    vel = 0, svel = 0, angvel = 0, horizvel = 0, keystatus[3] = 1;
               }
               deaths[snum]++;
               if( (option[4] == 0) && (numplayers == 1) ) {
                    fadeout(0,255,32,0,0,100);
               }
               if( option[4] != 0 ) {
                    netstartspot(&posx[snum],&posy[snum],&cursectnum[snum]);
                    if (cursectnum[snum] < 0 || cursectnum[snum] >= numsectors) {
                         crashgame("game818: Invalid sector for player %d (%d)",snum,cursectnum[snum]);
                    }
                    placerandompic(KLIPPIC);
                    placerandompic(MEDICKITPIC);
                    posz[snum] = sector[cursectnum[snum]].floorz-(1<<8);
                    ang[snum] = (krand_intercept("GAME 802")&2047);
               }
               else {
                    posx[snum]=startx;
                    posy[snum]=starty;
                    posz[snum]=startz;
                    ang[snum]=starta;
                    cursectnum[snum]=starts;
               }

               tekrestoreplayer(snum);

               if( (option[4] == 0) && (missionfailed() == 0) ) {
                    newgame(boardfilename);
                    keystatus[2]=1;
                    dofadein=32;
               }
               else {
                    // some sort of die anim here for multiplayer ?
               }
          }
#if 0                                                       // Les 10/01/95
          else {
               sprite[playersprite[snum]].xrepeat = max(((128+health[snum])>>1),0);
               sprite[playersprite[snum]].yrepeat = max(((128+health[snum])>>1),0);

               hvel[snum] += (TICSPERFRAME<<2);
               horiz[snum] = max(horiz[snum]-4,0);
               posz[snum] += hvel[snum];
               if( posz[snum] > globloz-(4<<8) ) {
                    posz[snum] = globloz-(4<<8);
                    horiz[snum] = min(horiz[snum]+5,200);
                    hvel[snum] = 0;
               }
          }
#endif                                                      // Les 10/01/95
     }
     if( (syncbits[snum]&64) != 0 ) {
          autocenter[snum]=1;
     }
     if( synchorizvel[snum] != 0 ) {
          horiz[snum] += synchorizvel[snum];
          if(horiz[snum] < 0) horiz[snum]=0;
          else if(horiz[snum] > 200) horiz[snum]=200;
          autocenter[snum]=0;
     }
     if( autocenter[snum] ) {
          if( horiz[snum] > 100 ) {
               horiz[snum]-=4;
               if( horiz[snum] < 100 ) {
                    horiz[snum]=100;
               }
          }
          else if( horiz[snum] < 100 ) {
               horiz[snum]+=4;
               if( horiz[snum] > 100 ) {
                    horiz[snum]=100;
               }
          }
          if( horiz[snum] == 100 ) {
               autocenter[snum]=0;
          }
     }

     // if (((syncbits[snum]&8) > 0) && (horiz[snum] > 100-(200>>1))) horiz[snum] -= 4;     //-
     // if (((syncbits[snum]&4) > 0) && (horiz[snum] < 100+(200>>1))) horiz[snum] += 4;   //+

     // 32 pixels above floor is where player should be
     goalz = globloz-(KENSPLAYERHEIGHT<<8);

     // kens slime sector
     if( sector[cursectnum[snum]].lotag == 4 ) {
          // if not on a sprite
          if( (globlohit&0xc000) != 49152 ) {
               goalz = globloz-(8<<8);
               if( posz[snum] >= goalz-(2<<8) ) {
                    clipmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],-(TICSPERFRAME<<14),-(TICSPERFRAME<<14),128L,4<<8,4<<8,CLIPMASK0);
                    frameinterpolate = 0;
                    if( slimesoundcnt[snum] >= 0 ) {
                         slimesoundcnt[snum] -= TICSPERFRAME;
                         while( slimesoundcnt[snum] < 0 ) {
                              slimesoundcnt[snum] += 120;
                         }
                    }
               }
          }
     }

     // case where ceiling & floor are too close
     if( goalz < globhiz+(16<<8) ) {
          goalz = ((globloz+globhiz)>>1);
     }

     // climb ladder or regular z movement
     if( (mission == 7) || sector[cursectnum[snum]].lotag == SECT_LOTAG_CLIMB ) {
          if( (syncbits[snum]&1) > 0 ) {
               if( posz[snum] > (sector[cursectnum[snum]].ceilingz+2048) ) {
                    posz[snum]-=64;
                    if( (syncbits[snum]&256) > 0 ) {
                         posz[snum]-=128;
                    }
                    if( mission == 7 ) {
                         posz[snum]-=256;
                    }
               }
          }
          else if( (syncbits[snum]&2) > 0 ) {
               if( posz[snum] < (sector[cursectnum[snum]].floorz-2048) ) {
                    posz[snum]+=64;
                    if( (syncbits[snum]&256) > 0 ) {
                         posz[snum]+=128;
                    }
                    if( mission == 7 ) {
                         posz[snum]+=256;
                    }
               }
          }
     }
     else {
          if( health[snum] >= 0 ) {
               // jump key
               if( (syncbits[snum]&1) > 0 ) {
                    if( posz[snum] >= globloz-(KENSPLAYERHEIGHT<<8) ) {
                    goalz -= (16<<8);
//                    if( (syncbits[snum]&256) > 0 ) {
                              goalz -= (24<<8);
//                         }
                    }
               }
               // crouch key
               if( (syncbits[snum]&2) > 0 ) {
                goalz += (12<<8);
//                if( (syncbits[snum]&256) > 0 ) {
                         goalz += (12<<8);
//                    }
               }
          }
          // player is on a groudraw area
         /*jonof: groudraw became slopes
          if( (sector[cursectnum[snum]].floorstat&2) > 0 ) {
               if( waloff[sector[cursectnum[snum]].floorheinum] == 0 ) {
                    loadtile(sector[cursectnum[snum]].floorheinum);
               }
               ptr = (char *)(waloff[sector[cursectnum[snum]].floorheinum]+(((posx[snum]>>4)&63)<<6)+((posy[snum]>>4)&63));
               goalz -= ((*ptr)<<8);
          }
         */
          // gravity, plus check for if on an elevator
          if( posz[snum] < goalz ) {
               hvel[snum] += (TICSPERFRAME<<5)+1;
          }
          else {
               if( (globlohit&0xC000) == 0xC000 ) {     // on a sprite
                    if ((globlohit-49152) < 0 || (globlohit-49152) >= MAXSPRITES) {
                         crashgame("game961: Invalid sprite index (%d)",globlohit-49152);
                    }
                    if( sprite[globlohit-49152].lotag >= 1500 ) {
                         onelev[snum]=1;
                    }
               }
               else if( sector[cursectnum[snum]].lotag == 1004 ||
                        sector[cursectnum[snum]].lotag == 1005 ) {
                    onelev[snum]=1;
               }
               else {
                    onelev[snum]=0;
               }
               if (onelev[snum] != 0 && (syncbits[snum]&2) == 0) {
                    hvel[snum]=0;
                    posz[snum]=globloz-(KENSPLAYERHEIGHT<<8);
               }
               else {
                    hvel[snum] = (((goalz-posz[snum])*TICSPERFRAME)>>5);
               }
          }
          tekchangefallz(snum,globloz,globhiz);
     }

     // overhead maps zoom in/out
     if( dimensionmode[snum] != 3 ) {
          if (((syncbits[snum]&32) > 0) && (zoom[snum] > 48)) zoom[snum] -= (zoom[snum]>>4);
          if (((syncbits[snum]&16) > 0) && (zoom[snum] < 4096)) zoom[snum] += (zoom[snum]>>4);
     }

     // update sprite representation of player
     // should be after movement, but before shooting code
     setsprite(playersprite[snum],posx[snum],posy[snum],posz[snum]+(KENSPLAYERHEIGHT<<8));
     sprite[playersprite[snum]].ang = ang[snum];

     // in wrong sector or is ceiling/floor smooshing player
     if( (cursectnum[snum] < 0) || (cursectnum[snum] >= numsectors) ) {
          changehealth(snum,-200);
          changescore(snum,-5);
     }
     else if( globhiz+(8<<8) > globloz ) {
          changehealth(snum,-200);
          changescore(snum,-5);
     }

     // kens waterfountain
     if( (waterfountainwall[snum] >= 0) && (health[snum] >= 0) ) {
          if (neartagwall < 0 || neartagwall >= numwalls) {
               crashgame("game1009: Invalid wall (%d)",neartagwall);
          }
          if( (wall[neartagwall].lotag != 7) || ((syncbits[snum]&1024) == 0) ) {
               i = waterfountainwall[snum];
               if (i < 0 || i >= numwalls) {
                    crashgame("game1014: Invalid wall index (%d)",i);
               }
               if( wall[i].overpicnum == USEWATERFOUNTAIN ) {
                    wall[i].overpicnum = WATERFOUNTAIN;
               }
               else if( wall[i].picnum == USEWATERFOUNTAIN ) {
                    wall[i].picnum = WATERFOUNTAIN;
               }
               waterfountainwall[snum] = -1;
          }
     }

     // enter throw
     if( (option[4] == 0 ) && (pickup.picnum != 0) && (keystatus[28] != 0) ) {
          toss(snum);
          keystatus[28]=0;
     }

     // space bar (use) code
     if( ((syncbits[snum]&1024) > 0) && (sector[cursectnum[snum]].lotag == 4444) ) {
          depositsymbol(snum);
     }
     else if( (syncbits[snum]&1024) > 0 ) {
          // continuous triggers
          neartag(posx[snum],posy[snum],(posz[snum]+(8<<8)),cursectnum[snum],ang[snum],&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1024L,3);
          if( neartagsector == -1 ) {
               i = cursectnum[snum];
               if( (sector[i].lotag|sector[i].hitag) != 0 ) {
                    neartagsector = i;
               }
          }
          // kens water fountain
          if( neartagwall >= 0 ) {
              if (neartagwall >= numwalls) {
                    crashgame("game1053: Invalid wall index (%d)",neartagwall);
               }
               if ( wall[neartagwall].lotag == 7 ) {
                    if( wall[neartagwall].overpicnum == WATERFOUNTAIN ) {
                         wall[neartagwall].overpicnum = USEWATERFOUNTAIN;
                         waterfountainwall[snum] = neartagwall;
                    }
                    else if( wall[neartagwall].picnum == WATERFOUNTAIN ) {
                         wall[neartagwall].picnum = USEWATERFOUNTAIN;
                         waterfountainwall[snum] = neartagwall;
                    }
                    if( waterfountainwall[snum] >= 0 ) {
                         waterfountaincnt[snum] -= TICSPERFRAME;
                         while( waterfountaincnt[snum] < 0 ) {
                              waterfountaincnt[snum] += 120;
                              changehealth(snum,2);
                         }
                    }
               }
          }
          // 1-time triggers
          if( (oflags[snum]&1024) == 0 ) {
               if( neartagsector >= 0 ) {
                    if (neartagsector >= numsectors) {
                         crashgame("game1070: Invalid sector index (%d)",neartagsector);
                    }
                    if( sector[neartagsector].hitag == 0 ) {
                         operatesector(neartagsector);
                    }
               }
               if( neartagwall >= 0 ) {
                    if (neartagwall >= numwalls) {
                         crashgame("game1078: Invalid wall index (%d)",neartagwall);
                    }
                    if( wall[neartagwall].lotag == 2 ) {
                         for( i=0; i<numsectors; i++ ) {
                              if( sector[i].hitag == wall[neartagwall].hitag ) {
                                   if( sector[i].lotag != 1 ) {
                                        operatesector(i);
                                   }
                              }
                         }
                         i = headspritestat[0];
                         while( i != -1 ) {
                              nexti = nextspritestat[i];
                              if( sprite[i].hitag == wall[neartagwall].hitag ) {
                                   operatesprite(i);
                              }
                              i = nexti;
                         }
                         j = wall[neartagwall].overpicnum;
                         if( j == SWITCH1ON ) {
                              wall[neartagwall].overpicnum = GIFTBOX;
                              wall[neartagwall].lotag = 0;
                              wall[neartagwall].hitag = 0;
                         }
                         if( j == GIFTBOX ) {
                              wall[neartagwall].overpicnum = SWITCH1ON;
                              wall[neartagwall].lotag = 0;
                              wall[neartagwall].hitag = 0;
                         }
                         if (j == SWITCH2ON)  wall[neartagwall].overpicnum = SWITCH2OFF;
                         if (j == SWITCH2OFF) wall[neartagwall].overpicnum = SWITCH2ON;
                         if (j == SWITCH3ON)  wall[neartagwall].overpicnum = SWITCH3OFF;
                         if (j == SWITCH3OFF) wall[neartagwall].overpicnum = SWITCH3ON;
                         i = wall[neartagwall].point2;
                         //dax = ((wall[neartagwall].x+wall[i].x)>>1);
                         //day = ((wall[neartagwall].y+wall[i].y)>>1);
                    }
               }
               if( neartagsprite >= 0 ) {
                    if (neartagsprite >= MAXSPRITES) {
                         crashgame("game1118: Invalid sprite index (%d)",neartagsprite);
                    }
                    if( sprite[neartagsprite].lotag == 4 ) {
                         tekswitchtrigger(snum);
                    }
                    else {
                         operatesprite(neartagsprite);
                    }
               }
          }
     }

     // fire weapon
     if( (syncbits[snum]&2048) > 0 ) {
          tekfiregun((syncbits[snum]>>13)&15,snum);
     }

     // map mode
     if( (syncbits[snum]&4096) > (oflags[snum]&4096) ) {
          if (dimensionmode[snum] == 3) {
               dimensionmode[snum]=1;
          }
          else {
               dimensionmode[snum]=3;
          }
#if 0               // eliminate map mode 2
          dimensionmode[snum]++;
          if( dimensionmode[snum] > 3 ) {
               dimensionmode[snum] = 1;
          }
#endif
          if( snum == screenpeek ) {
               if (dimensionmode[snum] == 2) setview(0L,0L,xdim-1,ydim-1);
               if (dimensionmode[snum] == 3) setup3dscreen();
          }
     }

     oflags[snum] = syncbits[snum];
}

void
drawscreen(short snum, int dasmoothratio)
{
     int      i, j, charsperline, tempint;
     int      x1, y1, x2, y2, ox1, oy1, ox2, oy2;
     int      cposx, cposy, cposz, choriz, czoom;
     short     cang;

     smoothratio = max(min(dasmoothratio,65536),0);

     cposx = oposx[snum]+mulscale(posx[snum]-oposx[snum],smoothratio,16);
     cposy = oposy[snum]+mulscale(posy[snum]-oposy[snum],smoothratio,16);
     cposz = oposz[snum]+mulscale(posz[snum]-oposz[snum],smoothratio,16);
     if( frameinterpolate == 0 ) {
          cposx = posx[snum]; cposy = posy[snum]; cposz = posz[snum];
     }
     cposz+=headbob;
     choriz = ohoriz[snum]+mulscale(horiz[snum]-ohoriz[snum],smoothratio,16);
     czoom = ozoom[snum]+mulscale(zoom[snum]-ozoom[snum],smoothratio,16);
     cang = oang[snum]+mulscale(((ang[snum]+1024-oang[snum])&2047)-1024,smoothratio,16);

     if( dimensionmode[snum] != 2 ) {
          if( (numplayers > 1) && (option[4] == 0) ) {
               for( i=connecthead; i>=0; i=connectpoint2[i] ) {
                    frame2draw[i] = 1;
               }
               redrawbackfx();
               for( i=connecthead,j=0; i>=0; i=connectpoint2[i],j++ ) {
                    if( frame2draw[i] != 0 ) {
                         if( numplayers <= 4 ) {
                              switch( j ) {
                                   case 0: setview(0,0,(xdim>>1)-1,(ydim>>1)-1); break;
                                   case 1: setview((xdim>>1),0,xdim-1,(ydim>>1)-1); break;
                                   case 2: setview(0,(ydim>>1),(xdim>>1)-1,ydim-1); break;
                                   case 3: setview((xdim>>1),(ydim>>1),xdim-1,ydim-1); break;
                              }
                         }
                         else {
                              switch( j ) {
                                   case 0: setview(0,0,(xdim>>2)-1,(ydim>>2)-1); break;
                                   case 1: setview(xdim>>2,0,(xdim>>1)-1,(ydim>>2)-1); break;
                                   case 2: setview(xdim>>1,0,xdim-(xdim>>2)-1,(ydim>>2)-1); break;
                                   case 3: setview(xdim-(xdim>>2),0,xdim-1,(ydim>>2)-1); break;
                                   case 4: setview(0,ydim>>2,(xdim>>2)-1,(ydim>>1)-1); break;
                                   case 5: setview(xdim>>2,ydim>>2,(xdim>>1)-1,(ydim>>1)-1); break;
                                   case 6: setview(xdim>>1,ydim>>2,xdim-(xdim>>2)-1,(ydim>>1)-1); break;
                                   case 7: setview(xdim-(xdim>>2),ydim>>2,xdim-1,(ydim>>1)-1); break;
                                   case 8: setview(0,ydim>>1,(xdim>>2)-1,ydim-(ydim>>2)-1); break;
                                   case 9: setview(xdim>>2,ydim>>1,(xdim>>1)-1,ydim-(ydim>>2)-1); break;
                                   case 10: setview(xdim>>1,ydim>>1,xdim-(xdim>>2)-1,ydim-(ydim>>2)-1); break;
                                   case 11: setview(xdim-(xdim>>2),ydim>>1,xdim-1,ydim-(ydim>>2)-1); break;
                                   case 12: setview(0,ydim-(ydim>>2),(xdim>>2)-1,ydim-1); break;
                                   case 13: setview(xdim>>2,ydim-(ydim>>2),(xdim>>1)-1,ydim-1); break;
                                   case 14: setview(xdim>>1,ydim-(ydim>>2),xdim-(xdim>>2)-1,ydim-1); break;
                                   case 15: setview(xdim-(xdim>>2),ydim-(ydim>>2),xdim-1,ydim-1); break;
                              }
                         }
                         if( i == snum ) {
                              drawrooms(cposx,cposy,cposz,cang,choriz,cursectnum[i]);
                         }
                         else {
                              drawrooms(posx[i],posy[i],posz[i],ang[i],horiz[i],cursectnum[i]);
                         }
                         analyzesprites(posx[i],posy[i]);
                         drawmasks();
                         tekdrawgun((syncbits[i]>>13)&15,i);
                    }
               }
          }
          else {
               redrawbackfx();
               drawrooms(cposx,cposy,cposz,cang,choriz,cursectnum[snum]);
               analyzesprites(posx[snum],posy[snum]);
               drawmasks();
               tekdrawgun((syncbits[screenpeek]>>13)&15,screenpeek);
          }
     }

     // move back pivot point for map
     if( dimensionmode[snum] != 3 ) {
          cposx += (sintable[(cang+512)&2047]<<6) / czoom;
          cposy += (sintable[cang&2047]<<6) / czoom;
          if( dimensionmode[snum] == 2 ) {
               clearview(0L);  //Clear screen to specified color
               drawmapview(cposx,cposy,czoom,cang);
          }
          drawoverheadmap(cposx,cposy,czoom,cang);
     }

     if( typemode != 0 ) {
          charsperline = 40;
          for( i=0; i<=typemessageleng; i+=charsperline ) {
               for( j=0; j<charsperline; j++ ) {
                    tempbuf[j] = typemessage[i+j];
               }
               if( typemessageleng < i+charsperline ) {
                    tempbuf[(typemessageleng-i)] = '_';
                    tempbuf[(typemessageleng-i)+1] = 0;
               }
               else {
                    tempbuf[charsperline] = 0;
               }
          }
     }
     else {
          if( dimensionmode[myconnectindex] == 3 ) {
               tempint = screensize;
               if( ((locbits&32) > (screensizeflag&32)) && (screensize > 64) ) {
                    screensize -= (screensize>>3);
                    if( tempint > 320 ) screensize = 320;
                    screensizeflag = 1;
               }
               if( ((locbits&16) > (screensizeflag&16)) && (screensize <= 320) ) {
                    screensize += (screensize>>3);
                    if( (screensize > 320) && (tempint == 320) ) screensize = 320+1;
                    else if (screensize > 320) screensize = 320;
                    screensizeflag = 1;
               }
               screensizeflag = (locbits&48) | (screensizeflag&1);
          }
     }

     if( getmessageleng > 0 ) {
          charsperline = 40;
          for( i=0; i<=getmessageleng; i+=charsperline ) {
               for( j=0; j<charsperline; j++ ) {
                    tempbuf[j] = getmessage[i+j];
               }
               if( getmessageleng < i+charsperline ) {
                    tempbuf[(getmessageleng-i)] = 0;
               }
               else {
                    tempbuf[charsperline] = 0;
               }
               printext256(0L,((i/charsperline)<<3)+(200-32-8)-(((getmessageleng-1)/charsperline)<<3),151,-1,(char *)tempbuf,0);
          }
          if( totalclock > getmessagetimeoff ) {
               getmessageleng = 0;
          }
     }

     // you are looking thru an opponent plaeyer's eyes
     if( (numplayers >= 2) && (screenpeek != myconnectindex) ) {
          strcpy((char *)tempbuf,"Other");
     }

    #ifdef OUTOFSYNCMESSAGE
     if( syncstat != 0 ) {
          printext256(68L,84L,31,0,"OUT OF SYNC!",0);
     }
     if( syncstate != 0 ) {
          printext256(68L,92L,31,0,"Missed Network packet!",0);
     }
    #endif

     tekscreenfx();

    #ifdef  NETWORKDIAGNOSTICS
     if( (option[4] > 0) ) {
          for( i=connecthead ; i >= 0 ; i=connectpoint2[i] ) {
               sprintf(tektempbuf,"%2d %5d %5d %5d %5d %3d %4d", i,posx[i],posy[i],posz[i],ang[i],horiz[i],health[i]);
               printext(2,i*8+2,tektempbuf,ALPHABET,255);
          }
          if( myconnectindex == connecthead ) {
               sprintf(tektempbuf,"%2d %s",myconnectindex,"M");
          }
          else {
               sprintf(tektempbuf,"%2d %s",myconnectindex,"S");
          }
          printext(windowx2-48,windowy2-64,tektempbuf,ALPHABET2,255);
     }
    #endif

    if (showfps) {
        int i,j;
        char fps[60];
        i = frameval[framecnt&(AVERAGEFRAMES-1)];
        j = frameval[framecnt&(AVERAGEFRAMES-1)] = getticks(); framecnt++;
        if (i != j) averagefps = ((mul3(averagefps)+((AVERAGEFRAMES*1000)/(j-i)) )>>2);
        snprintf(fps,sizeof(fps),"fps %d",averagefps);
        printext256(0L,0L,31,-1,fps,0);
    }

     nextpage();
     if( dofadein != 0 ) {
          fadein(0,255,dofadein);
     }

     // F12 key
     if( keystatus[0x58] > 0 ) {
          keystatus[0x58] = 0;
          screencapture("captxxxx.pcx",keystatus[0x2a]|keystatus[0x36]);
     }

    #ifdef STEREOMODE_ADJUSTMENT_ACTIVE
     if( stereofps != 0 ) {
          if( (keystatus[0x2a]|keystatus[0x36]) > 0 ) {
               if (keystatus[0x1a] > 0) stereopixelwidth--;   //Shift [
               if (keystatus[0x1b] > 0) stereopixelwidth++;   //Shift ]
          }
          else {
               if (keystatus[0x1a] > 0) stereowidth -= 512;   //[
               if (keystatus[0x1b] > 0) stereowidth += 512;   //]
          }
     }
    #endif

    #ifdef FAKEMULTIPLAYER_ACTIVE
     if( option[4] == 0 ) {
          if( keystatus[0xd2] > 0 ) {
               keystatus[0xd2] = 0;
               if( numplayers < MAXPLAYERS ) {
                    connectpoint2[numplayers-1] = numplayers;
                    connectpoint2[numplayers] = -1;
                    initplayersprite(numplayers);
                    clearallviews(0L);
                    numplayers++;
               }
          }
          if( keystatus[0xd3] > 0 ) {
               keystatus[0xd3] = 0;
               if( numplayers > 1 ) {
                    numplayers--;
                    connectpoint2[numplayers-1] = -1;
                    deletesprite(playersprite[numplayers]);
                    playersprite[numplayers] = -1;
                    if( myconnectindex >= numplayers ) {
                         myconnectindex = 0;
                    }
                    if( screenpeek >= numplayers ) {
                         screenpeek = 0;
                    }
                    if( numplayers < 2 ) {
                         setup3dscreen();
                    }
                    else {
                         clearallviews(0L);
                    }
               }
          }
          // scroll lock
          if( keystatus[0x46] > 0 ) {
               keystatus[0x46] = 0;
               myconnectindex = connectpoint2[myconnectindex];
               if( myconnectindex < 0 ) {
                    myconnectindex = connecthead;
               }
               screenpeek = myconnectindex;
          }
     }
    #endif
}

void
movethings()
{
     int      i;

     gotlastpacketclock = totalclock;
     for( i=connecthead; i>=0; i=connectpoint2[i] ) {
          baksyncvel[movefifoend][i] = fsyncvel[i];
          baksyncsvel[movefifoend][i] = fsyncsvel[i];
          baksyncangvel[movefifoend][i] = fsyncangvel[i];
          baksynchorizvel[movefifoend][i] = fsynchorizvel[i];
          baksyncbits[movefifoend][i] = fsyncbits[i];
     }
     movefifoend = ((movefifoend+1)&(MOVEFIFOSIZ-1));

     // do this for Master/Slave switching
     for( i=connectpoint2[connecthead]; i>=0; i=connectpoint2[i] ) {
          if( syncbits[i]&512 ) {
               ready2send = 0;
          }
     }

     tektime();
}

void
domovethings()
{
     short          i, j, startwall, endwall;
     walltype       *wal;

     for( i=connecthead; i>=0; i=connectpoint2[i] ) {
        if (dbgflag) {
            debugout(i);
        }
          syncvel[i] = baksyncvel[movefifoplc][i];
          syncsvel[i] = baksyncsvel[movefifoplc][i];
          syncangvel[i] = baksyncangvel[movefifoplc][i];
          synchorizvel[i] = baksynchorizvel[movefifoplc][i];
          syncbits[i] = baksyncbits[movefifoplc][i];
     }
     movefifoplc = ((movefifoplc+1)&(MOVEFIFOSIZ-1));
#if 0
     syncval[syncvalend] = getsyncstat();
     syncvalend = ((syncvalend+1)&(MOVEFIFOSIZ-1));
#endif
     for( i=connecthead; i>=0; i=connectpoint2[i] ) {
          oposx[i] = posx[i];
          oposy[i] = posy[i];
          oposz[i] = posz[i];
          ohoriz[i] = horiz[i];
          ozoom[i] = zoom[i];
          oang[i] = ang[i];
     }

     for( i=1; i<=8; i++ ) {
          if( i != 2 ) {
               for( j=headspritestat[i]; j>=0; j=nextspritestat[j] ) {
                    copybuf(&sprite[j].x,&osprite[j].x,3);
               }
          }
     }

     for( i=connecthead; i>=0; i=connectpoint2[i] ) {
          ocursectnum[i] = cursectnum[i];
     }

     if( (numplayers <= 2) && (recstat == 1) ) {
          j = 0;
          for( i=connecthead; i>=0; i=connectpoint2[i] ) {
               recsyncvel[reccnt][j] = syncvel[i];
               recsyncsvel[reccnt][j] = syncsvel[i];
               recsyncangvel[reccnt][j] = syncangvel[i];
               recsynchorizvel[reccnt][j] = synchorizvel[i];
               recsyncbits[reccnt][j] = syncbits[i];
               j++;
          }
          reccnt++;
          if( reccnt > 16383 ) {
               reccnt = 16383;
          }
     }

     lockclock += TICSPERFRAME;

     for( i=connecthead; i>=0; i=connectpoint2[i] ) {
          processinput(i);
          checktouchsprite(i,cursectnum[i]);
          startwall = sector[cursectnum[i]].wallptr;
          endwall = startwall + sector[cursectnum[i]].wallnum;
          for( j=startwall,wal=&wall[j]; j<endwall; j++,wal++ ) {
               if( wal->nextsector >= 0 ) {
                    checktouchsprite(i,wal->nextsector);
               }
          }
     }

     doanimations();

     tagcode();
     statuslistcode();

     checkmasterslaveswitch();
}

void
adjustbiasthreshhold(short mousy)
{
     biasthreshhold-=mousy;
     if( biasthreshhold < 8 )
          biasthreshhold=8;
     if( biasthreshhold > 512 )
          biasthreshhold=512;
}

//** Les START - 09/27/95
short moreoptionbits[NUMKEYS]={
     -1,                           //  0 move forward
     -1,                           //  1 move backward
     -1,                           //  2 turn right
     -1,                           //  3 turn left
      8,                           //  4 run
     -1,                           //  5 strafe
     11,                           //  6 shoot
     10,                           //  7 use
      0,                           //  8 jump
      1,                           //  9 crouch
      2,                           // 10 look up
      3,                           // 11 look down
     -1,                           // 12 slide left
     -1,                           // 13 slide right
     12,                           // 14 map
     -1,                           // 15 switch player
      4,                           // 16 zoom in
      5,                           // 17 zoom out
     -1,                           // 18 message
      6,                           // 19 autocenter
     -1,                           // 20 rearview
     -1,                           // 21 current item
     -1,                           // 22 health
     -1,                           // 23 crosshairs
     -1,                           // 24 elapsed time
     -1,                           // 25 score
     -1,                           // 26 inventory
      7,                           // 27 conceal weapon
     -1,                           // 28 mouse look mode
     -1,                           // 29 console
};
//** Les END   - 09/27/95

void
getinput()
{
     int      ch, keystate;
     int      i, j, clocbits=0;
     int      mousx=0, mousy=0, bstatus=0;
     int      mvel=0, msvel=0, mangvel=0, mhorizvel=0;
     int      jvel=0, jsvel=0, jangvel=0, jhorizvel=0;
     short    moving=0,strafing=0,turning=0;

     // normal game keys active
     if( typemode == 0 ) {
          // shift+shift+R
          if( (keystatus[0x2a]&keystatus[0x36]&keystatus[0x13]) > 0 ) {
               keystatus[0x13] = 0;
               playback();
          }
         #ifdef SWITCHINGACTIVE
          if( keystatus[keys[15]] > 0 ) {
               keystatus[keys[15]] = 0;
               screenpeek = connectpoint2[screenpeek];
               if( screenpeek < 0 ) {
                    screenpeek = connecthead;
               }
          }
         #endif
          for( i=7; i>=0; i-- ) {
               if( (keystatus[i+2] > 0) && tekhasweapon(i,screenpeek) ) {
                    keystatus[i+2] = 0;
                    locselectedgun = i;
                    break;
               }
          }
     }

     if( option[3] & 1 ) {
          getmousevalues(&mousx,&mousy,&bstatus);
          for(i=0; i<2; i++) {
               if (!(bstatus&(1<<i)) || moreoptions[i+1] < 0 || moreoptions[i+1] >= NUMKEYS-1)
                    continue;
               switch (moreoptions[i+1]) {
                    case 0: moving=1; break;
                    case 1: moving=-1; break;
                    case 2: turning=1; break;
                    case 3: turning=-1; break;
                    case 5: strafing=2; break;
                    case 12: strafing=-1; break;
                    case 13: strafing=1; break;
                    default:
                         if (moreoptionbits[moreoptions[i+1]] >= 0) {
                              clocbits |= 1<<moreoptionbits[moreoptions[i+1]];
                         }
                         break;
               }
          }
          oldmousebstatus = bstatus;
     }
     if ( option[3] & 2 ) {
          for(i=0; i<4; i++) {
               if (!(joyb&(1<<i)) || moreoptions[i+4] < 0 || moreoptions[i+4] >= NUMKEYS-1)
                    continue;
               switch (moreoptions[i+4]) {
                    case 0: moving=1; break;
                    case 1: moving=-1; break;
                    case 2: turning=1; break;
                    case 3: turning=-1; break;
                    case 5: strafing=2; break;
                    case 12: strafing=-1; break;
                    case 13: strafing=1; break;
                    default:
                         if (moreoptionbits[moreoptions[i+4]] >= 0) {
                              clocbits |= 1<<moreoptionbits[moreoptions[i+4]];
                         }
                         break;
               }
          }
          oldjoyb=joyb;
     }

     // keyboard survey - use to be keytimerstuff() called from keyhandler
     if( keystatus[keys[5]] == 0 && strafing == 0 ) {    // Les 09/28/95
          if( keystatus[keys[2]] > 0 || turning == 1) angvel = max(angvel-16*TICSPERFRAME,-128); // Les 09/28/95
          if( keystatus[keys[3]] > 0 || turning == -1) angvel = min(angvel+16*TICSPERFRAME,127); // Les 09/28/95
     }
     else {
          if (strafing == 0) {                                                                   // Les 09/28/95
               strafing=2;                                                                       // Les 09/28/95
          }                                                                                      // Les 09/28/95
          if( keystatus[keys[2]] > 0 || turning == 1 ) svel = min(svel+8*TICSPERFRAME,127);      // Les 09/28/95
          if( keystatus[keys[3]] > 0 || turning == -1 ) svel = max(svel-8*TICSPERFRAME,-128);    // Les 09/28/95
     }
     if( keystatus[keys[0]]  > 0 || moving == 1 ) vel  = min(vel+8*TICSPERFRAME,127);           // Les 09/28/95
     if( keystatus[keys[1]]  > 0 || moving == -1 ) vel  = max(vel-8*TICSPERFRAME,-128);         // Les 09/28/95
     if( keystatus[keys[12]] > 0 || strafing == 1 ) svel = min(svel+8*TICSPERFRAME,127);        // Les 09/28/95
     if( keystatus[keys[13]] > 0 || strafing == -1 ) svel = max(svel-8*TICSPERFRAME,-128);      // Les 09/28/95
     if( angvel < 0 ) angvel = min(angvel+12*TICSPERFRAME,0);
     if( angvel > 0 ) angvel = max(angvel-12*TICSPERFRAME,0);
     if( svel < 0 )   svel   = min(svel+2*TICSPERFRAME,0);
     if( svel > 0 )   svel   = max(svel-2*TICSPERFRAME,0);
     if( vel  < 0 )   vel    = min(vel+2*TICSPERFRAME,0);
     if( vel  > 0 )   vel    = max(vel-2*TICSPERFRAME,0);

     if( (option[4] == 0) && (numplayers == 2) ) {
          if( keystatus[0x4f] == 0 ) {
               if( keystatus[0x4b] > 0 ) angvel2 = max(angvel2-16*TICSPERFRAME,-128);
               if( keystatus[0x4d] > 0 ) angvel2 = min(angvel2+16*TICSPERFRAME,127);
          }
          else {
               if( keystatus[0x4b] > 0 ) svel2 = min(svel2+8*TICSPERFRAME,127);
               if( keystatus[0x4d] > 0 ) svel2 = max(svel2-8*TICSPERFRAME,-128);
          }
          if( keystatus[0x48] > 0 ) vel2 = min(vel2+8*TICSPERFRAME,127);
          if( keystatus[0x4c] > 0 ) vel2 = max(vel2-8*TICSPERFRAME,-128);
          if( angvel2 < 0 ) angvel2 = min(angvel2+12*TICSPERFRAME,0);
          if( angvel2 > 0 ) angvel2 = max(angvel2-12*TICSPERFRAME,0);
          if( svel2 < 0 )   svel2   = min(svel2+2*TICSPERFRAME,0);
          if( svel2 > 0 )   svel2   = max(svel2-2*TICSPERFRAME,0);
          if( vel2  < 0 )   vel2    = min(vel2+2*TICSPERFRAME,0);
          if( vel2  > 0 )   vel2    = max(vel2-2*TICSPERFRAME,0);
     }

     if( option[3] & 1 ) {
          if(keystatus[keys[5]]) msvel-=(mousx*mousesensitivity);
          else mangvel+=(mousx*mousesensitivity);

          if (mouselookmode) {
               if (keystatus[keys[28]]) {
                    mouselook ^= 1;
                    if (mouselook) showmessage("MOUSELOOK ON");
                    else showmessage("MOUSELOOK OFF");
                    keystatus[keys[28]]=0;
               }
          }
          else {
               mouselook = keystatus[keys[28]] > 0;
          }
          if (mouselook) mhorizvel-=(mousy>>1);
          else mvel-=(mousy*mousesensitivity);
     }
     if ( option[3] & 2 ) {
          const int deadzone = 4096;
          if (joynumaxes == 2) {
               jangvel  = joyaxis[0];
               jvel     =-joyaxis[1];
          } else if (joynumaxes >= 4) {
               jangvel  = joyaxis[2];
               jvel     =-joyaxis[1];
               jsvel    =-joyaxis[0];
               jhorizvel=-joyaxis[3];
          }
          if (klabs(jvel) < deadzone) jvel=0;
          if (klabs(jsvel) < deadzone) jsvel=0;
          if (klabs(jangvel) < deadzone) jangvel=0;
          if (klabs(jhorizvel) < deadzone) jhorizvel=0;
          jvel = 127*(jvel-ksgn(jvel)*deadzone)/(32767-deadzone);
          jsvel = 127*(jsvel-ksgn(jsvel)*deadzone)/(32767-deadzone);
          jangvel = 127*(jangvel-ksgn(jangvel)*deadzone)/(32767-deadzone);
          jhorizvel = (200/8)*(jhorizvel-ksgn(jhorizvel)*deadzone)/(32767-deadzone);
     }

     locvel = min(max(vel+mvel+jvel,-128+8),127-8);
     locsvel = min(max(svel+msvel+jsvel,-128+8),127-8);
     locangvel = min(max(angvel+mangvel+jangvel,-512+16),511-16);            // Les 09/27/95
     lochorizvel = max(-100,min(100,horizvel+mhorizvel+jhorizvel));

     locbits = clocbits|(locselectedgun<<13);
     if( typemode == 0 ) {
         #ifdef MASTERSWITCHING
          locbits |= (keystatus[0x32]<<9);                  //M (be master)
         #endif
          locbits |= ((keystatus[keys[14]]==1)<<12);        //Map mode
     }
     locbits |= keystatus[keys[8]];                         //Stand high
     locbits |= (keystatus[keys[9]] <<1);                   //Stand low
     //locbits |= (keystatus[keys[10]]<<2);                   //Look up
     //locbits |= (keystatus[keys[11]]<<3);                   //Look down
     locbits |= (keystatus[keys[16]]<<4);                   //Zoom in
     locbits |= (keystatus[keys[17]]<<5);                   //Zoom out
     locbits |= (keystatus[keys[19]]<<6);                   //AutoCenter     TekWar
     locbits |= (keystatus[keys[27]]<<7);                   //Conceal Weapin TekWar
     locbits |= (keystatus[keys[4]]<<8);                    //Run
     locbits |= ((keystatus[keys[7]]==1)<<10);              //Space
     locbits |= ((keystatus[keys[6]]==1)<<11);              //Shoot Kbd
//    locbits |= (((bstatus&6)>(oldmousebstatus&6))<<10);    //Space
//    locbits |= (((bstatus&1)>(oldmousebstatus&1))<<11);    //Shoot Mse

     if( typemode != 0 ) {
         #ifdef MASTERSWITCHING
          locbits &= ~(keystatus[0x32]<<9);
         #endif
          locbits &= ~((keystatus[keys[14]]==1)<<12);
     }

     /*if( (joyb == 236) || (joyb == 220) || (joyb == 124) || (joyb == 188) ) {
          keystatus[keys[moreoptions[4]]]=0;
          keystatus[keys[moreoptions[5]]]=0;
          keystatus[keys[moreoptions[6]]]=0;
          keystatus[keys[moreoptions[7]]]=0;
     }*/

     /*if( (locbits&2048) > 0 ) {
          oldmousebstatus &= ~1;
     }*/

     // trap print scrn key
     if( keystatus[0xb7] > 0 ) {
          keystatus[0xb7] = 0;
          //printscreeninterrupt();
     }

     // F9 brightness
     if( keystatus[67] > 0 ) {
          keystatus[67] = 0;
          brightness++;
          if( brightness > 8 ) brightness = 0;
          setbrightness(brightness,palette,0);
     }

    #ifdef OOGIE
     // F10 adjust bias threshhold
     if (keystatus[68] != 0) {
          keystatus[68]=0;
          if (dimensionmode[screenpeek] != 3) {
               setup3dscreen();
          }
          else {
               dimensionmode[screenpeek]=2;
               setview(0L,0L,xdim-1,ydim-1);
          }
     }
    #endif

     if( typemode == 0 ) {
         #ifdef PARALLAX_SETTING_ACTIVE
          if( keystatus[0x19] > 0 ) {
               keystatus[0x19] = 0;
               parallaxtype++;
               if (parallaxtype > 2) parallaxtype = 0;
          }
         #endif
         #ifdef VISIBILITY_SETTING_ACTIVE
          if( (keystatus[0x38]|keystatus[0xb8]) > 0 ) {
               if (keystatus[0x4a] > 0)  // Keypad -
                    visibility = min(visibility+(visibility>>3),16384);
               if (keystatus[0x4e] > 0)  // Keypad +
                    visibility = max(visibility-(visibility>>3),128);
          }
         #endif
          // if typing mode reset kbrd fifo
          if( (keystatus[keys[18]]) > 0 ) {
               keystatus[keys[18]] = 0;
               typemode = 1;
               keyfifoplc = keyfifoend;
          }
     }
     else {
          while( keyfifoplc != keyfifoend ) {
               ch = keyfifo[keyfifoplc];
               keystate = keyfifo[(keyfifoplc+1)&(KEYFIFOSIZ-1)];
               keyfifoplc = ((keyfifoplc+2)&(KEYFIFOSIZ-1));
               if( keystate != 0 ) {
                    // backspace key
                    if( ch == 0xe ) {
                         if( typemessageleng == 0 ) {
                              typemode = 0;
                              break;
                         }
                         typemessageleng--;
                    }
                    if( ch == 0xf ) {
                         keystatus[0xf] = 0;
                         typemode = 0;
                         break;
                    }
                    // either enter key
                    if( (ch == 0x1c) || (ch == 0x9c) ) {
                         keystatus[0x1c] = 0; keystatus[0x9c] = 0;
                         if( typemessageleng > 0 ) {
                              tempbuf[0] = 2;
                              // sending text is message type 4
                              for( j=typemessageleng-1; j>=0; j-- ) {
                                   tempbuf[j+1] = typemessage[j];
                              }
                              for( i=connecthead; i>=0; i=connectpoint2[i] ) {
                                   if( i != myconnectindex ) {
                                        sendpacket(i,tempbuf,typemessageleng+1);
                                   }
                              }
                              typemessageleng = 0;
                         }
                         typemode = 0;
                         break;
                    }
                    if( (typemessageleng < 159) && (ch < 128) ) {
                         if( (keystatus[0x2a]|keystatus[0x36]) != 0 ) {
                              ch = scantoascwithshift[ch];
                         }
                         else {
                              ch = scantoasc[ch];
                         }
                         if( ch != 0 ) {
                              typemessage[typemessageleng++] = ch;
                         }
                    }
               }
          }
          // here's a trick of making key repeat after a 1/2 second
          if( keystatus[0xe] > 0 ) {
               if( keystatus[0xe] < 30 ) {
                    keystatus[0xe] += TICSPERFRAME;
               }
               else {
                    if( typemessageleng == 0 ) {
                         typemode = 0;
                    }
                    else {
                         typemessageleng--;
                    }
               }
          }
     }

     tekprivatekeys();
}

void
playback()
{
     int i, j, k;

     ready2send = 0;
     recstat = 0; i = reccnt;
     while (keystatus[1] == 0)
     {
          while (totalclock >= lockclock+TICSPERFRAME)
          {
               if (i >= reccnt)
               {
                    prepareboard(boardfilename);
                    for(i=connecthead;i>=0;i=connectpoint2[i])
                         initplayersprite((short)i);
                    resettiming(); ototalclock = 0; gotlastpacketclock = 0;
                    i = 0;
               }

               k = 0;
               for(j=connecthead;j>=0;j=connectpoint2[j])
               {
                    fsyncvel[j] = recsyncvel[i][k];
                    fsyncsvel[j] = recsyncsvel[i][k];
                    fsyncangvel[j] = recsyncangvel[i][k];
                    fsynchorizvel[j] = recsynchorizvel[i][k];
                    fsyncbits[j] = recsyncbits[i][k];
                    k++;
               }
               movethings(); domovethings();
               i++;
          }
          drawscreen(screenpeek,(totalclock-lockclock)*(65536/TICSPERFRAME));

          if (keystatus[keys[15]] > 0)
          {
               keystatus[keys[15]] = 0;

               screenpeek = connectpoint2[screenpeek];
               if (screenpeek < 0) screenpeek = connecthead;
          }
          if (keystatus[keys[14]] > 0)
          {
               keystatus[keys[14]] = 0;
               dimensionmode[screenpeek]++;
               if (dimensionmode[screenpeek] > 3) dimensionmode[screenpeek] = 1;
               if (dimensionmode[screenpeek] == 2) setview(0L,0L,xdim-1,ydim-1);
               if (dimensionmode[screenpeek] == 3) setup3dscreen();
          }
     }

     musicoff();

     uninitmultiplayers();
     uninittimer();
     uninitinput();
     uninitengine();
     uninitsb();
     uninitgroupfile();
     cduninit();
     exit(0);
}

void
doanimations()
{
     int i, j;

     for(i=animatecnt-1;i>=0;i--)
     {
          j = *animateptr[i];

          if (j < animategoal[i])
               j = min(j+animatevel[i]*TICSPERFRAME,animategoal[i]);
          else
               j = max(j-animatevel[i]*TICSPERFRAME,animategoal[i]);
          animatevel[i] += animateacc[i];

          *animateptr[i] = j;

          if (j == animategoal[i])
          {
               animatecnt--;
               if (i != animatecnt)
               {
                    animateptr[i] = animateptr[animatecnt];
                    animategoal[i] = animategoal[animatecnt];
                    animatevel[i] = animatevel[animatecnt];
                    animateacc[i] = animateacc[animatecnt];
               }
          }
     }
}

int
getanimationgoal(int *animptr)
{
     int i;

     for(i=animatecnt-1;i>=0;i--)
          if (animptr == animateptr[i]) return(i);
     return(-1);
}

int
setanimation(int *animptr, int thegoal, int thevel, int theacc)
{
     int i, j;

     if (animatecnt >= MAXANIMATES) return(-1);

     j = animatecnt;
     for(i=animatecnt-1;i>=0;i--)
          if (animptr == animateptr[i])
               { j = i; break; }

     animateptr[j] = animptr;
     animategoal[j] = thegoal;
     animatevel[j] = thevel;
     animateacc[j] = theacc;
     if (j == animatecnt) animatecnt++;
     return(j);
}

void
checkmasterslaveswitch()
{
     int i, j;

     if (option[4] == 0) return;

     i = connecthead; j = connectpoint2[i];
     while (j >= 0)
     {
          if ((syncbits[j]&512) > 0)
          {
               connectpoint2[i] = connectpoint2[j];
               connectpoint2[j] = connecthead;
               connecthead = (short)j;

               olocvel = locvel+1; olocvel2 = locvel2+1;
               olocsvel = locsvel+1; olocsvel2 = locsvel2+1;
               olocangvel = locangvel+1; olocangvel2 = locangvel2+1;
               olochorizvel = lochorizvel+1; olochorizvel2 = lochorizvel2+1;
               olocbits = locbits+1; olocbits2 = locbits2+1;
               for(i=0;i<MAXPLAYERS;i++)
               {
                    osyncvel[i] = fsyncvel[i]+1;
                    osyncsvel[i] = fsyncsvel[i]+1;
                    osyncangvel[i] = fsyncangvel[i]+1;
                    osynchorizvel[i] = fsynchorizvel[i]+1;
                    osyncbits[i] = fsyncbits[i]+1;
               }

               syncvalplc = 0L; othersyncvalplc = 0L;
               syncvalend = 0L; othersyncvalend = 0L;
               syncvalcnt = 0L; othersyncvalcnt = 0L;

               totalclock = lockclock;
               ototalclock = lockclock;
               gotlastpacketclock = lockclock;
               masterslavetexttime = lockclock;
               ready2send = 1;
               return;
          }
          i = j; j = connectpoint2[i];
     }
}

void
faketimerhandler()
{
     short other;
     int i, j, k, l;

     if (totalclock < ototalclock+TICSPERFRAME) return;
     if (ready2send == 0) return;
     ototalclock = totalclock;

     // I am the MASTER (or 1 player game)
     if ((myconnectindex == connecthead) || (option[4] == 0))
     {
          if (option[4] != 0)
               getpackets();

          if (getoutputcirclesize() < 16)
          {
               getinput();
               fsyncvel[myconnectindex] = locvel;
               fsyncsvel[myconnectindex] = locsvel;
               fsyncangvel[myconnectindex] = locangvel;
               fsynchorizvel[myconnectindex] = lochorizvel;
               fsyncbits[myconnectindex] = locbits;

               if (option[4] != 0)
               {
                    tempbuf[0] = 0;
                    j = ((numplayers+1)>>1)+1;
                    for(k=1;k<j;k++) tempbuf[k] = 0;
                    k = (1<<3);
                    for(i=connecthead;i>=0;i=connectpoint2[i])
                    {
                         l = 0;
                         if (fsyncvel[i] != osyncvel[i] || fsynchorizvel[i] != osynchorizvel[i]) {
                              tempbuf[j++] = fsyncvel[i];
                              tempbuf[j++] = fsynchorizvel[i];
                              l |= 1;
                         }
//** Les START - 09/27/95
                         if (fsyncsvel[i] != osyncsvel[i]) {
                              tempbuf[j++]=(fsyncsvel[i]&0xFF);
                              tempbuf[j++]=(fsyncsvel[i]>>8);
                              l|=2;
                         }
                         if (fsyncangvel[i] != osyncangvel[i]) {
                              tempbuf[j++]=(fsyncangvel[i]&0xFF);
                              tempbuf[j++]=(fsyncangvel[i]>>8);
                              l|=4;
                         }
//** Les END   - 09/27/95
                         if (fsyncbits[i] != osyncbits[i])
                         {
                              tempbuf[j++] = (fsyncbits[i]&255);
                              tempbuf[j++] = ((fsyncbits[i]>>8)&255);
                              l |= 8;
                         }
                         tempbuf[k>>3] |= (l<<(k&7));
                         k += 4;

                         osyncvel[i] = fsyncvel[i];
                         osyncsvel[i] = fsyncsvel[i];
                         osyncangvel[i] = fsyncangvel[i];
                         osynchorizvel[i] = fsynchorizvel[i];
                         osyncbits[i] = fsyncbits[i];
                    }
#if 0
                    while (syncvalplc != syncvalend)
                    {
                         tempbuf[j] = (unsigned char)(syncval[syncvalplc]&255);
                         tempbuf[j+1] = (unsigned char)((syncval[syncvalplc]>>8)&255);
                         j += 2;
                         syncvalplc = ((syncvalplc+1)&(MOVEFIFOSIZ-1));
                    }
#endif
                    for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                         sendpacket(i,tempbuf,j);
               }
               else if (numplayers == 2)
               {
                    if (keystatus[0xb5] > 0)
                    {
                         keystatus[0xb5] = 0;
                         locselectedgun2++;
                         if (locselectedgun2 >= 3) locselectedgun2 = 0;
                    }

                    // second player on 1 computer mode
                    locvel2 = min(max(vel2,-128+8),127-8);
                    locsvel2 = min(max(svel2,-128+8),127-8);
                    locangvel2 = min(max(angvel2,-128+16),127-16);
                    lochorizvel2 = min(max(horizvel2,-128+8),127-8);
                    locbits2 = (locselectedgun2<<13);
                    locbits2 |= keystatus[0x45];                  //Stand high
                    locbits2 |= (keystatus[0x47]<<1);             //Stand low
                    locbits2 |= (1<<8);                           //Run
                    //locbits2 |= (keystatus[0x49]<<2);             //Look up
                    //locbits2 |= (keystatus[0x37]<<3);             //Look down
                    locbits2 |= (keystatus[0x50]<<10);            //Space
                    locbits2 |= (keystatus[0x52]<<11);            //Shoot

                    other = connectpoint2[myconnectindex];
                    if (other < 0) other = connecthead;

                    fsyncvel[other] = locvel2;
                    fsyncsvel[other] = locsvel2;
                    fsyncangvel[other] = locangvel2;
                    fsynchorizvel[other] = lochorizvel2;
                    fsyncbits[other] = locbits2;
               }
               movethings();  //Move EVERYTHING (you too!)
          }
     }
     else                        //I am a SLAVE
     {
          getpackets();

          if (getoutputcirclesize() < 16)
          {
               getinput();

               tempbuf[0] = 1; k = 0;
               j = 2;

               if (locvel != olocvel || lochorizvel != olochorizvel) {
                    tempbuf[j++] = locvel;
                    tempbuf[j++] = lochorizvel;
                    k |= 1;
               }
//** Les START - 09/27/95
               if (locsvel != olocsvel) {
                    tempbuf[j++]=locsvel&0xFF;
                    tempbuf[j++]=(locsvel>>8);
                    k|=2;
               }
               if (locangvel != olocangvel) {
                    tempbuf[j++]=locangvel&0xFF;
                    tempbuf[j++]=(locangvel>>8);
                    k|=4;
               }
//** Les END   - 09/27/95
               if ((locbits^olocbits)&0x00ff) tempbuf[j++] = (locbits&255), k |= 8;
               if ((locbits^olocbits)&0xff00) tempbuf[j++] = ((locbits>>8)&255), k |= 16;

               tempbuf[1] = k;

               olocvel = locvel;
               olocsvel = locsvel;
               olocangvel = locangvel;
               olochorizvel = lochorizvel;
               olocbits = locbits;

               sendpacket(connecthead,tempbuf,j);
          }
     }
}

void
getpackets()
{
     int i, j, k, l;
     int other, tempbufleng;

     if (option[4] == 0) return;

     while ((tempbufleng = getpacket(&other,tempbuf)) > 0)
     {
          switch(tempbuf[0])
          {
               case 0:  //[0] (receive master sync buffer)
                    j = ((numplayers+1)>>1)+1; k = (1<<3);
                    for(i=connecthead;i>=0;i=connectpoint2[i])
                    {
                         l = (tempbuf[k>>3]>>(k&7));
                         if (l&1) {
                              fsyncvel[i] = tempbuf[j++];
                              fsynchorizvel[i] = tempbuf[j++];
                         }
//** Les START - 09/27/95
                         if (l&2) {
                              fsyncsvel[i]=tempbuf[j++];
                              fsyncsvel[i]|=(tempbuf[j++]<<8);
                         }
                         if (l&4) {
                              fsyncangvel[i]=tempbuf[j++];
                              fsyncangvel[i]|=(tempbuf[j++]<<8);
                         }
//** Les END   - 09/27/95
                         if (l&8)
                         {
                              fsyncbits[i] = ((short)tempbuf[j])+(((short)tempbuf[j+1])<<8);
                              j += 2;
                         }
                         k += 4;
                    }
#if 0
                    while (j != tempbufleng)
                    {
                         othersyncval[othersyncvalend] = ((int)tempbuf[j]);
                         othersyncval[othersyncvalend] += (((int)tempbuf[j+1])<<8);
                         j += 2;
                         othersyncvalend = ((othersyncvalend+1)&(MOVEFIFOSIZ-1));
                    }

                    i = 0;
                    while (syncvalplc != syncvalend)
                    {
                         if (othersyncvalcnt > syncvalcnt)
                         {
                              if (i == 0) syncstat = 0, i = 1;
                              syncstat |= (syncval[syncvalplc]^othersyncval[syncvalplc]);
                         }
                         syncvalplc = ((syncvalplc+1)&(MOVEFIFOSIZ-1));
                         syncvalcnt++;
                    }
                    while (othersyncvalplc != othersyncvalend)
                    {
                         if (syncvalcnt > othersyncvalcnt)
                         {
                              if (i == 0) syncstat = 0, i = 1;
                              syncstat |= (syncval[othersyncvalplc]^othersyncval[othersyncvalplc]);
                         }
                         othersyncvalplc = ((othersyncvalplc+1)&(MOVEFIFOSIZ-1));
                         othersyncvalcnt++;
                    }
#endif

                movethings();        //Move all players and sprites
                    break;
               case 1:  //[1] (receive slave sync buffer)
                    j = 2; k = tempbuf[1];
                    if (k&1) {
                         fsyncvel[other] = tempbuf[j++];
                         fsynchorizvel[other] = tempbuf[j++];
                    }
//** Les START - 09/27/95
                     if (k&2) {
                         fsyncsvel[other]=tempbuf[j++];
                         fsyncsvel[other]|=(tempbuf[j++]<<8);
                     }
                     if (k&4) {
                         fsyncangvel[other]=tempbuf[j++];
                         fsyncangvel[other]|=(tempbuf[j++]<<8);
                     }
//** Les END   - 09/27/95
                    if (k&8) fsyncbits[other] = ((fsyncbits[other]&0xff00)|((short)tempbuf[j++]));
                    if (k&16) fsyncbits[other] = ((fsyncbits[other]&0x00ff)|(((short)tempbuf[j++])<<8));
                    break;
               case 2:
                    getmessageleng = tempbufleng-1;
                    for(j=getmessageleng-1;j>=0;j--) getmessage[j] = tempbuf[j+1];
                    getmessagetimeoff = totalclock+360+(getmessageleng<<4);
                    break;
               case 3:
                    break;
              #ifdef NETNAMES
               case 8:
                    memcpy(netnames[tempbuf[1]],&tempbuf[2],10);
                    netnames[tempbuf[1]][10]=0;
                    break;
              #endif
               case 5:
                    playerreadyflag[other] = tempbuf[1];
                    if ((other == connecthead) && (tempbuf[1] == 2))
                         sendpacket(connecthead,tempbuf,2);
                    break;
               case 255:  //[255] (logout)
                    deletesprite(playersprite[other]);
                    sprintf(tektempbuf,"%2d %8s HAS QUIT", other,netnames[other]);
                    showmessage(tektempbuf);
                    break;
          }
     }
}

void
waitforeverybody()
{
     int i, j, oldtotalclock;

     if (numplayers < 2) return;

     if (myconnectindex == connecthead)
     {
          for(j=1;j<=2;j++)
          {
               for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                    playerreadyflag[i] = 0;
               oldtotalclock = totalclock-8;
               do
               {
                    getpackets();
                    if (totalclock >= oldtotalclock+8)
                    {
                         oldtotalclock = totalclock;
                         tempbuf[0] = 5;
                         tempbuf[1] = j;
                         for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                              if (playerreadyflag[i] != j) sendpacket(i,tempbuf,2);
                    }
                    for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
                         if (playerreadyflag[i] != j) break;
               } while (i >= 0);
          }
     }
     else
     {
          playerreadyflag[connecthead] = 0;
          while (playerreadyflag[connecthead] != 2)
          {
               getpackets();
               if (playerreadyflag[connecthead] == 1)
               {
                    playerreadyflag[connecthead] = 0;
                    sendpacket(connecthead,tempbuf,2);
               }
          }
     }
}

#if 0
unsigned short
getsyncstat()
{
     int i, j;
     unsigned short crc;
     spritetype *spr;

     crc = 0;
     updatecrc16(crc,randomseed); updatecrc16(crc,randomseed>>8);
     for(i=connecthead;i>=0;i=connectpoint2[i])
     {
          updatecrc16(crc,posx[i]);
          updatecrc16(crc,posy[i]);
          updatecrc16(crc,posz[i]);
          updatecrc16(crc,ang[i]);
          updatecrc16(crc,horiz[i]);
          updatecrc16(crc,health[i]);
     }

    #if SPRITES_CRC_CHECK
     for( i=1000; i>=0; i-- ) {
          for( j=headspritestat[i]; j>=0; j=nextspritestat[j] ) {
               spr = &sprite[j];
               updatecrc16(crc,spr->x); //if (syncstat != 0) printf("%ld ",spr->x);
               updatecrc16(crc,spr->y); //if (syncstat != 0) printf("%ld ",spr->y);
               updatecrc16(crc,spr->z); //if (syncstat != 0) printf("%ld ",spr->z);
               updatecrc16(crc,spr->ang); //if (syncstat != 0) printf("%ld ",spr->ang);
          }
     }
    #endif

     return(crc);
}
#endif


static int osdcmd_showfps(const osdfuncparm_t *parm)
{
    if (parm->numparms == 1) {
        if (parm->numparms == 1) showfps = min(1,(unsigned)atoi(parm->parms[0]));
        buildprintf("showfps is %u\n", showfps);
        return OSDCMD_OK;
    }
    return OSDCMD_SHOWHELP;
}

static int osdcmd_vidmode(const osdfuncparm_t *parm)
{
    int newx = xdim, newy = ydim, newbpp = bpp, newfullscreen = fullscreen;

    if (!strcasecmp(parm->name, "restartvid")) {
        resetvideomode();
    } else {
        if (parm->numparms < 1 || parm->numparms > 4) return OSDCMD_SHOWHELP;
        switch (parm->numparms) {
            case 1:   // bpp switch
                newbpp = atoi(parm->parms[0]);
                break;
            case 2: // res switch
                newx = atoi(parm->parms[0]);
                newy = atoi(parm->parms[1]);
                break;
            case 3:   // res, bpp, fullscreen, display switch
            case 4:
            case 5:
                newx = atoi(parm->parms[0]);
                newy = atoi(parm->parms[1]);
                newbpp = atoi(parm->parms[2]);
                if (parm->numparms == 4) {
                    newfullscreen = (atoi(parm->parms[3]) != 0);
                } else if (parm->numparms == 5) {
                    newfullscreen = (atoi(parm->parms[3]) != 0) | (max(0,atoi(parm->parms[4]))<<8);
                }
                break;
        }
        if (checkvideomode(&newx, &newy, newbpp, newfullscreen, 1) < 0) {
            buildputs("vidmode: unrecognised video mode.\n");
            return OSDCMD_OK;
        }
    }

    if (setgamemode(newfullscreen,newx,newy,newbpp))
        buildputs("vidmode: Mode change failed!\n");
    else {
        xdimgame = newx;
        ydimgame = newy;
        bppgame = newbpp;
        fullscreen = newfullscreen;
        setup3dscreen();
    }
    return OSDCMD_OK;
}

static int osdcmd_setkeys(const osdfuncparm_t *parm)
{
    if (parm->numparms == 1) {
        if (!strcasecmp(parm->parms[0], "modern")) {
            memcpy(keys,modernkeys,sizeof(keys)-sizeof(keys[29]));
            buildputs("keys set to modern\n");
            return OSDCMD_OK;
        }
        else if (!strcasecmp(parm->parms[0], "default")) {
            memcpy(keys,defaultkeys,sizeof(keys)-sizeof(keys[29]));
            buildputs("keys set to default\n");
            return OSDCMD_OK;
        }
    }
    return OSDCMD_SHOWHELP;
}

static const struct {
    const char *name;
    int key;
} keymap[] = {
     { "backward", 1 },
     { "turnleft", 2 },
     { "turnright", 3 },
     { "run", 4 },
     { "strafe", 5 },
     { "fire", 6 },
     { "use", 7 },
     { "jump", 8 },
     { "crouch", 9 },
     { "lookup", 10 },
     { "lookdown", 11 },
     { "centre", 19 },
     { "strafeleft", 12 },
     { "straferight", 13 },
     { "map", 14 },
     // { "viewcycle", 15 },
     { "zoomin", 16 },
     { "zoomout", 17 },
     { "chat", 18 },
     { "rearview", 20 },
     { "prepareditem", 21 },
     { "healthmeter", 22 },
     { "crosshairs", 23 },
     { "elapsedtime", 24 },
     { "score", 25 },
     { "inventory", 26 },
     { "conceal", 27 },
     { "looking", 28 },
     { "console", 29 },
};
static int matchkeyname(const char *name) {
    for (unsigned i=0; i<Barraylen(keymap); i++)
        if (!strcasecmp(name, keymap[i].name))
            return keymap[i].key;
    return -1;
}
static const char * matchkeynum(int key) {
    for (unsigned i=0; i<Barraylen(keymap); i++)
        if (keymap[i].key == key)
            return keymap[i].name;
    return NULL;
}

static int osdcmd_setting(const osdfuncparm_t *parm)
{
    int button, action;
    const char *name;

    if (!strcasecmp(parm->name, "mouselookmode")) {
        if (parm->numparms == 1) mouselookmode = (int)max(0,min(1,(unsigned)atoi(parm->parms[0])));
        buildprintf("mouselookmode is %d\n", mouselookmode);
        return OSDCMD_OK;
    }
    else if (!strcasecmp(parm->name, "mousespeed")) {
        if (parm->numparms == 2) {
            moreoptions[11] = max(1,min(16,(unsigned)atoi(parm->parms[0])));
        }
        buildprintf("mousespeed is %d\n", moreoptions[11]);
        return OSDCMD_OK;
    }
    else if (!strcasecmp(parm->name, "mousebutton")) {
        if (parm->numparms == 1 && !strcasecmp("list", parm->parms[0])) {
            buildputs("mouse buttons\n");
            for (button=0; button<2; button++) {
                name = matchkeynum(moreoptions[button+1]);
                buildprintf(" %u ... %s\n", button+1, name ? name : "");
            }
            return OSDCMD_OK;
        } else if (parm->numparms == 2) {
            button = atoi(parm->parms[0]);
            action = matchkeyname(parm->parms[1]);
            if (button < 1 || button > 2) return OSDCMD_SHOWHELP;
            if (action < 0 || action == 29) return OSDCMD_SHOWHELP;
            moreoptions[button-1] = action;
            return OSDCMD_OK;
        }
    }
    else if (!strcasecmp(parm->name, "joystickbutton")) {
        if (parm->numparms == 1 && !strcasecmp("list", parm->parms[0])) {
            buildputs("joystick buttons\n");
            for (button=0; button<4; button++) {
                const char *name = matchkeynum(moreoptions[button+4]);
                buildprintf(" %u ... %s\n", button+1, name ? name : "");
            }
            return OSDCMD_OK;
        } else if (parm->numparms == 2) {
            button = atoi(parm->parms[0]);
            action = matchkeyname(parm->parms[1]);
            if (button < 1 || button > 4) return OSDCMD_SHOWHELP;
            if (action < 0 || action == 29) return OSDCMD_SHOWHELP;
            moreoptions[button-4] = action;
            return OSDCMD_OK;
        }
    }
    else if (!strcasecmp(parm->name, "keybutton")) {
        if (parm->numparms == 1 && !strcasecmp("list", parm->parms[0])) {
            buildputs("key buttons\n");
            for (int i=0; i < (int)Barraylen(keymap); i++)
                buildprintf(" %-11s ... %02X\n", keymap[i].name, keys[keymap[i].key]);
            return OSDCMD_OK;
        } else if (parm->numparms == 2) {
            button = (int)strtoul(parm->parms[0], NULL, 16);
            action = matchkeyname(parm->parms[1]);
            if (button < 2 || button > 255) return OSDCMD_SHOWHELP;
            if (action < 0) return OSDCMD_SHOWHELP;
            keys[action] = button;
            if (action == 29) OSD_CaptureKey(button);
            return OSDCMD_OK;
        }
    }
    return OSDCMD_SHOWHELP;
}
