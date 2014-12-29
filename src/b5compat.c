#include "build.h"

void permanentwritesprite(int thex, int they, short tilenum, signed char shade,
        int cx1, int cy1, int cx2, int cy2, unsigned char dapalnum) {
    rotatesprite(thex<<16, they<<16, 0, 0, tilenum, shade, dapalnum, 2, cx1, cy1, cx2, cy2);
}

void permanentwritespritetile(int thex, int they, short tilenum, signed char shade,
        int cx1, int cy1, int cx2, int cy2, unsigned char dapalnum) {
}

void overwritesprite(int thex, int they, short tilenum, signed char shade,
        char orientation, unsigned char dapalnum) {
    int stat = 0;

    if (!(orientation&1)) {
        // top-left origin
        stat |= 16;
    }
    if ((orientation&2)) {
        // seems to be the virtual 320x200 upscale judging by the code
        stat |= 2|8;
    }

    rotatesprite(thex<<16, they<<16, 0, 0, tilenum, shade, dapalnum, stat, 0, 0, xdim-1, ydim-2);
}

void printext(int x, int y, char buffer[42], short tilenum, char invisiblecol)
{
}

void resettiming()
{
    totalclock = 0;
}

void precache()
{
}