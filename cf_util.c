#include "checkfocus.h"

int debug=0;
int verbose=0;



/* we  need to mess around with fixed and floating point math */
Cf_stat adjust(Cf_stat input, int shiftr, Cf_fudge fudge_factor)
{
  Cf_stat output;

  if (debug)
    fprintf(stderr, "adjust given args input=%llu, shift=%d,fudge_factor=%1.3g\n",
	    input, shiftr, fudge_factor);
  
  output=input;       /* unless we change something */

  if (shiftr)         /* just scale all the numbers down ,, in case other tools can't cope */
    output>>=shiftr;

  if (debug)
    fprintf(stderr, "adjust following shift; output=%llu\n",
	    output);

  
  if (fudge_factor != 0.0 && fudge_factor != 1.0)  /* we lose precision doing floating point, so only get into it if necessary */
    output = (Cf_stat) (fudge_factor * (Cf_fudge)output );  /* fixed to float, float multiply, then back to fixed */

  if (debug)
    fprintf(stderr, "adjust following fudge_factoring; output=%llu\n",
	    output);
  
  return (output);
}


CF_BOOL inbox(struct box * p, Tcol column, Trow row)
{
  return ( row >= p->first_row && row <= p->last_row  &&  column >= p->first_column && column <= p->last_column);
}

CF_BOOL box_defined(struct box *p)
{
  return (p->first_row < p->last_row  &&  p->first_column < p->last_column );
}

void copy_box(struct box *to, struct box *from)
{
  to->first_row    = from->first_row;   
  to->last_row     = from->last_row;    
  to->first_column = from->first_column;
  to->last_column  = from->last_column;

  return;
}


/* the "meat" of the whole roadshow:
 *
 * A contrastly(?) image might have values 0, 0, 0,16,16,16,16,16,16
 * A blury one                             0, 2, 4, 6, 8,10,12,14,16
 *
 * If we sum the differences "cell to out right"  we get (0-0)+(0-0)+(0-16)+(16-16)+(16-16)+(16-16)+(16-16)+(16-16) = -16
 * verses                                                (0-2)+(2-4)+(4-6)+(6-8)+(8-10)+(10-12)+(12-14)+(14-16)     = -16
 *  If however we square them before adding, we get      -16*-16 = 256   vs -2*-2 =4 (8 times) = 32
 *
 * So squaring makes the sharp transition much more noticable (and has the added advantage of making it +ve)
 */


Cf_stat diff(unsigned char one, unsigned char other )
{
  int x = one;
  int y = other;  /* we want signed arithmetic) */
  Cf_stat result;

  result = (Cf_stat)((x-y)*(x-y)); /* result will always be +ve, so we lose nothing ...doing unsigned subtraction would cause big numbers */

  return (result);
}


char *colorspace_string(J_COLOR_SPACE space)
{
  char *name;
  
  switch(space)
    {
    case (JCS_UNKNOWN):
      name="JCS_UNKNOWN"; /* error/unspecified */
      break;
	      
    case (JCS_GRAYSCALE):
      name="JCS_GRAYSCALE"; /* monochrome */
      break;
	      
    case (JCS_RGB):
      name="JCS_RGB"; /* red/green/blue as specified by the RGB_RED, RGB_GREEN, RGB_BLUE, and RGB_PIXELSIZE macros */
      break;
                             
    case (JCS_YCbCr):
      name="JCS_YCbCr"; /* Y/Cb/Cr (also known as YUV) */
      break;
	      
    case (JCS_CMYK):
      name="JCS_CMYK"; /* C/M/Y/K */
      break;
	      
    case (JCS_YCCK):
      name="JCS_YCCK"; /* Y/Cb/Cr/K */
      break;
      
#if defined(HAVE_DECL_JCS_EXT_RGB) && (HAVE_DECL_JCS_EXT_RGB==1)
    case (JCS_EXT_RGB):
      name="JCS_EXT_RGB"; /* red/green/blue */
      break;
	      
    case (JCS_EXT_RGBX):
      name="JCS_EXT_RGBX"; /* red/green/blue/x */
      break;
	      
    case (JCS_EXT_BGR):
      name="JCS_EXT_BGR"; /* blue/green/red */
      break;
	      
    case (JCS_EXT_BGRX):
      name="JCS_EXT_BGRX"; /* blue/green/red/x */
      break;
	      
    case (JCS_EXT_XBGR):
      name="JCS_EXT_XBGR"; /* x/blue/green/red */
      break;
	      
    case (JCS_EXT_XRGB):
      name="JCS_EXT_XRGB"; /* x/red/green/blue */
      break;
	      
    case (JCS_EXT_RGBA):
      name="JCS_EXT_RGBA"; /* red/green/blue/alpha */
      break;
	      
    case (JCS_EXT_BGRA):
      name="JCS_EXT_BGRA"; /* blue/green/red/alpha */
      break;
	      
    case (JCS_EXT_ABGR):
      name="JCS_EXT_ABGR"; /* alpha/blue/green/red */
      break;
	      
    case (JCS_EXT_ARGB):
      name="JCS_EXT_ARGB"; /* alpha/red/green/blue */
      break;
	      
    case (JCS_RGB565):     /* 5-bit red/6-bit green/5-bit blue */
      name="JCS_RGB565";
    break;
#endif

    default:
      name="unknown";
      break;
    }
  return (name);
};


void dump_3component(unsigned char comp[])
{
  fprintf(stderr, "[%d,%d,%d]", comp[0], comp[1], comp[2]);
}

void dump_1component(unsigned char comp[])
{
  fprintf(stderr, "[%d]", comp[0]);
}

/* Note output width is in terms of components */
void dump_1row(int output_components, JSAMPARRAY buffer, Tcol output_width)
{
  // NB
  // typedef JSAMPLE *JSAMPROW;      /* ptr to one image row of pixel samples. */
  // typedef JSAMPROW *JSAMPARRAY;   /* ptr to some rows (a 2-D sample array) */
  // typedef JSAMPARRAY *JSAMPIMAGE; /* a 3-D sample array: top index is color */

  int            i;
  unsigned char *p;

  p = *buffer;

  fprintf(stderr, "row:{");
  
  for (i=0; i<output_width; ++i)
    {
      if (output_components == 3)
	{
	  dump_3component(p);
	  p+=3;
	}
      else if (output_components == 1)
	{
	  dump_1component(p);
	  p+=1;
	}
      else
	{
	  fprintf(stderr, "unable to dump components of size=%d", output_components);
	}
    }
  
  fprintf(stderr, "}\n");
}
