/*
 * 422_to_420fullres converts 4:2:2 CbYCrY video to 4:2:0 plane sequential video.
 * The plane sequential 4:2:0 is written to standard output.  The input size
 * is fixed a 720x486 ntsc or 720x576 pal and the output is 720x480 ntsc or 720x576 pal.
 *
 * The chroma is filtered using the MPEG-2 TM5 filters (interlaced ==> 4:2:0 interlaced)
 *
 * Usage:  422_to_420fullres size < 422input > 420output
 *
 * The size parameter is either n or p:
 *    n for input 720x486
 *    p for input 720x576
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	INHEIGHT	576
#define	INWIDTH		720

#define	OUTHEIGHT	576
#define	OUTWIDTH	720
#define	OUTFRAME	(OUTWIDTH*OUTHEIGHT)

typedef unsigned char u_char;

/* forward declaration */
void tm5filter(u_char *in, u_char *out, int outheight, int outwidth);

int
main(int argc, char **argv)
{
    u_char *yin = (u_char*)malloc(OUTFRAME*sizeof(u_char));
    u_char *crin = (u_char*)malloc(OUTFRAME/2*sizeof(u_char));
    u_char *cbin = (u_char*)malloc(OUTFRAME/2*sizeof(u_char));
    u_char *crout = (u_char*)malloc(OUTFRAME/4*sizeof(u_char));
    u_char *cbout = (u_char*)malloc(OUTFRAME/4*sizeof(u_char));

    int n, i, j;
    u_char *y, *cr, *cb;
    int inheight, inwidth, outheight, outwidth, topclip, bottomclip;

    FILE *fpin = NULL;
    FILE *fpout = NULL;

    if (argc < 4) {
	    fprintf(stderr,
	        "Usage: 422_to_420fullres n|p 422inputfile 420outputfile\n");
	    exit(1);
    }

    fpin = fopen(argv[2],"rb");
    fpout = fopen(argv[3],"wb");
    
    if (!strcmp(argv[1], "n")) {
      inheight = 486;
      outheight = 480;
      topclip = 2;
      bottomclip = 4;
      inwidth = outwidth = 720;
    }
    else if (!strcmp(argv[1], "p")) {
      inheight = outheight = 576;
      inwidth = outwidth = 720;
      topclip = bottomclip = 0;
    }
    else {
	    fprintf(stderr, "Unknown size parameter: %s\n", argv[1]);
	    fprintf(stderr, "Valid size parameters are n or p\n");
	    exit(1);
    }



    for (;;) {
	    /*
	     * Read a 4:2:2 frame (cb y cr y  format) clipping the top and bottom to
	     * give the central outwidth by outheight image.
	     */

	      y = yin;
	      cr = crin;
	      cb = cbin;
	      if ((n = getc(fpin)) == EOF) /* no more pictures to read */
	          exit(0);
	      for (i = 0; i < inwidth*topclip*2; i++) /* clip top lines */
	          if ((n = getc(fpin)) == EOF)
		      goto out;
	      for (j = 0; j < outheight*outwidth-2; j += 2) {
	          *cb++ = n;
	          if ((n = getc(fpin)) == EOF)
	            goto out;
	          *y++  = n;
	          if ((n = getc(fpin)) == EOF)
	            goto out;
	          *cr++ = n;
	          if ((n = getc(fpin)) == EOF)
	            goto out;
	          *y++  = n;
	          if ((n = getc(fpin)) == EOF) {
	            goto out;
	          }
	      }

	      /* get the last quad */
	      *cb++ = n;
	      if ((n = getc(fpin)) == EOF)
	        goto out;
	      *y++  = n;
	      if ((n = getc(fpin)) == EOF)
	        goto out;
	      *cr++ = n;
	      if ((n = getc(fpin)) == EOF)
	        goto out;
	      *y++  = n;

	      for (i = 0; i < inwidth*bottomclip*2; i++) /* clip bottom lines */
	        if (getc(fpin) == EOF)
	          goto out;

	      /*
	       * filter the chroma vertically by 2
	       */
	      tm5filter(crin, crout, outheight, outwidth);
	      tm5filter(cbin, cbout, outheight, outwidth);

	      /*
	       * Write a 4:2:0 frame (plane sequential)
	       */
	      y = yin;
	      for (i = 0; i < outheight*outwidth; i++)
	            putc(*y++, fpout);
	      cb = cbout;
	      for (i = 0; i < outheight*outwidth/4; i++)
	            putc(*cb++, fpout);
	      cr = crout;
	      for (i = 0; i < outheight*outwidth/4; i++)
	            putc(*cr++, fpout);
    }

out:
    fprintf(stderr, "Unexpected EOF on <fpin>\n");
    exit(1);

}

/*
 * tm5filter() decimates the chroma by 2 vertically as decribed in TM5.
 * Chroma filtering is done such that data from the two fields are never mixed.
 *
 * In the top field, the output chroma lines will be have the same spatial
 * locations input lines 0, 4, 8 ....  The low pass filter is:
 *
 *               (-29, 0, 88, 138, 88, 0, -29) // 256
 *
 * In the bottom field, the surviving chroma lines will be half way between
 * the original chroma chroma lines 1&3, 5&7, 9&11 ... That is these lines
 * have the same spatial position as lines 2, 6, 10 (which are part of the 
 * top field.  The decimation filter is:
 *
 *		 	( 1, 7, 7, 1) // 16
 *
 * In both cases, constant extension is used to handle the boundry conditions.
 *
 * Note that for CCIR601 525, the top field is F1 and the first line is L22.
 */

void tm5filter(u_char *in, u_char *out, int outheight, int outwidth)
{
    int ym6, ym2, yp2, yp4, yp6, hm6, w2, i, j, n;

    w2 = outwidth;
    yp6 = (yp4 = (yp2 = w2) + w2) + w2;
    hm6 = outheight/2 - 6;
    for (i = 0; i < outheight/2; i += 2) {
	if (i <= 4) switch (i) {
	    case 0: ym6 = ym2 = 0; break;
	    case 2: ym6 = (ym2 = -w2) - w2; break;
	    case 4: ym6 -= w2; break;
	}
	if (i >= hm6) switch (i - hm6) {
	    case 0: yp6 = yp4; break;
	    case 2: yp6 = yp4 = yp2; break;
	    case 4: yp6 = yp4 = yp2 = 0; break;
	}
	for (j = 0; j < w2; j += 2) {		/* Do a top field line */
	    n = (int)(-29 * (int)(in[ym6] + in[yp6]) +
		       88 * (int)(in[ym2] + in[yp2]) +
		      138 * (int)in[0] + 128) >> 8;
	    if (n < 0) n = 0;
	    if (n > 255) n = 255;
	    *out++ = n;
	    in++;
	}
	for (j = 0; j < w2; j += 2) {		/* Do a bottom field line */
	    *out++ = (int)(in[ym2] + 7 * (in[0] + in[yp2]) +
		in[yp4] + 8) >> 4;
	    in++;
	}
	in += w2;
    }
}

