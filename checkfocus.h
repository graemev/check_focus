/* exported header from checkfocus */
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h> 
#include <unistd.h>


/* in C99 use stdbool.h ... but here just avoid dependency */
#define CF_BOOL  int
#define CF_TRUE  1
#define CF_FALSE 0

typedef unsigned long long Cf_stat;      /* Holds a very big unsigned number */


struct Cf_stats
{
  Cf_stat  overall;	 /* sum of nearest neighbour differences */
  Cf_stat  horizontal;   /* sum of nearest horizontal neighbour differences */
  Cf_stat  vertical;     /* sum of nearest vertical neighbour differences */
			 /* repeated for a sample square */
  Cf_stat  square;
  Cf_stat  square_horizontal;
  Cf_stat  square_vertical;
} ;
  

extern int read_jpeg_file(FILE * const input_file, struct Cf_stats * result);

/* originaly defined by jmorecfg.h, but now oftem suppressed */
#ifndef RGB_RED 
#define RGB_RED         0       /* Offset of Red in an RGB scanline element */
#define RGB_GREEN       1       /* Offset of Green */
#define RGB_BLUE        2       /* Offset of Blue */
#define RGB_PIXELSIZE   3       /* JSAMPLEs per RGB scanline element */
#endif



struct box
{
  int first_row;        /* We are "in the box iff: */
  int last_row;         /* row >= first_row       AND <= last_row ... AND .. */
  int first_column;     /* column >= first_column AND <= last_column         */
  int last_column;
} ;

/* Method signatures */

extern CF_BOOL inbox            (struct box *p, int row, int column);
extern CF_BOOL box_defined      (struct box *p);
extern void    copy_box         (struct box *to, struct box *from);
extern Cf_stat diff             (unsigned char one, unsigned char other );
extern void    dump_3component  (unsigned char comp[]);
extern void    dump_1component  (unsigned char comp[]);
extern void    dump_1row        (int output_components, JSAMPARRAY buffer, JDIMENSION output_width);  
extern char   *colorspace_string(J_COLOR_SPACE space);
