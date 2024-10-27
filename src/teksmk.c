/***************************************************************************
 *   TEKSMK.C  -   smack flic stuff etc. for Tekwar                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "pragmas.h"
#include "cache1d.h"

#include "tekwar.h"

#include "fx_man.h"
#include "smacker.h"

/* jump to and render a specific frame */
char smk_render_frame(smk object, unsigned long frame);

static smk smkmenu, smkseq;
static unsigned char *smkmenubuf;
#define NUMSMKTILES 3
#define SMKPICNUM0 (MAXTILES-NUMSMKTILES)

#define SMKAUDIOBUFSZ (32768)
static char smkaudiobuf[SMKAUDIOBUFSZ];
static unsigned long smkaudiowrite = 0, smkaudioread = 0;

static void smkplayfeeder(char **ptr, unsigned int *length) {
     *ptr = &smkaudiobuf[smkaudioread];
     *length = 2048;
     smkaudioread = (smkaudioread + 2048) % SMKAUDIOBUFSZ;
}

void
smkplayseq(char *name)
{
     char path[BMAX_PATH], result = SMK_DONE;
     int fh, flen, i;

     unsigned char smkpal[NUMSMKTILES][768], smkchannels[7], smkbitdepth[7], *smkbuf = NULL;
     unsigned char *smkframes = NULL;
     unsigned long smkx, smky, smkaudrate[7];

     unsigned long playframe, decodeframe;
     double usf, clocktime, nowtime;
     int voice = -1, frame;

     snprintf(path, sizeof(path), "smk/%s.smk", name);
     if ((fh = kopen4load(path, 0)) < 0) {
          snprintf(path, sizeof(path), "%s.smk", name);
          fh = kopen4load(path, 0);
     }
     if (fh < 0) {
          debugprintf("smkplayseq(\"%s\") failed\n", name);
          return;
     }

     flen = kfilelength(fh);
     smkbuf = (unsigned char *)malloc(flen);
     if (smkbuf == NULL) {
          kclose(fh);
          debugprintf("smkplayseq(\"%s\") malloc of %d bytes failed\n", name, flen);
          return;
     }

     kread(fh, smkbuf, flen);
     kclose(fh);

     smkseq = smk_open_memory(smkbuf, flen);
     if (!smkseq) {
          free(smkbuf);
          debugprintf("smk_open_memory() returned null\n");
          return;
     }

     smk_enable_all(smkseq, SMK_VIDEO_TRACK | SMK_AUDIO_TRACK_0);
     smk_info_all(smkseq, NULL, NULL, &usf);
     smk_info_video(smkseq, &smkx, &smky, NULL);
     smk_info_audio(smkseq, NULL, smkchannels, smkbitdepth, smkaudrate);
     if (smkbitdepth[0] != 8 || smkchannels[0] > 2) {
          debugprintf("smkplayseq(\"%s\") is not 8-bit mono/stereo audio\n", name);
          goto end;
     }

     // Smacker frames are decoded linewise, but BUILD expects its
     // tiles columnwise, so we will treat the tile as though it's
     // rotated 90 degrees and flipped horizontally.

     smkframes = malloc(NUMSMKTILES * smkx*smky);
     if (!smkframes) {
          debugprintf("smkplayseq(\"%s\") malloc of frames failed\n", name);
          goto end;
     }
     for (i = 0; i < NUMSMKTILES; i++) {
          tilesizx[SMKPICNUM0+i] = smky;
          tilesizy[SMKPICNUM0+i] = smkx;
          waloff[SMKPICNUM0+i] = (intptr_t)&smkframes[i*smkx*smky];
     }

     smkaudiowrite = smkaudioread = 0;
     decodeframe = playframe = 0;
     result = smk_first(smkseq);

     while (result == SMK_MORE || (playframe < decodeframe && result == SMK_LAST)) {
          while (decodeframe - playframe < NUMSMKTILES && result == SMK_MORE) {
               unsigned long audiolen, slice, downsamp;
               const unsigned char *audiodata, *palptr;

               frame = decodeframe % NUMSMKTILES;
               memcpy(&smkframes[frame*smkx*smky], smk_get_video(smkseq), smkx*smky);
               for (palptr = smk_get_palette(smkseq), i=256*3-1; i>=0; i--)
                   smkpal[frame][i] = palptr[i] >> 2;

               audiolen = smk_get_audio_size(smkseq, 0);
               audiodata = smk_get_audio(smkseq, 0);
               if (smkchannels[0] == 2) audiolen >>= 1;     // Stereo is downmixed for JFAudiolib.
               while (audiolen > 0) {
                    if (smkaudiowrite + audiolen > SMKAUDIOBUFSZ) {
                         slice = SMKAUDIOBUFSZ - smkaudiowrite;
                    } else {
                         slice = audiolen;
                    }
                    if (smkchannels[0] == 2) {
                         downsamp = slice - 1;
                         do { smkaudiobuf[smkaudiowrite+downsamp] =
                              (audiodata[(downsamp<<1)+0] >> 1) +
                              (audiodata[(downsamp<<1)+1] >> 1);
                         } while (downsamp--);
                    } else {
                         memcpy(&smkaudiobuf[smkaudiowrite], audiodata, slice);
                    }
                    audiodata += slice * smkchannels[0];
                    audiolen -= slice;
                    smkaudiowrite = (smkaudiowrite + slice) % SMKAUDIOBUFSZ;
               }

               decodeframe++;
               result = smk_next(smkseq);
               if (result == SMK_ERROR) goto end;
          }

          nowtime = (double)getusecticks();

          if (voice < 0) {
               voice = FX_StartDemandFeedPlayback(smkplayfeeder, (int)smkaudrate[0],
                    0, 63, 63, 63, 1, (unsigned int)-1);
               if (voice < 0) {
                    debugprintf("smkplayseq(\"%s\") failed to start audio playback\n", name);
                    goto end;
               }

               clocktime = nowtime;
          }

          while (nowtime - clocktime >= usf) clocktime += usf, playframe++;
          frame = playframe % NUMSMKTILES;

          setbrightness(-1, smkpal[frame], 2|4);
#if USE_POLYMOST && USE_OPENGL
          invalidatetile(SMKPICNUM0+frame, 0, -1);
#endif
          clearallviews(0);
          rotatesprite(160<<16, 100<<16, divscale16(200, tilesizx[SMKPICNUM0+frame]),
                      512, SMKPICNUM0+frame, 0, 0, 2+4+64, 0, 0, xdim-1, ydim-1);
          nextpage();

          handleevents();
          if (keystatus[0x1]|keystatus[0x1c]|keystatus[0x39]|keystatus[0x9c]) {
               keystatus[0x1]=keystatus[0x1c]=keystatus[0x39]=keystatus[0x9c]=0;
               break;
          }
          if (!FX_SoundActive(voice)) { // If the audio stops for some reason.
               break;
          }
     }

end:
     if (result == SMK_ERROR) debugprintf("smkplayseq(\"%s\") error at decodeframe %lud (%d)\n", name, decodeframe, result);
     if (voice >= 0) FX_StopSound(voice);
     if (smkseq) smk_close(smkseq);
     smkseq = NULL;
     if (smkbuf) free(smkbuf);

     for (i = 0; i < NUMSMKTILES; i++) waloff[SMKPICNUM0+i] = 0;
     if (smkframes) free(smkframes);
}

void
smkopenmenu(char *name)
{
     unsigned long smkx, smky;
     int fh, flen;

     fh = kopen4load(name, 0);
     if (fh < 0) {
          debugprintf("smkopenmenu(\"%s\") failed\n", name);
          return;
     }

     flen = kfilelength(fh);
     smkmenubuf = (unsigned char *)malloc(flen);

     if (smkmenubuf == NULL) {
          kclose(fh);
          debugprintf("smkopenmenu(\"%s\") malloc of %d bytes failed\n", name, flen);
          return;
     }

     kread(fh, smkmenubuf, flen);
     kclose(fh);

     smkmenu = smk_open_memory(smkmenubuf, flen);
     if (!smkmenu) {
          free(smkmenubuf);
          smkmenubuf = NULL;
          debugprintf("smk_open_memory() returned null\n");
          return;
     }

     // Smacker frames are decoded linewise, but BUILD expects its
     // tiles columnwise, so we will treat the tile as though it's
     // rotated 90 degrees and flipped horizontally.

     smk_enable_all(smkmenu, SMK_VIDEO_TRACK);
     smk_info_video(smkmenu, &smkx, &smky, NULL);
     tilesizx[SMKPICNUM0] = smky;
     tilesizy[SMKPICNUM0] = smkx;
}

void
smkmenuframe(int fn)
{
     unsigned char spal[768];
     const unsigned char *palptr;
     int i;

     if (!smkmenu) {
          return;
     }

     // Render the first decodeframe as the menu background.
     smk_first(smkmenu);

     // Next, render the particular decodeframe requested.
     smk_render_frame(smkmenu, fn-1);

     // Convert the palette to the VGA (0-63) range BUILD uses.
     for (palptr = smk_get_palette(smkmenu), i=256*3-1; i>=0; i--)
          spal[i] = palptr[i] >> 2;

     waloff[SMKPICNUM0] = (intptr_t)smk_get_video(smkmenu);
#if USE_POLYMOST && USE_OPENGL
     invalidatetile(SMKPICNUM0, 0, -1);
#endif

     setbrightness(-1, spal, 2|4);
}

void
smkshowmenu()
{
     if (!smkmenu) {
          return;
     }

     clearallviews(0);
     rotatesprite(160<<16, 100<<16, divscale16(200, tilesizx[SMKPICNUM0]),
                 512, SMKPICNUM0, 0, 0, 2+4, 0, 0, xdim-1, ydim-1);
     nextpage();
}

void
smkclosemenu()
{
     if (smkmenu) smk_close(smkmenu);
     if (smkmenubuf) free(smkmenubuf);

     waloff[SMKPICNUM0] = 0;
     smkmenu = NULL;
     smkmenubuf = NULL;
}
