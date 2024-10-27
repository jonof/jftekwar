#include "smacker.c"

/* jump to and render a specific frame */
char smk_render_frame(smk s, unsigned long f)
{
    /* null check */
    if (s == NULL) {
        fputs("libsmacker::smk_render_frame() - ERROR: smk is NULL\n", stderr);
        return -1;
    }
    
    /* rewind (or fast forward!) exactly to f */
    s->cur_frame = f;
    
    /* render the frame: we're ready */
    if ( smk_render(s) < 0) {
        fprintf(stderr, "libsmacker::smk_render_frame(s,%lu) - Warning: frame %lu: smk_render returned errors.\n", f, s->cur_frame);
        goto error;
    }
    
    return 0;
    
error:
    return -1;
}

