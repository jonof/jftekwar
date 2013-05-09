#define   TEKWAR
/***************************************************************************
*	NAME: EXAMPLE.C
**	COPYRIGHT:
**	"Copyright (c) 1994, by FORTE"
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 06/01/94
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*             06/20/94     Example  SENSE example interface code.
*             06/27/94              Modified for support for Windows
*                                   Tracking support.
*                                   (plus example video mode changes)
***************************************************************************/

#ifndef TEKWAR
#ifdef __WATCOMC__
#include <graph.h>

#elif defined(_WINDOWS)
#include <windows.h>
unsigned long win_memory;
#endif

#if defined(__WATCOMC__)  || defined(_WINDOWS)
#include "dpmi.h"
#endif
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#include "vfx1.h"
#ifndef TEKWAR
#include "vesa.h"
#endif

#define MAX_CONFIG_SIZE     256
#define MAX_REPORT_SIZE     256

#define DOS_ERROR           0xFF

#define PALETTE_SIZE        768 
#define MONO_320_200        0x13    

#define LEFT_EYE            0
#define RIGHT_EYE           1
#define BOTH_EYES           2

#define MAX_HTDS            6
#define MAX_HPDS            6

SYSTEM_CPKT *system_cfg;
VENDOR_CPKT *vendor_cfg;
HMS_CPKT    *hms_cfg;
HTD_CPKT    *htd_cfg;
HPD_CPKT    *hpd_cfg;

// Structures specific to what the application enables
// Set Video Mode for a "Something" mounted display 
struct svm_struct
{
SVM_CPKT    cfg;
NULL_CPKT   null_end;
};
typedef struct svm_struct SVM_STRUCT;

// Head tracker devices
struct htd_struct
{
short     yaw;
short     pitch;
short     roll;
};
typedef struct htd_struct HTD_STRUCT;

// Hand held devices
struct hpd_struct
{
short     yaw;
short     pitch;
short     roll;
char      buttons;
};
typedef struct hpd_struct HPD_STRUCT;

SVM_STRUCT      svm;
HTD_STRUCT far *htd_data[ MAX_HTDS ];
HPD_STRUCT far *hpd_data[ MAX_HPDS ];

#ifndef TEKWAR
union  REGS     regs;
struct SREGS    sregs;
#endif

// Fields required for ISA bus io.
int             vip_air_port;
int             vip_csr_port;

#ifndef TEKWAR
// Holding array for current VGA palette
char    vga_palette[ PALETTE_SIZE ];

// Fields required for MONO video example.
char    mono_colors[ PALETTE_SIZE ];

// Fields required for STEREO video example.
char    left_colors[ PALETTE_SIZE ];
char    right_colors[ PALETTE_SIZE ];
#endif

int     SenseStatus( void );
int     SenseGetConfiguration( int function, int *count, char *config_buffer);
int     SenseSetConfiguration( int function, char *config_buffer );
int     SenseReport( int function, int *count, char *report_buffer );
#ifndef TEKWAR
void    ShowMonoPattern( void );
void    PutPalette( int which_eye, char *pPalette );
void    GetPalette( char *pPalette );
void    VideoChannel( int stereo_mode, int page );
void    ShowStereoPattern( void );
void    BuildPalettes( void );
void    CleanUp( void );
#endif

#ifdef TEKWAR
#define DPMI_INT    0x31
#define MOUSE_INT   0x33

#define AX(r) ((r).x.eax)
#define BX(r) ((r).x.ebx)
#define CX(r) ((r).x.ecx)
#define DX(r) ((r).x.edx)
#define SI(r) ((r).x.esi)
#define DI(r) ((r).x.edi)

void *pdosmem;

short segment,
     selector;

int  sense_function;

char vfx1_cyberpuck;
char config_buffer[MAX_CONFIG_SIZE];
char *report_offset;
char report_buffer[MAX_REPORT_SIZE];

enum {
     OPT_RUN,
     OPT_SHOOT,
     OPT_USEOPEN,
     OPT_JUMP,
     OPT_CROUCH,
     OPT_MAP,
     OPT_MAPZOOMIN,
     OPT_MAPZOOMOUT,
     OPT_CONCEAL,
     NUMBUTOPTIONS
};

char *butkeystr[]={
     "BUTTON1:",
     "BUTTON2:",
     "BUTTON3:"
};

char *butoptstr[]={
     "RUN",
     "SHOOT",
     "USE_OPEN",
     "JUMP",
     "CROUCH",
     "MAP",
     "MAP_ZOOM_IN",
     "MAP_ZOOM_OUT",
     "CONCEAL_WEAPON"
};

char puckbuttons;

short butoptval[]={
      8,11,10, 0, 1,12, 4, 5, 7
};

#define   NUMPUCKBUTTONS      3

short puckpitch,
     puckroll,
     puckbutton[NUMPUCKBUTTONS];

FILE *fp;
char buf[256];

extern
union REGS regs;
extern
struct SREGS sregs;

extern
int  vfx1enabled;

static struct rminfo {
	long di;
    long si;
	long bp;
	long reserved_by_system;
	long bx;
	long dx;
	long cx;
	long ax;
	short flags;
	short es, ds, fs, gs, ip, cs, sp, ss;
} SENSE;

void *
allocDOS(unsigned nbytes, short *pseg, short *psel)
{
     unsigned npara=(nbytes+15)/16;
     void *pprot;

     pprot=NULL;
     *pseg=0;
     *psel=0;
     segread(&sregs);
     AX(regs)=0x0100;           // DPMI: Allocate DOS Memory
     BX(regs)=npara;            // number of paragraphs to alloc
     int386( DPMI_INT, &regs, &regs);
     if (regs.w.cflag == 0) {
          *pseg=AX(regs);
          *psel=DX(regs);
          pprot=(void *)((unsigned)*pseg<<4);
     }
     return(pprot);
}

void
VFX1MouseInt(struct rminfo *prmi)
{
     memset(&sregs,0,sizeof (sregs));
     memset(&regs,0,sizeof (regs));
     AX(regs)=0x0300;       // DPMI: simulate interrupt
     BX(regs)=MOUSE_INT;
     CX(regs)=0;
     DI(regs)=FP_OFF(prmi);
     sregs.es=FP_SEG(prmi);
     int386x(DPMI_INT,&regs,&regs,&sregs);
}

void
vfx1_read(short *tyaw,short *tpitch,short *troll,
     short *ppitch,short *proll,char *pbuttons)
{
     int  count,ret;

     if (ret=SenseReport(sense_function,&count,report_buffer)) {
          return;
     }
     *tyaw=htd_data[0]->yaw;
     *tpitch=htd_data[0]->pitch;
     *troll=htd_data[0]->roll;
     *ppitch=hpd_data[0]->pitch;
     *proll=hpd_data[0]->roll;
     *pbuttons=hpd_data[0]->buttons;
}
#endif

int     SenseStatus( void )
{
int function, result, i;

    for( i=MIN_SENSE_FUNCTION; i <= MAX_SENSE_FUNCTION; i++ )
        {
        result    = i;
        result   |= (SENSE_DRIVER_STATUS << 8);
        function  = SENSE_DRIVER_STATUS;
        function |= (i << 8);        

#if defined(__WATCOMC__)  || defined(_WINDOWS)

        memset(&SENSE, 0, sizeof(SENSE));
        SENSE.ax = function;			
        SENSE.bx = 0;
        SENSE.cx = 0;
	SENSE.dx = 0;
        VFX1MouseInt( &SENSE ); // Generate the INT 33

        if( SENSE.ax == result )        
            return( function & 0xFF00 );                   
#else           
        regs.x.cx = 0;
        regs.x.dx = 0;
        regs.x.bx = 0;
        regs.x.ax = function;
        int86( SENSE_VECTOR, &regs, &regs );
        
        if( regs.x.ax == result )
            return( function & 0xFF00 );
#endif         
        }
    
    return( 0 );
}

int     SenseGetConfiguration( int function, int *count, char *config_buffer )
{
#if defined(__WATCOMC__) || defined(_WINDOWS)    
char			*src,*dest;
int				i;

    memset(&SENSE, 0, sizeof(SENSE));
    SENSE.ax = function | GET_CONFIGURATION;			
    SENSE.cx = 0;
    SENSE.es = segment;
    SENSE.dx = 0;		
    VFX1MouseInt( &SENSE );

    src = (char *)pdosmem;  
    for( i=0; i < SENSE.cx; i++ )
        config_buffer[i] = src[i];
    
    *count = SENSE.cx;
    
    return( SENSE.ax );
#else    
    regs.x.ax = function | GET_CONFIGURATION;
    regs.x.cx = 0;
    regs.x.dx = FP_OFF( config_buffer );    
    sregs.es  = FP_SEG( config_buffer );

    int86x( SENSE_VECTOR, &regs, &regs, &sregs);

    *count = regs.x.cx;

    return( regs.x.ax );
#endif    
}

int     SenseSetConfiguration( int function, char *config_buffer )
{
#if defined(__WATCOMC__) || defined(_WINDOWS) 
char			*src,*dest;
int				i;

    dest = (char *)pdosmem;
    for (i = 0; i < MAX_CONFIG_SIZE; i++)
        dest[i] = config_buffer[i];

    memset(&SENSE, 0, sizeof(SENSE));
    SENSE.ax = function | SET_CONFIGURATION;			
    SENSE.cx = 0;
    SENSE.es = segment;
    SENSE.dx = 0;		

    VFX1MouseInt( &SENSE );

    return( SENSE.ax );
#else        
    regs.x.ax = function | SET_CONFIGURATION;
    regs.x.cx = 0;
    regs.x.dx = FP_OFF( config_buffer );    
    sregs.es  = FP_SEG( config_buffer );
    
    int86x( SENSE_VECTOR, &regs, &regs, &sregs);

    return( regs.x.ax );
#endif    
}

int     SenseReport( int function, int *count, char *report_buffer )
{
#if defined(__WATCOMC__)  || defined(_WINDOWS) 
char			*src;
int				i;

    memset(&SENSE, 0, sizeof(SENSE));
    SENSE.ax = function | REPORT;
    SENSE.cx = 0;
    SENSE.es = segment;
    SENSE.dx = 0;		
    VFX1MouseInt( &SENSE );

    src = (char *)pdosmem;
    for (i = 0; i < SENSE.cx; i++)
        report_buffer[i] = src[i];
        
    *count = SENSE.cx;
    
    return( SENSE.ax );
#else            
    regs.x.ax = function | REPORT;
    regs.x.cx = 0;
    regs.x.dx = FP_OFF( report_buffer );    
    sregs.es  = FP_SEG( report_buffer );
    
    int86x( SENSE_VECTOR, &regs, &regs, &sregs );

    *count = regs.x.cx;
    
    return( regs.x.ax );
#endif    
}

#ifndef TEKWAR
void    ShowMonoPattern( void )
{
int     color;
int     i,j;
char    *vga_screen;

// While waiting for a key press show a simple pattern    
    color      = 0;    
    vga_screen = (char *) SCREEN_BASE_LA;
    for( j=0; j < 199; j++ ) {
        for( i=0; i < 320; i++ )
            vga_screen[ j*320 + i ] = color;
        color++;
        }
        
    getch();
}

void    PutPalette( int which_eye, char *pPalette )
{
int		i;
char	*pTmp = pPalette;
char    palette_control_bits;

	/* turn on palette */
	outp( vip_air_port,VIP_PPMR );
	outp( vip_csr_port, 0xFF );

	// Enable write of proper selected palettes (right and/or left eyes)
        
	outp( vip_air_port, VIP_PCR );
    palette_control_bits = inp( vip_csr_port );
    if( which_eye == LEFT_EYE )// Left eye update only.
        {
        palette_control_bits |= 0x02; // Select left.
        palette_control_bits &= ~0x04;// Turn off update of both.
        palette_control_bits |= 0x01;// Turn off snoop.
        }
    else
    if( which_eye == RIGHT_EYE )// Right eye update only.
        {
        palette_control_bits &= ~0x02;// Select right.
        palette_control_bits &= ~0x04;// Turn off update of both.
        palette_control_bits |= 0x01;// Turn off snoop.            
        }
    else
        { // Enable both eye updates.
        palette_control_bits |= 0x04;// Turn on update of both.
        palette_control_bits &= ~0x01;// Turn on snoop.            
        }
    
	outp( vip_csr_port, palette_control_bits );

    // Setup the VGA's video palette.
    outp(0x3c8,0);    
    for (i=0; i<256; i++) {
        outp(0x3c9,*pPalette++);
        outp(0x3c9,*pPalette++);
        outp(0x3c9,*pPalette++);
        }
       
	// Setup the VIP video palette or palettes 
	outp(vip_air_port,VIP_PWAR);
	outp(vip_csr_port,0);
	outp(vip_air_port,VIP_PCRR);
    
	for (i=0; i < 256; i++)	{
		outp(vip_csr_port,*pTmp++);
		outp(vip_csr_port,*pTmp++);
		outp(vip_csr_port,*pTmp++);
		}
}

void    GetPalette( char *pPalette )
{
int		i;
    // Read the VGA's video palette.
    outp(0x3c8,0);    
    for (i=0; i<PALETTE_SIZE; i++) pPalette[i] = inp( 0x3c9 );
}

void    VideoChannel( int stereo_mode, int page )
{    
// This procedure will use the ISA bus to set which quardrant the
// left and right eyes will be looking at. 
	outp( vip_air_port, VIP_PSR );

	if( page )
		{
		if( stereo_mode )
            // Stereo on page one is defined as lower left(left eye), 
            // lower right(right eye).            
			outp( vip_csr_port, 0x23);            
		else
            {
            // Mono on page one is defined as lower left corner both eyes.
			outp( vip_csr_port, 0x22);        
            }
		}
	else
		{
		if( stereo_mode )
            // Stereo on page zero is defined as upper left(left eye), 
            // upper right(right eye).
			outp( vip_csr_port, 0x01);
		else
            {
            // Mono on page zero is defined as upper left corner both eyes.
			outp( vip_csr_port, 0x00);                    
            }
		}
}

void    ShowStereoPattern( void )
{
long            Offset;
long            over;
int				VESAWriteWindow = 0;    
int             SubWindow;
int             RetVal;
int             sPageOffset;
int             stereo_mode = 0;
int             i,j;
int             left_index;
int             right_index;
char            key;
char            *pDst;
char            *pPageBase;
char            *vga_screen;

	RetVal = VESAExists();

	if (RetVal == VESA_NO_SUPPORT) {
		printf("VESA Function 00h: Super VGA Information not supported.");
        return;
		}

	if (RetVal == VESA_BAD_INFO) {
		printf("VESA Function 00h: Super VGA Information failed.\n");
        return;
		}
    
	if( VESAGetModeInfo( VESA_G640x480x256 ) ) {
        printf("VESA mode %x is NOT supported\n", VESA_G640x480x256 );        
        return;
        }
    
	if (!(VesaModeInfo.ModeAttributes & VESA_MA_MODE_SUPPORTED)) {
        printf("VESA mode %x is NOT supported\n", VESA_G640x480x256 );
        return;
		}

	if ((VesaModeInfo.WinAAttributes & VESA_WA_MODE_SUPPORTED)
	 && (VesaModeInfo.WinAAttributes & VESA_WA_WRITABLE)) {
		VESAWriteWindow = 0;
		}
	else
		{
		if ((VesaModeInfo.WinBAttributes & VESA_WA_MODE_SUPPORTED)
		 && (VesaModeInfo.WinBAttributes & VESA_WA_WRITABLE)) {
			VESAWriteWindow = 1;
			}
		else {
            printf("Cannot write to VESA Windows\n");
			return;
			}
		}

    stereo_mode = 1;
    sPageOffset = 0;
    vga_screen  = (char *) SCREEN_BASE_LA;

// Now show the same pattern only in stereo
while( 1 ) {            
    VESASetMode( VESA_G640x480x256 );    
    
    VideoChannel( stereo_mode, sPageOffset / 640 );

    PutPalette( LEFT_EYE, left_colors );    
    PutPalette( RIGHT_EYE, right_colors );

    SubWindow   = 0;    
    Offset      = sPageOffset;
    pPageBase   = (char *)(vga_screen + sPageOffset);
    pDst        = (char *)pPageBase;			/* start address on VGA */
    left_index  = 0;
    right_index = 200;
        
    VESASetWin( VESAWriteWindow, SubWindow );

    for (i = 0; i < 200; i++) {
        if (Offset + 640L < 65536L) {
            for (j = 0; j < 320; j++) *pDst++ = left_index;
            for (j = 0; j < 320; j++) *pDst++ = right_index;
            
            pDst   += 640;
            Offset += 1280L;

            if (Offset >= 65536L) {
                Offset &= 0x0000FFFF;
                pDst = (char *)(pPageBase + Offset);
                VESASetWin( VESAWriteWindow, ++SubWindow );
                }
            }
        else {
            if (Offset + 320L < 65536L) {
                for (j = 0; j < 320; j++) *pDst++ = left_index;
                over = (Offset + 640L) - 65536L;
                for (j = 0; j < (320L - over); j++) *pDst++ = right_index;

                pDst = (char *)pPageBase;
                VESASetWin( VESAWriteWindow,++SubWindow );

                for (j = 0; j < over; j++) *pDst++ = right_index;
                
                pDst  += 640;
                Offset = 640L + over;
                }
            else {
                over = (Offset + 320L) - 65536L;
                for (j = 0; j < (320L - over); j++) *pDst++ = left_index;

                pDst = (char *)pPageBase;
                VESASetWin( VESAWriteWindow, ++SubWindow );

                for (j = 0; j < over; j++ ) *pDst++ = left_index;
                for (j = 0; j < 320; j++ )  *pDst++ = right_index;
                
                pDst  += 640;
                Offset = 960L + over;
                }
            }
        left_index++;
        right_index--;
        pPageBase = (char *)(vga_screen);
        }
    
    #ifdef __WATCOMC__
        _settextposition( 1,1 );
    #else
        gotoxy( 1,1 );
    #endif        
    printf("Enter: Toggle stereo/mono  Space: Toggle page 0/1  Esc: exit\n");
    printf("Current settings:\n");    
    if( sPageOffset ) printf("Page 1:  ");
    else              printf("Page 0:  ");                    
    if( stereo_mode ) printf("Stereo mode:\n");
    else              printf("Mono mode:\n");
    
    key = getch();
    if( key == 27 ) break;// Simply exit this example code.    
    if( key == 13 )  // Simply toggle to and from stereo/mono mode.
        stereo_mode = !stereo_mode;    
    if( key == 32 ) // Simply flip to other page on press of space bar
        {
        if( sPageOffset ) sPageOffset = 0;            
        else              sPageOffset = 640;
        }        
    }    
}

void    BuildPalettes( void )
{
int i;

// Both eyes palette (mono test)
    for( i = 0; i < PALETTE_SIZE; ) {
        mono_colors[i++] = 0x0;
        mono_colors[i++] = 0x0;
        mono_colors[i++] = i % 100;                
        }            
    
// All red in left eye (stereo test)
    for( i=0; i<PALETTE_SIZE; i++ ) {
        if( !(i % 3) ) left_colors[i] = i;    
        else           left_colors[i] = 0;                
        }    
// All green in right eye (stereo test)
    for( i=0; i<PALETTE_SIZE; i++ ) {
        if( !((i+2) % 3) ) right_colors[i] = i;    
        else               right_colors[i] = 0;                

        }
}

void    CleanUp( void )
{
#if defined(__WATCOMC__) || defined(_WINDOWS) 
// Free the dos memory.
      if (pdosmem) {
         freeDOS(selector);
         pdosmem = 0;
      }
#endif  
}
#endif                                                      // Les 09/29/95

void    vfx1_init(void)
{
#ifndef TEKWAR
int     sense_function;
int     current_video_mode;
#endif
int     htd_index, hpd_index;
int     count, i, j;
int     packet_size;
int     ret;
#ifndef TEKWAR
char    vfx1_cyberpuck;
char    config_buffer[ MAX_CONFIG_SIZE ];
char    *report_offset;
char    report_buffer[ MAX_REPORT_SIZE ];
#endif
char    *env_ptr, *dptr;

#if defined(__WATCOMC__)  || defined(_WINDOWS) 
   // allocate a DOS real-mode buffer
   pdosmem = allocDOS(MAX_CONFIG_SIZE, &segment, &selector);
   if (!pdosmem) {
      printf("Error getting DOS memory!\n");
      exit(0);
   }
#endif

// Should use environment string here.
// Get the VIPPORT Environment string and setup the port variables.
    env_ptr = getenv("VIPPORT");
    if (env_ptr == NULL) {
#ifndef TEKWAR
        printf("VIPPORT Environment string not setup, using default.\n");
#endif
        vip_air_port = 0x300;
        }
    else {
        vip_air_port = strtol( env_ptr, &dptr, 16 );
        }
    
    vip_csr_port = vip_air_port + VIP_CSR;            
        
    if( (sense_function = SenseStatus()) == 0 ) {
#ifndef TEKWAR
        printf("No Sense devices installed\n");
        CleanUp();
        exit(2);        
#endif
        }

#ifndef TEKWAR
    printf("Sense devices installed on function %x\n", sense_function );
#endif

    if( ret=SenseGetConfiguration( sense_function, &count, config_buffer ) ) {
#ifndef TEKWAR
        printf("Error#(%d) getting configuration packets\n", ret );
        CleanUp();        
        exit(3);
#endif
        }

#ifndef TEKWAR
    printf("Configuration packet sizes %d\n", count );
#endif

    report_offset = report_buffer;    
    htd_index = hpd_index = 0;
    
    for( i=0; i < count; i += packet_size ) {
        packet_size = (int)config_buffer[i];        

        switch( config_buffer[i+1] )
            {
            case NULLP:
// All done packets break out                
                i = count; 
            break;
            case SYSI:              
                system_cfg = (SYSTEM_CPKT *) &config_buffer[i];

            break;
            case VNDI:
                vendor_cfg = (VENDOR_CPKT *) &config_buffer[i];
#ifndef TEKWAR
                printf("%s\n", vendor_cfg->prdm );
                printf("%s\n", vendor_cfg->prdn );
                printf("Serial #: %s\n", vendor_cfg->prdsn );
                printf("Revision level: %s\n", vendor_cfg->prdrl );
#endif
                if( !strcmp( vendor_cfg->prdn, PRDN ) )
                    vfx1_cyberpuck = 1;
                else
                    vfx1_cyberpuck = 0;
            break;
            case HMS:
                hms_cfg = (HMS_CPKT *) &config_buffer[i];
                
                svm.cfg.psiz = sizeof( SVM_CPKT );
                svm.cfg.ptyp = SVM;
                svm.cfg.dcls = hms_cfg->dcls;
                svm.cfg.dnum = hms_cfg->dnum;

            break;
            case HTD:
                if( htd_index >= MAX_HTDS ) break;                 
                htd_data[htd_index++] = (HTD_STRUCT far *) report_offset;
                report_offset += sizeof( HTD_STRUCT );

                htd_cfg = (HTD_CPKT *) &config_buffer[i];
                // Only enable if Head tracker is for a Vfx1 Cyberbuck
                if( vfx1_cyberpuck )                    
                    {// Vfx1 Cyberpuck Head tracker.                
                    htd_cfg->ywcd.rcf.den = 1;
                    htd_cfg->ptcd.rcf.den = 1;
                    htd_cfg->rlcd.rcf.den = 1;                
                    }
            break;
            case HPD:    
                if( hpd_index >= MAX_HPDS ) break;               
                hpd_data[hpd_index++] = (HPD_STRUCT far *) report_offset;
                report_offset += sizeof( HPD_STRUCT );
                
                hpd_cfg = (HPD_CPKT *) &config_buffer[i];

                if( vfx1_cyberpuck )                    
                    {// Vfx1 Cyberpuck Head tracker.                
                    hpd_cfg->ywcd.rcf.den = 1;
                    hpd_cfg->ptcd.rcf.den = 1;
                    hpd_cfg->rlcd.rcf.den = 1;
                    hpd_cfg->btcd.den     = 1;
                    }
            break;
            default:
#ifndef TEKWAR
                printf("Strange packet type here\n");
#endif
            break;
            }
        }

    if( ret=SenseSetConfiguration( sense_function, config_buffer ) ) {
#ifndef TEKWAR
        printf("Error#(%d) enabling configuration packets\n",ret);
        CleanUp();                
        exit(4);
#endif
        }   

#ifndef TEKWAR
    getch();

    #ifdef __WATCOMC__
        _clearscreen( 0 );
    #else
        clrscr();
    #endif
#endif    
            
// Configuration packets are now enabled and ready for reporting
#ifndef TEKWAR
    if( htd_index )
        printf("VFX1 Head tracker device installed\n" );
    if( hpd_index )
        printf("CyberPuck Pointing device installed\n" );    

    while( !kbhit() ) {       
        if( ret=SenseReport( sense_function, &count, report_buffer ) ) {
            printf("Error#(%d) during reporting packets\n",ret);
            CleanUp();                    
            exit(5);                            
            }
        
        #ifdef __WATCOMC__
            _settextposition( 1,5 );
        #else
            gotoxy( 1,5 );
        #endif

        if( htd_index )
            printf("VFX1: yaw(%-05d) pitch(%-05d) roll(%-05d)  \n",
                htd_data[0]->yaw, htd_data[0]->pitch, htd_data[0]->roll);
            
        if( hpd_index )
            printf("CyberPuck: pitch(%-05d) roll(%-05d) Buttons(%03x)    \n",
                hpd_data[0]->pitch, hpd_data[0]->roll, hpd_data[0]->buttons);
        }
    getch();
#endif

#ifndef TEKWAR
#ifndef _WINDOWS    
// Now let's do both a stero and a mono video test.
// First build the palettes for both tests.    
    BuildPalettes();

// Store current VGA mode.
    current_video_mode = VESAGetMode( );

// First let's do a simple MONO video test.        
    VESASetMode( MONO_320_200 );

// Get the VGA palette
    GetPalette( vga_palette );    

// Now put the "Something" mounted display in mono mode.
    svm.cfg.vmod = 0;
    if( ret=SenseSetConfiguration( sense_function, (char *)&svm ) ) {
        VESASetMode( current_video_mode );            
        printf("Error#(%d) setting mono video mode\n",ret);
        }    
    else {
        PutPalette( BOTH_EYES, mono_colors );

// Put up a mono screen.    
        ShowMonoPattern( );
        }
    
// Finally let's do a simple STEREO video test.        
    VESASetMode( VESA_G640x480x256 );

// Now put the "Something" mounted display in stereo mode.
    svm.cfg.vmod = 1;
    if( ret=SenseSetConfiguration( sense_function, (char *)&svm ) ) {
        VESASetMode( current_video_mode );                    
        printf("Error#(%d) setting mono video mode\n",ret);
        }
    else {  
        PutPalette( LEFT_EYE, left_colors );    
        PutPalette( RIGHT_EYE, right_colors );    

// Put up a stereo screen. This routine also performs stereo tests.   
        ShowStereoPattern();
        }
    
// Now put the "Something" mounted display back in mono mode.
    VESASetMode( MONO_320_200 );
    svm.cfg.vmod = 0;
    if( ret=SenseSetConfiguration( sense_function, (char *)&svm ) ) {
        VESASetMode( current_video_mode );                    
        printf("Error#(%d) setting mono video mode\n",ret);
        }
    
// Reset the VGA palette
    PutPalette( BOTH_EYES, vga_palette );    

// Reset VGA mode
    VESASetMode( current_video_mode );        
#else
// Now put the "Something" mounted display in stereo mode.
    svm.cfg.vmod = 1;
    if( ret=SenseSetConfiguration( sense_function, (char *)&svm ) ) {
        VESASetMode( current_video_mode );            
        printf("Error#(%d) setting mono video mode\n",ret);
        }    
    else {
        printf("Now in STEREO Mode\n");
        getch();
        }
    
// Now put the "Something" mounted display in mono mode.
    svm.cfg.vmod = 0;
    if( ret=SenseSetConfiguration( sense_function, (char *)&svm ) ) {
        VESASetMode( current_video_mode );            
        printf("Error#(%d) setting mono video mode\n",ret);
        }    
    else {
        printf("Now in MONO Mode\n");
        getch();
        }
#endif

    CleanUp();
#else
     vfx1enabled=1;
     fp=fopen("VFX1.CFG","r");
     if (fp != NULL) {
          while (fgets(buf,256,fp) != NULL) {
               for (i=0 ; i < NUMPUCKBUTTONS ; i++) {
                    if (strstr(buf,butkeystr[i]) != NULL) {
                         for (j=0 ; j < NUMBUTOPTIONS ; j++) {
                              if (strstr(buf,butoptstr[j]) != NULL) {
                                   switch (j) {
                                   case OPT_RUN:
                                   case OPT_SHOOT:
                                   case OPT_USEOPEN:
                                   case OPT_JUMP:
                                   case OPT_CROUCH:
                                   case OPT_MAP:
                                   case OPT_MAPZOOMIN:
                                   case OPT_MAPZOOMOUT:
                                   case OPT_CONCEAL:
                                        puckbutton[i]=butoptval[j];
                                        break;
                                   }
                              }
                         }
                    }
               }
          }
          fclose(fp);
     }
#endif
}
