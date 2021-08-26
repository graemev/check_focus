/* exported header from checkfocus */
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h> 
#include <unistd.h>
#include <math.h>


/*
    checkfocus - report statistics on a jpeg level of focus
    Copyright (C) 2021  Graeme Vetterlein

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/




extern int debug;
extern int verbose;



/* in C99 use stdbool.h ... but here just avoid dependency */
#define CF_BOOL  int
#define CF_TRUE  1
#define CF_FALSE 0

/* For debugging , Col = signed, Row = Unsigned ...then if we get the wrong way around -Wconversion will warn us
 */

typedef int Trow;
typedef unsigned long Tcol;






typedef unsigned long long Cf_stat;      /* Holds a very big unsigned number */
typedef double             Cf_fudge;     /* A real number , a fudge factor   */


struct Cf_scores
{
  Cf_stat  overall;	 /* sum of nearest neighbour differences */
  Cf_stat  horizontal;   /* sum of nearest horizontal neighbour differences */
  Cf_stat  vertical;     /* sum of nearest vertical neighbour differences */
} ;




struct Cf_stats_NOT
{
  Cf_stat  overall;	 /* sum of nearest neighbour differences */
  Cf_stat  horizontal;   /* sum of nearest horizontal neighbour differences */
  Cf_stat  vertical;     /* sum of nearest vertical neighbour differences */
			 /* repeated for a sample square */
  Cf_stat  square;
  Cf_stat  square_horizontal;
  Cf_stat  square_vertical;
} ;
  


/* originaly defined by jmorecfg.h, but now often suppressed */
#ifndef RGB_RED 
#define RGB_RED         0       /* Offset of Red in an RGB scanline element */
#define RGB_GREEN       1       /* Offset of Green */
#define RGB_BLUE        2       /* Offset of Blue */
#define RGB_PIXELSIZE   3       /* JSAMPLEs per RGB scanline element */
#endif


struct box
{
  Tcol first_column;     /* We are "in the box iff: */
  Trow first_row;        /* column >= first_column AND <= last_column         */
  Tcol last_column;	/* row >= first_row       AND <= last_row ... AND .. */
  Trow last_row;     
} ;

/* Method signatures */

extern Cf_stat adjust		(Cf_stat input, int shiftr, Cf_fudge fudge_factor);
extern Cf_stat diff             (unsigned char one, unsigned char other );

extern CF_BOOL inbox            (struct box *p, Tcol column, Trow row);
extern CF_BOOL box_defined      (struct box *p);
extern void    copy_box         (struct box *to, struct box *from);

extern void    dump_3component  (unsigned char comp[]);
extern void    dump_1component  (unsigned char comp[]);
extern void    dump_1row        (int output_components, JSAMPARRAY buffer, Tcol output_width);
extern char   *colorspace_string(J_COLOR_SPACE space);
