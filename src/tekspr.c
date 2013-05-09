/***************************************************************************
 *   TEKSPR.C  - checktouch, analyze, etc. for sprites                     *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "string.h"
#include "build.h"
#include "names.h"

#include "tekwar.h"

#pragma aux mulscale =        \
	"imul ebx",              \
	"shrd eax, edx, cl",     \
	parm [eax][ebx][ecx]     \
	modify [edx]             \

// from tekstat
#define   FLOATING       322  
#define   PINBALL        403     

#define   AI_NULL        0x00
#define   AI_FRIEND      0x01
#define   AI_FOE         0x02
#define   AI_JUSTSHOTAT  0x04
#define   AI_CRITICAL    0x08
#define   AI_WASDRAWN    0x10

extern    int       difficulty;
extern    char      rearviewdraw;
extern    long      flags32[],weapons[];                                                  
extern    void      placerandompic(long picnum);

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



short
kenmovesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype)
{
	long daz, zoffs, templong;
	short retval, dasectnum, tempshort;
	spritetype *spr;

    #ifdef PLRSPRDEBUG
     if( isaplayersprite(spritenum) ) {
          crash("messing w plrsprite at 17");
     }
    #endif

	spr = &sprite[spritenum];

	if ((spr->cstat&128) == 0)
		zoffs = -((tilesizy[spr->picnum]*spr->yrepeat)<<1);
	else
		zoffs = 0;

	dasectnum = spr->sectnum;  //Can't modify sprite sectors directly becuase of linked lists
	daz = spr->z+zoffs;  //Must do this if not using the new centered centering (of course)
	retval = clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
							((long)spr->clipdist)<<2,ceildist,flordist,cliptype);

	if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
		changespritesect(spritenum,dasectnum);

		//Set the blocking bit to 0 temporarly so getzrange doesn't pick up
		//its own sprite
	tempshort = spr->cstat; spr->cstat &= ~1;
	getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,
				 &globhiz,&globhihit,&globloz,&globlohit,
				 ((long)spr->clipdist)<<2,cliptype);
	spr->cstat = tempshort;

	daz = spr->z+zoffs + dz;
	if ((daz <= globhiz) || (daz > globloz))
	{
		if (retval != 0) return(retval);
		return(16384+dasectnum);
	}
	spr->z = daz-zoffs;
	return(retval);
}

short
floatmovesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype)
{
	long daz, zoffs, templong;
	short retval, dasectnum, tempshort;
	spritetype *spr;

    #ifdef PLRSPRDEBUG
     if( isaplayersprite(spritenum) ) {
          crash("messing w plrsprite at 18");
     }
    #endif

	spr = &sprite[spritenum];

	if ((spr->cstat&128) == 0)
		zoffs = -((tilesizy[spr->picnum]*spr->yrepeat)<<1);
	else
		zoffs = 0;

	dasectnum = spr->sectnum;  //Can't modify sprite sectors directly becuase of linked lists
	daz = spr->z+zoffs;  //Must do this if not using the new centered centering (of course)
	retval = clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
							((long)spr->clipdist)<<2,ceildist,flordist,cliptype);

	if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
		changespritesect(spritenum,dasectnum);

	return(retval);
}

short
movesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype)
{
	long           daz,zoffs;
     long           jumpz,deltaz;
     long           px,py,pz;
	short          retval,dasectnum,tempshort;
     short          failedsectnum;
	spritetype     *spr;

    #ifdef PLRSPRDEBUG
     if( isaplayersprite(spritenum) ) {
          crash("messing w plrsprite at 19");
     }
    #endif

	spr = &sprite[spritenum];

	if ((spr->cstat&128) == 0)
		zoffs = -((tilesizy[spr->picnum]*spr->yrepeat)<<1);
	else
		zoffs = 0;

	dasectnum = spr->sectnum;
     px=spr->x;
     py=spr->y;
     pz=spr->z;
	daz = spr->z+zoffs; 
	retval = clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
							((long)spr->clipdist)<<2,ceildist,flordist,cliptype);
	if( (dasectnum != spr->sectnum) && (dasectnum >= 0) ) {
		changespritesect(spritenum,dasectnum);
     }

     if( (sectptr[dasectnum]->lotag == 4) && (spr->extra != -1) ) {
          switch( spr->statnum ) {
          case FLOATING:
               break;
          default:
               spr->z=sectptr[spr->sectnum]->floorz-zoffs;
               break;
          }
          return(retval);
     }

	tempshort = spr->cstat; spr->cstat &= ~1;
	getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,
				 &globhiz,&globhihit,&globloz,&globlohit,
				 ((long)spr->clipdist)<<2,cliptype);
	spr->cstat = tempshort;
	daz = spr->z+zoffs + dz;
	if( (daz <= globhiz) || (daz > globloz) ) {
		if( retval != 0 ) {
               return(retval);
          }
          else {
		     return(16384+dasectnum);
          }
	}
	if( (globloz != pz) && (spr->extra >= 0) && (spr->extra < MAXSPRITES) ) {
          spr->z=globloz;
          deltaz=labs(pz-globloz);
          jumpz=tilesizy[spr->picnum]+(spr->yrepeat-64);
          jumpz<<=8;
          if( deltaz > jumpz ) {
               failedsectnum=spr->sectnum;
               setsprite(spritenum,px,py,pz);
               retval=failedsectnum+16384;
          }
     }
     else {
	     spr->z = daz-zoffs;
     }

	return(retval);
}

short
flymovesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype)
{
	long           daz, zoffs, templong;
	short          retval, dasectnum, tempshort;
	spritetype *spr;

    #ifdef PLRSPRDEBUG
     if( isaplayersprite(spritenum) ) {
          crash("messing w plrsprite at 20");
     }
    #endif

	spr = &sprite[spritenum];

	dasectnum = spr->sectnum; 
	retval = clipmove(&spr->x,&spr->y,&spr->z,&dasectnum,dx,dy,
							((long)spr->clipdist)<<2,ceildist,flordist,cliptype);

	if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
		changespritesect(spritenum,dasectnum);

     if( spr->statnum != PINBALL ) {
	     tempshort = spr->cstat; spr->cstat &= ~1;
	     getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,
	     			 &globhiz,&globhihit,&globloz,&globlohit,
	     			 ((long)spr->clipdist)<<2,cliptype);
	     spr->cstat = tempshort;
          daz=(globloz+globhiz);
          spr->z=(daz>>1);
     }
 
	return(retval);
}

analyzesprites(long dax, long day)
{
	long           i, j, k;
     int            ext;
	point3d        *ospr;
	spritetype     *tspr;

	for( i=0,tspr=&tsprite[0]; i<spritesortcnt; i++,tspr++ ) {

          ext=tspr->extra;
          if( validext(ext) ) {
               if( rearviewdraw == 0 ) {
                    sprXTptr[ext]->aimask|=AI_WASDRAWN;
               }
          }

	     k = getangle(tspr->x-dax,tspr->y-day);
		k = (((tspr->ang+3072+128-k)&2047)>>8)&7;

		switch( tspr->picnum ) {
		case DOOMGUY:
          case RUBWALKPIC:
          case FRGWALKPIC:
          case SAMWALKPIC:
          case COP1WALKPIC:
          case ANTWALKPIC:
          case SARAHWALKPIC:
          case MAWALKPIC:
          case ERWALKPIC:
          case DIWALKPIC:
          case RATPIC:
          case SUNGWALKPIC:
          case COWWALKPIC:
          case COPBWALKPIC:
          case NIKAWALKPIC:
          case REBRWALKPIC:
          case TRENWALKPIC:
          case WINGWALKPIC:
          case HALTWALKPIC:
          case REDHWALKPIC:
          case ORANWALKPIC:
          case BLKSWALKPIC:
          case SFROWALKPIC:
          case SSALWALKPIC:
          case SGOLWALKPIC:
          case SWATWALKPIC:
			if( k <= 4 ) {
			     tspr->picnum += (k<<2);
   			     tspr->cstat &= ~4;   //clear x-flipping bit
   			}
   			else {
   				tspr->picnum += ((8-k)<<2);
   				tspr->cstat |= 4;    //set x-flipping bit
   			}
   			break;
          case AUTOGUN:
		     if (k <= 4) {
				tspr->picnum += k;    
				tspr->cstat &= ~4; 
			}
			else {
				tspr->picnum += (8-k); 
				tspr->cstat |= 4;     
			}
			break;
          case JAKESTANDPIC:
          case RUBSTANDPIC:
          case FRGSTANDPIC:
          case SAMSTANDPIC:
          case COP1STNDPIC:
          case ANTSTANDPIC:
          case MASTANDPIC:
          case DISTANDPIC:
          case ERSTANDPIC:
          case SUNGSTANDPIC:
          case COWSTANDPIC:
          case COPBSTANDPIC:
          case NIKASTANDPIC:
          case REBRSTANDPIC:
          case TRENSTANDPIC:
          case WINGSTANDPIC:
          case HALTSTANDPIC:
          case REDHSTANDPIC:
          case ORANSTANDPIC:
          case BLKSSTANDPIC:
          case SFROSTANDPIC:
          case SSALSTANDPIC:
          case SGOLSTANDPIC:
          case SWATSTANDPIC:
          case PROBE1:
          case RS232:
               if (k <= 4) {           
                    tspr->picnum += k;
                    tspr->cstat &= ~4; 
               }
               else {
                    tspr->picnum += ((8-k));
                    tspr->cstat |= 4;
               }
			break;
          case RUBATAKPIC:
          case FRGATTACKPIC:
          case SAMATTACKPIC:
          case COPATTACKPIC:
          case ANTATTACKPIC:
          case MAATTACKPIC:
          case DIATTACKPIC:
          case SUNGATTACKPIC:
          case COWATTACKPIC:
          case COPBATTACKPIC:
          case NIKAATTACKPIC:
          case REBRATTACKPIC:
          case WINGATTACKPIC:
          case HALTATTACKPIC:
          case REDHATTACKPIC:
          case ORANATTACKPIC:
          case BLKSATTACKPIC:
          case SFROATTACKPIC:
          case SSALATTACKPIC:
          case SGOLATTACKPIC:
          case SWATATTACKPIC:
               if( k <= 4 ) {           
                    tspr->picnum += (k<<1);
				tspr->cstat &= ~4;   //clear x-flipping bit
               }
               else {
                    tspr->picnum += ((8-k)<<1);
                    tspr->cstat |= 4;
               }
			break;
          case JAKEPAINPIC:
          case RUBPAINPIC:
          case FRGPAINPIC:
          case SAMPAINPIC:
          case COP1PAINPIC:
          case ANTPAINPIC:
          case MAPAINPIC:
          case ERPAINPIC:
          case SUNGPAINPIC:
          case COWPAINPIC:
          case NIKAPAINPIC:
          case REBRPAINPIC:
          case TRENPAINPIC:
          case HALTPAINPIC:
          case ORANPAINPIC:
          case BLKSPAINPIC:
          case SFROPAINPIC:
          case SGOLPAINPIC:
          case SWATPAINPIC:
          case JAKEDEATHPIC:
          case JAKEDEATHPIC+1:
          case JAKEDEATHPIC+2:
          case JAKEDEATHPIC+3:
          case JAKEDEATHPIC+4:
          case JAKEDEATHPIC+5:
          case JAKEDEATHPIC+6:
          case JAKEDEATHPIC+7:
          case JAKEDEATHPIC+8:
               if( k <= 4 ) { 
                    tspr->picnum += (k);
				tspr->cstat &= ~4;   //clear x-flipping bit
               }
               else {
                    tspr->picnum += ((8-k));
                    tspr->cstat |= 4;
               }
			break;
          // mirrorman
          case 1079:  
          case 1074:
               if( k <= 4 ) { 
                    tspr->picnum += (k);
                    tspr->cstat |= 4;
               }
               else {
                    tspr->picnum += ((8-k));
				tspr->cstat &= ~4;   //clear x-flipping bit
               }
			break;
          case JAKEATTACKPIC:
          case JAKEATTACKPIC+1:
               if (k <= 4) {
                    tspr->picnum+=(k*3);
                    tspr->cstat&=~4;
               }
               else {
                    tspr->picnum+=((8-k)*3);
                    tspr->cstat|=4;
               }
               break;
          default:
               break;
		}

		k=tspr->statnum;
		if( (k >= 1) && (k <= 8) && (k != 2) ) {  //Interpolate moving sprite
			ospr = &osprite[tspr->owner];
			k = tspr->x-ospr->x; tspr->x = ospr->x;
			if (k != 0) tspr->x += mulscale(k,smoothratio,16);
			k = tspr->y-ospr->y; tspr->y = ospr->y;
			if (k != 0) tspr->y += mulscale(k,smoothratio,16);
			k = tspr->z-ospr->z; tspr->z = ospr->z;
			if (k != 0) tspr->z += mulscale(k,smoothratio,16);
		}

	}
}

checktouchsprite(short snum, short sectnum)
{
	long      i, nexti;
     int       healthmax;
     char      dosound=0;
     char      str[30];

	if( (sectnum < 0) || (sectnum >= numsectors) ) {
          return;
     }

     memset(str, 0, 30);

	i = headspritesect[sectnum];
	while (i != -1)
	{
		nexti = nextspritesect[i];
		if ((labs(posx[snum]-sprite[i].x)+labs(posy[snum]-sprite[i].y) < 512) && (labs((posz[snum]>>8)-((sprite[i].z>>8)-(tilesizy[sprite[i].picnum]>>1))) <= 40))
		{
               // must jive with tekinitplayer settings
               switch( difficulty ) {
               case 3:
                    healthmax=(MAXHEALTH>>1);
                    break;
               default:
                    healthmax=MAXHEALTH;
                    break;
               }
			switch( sprite[i].picnum ) {
               case MEDICKIT2PIC:
                    if (health[snum] < healthmax) {
                         changehealth(snum,100);
                         jsdeletesprite(i);
                         strcpy(str,"MEDIC KIT");
                         dosound++;
                    }
                    break;
               case MEDICKITPIC:
                    if (health[snum] < healthmax) {
                         strcpy(str,"MEDIC KIT");
                         changehealth(snum,250);
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3637:
                    if (health[snum] < healthmax) {
                         strcpy(str,"ENERGY PELLET");
                         changehealth(snum,50);
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case DONUTSPIC:
                    if (health[snum] < healthmax) {
                         strcpy(str,"MMMMMMM DONUTS");
                         changehealth(snum,50);
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case ACCUTRAKPIC:
                    invaccutrak[snum]=1;
                    strcpy(str,"ACCU TRAK");
                    jsdeletesprite(i);
                    dosound++;
                    break;
               // gun pick ups
               case 3092:
                    if( ((weapons[snum])&flags32[GUN2FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN2FLAG];
                         ammo2[snum]+=25;
                         if (ammo2[snum] > MAXAMMO) {
                              ammo2[snum]=MAXAMMO;
                         }
                         strcpy(str,"PISTOL");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3094:
                    if( ((weapons[snum])&flags32[GUN3FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN3FLAG];
                         ammo3[snum]+=15;
                         if (ammo3[snum] > MAXAMMO) {
                              ammo3[snum]=MAXAMMO;
                         }
                         strcpy(str,"SHRIKE DBK");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3093:
                    if( ((weapons[snum])&flags32[GUN4FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN4FLAG];
                         ammo4[snum]+=15;
                         if (ammo4[snum] > MAXAMMO) {
                              ammo4[snum]=MAXAMMO;
                         }
                         strcpy(str,"ORLOW");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3095:
                    if( ((weapons[snum])&flags32[GUN5FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN5FLAG];
                         ammo5[snum]+=15;
                         if (ammo5[snum] > MAXAMMO) {
                              ammo5[snum]=MAXAMMO;
                         }
                         strcpy(str,"EMP GUN");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3091:
                    if( ((weapons[snum])&flags32[GUN6FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN6FLAG];
                         ammo6[snum]+=15;
                         if (ammo6[snum] > MAXAMMO) {
                              ammo6[snum]=MAXAMMO;
                         }
                         strcpy(str,"FLAMER");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3090:
                    if( ((weapons[snum])&flags32[GUN7FLAG]) == 0 ) {
                         weapons[snum]|=flags32[GUN7FLAG];
                         ammo7[snum]+=10;
                         if (ammo7[snum] > MAXAMMO) {
                              ammo7[snum]=MAXAMMO;
                         }
                         strcpy(str,"BAZOOKA");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               // ammo pick ups
               case 924:
               case 3102:
                    if( (ammo2[snum] < MAXAMMO) ) { // && ((weapons[snum]&flags32[GUN2FLAG]) != 0) ) {
                         ammo2[snum]+=20;
                         if( ammo2[snum] > MAXAMMO ) {
                              ammo2[snum]=MAXAMMO;
                         }
                         strcpy(str,"PISTOL KLIP");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3104:
                    if( (ammo3[snum] < MAXAMMO) ) { // && ((weapons[snum]&flags32[GUN3FLAG]) != 0) ) {
                         ammo3[snum]+=10;
                         if( ammo3[snum] > MAXAMMO ) {
                              ammo3[snum]=MAXAMMO;
                         }
                         strcpy(str,"SHRIKE CHARGES");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3103:
                    if( (ammo4[snum] < MAXAMMO) ) { // && ((weapons[snum]&flags32[GUN4FLAG]) != 0) ) {
                         ammo4[snum]+=10;
                         if( ammo4[snum] > MAXAMMO ) {
                              ammo4[snum]=MAXAMMO;
                         }
                         strcpy(str,"ORLOW CHARGES");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 925:
               case 3101:
                    if( (ammo6[snum] < MAXAMMO) ) { // && ((weapons[snum]&flags32[GUN6FLAG]) != 0) ) {
                         ammo6[snum]+=10;
                         if( ammo6[snum] > MAXAMMO ) {
                              ammo6[snum]=MAXAMMO;
                         }
                         strcpy(str,"FUEL BOTTLE");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case CHARGEPAKPIC:
               case 3100:
                    if( (ammo7[snum] < MAXAMMO) ) { // && ((weapons[snum]&flags32[GUN7FLAG]) != 0) ) {
                         ammo7[snum]+=5;
                         if( ammo7[snum] > MAXAMMO ) {
                              ammo7[snum]=MAXAMMO;
                         }
                         strcpy(str,"BAZOOKA FUEL");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case 3836:
                    if( (ammo8[snum] < MAXAMMO) ) { 
                         ammo8[snum]+=25;
                         if( ammo8[snum] > MAXAMMO ) {
                              ammo8[snum]=MAXAMMO;
                         }
                         strcpy(str,"GLOVE CHARGE");
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;

#if 0
               case CHARGEPAKPIC:
                    if (health[snum] < healthmax) {
                         strcpy(str,"HEALTH CHARGE PACK");
                         changehealth(snum,50);
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
#endif
               case RED_KEYCARD:
                    if( invredcards[snum] == 0 ) {
                         strcpy(str,"RED KEY CARD");
                         invredcards[snum]=1;
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               case BLUE_KEYCARD:
                    if( invbluecards[snum] == 0 ) {
                         strcpy(str,"BLUE KEY CARD");
                         invbluecards[snum]=1;
                         jsdeletesprite(i);
                         dosound++;
                    }
                    break;
               default:
                    break;
			}
		}
          if( dosound && (screenpeek == snum) ) {
               playsound(S_PICKUP_BONUS,0,0,0,ST_UNIQUE);
               showmessage(str);
          }
          dosound=0;

		i = nexti;
     }
}

extern
int  switchlevelsflag;

operatesprite(short dasprite)
{
	long           datag;
     int            healthmax;
     long           dx,dy,dz;
     spritetype     *spr;
     int            pu;

     // from tekstat.c
     #define   SPR_LOTAG_PICKUP    2004

     if( (option[4] == 0) && (sprptr[dasprite]->lotag == SPR_LOTAG_PICKUP) ) {
          pu=pickupsprite(dasprite);
          switch( sprptr[dasprite]->picnum ) {
          case RATPIC:
               if( pu ) {
                    showmessage("LIVE RAT");
               }
               break;
          case TUBEBOMB+1:
          case DARTBOMB+1:
               if( pu ) {
                    showmessage("GRENADE");
               }
          default:
               break;
          }
          return;
     }

     switch( sprptr[dasprite]->picnum ) {
     case 1361:   // the witchaven poster for demo
          playsound(S_WITCH,sprite[dasprite].x,sprite[dasprite].y,0,ST_UNIQUE);
          break;
     case 592:   // the witchaven poster for demo
          playsound(S_FLUSH,sprite[dasprite].x,sprite[dasprite].y,0,ST_UNIQUE);
          break;
     default:
          break;
     }

     spr=sprptr[dasprite];
     datag=spr->lotag;
     switch (datag) {
     case 6:
          if ((sprptr[dasprite]->cstat&0x001) != 0) {
               setanimpic(&spr->picnum,TICSPERFRAME*3,4);
          }
          else {
               setanimpic(&spr->picnum,TICSPERFRAME*3,-4);
          }
          sprptr[dasprite]->cstat^=0x101;
          teksetdelayfunc(operatesprite,CLKIPS*4,dasprite);
          break;
     case 4:
          if (!switchlevelsflag) {
               break;
          }
          if (option[4] != 0 && (spr->picnum == 182 || spr->picnum == 803)) {
               playsound(S_WH_SWITCH,sprite[dasprite].x,sprite[dasprite].y,0,ST_UNIQUE);
               nextnetlevel();
          }
          break;
     }
}

#define   DROPSIES       406    
#define   MAXDROPANGLES  6
char      dropanglecnt;
short     dropangles[MAXDROPANGLES] = { 0, 1792, 512, 768, 1536, 1024 };

dropit(long x, long y, long z, short sect, int pic)
{
     int       j,ang;

     j=jsinsertsprite(sect, DROPSIES);
     if( j != -1 ) {
          ang=dropangles[dropanglecnt];
	     fillsprite(j,x,y,z,0,
                     0,0,12,
                     16,16,0,0,pic,ang,
		           sintable[(ang+2560)&2047]>>6,sintable[(ang+2048)&2047]>>6,
		           30L,0,sect,DROPSIES,0,0,0);
     }
     dropanglecnt++;
     if( dropanglecnt >= MAXDROPANGLES ) {
          dropanglecnt=0;
     }
}

playerdropitems(int snum)
{
     if( !validplayer(snum) ) {
          crash("dropitems on bad plrnum");
     }

     if( (weapons[snum]&flags32[GUN2FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],924);
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3102);
     }
     if( (weapons[snum]&flags32[GUN3FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3104);
     }
     if( (weapons[snum]&flags32[GUN4FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3103);
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3093);
     }
     if( (weapons[snum]&flags32[GUN5FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3095);
     }
     if( (weapons[snum]&flags32[GUN6FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],925);
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3101);
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3091);
     }
     if( (weapons[snum]&flags32[GUN7FLAG]) != 0 ) {
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3100);
          dropit(posx[snum],posy[snum],posz[snum]-(32<<8),cursectnum[snum],3090);
     }
}
