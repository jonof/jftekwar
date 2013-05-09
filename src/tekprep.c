/***************************************************************************
 *   TEKPREP.C -                                                           *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include "build.h"
#include "names.h"

#include "tekwar.h"

#pragma aux copybuf =         \
	"rep movsd",             \
	parm [esi][edi][ecx]     \

#define fillsprite(newspriteindex2,x2,y2,z2,cstat2,shade2,pal2,            \
		clipdist2,xrepeat2,yrepeat2,xoffset2,yoffset2,picnum2,ang2,      \
		xvel2,yvel2,zvel2,owner2,sectnum2,statnum2,lotag2,hitag2,extra2) \
{                                                                          \
	spritetype *spr2;                                                     \
	spr2 = &sprite[newspriteindex2];                                      \
	spr2->x = x2; spr2->y = y2; spr2->z = z2;                             \
	spr2->cstat = cstat2; spr2->shade = shade2;                           \
	spr2->pal = pal2; spr2->clipdist = clipdist2;                         \
	spr2->xrepeat = xrepeat2; spr2->yrepeat = yrepeat2;                   \
	spr2->xoffset = xoffset2; spr2->yoffset = yoffset2;                   \
	spr2->picnum = picnum2; spr2->ang = ang2;                             \
	spr2->xvel = xvel2; spr2->yvel = yvel2; spr2->zvel = zvel2;           \
	spr2->owner = owner2;                                                 \
	spr2->lotag = lotag2; spr2->hitag = hitag2; spr2->extra = -1;         \ 
	copybuf(&spr2->x,&osprite[newspriteindex2].x,3);                      \
}

#define   lm(_str_) printf(" %s...\n", _str_);

#pragma aux setvmode =        \
 	"int 0x10",              \
	parm [eax]               \

#pragma aux mulscale =        \
	"imul ebx",              \
	"shrd eax, edx, cl",     \
	parm [eax][ebx][ecx]     \
	modify [edx]             \

extern    void      cdpreinit(void);
extern    int       ovmode;
extern    char      moreoptions[];
extern    char      toggles[];
extern    int       difficulty,soundv, musicv,mousesensitivity,headbobon;
extern    char      activemenu;
extern    char      generalplay;
extern    char      singlemapmode;

void      placerandompic(long picnum);

sectortype          *sectptr[MAXSECTORS];
spritetype          *sprptr[MAXSPRITES];
walltype            *wallptr[MAXWALLS];
struct    spriteextension     spriteXT[MAXSPRITES];
struct    spriteextension     *sprXTptr[MAXSPRITES];
long      startx,starty,startz,starta,starts;

#define   MAXSTARTSPOTS  16
int       startspotcnt;
struct    startspottype {
     long      x,y,z;
     short     sectnum;
};
struct    startspottype       startspot[MAXSTARTSPOTS];

extern    int  accessiblemap(int);
int       firsttimethru=1;

int       subwaysound[4];

prepareboard(char *daboardfilename)
{
	short     startwall, endwall, dasector;
	long      i, j, k, s, dax, day, daz, dax2, day2;
     int       rdonly;
     int       l;

     initsprites();   

     if( firsttimethru ) {
	     getmessageleng = 0;
	     typemessageleng = 0;
     	randomseed = 17L;
     }

	// clear (do)animation's list
	animatecnt = 0;
	typemode = 0;
	locselectedgun=1;
	locselectedgun2=1;

     // for when revision control has map files attrib +r set
     rdonly=( access(daboardfilename, W_OK) != 0 );
     if( rdonly ) {
          chmod(daboardfilename, S_IRWXU|S_IRWXG|S_IRWXO);
     }
	if (loadboard(daboardfilename,&posx[0],&posy[0],&posz[0],&ang[0],&cursectnum[0]) == -1)
	{
		musicoff();
		uninitmultiplayers();
		uninittimer();
		uninitkeys();
		uninitengine();
		uninitsb();
		setvmode(ovmode);        //Set back to text mode
		printf("Board not found\n");
		exit(0);
	}
     if( rdonly ) {
          chmod(daboardfilename, S_IROTH);
     }

     startx=posx[0];
     starty=posy[0];
     startz=posz[0];
     starta=ang[0];
     starts=cursectnum[0];

	for(i=0;i<MAXPLAYERS;i++)
	{
		posx[i] = posx[0];
		posy[i] = posy[0];
		posz[i] = posz[0];
		ang[i] = ang[0];
		cursectnum[i] = cursectnum[0];
		ocursectnum[i] = cursectnum[0];
		horiz[i] = 100;
		lastchaingun[i] = 0;
		health[i] = 100;
         #ifdef RESETSCORE
		score[i] = 0L;
         #endif
		dimensionmode[i] = 3;
		numbombs[i] = -1;
		zoom[i] = 768L;
		deaths[i] = 0L;
		playersprite[i] = -1;
		saywatchit[i] = -1;
		oposx[i] = posx[0];
		oposy[i] = posy[0];
		oposz[i] = posz[0];
		ohoriz[i] = horiz[0];
		ozoom[i] = zoom[0];
		oang[i] = ang[0];
	}

	setup3dscreen();

     if( firsttimethru ) {
     	olocvel = 0; olocvel2 = 0;
     	olocsvel = 0; olocsvel2 = 0;
     	olocangvel = 0; olocangvel2 = 0;
     	olocbits = 0; olocbits2 = 0;
     	for(i=0;i<MAXPLAYERS;i++) {
     		fsyncvel[i] = syncvel[i] = osyncvel[i] = 0;
     		fsyncsvel[i] = syncsvel[i] = osyncsvel[i] = 0;
     		fsyncangvel[i] = syncangvel[i] = osyncangvel[i] = 0;
     		fsyncbits[i] = syncbits[i] = osyncbits[i] = 0;
     	}
     }

	for(i=0;i<MAXPLAYERS;i++)
	{
		waterfountainwall[i] = -1;
		waterfountaincnt[i] = 0;
	}
	slimesoundcnt[i] = 0;
	warpsectorcnt = 0;      //Make a list of warping sectors
	xpanningsectorcnt = 0;  //Make a list of wall x-panning sectors
	floorpanningcnt = 0;    //Make a list of slime sectors
	dragsectorcnt = 0;      //Make a list of moving platforms
	swingcnt = 0;           //Make a list of swinging doors
	revolvecnt = 0;         //Make a list of revolving doors
	subwaytrackcnt = 0;     //Make a list of subways

     // intitialize subwaysound[]s
     for( i=0; i<4; i++ ) {
          subwaysound[i]=-1;
     }

	// scan sector tags
	for(i=0;i<numsectors;i++)
	{
		switch(sector[i].lotag)
		{
			case 4:
				floorpanninglist[floorpanningcnt++] = i;
				break;
               case 5060:
                    if( option[4] != 0 ) {
                         sector[i].lotag=0;
                    }
                    break;
               case 25:
                    if( (singlemapmode != 0) || (generalplay != 0) || (option[4] != 0) ) {
                         sector[i].lotag=0;
                    }
                    break;
			case 10:
                    if( (generalplay == 0) && (option[4] == 0) && (warpsectorcnt < 64) ) {
	                    warpsectorlist[warpsectorcnt++] = i;
                    }
				break;
			case 11:
				xpanningsectorlist[xpanningsectorcnt++] = i;
				break;
			case 12:
				dasector = i;
				dax = 0x7fffffff;
				day = 0x7fffffff;
				dax2 = 0x80000000;
				day2 = 0x80000000;
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dax) dax = wall[j].x;
					if (wall[j].y < day) day = wall[j].y;
					if (wall[j].x > dax2) dax2 = wall[j].x;
					if (wall[j].y > day2) day2 = wall[j].y;
					if (wall[j].lotag == 3) k = j;
				}
				if (wall[k].x == dax) dragxdir[dragsectorcnt] = -16;
				if (wall[k].y == day) dragydir[dragsectorcnt] = -16;
				if (wall[k].x == dax2) dragxdir[dragsectorcnt] = 16;
				if (wall[k].y == day2) dragydir[dragsectorcnt] = 16;

				dasector = wall[startwall].nextsector;
				dragx1[dragsectorcnt] = 0x7fffffff;
				dragy1[dragsectorcnt] = 0x7fffffff;
				dragx2[dragsectorcnt] = 0x80000000;
				dragy2[dragsectorcnt] = 0x80000000;
				startwall = sector[dasector].wallptr;
				endwall = startwall+sector[dasector].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dragx1[dragsectorcnt]) dragx1[dragsectorcnt] = wall[j].x;
					if (wall[j].y < dragy1[dragsectorcnt]) dragy1[dragsectorcnt] = wall[j].y;
					if (wall[j].x > dragx2[dragsectorcnt]) dragx2[dragsectorcnt] = wall[j].x;
					if (wall[j].y > dragy2[dragsectorcnt]) dragy2[dragsectorcnt] = wall[j].y;
				}

				dragx1[dragsectorcnt] += (wall[sector[i].wallptr].x-dax);
				dragy1[dragsectorcnt] += (wall[sector[i].wallptr].y-day);
				dragx2[dragsectorcnt] -= (dax2-wall[sector[i].wallptr].x);
				dragy2[dragsectorcnt] -= (day2-wall[sector[i].wallptr].y);

				dragfloorz[dragsectorcnt] = sector[i].floorz;

				dragsectorlist[dragsectorcnt++] = i;
				break;
			case 13:
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].lotag == 4)
					{
						k = wall[wall[wall[wall[j].point2].point2].point2].point2;
						if ((wall[j].x == wall[k].x) && (wall[j].y == wall[k].y))
						{     //Door opens counterclockwise
							swingwall[swingcnt][0] = j;
							swingwall[swingcnt][1] = wall[j].point2;
							swingwall[swingcnt][2] = wall[wall[j].point2].point2;
							swingwall[swingcnt][3] = wall[wall[wall[j].point2].point2].point2;
							swingangopen[swingcnt] = 1536;
							swingangclosed[swingcnt] = 0;
							swingangopendir[swingcnt] = -1;
						}
						else
						{     //Door opens clockwise
							swingwall[swingcnt][0] = wall[j].point2;
							swingwall[swingcnt][1] = j;
							swingwall[swingcnt][2] = lastwall(j);
							swingwall[swingcnt][3] = lastwall(swingwall[swingcnt][2]);
							swingwall[swingcnt][4] = lastwall(swingwall[swingcnt][3]);
							swingangopen[swingcnt] = 512;
							swingangclosed[swingcnt] = 0;
							swingangopendir[swingcnt] = 1;
						}
						for(k=0;k<4;k++)
						{
							swingx[swingcnt][k] = wall[swingwall[swingcnt][k]].x;
							swingy[swingcnt][k] = wall[swingwall[swingcnt][k]].y;
						}

						swingsector[swingcnt] = i;
						swingang[swingcnt] = swingangclosed[swingcnt];
						swinganginc[swingcnt] = 0;
						swingcnt++;
					}
				}
				break;
			case 14:
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				dax = 0L;
				day = 0L;
				for(j=startwall;j<=endwall;j++)
				{
					dax += wall[j].x;
					day += wall[j].y;
				}
				revolvepivotx[revolvecnt] = dax / (endwall-startwall+1);
				revolvepivoty[revolvecnt] = day / (endwall-startwall+1);

				k = 0;
				for(j=startwall;j<=endwall;j++)
				{
					revolvex[revolvecnt][k] = wall[j].x;
					revolvey[revolvecnt][k] = wall[j].y;
					k++;
				}
				revolvesector[revolvecnt] = i;
				revolveang[revolvecnt] = 0;


				revolvecnt++;
				break;
			case 15:
				subwaytracksector[subwaytrackcnt][0] = i;
				subwaystopcnt[subwaytrackcnt] = 0;
				dax = 0x7fffffff;
				day = 0x7fffffff;
				dax2 = 0x80000000;
				day2 = 0x80000000;
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dax) dax = wall[j].x;
					if (wall[j].y < day) day = wall[j].y;
					if (wall[j].x > dax2) dax2 = wall[j].x;
					if (wall[j].y > day2) day2 = wall[j].y;
				}
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].lotag == 5)
					{
          			     if( (wall[j].x > dax) && (wall[j].y > day) && (wall[j].x < dax2) && (wall[j].y < day2) ) {
						     subwayx[subwaytrackcnt] = wall[j].x;
						}
						else {
						     subwaystop[subwaytrackcnt][subwaystopcnt[subwaytrackcnt]] = wall[j].x;
                                   if( accessiblemap(wall[j].hitag) == 0 ) {
						          subwaystop[subwaytrackcnt][subwaystopcnt[subwaytrackcnt]]=0;
                                   }
						     subwaystopcnt[subwaytrackcnt]++;
						}
					}
				}
                    // de-sparse stoplist but keep increasing x order
                    for( j=0; j<subwaystopcnt[subwaytrackcnt]; j++ ) {
                         if( subwaystop[subwaytrackcnt][j] == 0 ) {
                              for( l=j+1; l<subwaystopcnt[subwaytrackcnt]; l++ ) {
                                   if( subwaystop[subwaytrackcnt][l] != 0 ) {
                                        subwaystop[subwaytrackcnt][j]=subwaystop[subwaytrackcnt][l];
                                        subwaystop[subwaytrackcnt][l]=0;
                                        l=subwaystopcnt[subwaytrackcnt];
                                   }
                              }
                         }
                    }
                    // recount stopcnt
                    subwaystopcnt[subwaytrackcnt]=0;
                    while( subwaystop[subwaytrackcnt][subwaystopcnt[subwaytrackcnt]] != 0 ) {
				     subwaystopcnt[subwaytrackcnt]++;
                    }

				for(j=1;j<subwaystopcnt[subwaytrackcnt];j++)
					for(k=0;k<j;k++)
						if (subwaystop[subwaytrackcnt][j] < subwaystop[subwaytrackcnt][k])
						{
							s = subwaystop[subwaytrackcnt][j];
							subwaystop[subwaytrackcnt][j] = subwaystop[subwaytrackcnt][k];
							subwaystop[subwaytrackcnt][k] = s;
						}

				subwaygoalstop[subwaytrackcnt] = 0;
				for(j=0;j<subwaystopcnt[subwaytrackcnt];j++)
					if (labs(subwaystop[subwaytrackcnt][j]-subwayx[subwaytrackcnt]) < labs(subwaystop[subwaytrackcnt][subwaygoalstop[subwaytrackcnt]]-subwayx[subwaytrackcnt]))
						subwaygoalstop[subwaytrackcnt] = j;

				subwaytrackx1[subwaytrackcnt] = dax;
				subwaytracky1[subwaytrackcnt] = day;
				subwaytrackx2[subwaytrackcnt] = dax2;
				subwaytracky2[subwaytrackcnt] = day2;

				subwaynumsectors[subwaytrackcnt] = 1;
				for(j=0;j<numsectors;j++)
					if (j != i)
					{
						startwall = sector[j].wallptr;
						if (wall[startwall].x > subwaytrackx1[subwaytrackcnt])
							if (wall[startwall].y > subwaytracky1[subwaytrackcnt])
								if (wall[startwall].x < subwaytrackx2[subwaytrackcnt])
									if (wall[startwall].y < subwaytracky2[subwaytrackcnt])
									{
										if (sector[j].lotag == 16)
											sector[j].lotag = 17;   //Make special subway door

										if (sector[j].floorz != sector[i].floorz)
										{
											sector[j].ceilingstat |= 64;
											sector[j].floorstat |= 64;
										}
										subwaytracksector[subwaytrackcnt][subwaynumsectors[subwaytrackcnt]] = j;
										subwaynumsectors[subwaytrackcnt]++;
									}
					}

				subwayvel[subwaytrackcnt] = 32;  // orig 64
				subwaypausetime[subwaytrackcnt] = 720;
				subwaytrackcnt++;
				break;
		}
	}

	// scan wall tags
	ypanningwallcnt = 0;
	for(i=0;i<numwalls;i++)
	{
		if (wall[i].lotag == 1) ypanningwalllist[ypanningwallcnt++] = i;
	}

	// scan sprite tags&picnum's
	rotatespritecnt = 0;
     startspotcnt=0;
	for(i=0;i<MAXSPRITES;i++)
	{
          if( sprite[i].picnum == STARTPOS ) {
               if( startspotcnt < MAXSTARTSPOTS ) {
                    startspot[startspotcnt].x=sprite[i].x;
                    startspot[startspotcnt].y=sprite[i].y;
                    startspot[startspotcnt].z=sprite[i].z;
                    startspot[startspotcnt].sectnum=sprite[i].sectnum;
                    startspotcnt++;
               }
               jsdeletesprite(i);
          }
          else if( sprite[i].lotag == 3 ) {
               rotatespritelist[rotatespritecnt++] = i;
          }
          else if( option[4] != 0 ) {
               if( sprite[i].lotag == 1009 ) {
                    jsdeletesprite(i);
               }
          }
	}
     if( (startspotcnt == 0) && (option[4] != 0) ) {
          crash("no net startspots");
     }

	for(i=0;i<(MAXSECTORS>>3);i++) show2dsector[i] = 0xff;
	for(i=0;i<(MAXWALLS>>3);i++) show2dwall[i] = 0xff;
     automapping = 0;  
     // tags that make wall/sector not show up on 2D map
     for( i=0; i < MAXSECTORS; i++ ) {
          if( sector[i].lotag == 9901 ) {
               show2dsector[i>>3] &= ~(1<<(i&7));
		     startwall = sector[i].wallptr;
			endwall = startwall+sector[i].wallnum-1;
			for( j=startwall; j<=endwall; j++) {
                    show2dwall[j>>3] &= ~(1<<(j&7));
                    show2dwall[(wall[j].nextwall)>>3] &= ~(1<<((wall[j].nextwall)&7));
			}
          }
     }
     for( i=0; i < MAXWALLS; i++ ) {
          if( wall[i].lotag == 9900 ) {
               show2dwall[i>>3] &= ~(1<<(i&7));
          }
     }

     if( firsttimethru ) {
	     lockclock = 0;
	     ototalclock = 0;
	     gotlastpacketclock = 0;
	     masterslavetexttime = 0;
     }

     if( option[4] != 0 ) {
          firsttimethru=0;
     }

     tekpreptags();
     initspriteXTs();

     // put guns somewhere on map
     if( option[4] > 0 ) {
          placerandompic(3094L);
          placerandompic(3093L);
          placerandompic(3095L);
          placerandompic(3091L);
          placerandompic(3090L);
     }
}

findrandomspot(long *x, long *y, short *sectnum)
{
	short startwall, endwall, s;
	long dax, day, minx, maxx, miny, maxy, cnt, k;

	cnt = 256;
	while (cnt > 0)
	{
		do
		{
            k = mulscale(krand_intercept("PREP 521"),numsectors,16);
		} while ((sector[k].ceilingz >= sector[k].floorz) || (sector[k].lotag != 0) || ((sector[k].floorstat&2) != 0));

		startwall = sector[k].wallptr;
		endwall = startwall+sector[k].wallnum-1;
		if (endwall > startwall)
		{
			dax = 0L;
			day = 0L;
			minx = 0x7fffffff; maxx = 0x80000000;
			miny = 0x7fffffff; maxy = 0x80000000;

			for(s=startwall;s<=endwall;s++)
			{
				dax += wall[s].x;
				day += wall[s].y;
				if (wall[s].x < minx) minx = wall[s].x;
				if (wall[s].x > maxx) maxx = wall[s].x;
				if (wall[s].y < miny) miny = wall[s].y;
				if (wall[s].y > maxy) maxy = wall[s].y;
			}
			if ((maxx-minx > 256) && (maxy-miny > 256))
			{
				dax /= (endwall-startwall+1);
				day /= (endwall-startwall+1);
				if (inside(dax,day,k) == 1)
				{
					*x = dax;
					*y = day;
					*sectnum = k;
					return;
				}
			}
		}
		cnt--;
	}
}

void
netstartspot(long *x, long *y,short *sectnum)
{
     int       rv;

     rv=( int)((krand_intercept("PREP 564"))&15L);
     if (rv >= startspotcnt) {
          rv=0;
     }
     *x=startspot[rv].x;
     *y=startspot[rv].y;
     *sectnum=startspot[rv].sectnum;
}

void
placerandompic(long picnum)
{
	short     startwall, endwall, s;
	long      dax, day, minx, maxx, miny, maxy, cnt, k;
     int       j;

	cnt = 256;
	while (cnt > 0)
	{
		do
		{
            k = mulscale(krand_intercept("PREP 585"),numsectors,16);
		} while ((sector[k].ceilingz >= sector[k].floorz) || (sector[k].lotag != 0) || ((sector[k].floorstat&2) != 0));

		startwall = sector[k].wallptr;
		endwall = startwall+sector[k].wallnum-1;
		if (endwall > startwall)
		{
			dax = 0L;
			day = 0L;
			minx = 0x7fffffff; maxx = 0x80000000;
			miny = 0x7fffffff; maxy = 0x80000000;

			for(s=startwall;s<=endwall;s++)
			{
				dax += wall[s].x;
				day += wall[s].y;
				if (wall[s].x < minx) minx = wall[s].x;
				if (wall[s].x > maxx) maxx = wall[s].x;
				if (wall[s].y < miny) miny = wall[s].y;
				if (wall[s].y > maxy) maxy = wall[s].y;
			}
			if ((maxx-minx > 256) && (maxy-miny > 256))
			{
				dax /= (endwall-startwall+1);
				day /= (endwall-startwall+1);
				if (inside(dax,day,k) == 1)
				{
                         j=jsinsertsprite(k,0);
                         if( j != -1 ) { 
	                         fillsprite(j,dax,day,sector[k].floorz,0,-8,0,
                                         12,16,16,0,0,0,0,
                                         0,0,0,0,k,0,0,0,-1);
                              sprite[j].z-=((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<1);
                              switch( picnum ) {
                              }
                              sprite[j].picnum=picnum;
                         }
					return;
				}
			}
		}
		cnt--;
	}
}

extern
short dieframe[];

tekrestoreplayer(short snum)
{
	setsprite(playersprite[snum],posx[snum],posy[snum],posz[snum]+(KENSPLAYERHEIGHT<<8));
	sprite[playersprite[snum]].ang = ang[snum];
	sprite[playersprite[snum]].xrepeat = 24;
	sprite[playersprite[snum]].yrepeat = 24;
	horiz[snum] = 100;
     health[snum]=MAXHEALTH;
     fireseq[snum]=0;
     restockammo(snum);
     stun[snum]=MAXSTUN;
     fallz[snum]=0;
     drawweap[snum]=0;
     invredcards[snum]=0;
     invbluecards[snum]=0;
     invaccutrak[snum]=0;
     dieframe[snum]=0;
     if( option[4] != 0 ) {
          weapons[snum]=0;
          weapons[snum]=flags32[GUN2FLAG]|flags32[GUN3FLAG];
     }
     else {
          weapons[snum]=0;
          weapons[snum]=flags32[GUN1FLAG]|flags32[GUN2FLAG]|flags32[GUN3FLAG];
     }
}

initplayersprite(short snum)
{
	long      i;

	if (playersprite[snum] >= 0) return;

     i=jsinsertsprite(cursectnum[snum], 8);
     if( i == -1 ) {
          crash("initplayersprite: jsinsertsprite on player %d failed", snum);
     }
     playersprite[snum]=i;
     sprite[i].x=posx[snum];
     sprite[i].y=posy[snum];
     sprite[i].z=posz[snum]+(KENSPLAYERHEIGHT<<8);
     sprite[i].cstat = 0x101;
     sprite[i].shade=0;
     if( option[4] == 0 ) {
          sprite[i].pal=0;
     }
     else {
          sprite[i].pal=snum+16;
     }
     sprite[i].clipdist=32;
     sprite[i].xrepeat=24;
     sprite[i].yrepeat=24;
     sprite[i].xoffset=0;
     sprite[i].yoffset=0;
     sprite[i].picnum=DOOMGUY;
     sprite[i].ang=ang[snum];
     sprite[i].xvel=0;
     sprite[i].yvel=0;
     sprite[i].zvel=0;
     sprite[i].owner=snum+4096;
     sprite[i].sectnum=cursectnum[snum];
     sprite[i].statnum=8;
     sprite[i].lotag=0;
     sprite[i].hitag=0;
     // important to set extra = -1
     sprite[i].extra=-1;     

     tekrestoreplayer(snum);
}

initptrlists()
{
     int       i;

     for( i=0; i < MAXSPRITES; i++ ) {
          sprptr[i]=&sprite[i];
          sprXTptr[i]=&spriteXT[i];
     }
     for( i=0; i < MAXSECTORS; i++ ) {
          sectptr[i]=&sector[i];
     }
     for( i=0; i < MAXWALLS; i++ ) {
          wallptr[i]=&wall[i];
     }

     return 0;
}

tekpreinit(void)
{
     int  i,j,k,l;
     FILE *fp;

     cdpreinit();

     if( (fp=fopen("lookup.dat","rb")) != NULL ) {  
          l=getc(fp);
          for (j=0 ; j < l ; j++) {
               k=getc(fp);
               for (i=0 ; i < 256 ; i++) {
                    tempbuf[i]=getc(fp);
               }
               makepalookup(k,tempbuf,0,0,0,1);
          }
          fclose(fp);
     }
     if( (option[4] != 0) && ((fp=fopen("nlookup.dat","rb")) != NULL) ) {  
          l=getc(fp);
          for (j=0 ; j < l ; j++) {
               k=getc(fp);
               for (i=0 ; i < 256 ; i++) {
                    tempbuf[i]=getc(fp);
               }
               makepalookup(k+15,tempbuf,0,0,0,1);
          }
          fclose(fp);
     }

     makepalookup(255,tempbuf,60,60,60,1);

     pskyoff[0]=0;  // 2 tiles
     pskyoff[1]=1;
     pskyoff[2]=2;
     pskyoff[3]=3;
     pskybits=2;    // tile 4 times, every 90 deg.
     parallaxtype=1;   
     parallaxyoffs=112;
     initptrlists();
     initpaletteshifts();
     initmenu();
     initmoreoptions();
     if( screensize == 0 ) {
          screensize=xdim+8;
     }
     activemenu=0;

     return 0;
}

#define   NUMSETOPTS          2

short     comp;
long      bps;

char *setopts[]={
	 "COM PORT:",
	 "BAUD RATE:"
};

void
mdmreadsettings(void)
{
	 short i,n;
	 long l;
	 char buf[80],*ptr;
	 FILE *fp;

	 fp=fopen("modem.dat","r");
	 if (fp == NULL) {
		  return;
	 }
	 while (fgets(buf,80,fp) != NULL) {
		  for (i=0 ; i < NUMSETOPTS ; i++) {
			   if ((ptr=strstr(buf,setopts[i])) != NULL) {
					ptr+=strlen(setopts[i]);
					switch (i) {
					case 0:   // com port
						 sscanf(ptr,"%d",&n);
						 if (n < 1 || n > 4) {
							  break;
						 }
						 comp=n;
						 break;
					case 1:   // baud rate
						 sscanf(ptr,"%ld",&l);
						 if (l < 2400L || l > 115200L) {
							  break;
						 }
						 bps=l;
						 break;
					}
			   }
		  }
	 }
	 fclose(fp);
}

tekinitmultiplayers()
{
     char      opt5=0;

     if( (option[4] > 0) && (option[4] < 5) ) {
          mdmreadsettings();
          switch( bps ) {
          case 2400:  opt5=0x00; break;
          case 4800:  opt5=0x01; break;
          case 14400: opt5=0x03; break;
          case 19200: opt5=0x04; break;
          case 28800: opt5=0x05; break;
          default:    opt5=0x02; break;
          }
          if( (comp == 1) || (comp == 3) ) {
               opt5|=0x20;
          }
          else {
               opt5|=0x10;
          }
	     initmultiplayers(comp,opt5);
     }
     else {
	     initmultiplayers(option[4],option[5]);
     }
}

extern
char palette[],
     palette1[256][3],
     palette2[256][3];

extern
int  noenemiesflag;

short mappic[]={
     1608,1609,1610,1611,1612,1613,1614
};

int  coopmode,
     switchlevelsflag;

void
teknetpickmap(void)
{
     short i,map=0;
     long lastclock=0L,rotangle=0L,zoom=0L;

     noenemiesflag=1;
     if (coopmode) {
          noenemiesflag=0;
     }
     if (switchlevelsflag) {
          strcpy(boardfilename,"NET1.MAP");
     }
     setgamemode();
     initpaletteshifts();
     memcpy(palette1, palette, 768);
     memset(palette, 0, 768);
     clearview(0);
     if (switchlevelsflag) {
          goto skippick;
     }
     lastclock=totalclock;
     do {
          if (keystatus[0x49] || keystatus[0x4B] || keystatus[0xC9] || keystatus[0xCB]) {
               if (map > 0) {
                    map--;
               }
               else {
                    map=6;
               }
               keystatus[0x49]=keystatus[0x4B]=0;
               keystatus[0xC9]=keystatus[0xCB]=0;
               zoom=0L;
          }
          if (keystatus[0x51] || keystatus[0x4D] || keystatus[0xD1] || keystatus[0xCD]) {
               if (map < 6) {
                    map++;
               }
               else {
                    map=0;
               }
               keystatus[0x51]=keystatus[0x4D]=0;
               keystatus[0xD1]=keystatus[0xCD]=0;
               zoom=0L;
          }
          zoom+=(totalclock-lastclock)<<10;
          rotangle+=((totalclock-lastclock)<<5);
          lastclock=totalclock;
          if (zoom > 65536L) {
               zoom=65536L;
               rotangle=0;
          }
          clearview(0);
          rotatesprite(xdim<<15,ydim<<15,zoom,rotangle,mappic[map],0,0,0);
          overwritesprite((xdim>>1)-160,0,408,0,0,0);
          sprintf(tempbuf,"MULTIPLAYER MAP %d",map+1);
          printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-16,tempbuf,ALPHABET2,255);
          nextpage();
     } while (keystatus[0x1C] == 0 && keystatus[0x9C] == 0 && keystatus[0x01] == 0);
     if (keystatus[0x1C] || keystatus[0x9C]) {
          keystatus[0x1C]=0;
          sprintf(boardfilename,"NET%d.MAP",map+1);
     }
     else if (keystatus[0x01]) {
          keystatus[0x01]=0;
          crash("Multiplayer game aborted!");
     }
skippick:
     prepareboard(boardfilename);
     precache();
     clearview(0);
     memcpy(palette, palette1, 768);
     fadein(0,255,16);
}

tekloadsetup()
{
	int  fil;
     
	if ((fil = open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
	{
		read(fil,&option[0],NUMOPTIONS);
		read(fil,&keys[0],NUMKEYS);
          tekloadmoreoptions(fil);
		close(fil);
	}
	switch( option[0] ) {
		case 0: initengine(0,chainxres[option[6]&15],chainyres[option[6]>>4]); break;
		case 1: initengine(1,vesares[option[6]&15][0],vesares[option[6]&15][1]); break;
		case 2: initengine(2,320L,200L); break;
		case 3: initengine(3,320L,200L); break;
		case 4: initengine(4,320L,200L); break;
		case 5: initengine(5,320L,200L); break;
		case 6: initengine(6,320L,200L); break;
	}
}

teksavesetup(void)
{
     int  fil;

     if ((fil=open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1) {
          if (option[4] < 5) {
               option[4]=0;
          }
          write(fil,&option[0],NUMOPTIONS);
          write(fil,&keys[0],NUMKEYS);
          teksavemoreoptions(fil);
          close(fil);
     }
}

tekview(long *x1,long *y1, long *x2,long *y2)
{
     if( screensize <= xdim ) {
          *y1+=16;
          *y2+=16;
     }
}
