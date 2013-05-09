/***************************************************************************
 *   TEKWAR.H  -  for stuff that probably all modules would like to        *
 *                know about                                               *
 *                                                                         *
 ***************************************************************************/

#define TEKWAR 

#define BETA   1
#define DEBUG  1

#define CLKIPS      120
#define MSGBUFSIZE  40             
#define DOOMGUY     999
#define KENSPLAYERHEIGHT    34

#define TICSPERFRAME 3
#define MOVEFIFOSIZ 256

#define TEKTEMPBUFSIZE  256

#define   WASSTATIC extern

#define GAMECVARSGLOBAL
#ifdef  GAMECVARSGLOBAL
typedef struct
{
	long x, y, z;
} point3d;

#define KEYFIFOSIZ 64
extern    void (__interrupt __far *oldkeyhandler)();
extern    void __interrupt __far keyhandler(void);
extern    volatile char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
extern    volatile char readch, oldreadch, extended, keytemp;

WASSTATIC long vel, svel, angvel;
WASSTATIC long vel2, svel2, angvel2;

extern volatile long recsnddone, recsndoffs;
WASSTATIC long recording;

WASSTATIC long chainxres[4];
WASSTATIC long chainyres[11];
WASSTATIC long vesares[7][2];

#ifdef    GAMEC
#define NUMOPTIONS 8
#define NUMKEYS 19
WASSTATIC char option[NUMOPTIONS];
WASSTATIC char keys[NUMKEYS];
WASSTATIC long digihz[7];
#endif

#ifdef    TEKWAR
#define   NUMOPTIONS          8
#define   NUMKEYS             32
#define   MAXMOREOPTIONS      21
#define   MAXTOGGLES          16
#define   MAXGAMESTUFF        16
extern    char option[NUMOPTIONS];
extern    char keys[NUMKEYS];
extern    char moreoptions[MAXMOREOPTIONS];
extern    char toggles[MAXTOGGLES];
extern    int  gamestuff[MAXGAMESTUFF];
extern    int  krand_intercept(char *);
#endif

WASSTATIC char frame2draw[MAXPLAYERS];
WASSTATIC long frameskipcnt[MAXPLAYERS];
WASSTATIC char gundmost[320];

#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32
WASSTATIC char lavabakpic[(LAVASIZ+2)*(LAVASIZ+2)], lavainc[LAVASIZ];
WASSTATIC long lavanumdrops, lavanumframes;
WASSTATIC long lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
WASSTATIC long lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
WASSTATIC long lavaradx[32][128], lavarady[32][128], lavaradcnt[32];

	//Shared player variables
WASSTATIC long posx[MAXPLAYERS], posy[MAXPLAYERS], posz[MAXPLAYERS];
WASSTATIC long horiz[MAXPLAYERS], zoom[MAXPLAYERS], hvel[MAXPLAYERS];
WASSTATIC short ang[MAXPLAYERS], cursectnum[MAXPLAYERS], ocursectnum[MAXPLAYERS];
WASSTATIC short playersprite[MAXPLAYERS], deaths[MAXPLAYERS];
WASSTATIC long lastchaingun[MAXPLAYERS];
WASSTATIC long health[MAXPLAYERS], score[MAXPLAYERS], saywatchit[MAXPLAYERS];
WASSTATIC short numbombs[MAXPLAYERS], oflags[MAXPLAYERS];
WASSTATIC char dimensionmode[MAXPLAYERS];
WASSTATIC char revolvedoorstat[MAXPLAYERS];
WASSTATIC short revolvedoorang[MAXPLAYERS], revolvedoorrotang[MAXPLAYERS];
WASSTATIC long revolvedoorx[MAXPLAYERS], revolvedoory[MAXPLAYERS];

	//ENGINE CONTROLLED MULTIPLAYER VARIABLES:
extern short numplayers, myconnectindex;
extern short connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)

	//Local multiplayer variables
WASSTATIC long locselectedgun;
WASSTATIC signed char locvel, olocvel;
WASSTATIC short locsvel, olocsvel;                          // Les 09/30/95
WASSTATIC short locangvel, olocangvel;                      // Les 09/30/95
WASSTATIC short locbits, olocbits;

	//Local multiplayer variables for second player
WASSTATIC long locselectedgun2;
WASSTATIC signed char locvel2, olocvel2;
WASSTATIC short locsvel2, olocsvel2;                        // Les 09/30/95
WASSTATIC short locangvel2, olocangvel2;                    // Les 09/30/95
WASSTATIC short locbits2, olocbits2;

  //Multiplayer syncing variables
WASSTATIC signed char fsyncvel[MAXPLAYERS], osyncvel[MAXPLAYERS], syncvel[MAXPLAYERS];
WASSTATIC short fsyncsvel[MAXPLAYERS], osyncsvel[MAXPLAYERS], syncsvel[MAXPLAYERS];  // Les 09/30/95
WASSTATIC short fsyncangvel[MAXPLAYERS], osyncangvel[MAXPLAYERS], syncangvel[MAXPLAYERS]; // Les 09/30/95
WASSTATIC unsigned short fsyncbits[MAXPLAYERS], osyncbits[MAXPLAYERS], syncbits[MAXPLAYERS];

WASSTATIC char frameinterpolate, detailmode, ready2send;
WASSTATIC long ototalclock, gotlastpacketclock, smoothratio;
WASSTATIC long oposx[MAXPLAYERS], oposy[MAXPLAYERS], oposz[MAXPLAYERS];
WASSTATIC long ohoriz[MAXPLAYERS], ozoom[MAXPLAYERS];
WASSTATIC short oang[MAXPLAYERS];

WASSTATIC point3d osprite[MAXSPRITESONSCREEN];

WASSTATIC long movefifoplc, movefifoend;
WASSTATIC signed char baksyncvel[MOVEFIFOSIZ][MAXPLAYERS];
WASSTATIC short baksyncsvel[MOVEFIFOSIZ][MAXPLAYERS];       // Les 09/30/95
WASSTATIC short baksyncangvel[MOVEFIFOSIZ][MAXPLAYERS];     // Les 09/30/95
WASSTATIC short baksyncbits[MOVEFIFOSIZ][MAXPLAYERS];

	//MULTI.OBJ sync state variables
extern char syncstate;
	//GAME.C sync state variables
WASSTATIC short syncstat;
WASSTATIC long syncvalplc, othersyncvalplc;
WASSTATIC long syncvalend, othersyncvalend;
WASSTATIC long syncvalcnt, othersyncvalcnt;
WASSTATIC short syncval[MOVEFIFOSIZ], othersyncval[MOVEFIFOSIZ];

extern long crctable[256];
#define updatecrc16(dacrc,dadat) dacrc = (((dacrc<<8)&65535)^crctable[((((unsigned short)dacrc)>>8)&65535)^dadat])
WASSTATIC char playerreadyflag[MAXPLAYERS];

	//Game recording variables
WASSTATIC long reccnt, recstat;
WASSTATIC signed char recsyncvel[16384][2];
WASSTATIC short recsyncsvel[16384][2];                      // Les 09/30/95
WASSTATIC short recsyncangvel[16384][2];                    // Les 09/30/95
WASSTATIC short recsyncbits[16384][2];

	//Miscellaneous variables
WASSTATIC char tempbuf[], boardfilename[80];
WASSTATIC short screenpeek, oldmousebstatus, brightness;
WASSTATIC short screensize, screensizeflag;
WASSTATIC short neartagsector, neartagwall, neartagsprite;
WASSTATIC long lockclock, neartagdist, neartaghitdist;
WASSTATIC long masterslavetexttime;
extern long frameplace, pageoffset, ydim16, chainnumpages;
WASSTATIC long globhiz, globloz, globhihit, globlohit;
extern long stereofps, stereowidth, stereopixelwidth;

	//Board animation variables
WASSTATIC short rotatespritelist[16], rotatespritecnt;
WASSTATIC short warpsectorlist[64], warpsectorcnt;
WASSTATIC short xpanningsectorlist[16], xpanningsectorcnt;
WASSTATIC short ypanningwalllist[64], ypanningwallcnt;
WASSTATIC short floorpanninglist[64], floorpanningcnt;
WASSTATIC short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
WASSTATIC long dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];
WASSTATIC short swingcnt, swingwall[32][5], swingsector[32];
WASSTATIC short swingangopen[32], swingangclosed[32], swingangopendir[32];
WASSTATIC short swingang[32], swinganginc[32];
WASSTATIC long swingx[32][8], swingy[32][8];
WASSTATIC short revolvesector[4], revolveang[4], revolvecnt;
WASSTATIC long revolvex[4][16], revolvey[4][16];
WASSTATIC long revolvepivotx[4], revolvepivoty[4];
WASSTATIC short subwaytracksector[4][128], subwaynumsectors[4], subwaytrackcnt;
WASSTATIC long subwaystop[4][8], subwaystopcnt[4];
WASSTATIC long subwaytrackx1[4], subwaytracky1[4];
WASSTATIC long subwaytrackx2[4], subwaytracky2[4];
WASSTATIC long subwayx[4], subwaygoalstop[4], subwayvel[4], subwaypausetime[4];
WASSTATIC short waterfountainwall[MAXPLAYERS], waterfountaincnt[MAXPLAYERS];
WASSTATIC short slimesoundcnt[MAXPLAYERS];

	//Variables that let you type messages to other player
WASSTATIC char getmessage[162], getmessageleng;
WASSTATIC long getmessagetimeoff;
WASSTATIC char typemessage[162], typemessageleng, typemode;
WASSTATIC char scantoasc[128];
WASSTATIC char scantoascwithshift[128];

	//These variables are for animating x, y, or z-coordinates of sectors,
	//walls, or sprites (They are NOT to be used for changing the [].picnum's)
	//See the setanimation(), and getanimategoal() functions for more details.
#define MAXANIMATES 512
WASSTATIC long *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
WASSTATIC long animatevel[MAXANIMATES], animateacc[MAXANIMATES], animatecnt;
#endif


#define   FT_FULLSCREEN   0
#define   FT_DATALINK     1

#define   MAXHEALTH      1000
#define   MAXSTUN        1000
#define   MAXAMMO        100

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

#define   MAPXT        1

struct    picattribtype  {
     char      numframes;
     char      animtype;
     signed    char      ycenteroffset,xcenteroffset;
     char      animspeed;
};
struct    spriteextension {
     char      class;
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
     char      lock;
     char      weapon;
     short     ext2;
     };

// use this function whenever you need to verify your 
// extension index
extern    int       validext(int ext);
extern    int       validplayer(int snum);
extern    short     jsinsertsprite(short sect, short stat);
extern    short     jsdeletesprite(short spritenum);
extern    volatile  int   seconds;
extern    volatile  int   minutes;
extern    volatile  int   hours;
extern    volatile  int   messageon;
extern    unsigned  criticalerror;
extern    char      messagebuf[MSGBUFSIZE];
extern    char      tektempbuf[TEKTEMPBUFSIZE];
extern    char      notininventory;
extern    struct    picattribtype       *picinfoptr;
extern    sectortype          *sectptr[MAXSECTORS];
extern    spritetype          *sprptr[MAXSPRITES];
extern    walltype            *wallptr[MAXWALLS];
extern    struct    spriteextension     spriteXT[MAXSPRITES];
extern    struct    spriteextension     *sprXTptr[MAXSPRITES];

// additional player arrays
extern    long      fallz[],stun[];
extern    long      invredcards[MAXPLAYERS], invbluecards[MAXPLAYERS];
extern    long      invaccutrak[MAXPLAYERS];
extern    int       fireseq[MAXPLAYERS];
extern    short     ammo1[MAXPLAYERS];
extern    short     ammo2[MAXPLAYERS];
extern    short     ammo3[MAXPLAYERS];
extern    short     ammo4[MAXPLAYERS];
extern    short     ammo5[MAXPLAYERS];
extern    short     ammo6[MAXPLAYERS];
extern    short     ammo7[MAXPLAYERS];
extern    short     ammo8[MAXPLAYERS];
extern    long      weapons[MAXPLAYERS];
extern    char      dead[MAXPLAYERS];
extern    long      firedonetics[MAXPLAYERS];
extern    long      lastgun[MAXPLAYERS];
extern    int       drawweap[MAXPLAYERS];

extern    long      flags32[];                                                  

#define   TOGGLE_RETICULE     0
#define   TOGGLE_TIME         1
#define   TOGGLE_SCORE        2
#define   TOGGLE_SOUND        3
#define   TOGGLE_MUSIC        4
#define   TOGGLE_REARVIEW     5
#define   TOGGLE_UPRT         6
#define   TOGGLE_GODMODE      7
#define   TOGGLE_HEALTH       8
#define   TOGGLE_OVERSCAN     9
#define   TOGGLE_INVENTORY    10

extern    void crash(char *, ...);
extern    void getpicinfo(short);
extern    void showmessage(char *, ...);
extern    int  playsound(int,long,long,int,short);

#define   GUN1FLAG            1
#define   GUN2FLAG            2
#define   GUN3FLAG            3
#define   GUN4FLAG            4
#define   GUN5FLAG            5
#define   GUN6FLAG            6
#define   GUN7FLAG            7
#define   GUN8FLAG            8

#define   BASESONG      0

#define   ST_NULL                  0x0000
#define   ST_IMMEDIATE             0x0001
#define   ST_UPDATE                0x0002
#define   ST_NOUPDATE              0x0004
#define   ST_UNIQUE                0x0008
#define   ST_DELAYED               0x0010
#define   ST_VARPITCH              0x0020
#define   ST_BACKWARDS             0x0040
#define	ST_AMBUPDATE	          0x0060
#define	ST_VEHUPDATE             0x0080
#define   ST_TOGGLE                0x0100

#define   MAXSOUNDS                     16
#define   TOTALSOUNDS                   208

#define   S_WATERFALL       	          0
#define   S_ALARM        	          1

#define   S_SEWERLOOP1                  2          //sewer sounds
#define   S_SEWERLOOP2                  3 
#define   S_SEWERLOOP3                  4                  
#define   S_SEWERLOOP4                  5                  
#define   S_SEWERSPLASH1                6
#define   S_SEWERSPLASH2                7
#define   S_SEWERSPLASH3                8
#define   S_SPLASH2                     9
#define   S_SPLASH3                     10
#define   S_SPLASH                      11

#define   S_MATRIX1                     12        //matrix sounds
#define   S_MATRIX2                     13
#define   S_MATRIX3                     14
#define   S_MATRIX4                     15
#define   S_MATRIX5                     16
#define   S_MATRIX6                     17
#define   S_MATRIX7                     18
#define   S_MATRIX8                     19
#define   S_MATRIX9                     20
#define   S_MATRIX10                    21
#define   S_MATRIX11                    22
#define   S_MATRIX12                    23

#define   S_WH_SWITCH                   24        //warehouse
#define   S_WH_GRINDING                 25             
#define   S_WH_TRACKSTOP                26
#define   S_WH_STEAMPRESS               27
#define   S_WH_CLANK                    28
#define   S_WH_6                        29
#define   S_WH_7                        30
#define   S_WH_8                        31

#define   S_WAVES1                      32        //beach and city
#define   S_WAVES2                      33
#define   S_BIRDIES                     34

#define   S_STEAM                       35
#define   S_FIRELOOP                    36
#define   S_SPINNING_DOOR               37

#define   S_TRUCKHORN                   38
#define   S_CITY_AMBIENCE               39
#define   S_WIND                        40
#define   S_AMBULANCE                   41

#define   S_ROBOT_ACCESS                42        //matrix voices
#define   S_ROBOT_INTRUDER              43
#define   S_ROBOT_VIRUS                 44

#define   S_SUBWAY_TRACK                45
#define   S_ALARM2                      46
#define   S_ALARM3                      47

#define	S_STATUS1                     48        //menus and status	     
#define	S_STATUS2			          49	
#define	S_HEALTHMONITOR	          50  	
#define	S_REARMONITOR		          51
#define	S_MENUSOUND1		          52
#define	S_MENUSOUND2		          53 
#define	S_BEEP			          54
#define	S_BOOP			          55

#define	S_MATRIXDOOR1		          56      //doors
#define	S_MATRIXDOOR2		          57
#define	S_MATRIXDOOR3		          58	
#define	S_SIDEDOOR1		          59
#define	S_SIDEDOOR2		          60
#define	S_BIGSWINGOP		          61 
#define	S_BIGSWINGCL		          62 
#define	S_FLOOROPEN		          63
#define	S_BAYDOOR_OPEN		          64  
#define	S_BAYDOOR_CLOSE	          65
#define	S_BAYDOORLOOP		          66
#define	S_UPDOWNDR2_OP		          67
#define	S_UPDOWNDR2_CL		          68	
#define	S_DOORKLUNK	  	          69
#define   S_AIRDOOR                     70
#define   S_AIRDOOR_OPEN                71 
#define   S_AIRDOOR_CLOSE               72 
#define   S_ELEVATOR_DOOR               73

#define	S_SUBSTATIONLOOP              74        //vehicles     
#define	S_SUBWAYLOOP		          75
#define	S_SUBWAYSTART		          76 
#define	S_SUBWAYSTOP		          77
#define	S_TRUCKLOOP		          78
#define	S_TRUCKSTART		          79
#define	S_TRUCKSTOP		          80 
#define   S_TRAMBUSLOOP                 81
#define   S_BOATLOOP                    82
#define   S_CARTLOOP                    83
#define   S_FORKLIFTLOOP                84               


#define	S_RIC1                        85        //guns 
#define	S_RIC2			          86  
#define	S_WEAPON1			          87  
#define	S_WEAPON2			          88  
#define	S_WEAPON3			          89

#define   S_WEAPON4                     90
#define   S_WEAPON5                     91
#define   S_WEAPON6                     92
#define   S_WEAPON7                     93
#define   S_WEAPON8                     94



#define	S_ENEMYGUN1                   95		          
#define	S_ENEMYGUN2		          96
#define	S_ENEMYGUN3		          97
#define	S_ENEMYGUN4		          98
#define	S_WITCH			          99  
#define	S_PLATFORMSTART	          100
#define	S_PLATFORMSTOP		          101
#define	S_PLATFORMLOOP		          102
#define	S_FORCEFIELD1		          103
#define	S_FORCEFIELD2		          104
#define	S_AUTOGUN			          105
#define	S_AUTOGUNEXPLODE	          106
#define   S_BUSHIT       	          107
#define   S_FORCEFIELDHUMLOOP           108
#define   S_KEYCARDBLIP                 109 
#define   S_PICKUP_BONUS                110
#define   S_JUMP                        111
#define   S_GLASSBREAK1                 112
#define   S_GLASSBREAK2                 113                                
#define   S_EXPLODE1                    114
#define   S_EXPLODE2                    115
#define	S_SMALLGLASS1				116
#define	S_SMALLGLASS2				117
#define	S_GORE1					118
#define	S_GORE2					119
#define	S_GORE3					120
#define   S_FLUSH                       121
#define   S_REVOLVEDOOR                 122
#define	S_PAIN1			          123
#define	S_PAIN2			          124
#define	S_SCREAM1			          125
#define	S_SCREAM2			          126
#define	S_SCREAM3			          127
#define   S_PLAYERDIE    	          128
#define	S_MANDIE1			          129
#define	S_MANDIE2  		          130
#define	S_MANDIE3  		          131
#define	S_MANDIE4  		          132
#define	S_MANDIE5  		          133
#define	S_MANDIE6  		          134
#define	S_MANDIE7  		          135
#define	S_MANDIE8  		          136
#define	S_MANDIE9  		          137
#define	S_MANDIE10 		          138
#define	S_GIRLDIE1		          139
#define	S_GIRLDIE2		          140
#define	S_GIRLDIE3		          141
#define	S_GIRLDIE4		          142
#define	S_GIRLDIE5		          143
#define	S_GIRLDIE6		          144
#define   S_ANDROID_DIE                 145
#define   S_MATRIX_ATTACK               146
#define   S_MATRIX_ATTACK2              147
#define   S_MATRIX_DIE1                 148
#define   S_MATRIX_DIE2                 149
#define   S_TRANSITION                  150
#define   S_MALE_COMEONYOU          	151
#define   S_MALE_TAKEYOUINSLEEP     	152
#define   S_MALE_YOULOSER           	153
#define   S_MALE_YOUPUNK            	154
#define   S_MALE_YOUMORON           	155
#define   S_MALE_LIKESHOOTINGDUX    	156
#define   S_MALE_SCRAPEBOTBARREL    	157
#define   S_MALE_DONTHURT            	158
#define   S_MALE_DONTSHOOT            	159
#define   S_MALE_PLEASEDONTSHOOT       	160 
#define   S_MALE_DONTSHOOT2          	161
#define   S_MALE_PLEASEDONTSHOOT2     	162
#define   S_MALE_OHMYGOD                163
#define   S_MALE_GETDOWNTAKECOVER       164 
#define   S_MALE_HESGOTGUN              165
#define   S_MALE_IDONTBELIEVE           166
#define   S_MALE_RUNAWAY                167
#define   S_MALE_TAKECOVER              168
#define   S_MALE_HESGOTGUN2             169
#define   S_MALE_ICANTBELIEVE           170
#define   S_MALE_CALLTHEPOLICE          171
#define   S_MALE_HELPCALLPOLICE         172
#define   S_MALE_HEYBACKOFF         	173
#define   S_MALE_HESGOTAGUN         	174 
#define   S_FEM_RUNHEGOTGUN        	175
#define   S_FEM_RUN                     176
#define   S_FEM_EVRYRUNHEGOTGUN        	177
#define   S_FEM_CALLACOP                178      
#define   S_FEM_OHNO                    179
#define   S_FEM_MOVEHEKILLUS            180
#define   S_FEM_HESGOTAGUN1   	     181
#define   S_FEM_EEKRUN                  182
#define   S_FEM_PSYCHOGOTGUN            183
#define   S_FEM_OHMYGOD                 184
#define   S_FEM_HURRYOUTAMYWAY          185
#define   S_FEM_CALLAMBULANCE           186
#define   S_DIANE_DONTSHOOTP            187
#define   S_DIANE_HELPMEPLEASE          188
#define   S_DIANE_OHH                   189
#define   S_DIANE_PLEASEDONTSHOOT       190
#define   S_DIANE_DONTSHOOTP2           191
#define   S_KATIE_DONTSHOOT             192
#define   S_KATIE_PLEASEDONTSHOOT       193
#define   S_MAR_THINKYOUTAKE  	     194
#define   S_MAR_ISTHATALL     	     195
#define   S_MAR_KISSYOURASS   	     196
#define   S_GRD_WHATDOINGHERE 	     197
#define   S_GRD_IDLEAVE       	     198
#define   S_GRD_HANDSUP       	     199
#define   S_GRD_HOLDIT       	     	200
#define   S_GRD_DROPIT       	     	201
#define   S_DIM_WANTSOMETHIS        	202
#define   S_DIM_THINOUCANTAKEME     	203
#define   S_DIM_LAUGHTER               	204
#define   S_DIM_CANTSTOPTEKLORDS    	205
#define   S_DIM_TEKRULES            	206
#define   S_HOLOGRAMDIE                 207
