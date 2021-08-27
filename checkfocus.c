#include "config.h"

#include "checkfocus.h"
#include <getopt.h>
#include <string.h>

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

/* defined by jmorecfg.h ... */
//#define RGB_RED         0       /* Offset of Red in an RGB scanline element */
//#define RGB_GREEN       1       /* Offset of Green */
//#define RGB_BLUE        2       /* Offset of Blue */
//#define RGB_PIXELSIZE   3       /* JSAMPLEs per RGB scanline element */


static int   channel = RGB_GREEN;

static struct box box = {0,0,0,0};

static int shift=0;              /* Shift right N places on output */

/* default Fudge image size (a 96MP image) other images are in relation to this */
#define FUDGEROWS 8000
#define FUDGECOLS 12000

static int fudge_rows=0;
static int fudge_columns=0;


static Cf_fudge hfudge=1.0;	 /* fudge factor for scores horozontal/vertical/overall */
static Cf_fudge vfudge=1.0;
static Cf_fudge fudge=1.0;

/* Limits set quite broad, not really under/over exposed, but if eg. 98% of your image is "dark" it's proably no good */
static unsigned char overexposed_limit  = 255-25;
static unsigned char underexposed_limit = 0+25;


struct Scores
{
  struct Cf_scores  image;       /* scores for the whole image     */
  struct Cf_scores  centre_box;  /* scores for just the centre box */
  int               overexposed; /* Number of cells (on the given channel) at or above limit */
  int               underexposed;
  int               samples;
};

static char *current_filename;   /* used ONLY for error messages ... poor encapsulation */


/*
 * Descr:   Collects status info on the focus of a given image
 * @Author: Graeme Vetterlein
 */


/* If a function has more than six argumensts, you missed one */
static void calculate_contrast(Trow              row,
			       Tcol              output_width,
			       int               output_components,
			       int		 focus_channel,
			       JSAMPARRAY        prev_buffer,
			       JSAMPARRAY        this_buffer,
			       struct box      * this_box,
			       struct Scores   * result)
{
  Tcol            col;
  unsigned char * p;
  unsigned char * q;

  p = *this_buffer;
  q = *prev_buffer;

  for (col=0; col<output_width-1; ++col)  /* NB, stop one component short */
    {
      Cf_stat interrow_value = diff(p[focus_channel],q[focus_channel]);  /* for greyscale [0] for green [1] etc */
      Cf_stat intercol_value = diff(p[focus_channel],(p+output_components)[focus_channel]);  /* for greyscale [0] for green [1] etc */

      if (debug > 2)
	{
	  fprintf(stderr, "col=%d, row=%d, interrow=%llu, intercol=%llu\n",  col, row, interrow_value, intercol_value);
	}

      
      result->image.overall    += interrow_value + intercol_value;
      result->image.horizontal += intercol_value;
      result->image.vertical   += interrow_value;

      if (inbox(this_box, col, row))
	{
	  result->centre_box.overall    += interrow_value + intercol_value;
	  result->centre_box.horizontal += intercol_value;
	  result->centre_box.vertical   += interrow_value;

	  if (debug >2)
	    fprintf(stderr, "col=%d,row=%d is (score=%llu) is inside box stat=%llu\n", col, row, interrow_value + intercol_value,result->centre_box.overall);
	}

      if (p[focus_channel] >= overexposed_limit)
	++result->overexposed;

      if (p[focus_channel] <= underexposed_limit)
	++result->underexposed;

      ++result->samples;
      
	/* step to the next component */
      p+=output_components;
      q+=output_components;
    }
}


static int read_jpeg_file(FILE * const infile, struct Scores * result)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  JSAMPARRAY                    buffer1;
  JSAMPARRAY                    buffer2;

  JSAMPARRAY                  * this_row;
  JSAMPARRAY                  * prev_row;
  JSAMPARRAY                  * temp_row;

  
  J_COLOR_SPACE out_color_space;       /* colorspace for output */
  Tcol		output_width;          /* scaled image width */
  Trow		output_height;         /* scaled image height */
  int           out_color_components;  /* # of color components in out_color_space */
  int           output_components;     /* # of color components returned */

  Trow		row;
  Trow		row_stride;

  int		focus_channel;

  struct box	this_box;

  result->image.overall    = 0;
  result->image.horizontal = 0;
  result->image.vertical   = 0;

  result->centre_box.overall    = 0;
  result->centre_box.horizontal = 0;
  result->centre_box.vertical   = 0;

  result->overexposed           = 0;
  result->underexposed          = 0;
  result->samples               = 0;
  
  cinfo.err = jpeg_std_error(&jerr);   /* standard error handling (writes to stderr) */
        
   
  jpeg_create_decompress(&cinfo);	/* init */
  jpeg_stdio_src(&cinfo, infile);	/* Set input */
 
  (void) jpeg_read_header(&cinfo, TRUE); /* From now we have the basic stats of the jpeg (e.g. width& height) */

  /* now we set any  "parameters for decompression" (we have none, defaults were set by jpeg_read_header) */

  (void) jpeg_start_decompress(&cinfo);  /* now we're live, using the parameters just set */


  /* mostly a documentation aid, to make it clear whch values we use from "cinfo" */
  out_color_space      = cinfo.out_color_space;       /* colorspace for output */
  output_width         = (Tcol)cinfo.output_width;          /* scaled image width */
  output_height        = (Trow)cinfo.output_height;         /* scaled image height */
  out_color_components = cinfo.out_color_components;  /* # of color components in out_color_space */
  output_components    = cinfo.output_components;     /* # of color components returned */

  focus_channel        = (out_color_components == 1) ? 0 : channel; /* greyscale only has index 0 */
  
  if (box_defined(&box))
    copy_box(&this_box, &box);
  else
    {
      Tcol box_half_width  = (output_width  * 5 / 100);  /* 5% */
      Trow box_half_height = (output_height * 5 / 100);  /* 5% */

      Trow mid_row         = output_height/2;
      Tcol mid_column      = output_width/2;

      this_box.first_row     = mid_row - box_half_height;  /* so total box is 25% of image, in the centre */
      this_box.last_row      = mid_row + box_half_height;
      this_box.first_column  = mid_column - box_half_width;
      this_box.last_column   = mid_column + box_half_width;
    }

  if (verbose)
    {
      fprintf(stderr, "Box used %d:%d-%d:%d\n", this_box.first_column, this_box.first_row, this_box.last_column, this_box.last_row);
    }
  
  
  if (verbose)
    {
      fprintf(stderr, "Colour Space     : %s\n", colorspace_string(out_color_space));
      fprintf(stderr, "Output Width     : %d\n", output_width);
      fprintf(stderr, "Output Height    : %d\n", output_height);
      fprintf(stderr, "Output Colour Components: %d Number of components in colour Space)\n", out_color_components);
      fprintf(stderr, "Output Components       : %d (actual number per Pixel [1 iff indexed])\n", output_components);
    }
  
  row_stride = (Trow)cinfo.output_width * (Trow)cinfo.output_components; /* in bytes */

  if (fudge_rows)
    {
      double xscale;
      double yscale;
      double scale;

      if (output_width > fudge_columns || output_height > fudge_rows)
	{
	  fprintf(stderr, "Actual image %s cols:row (%d:%d) exceeds fudge size (%d:%d), no fudging will be done\n",
		  current_filename, output_width , output_height, fudge_columns,  fudge_rows);

	  hfudge = vfudge = fudge = 1.0;
	}

      xscale = (double)fudge_columns              / (double)output_width;   /* >= 1 */
      yscale = (double)fudge_rows                 / (double)output_height;  /* >= 1 */
      scale =  (double)(fudge_rows+fudge_columns) / (double)(output_height+output_height);  /* >= 1 */

      /* consider fudge size was (the default of) 12000x8000 an actual image was
	 6000x4000, then scale=xscale=yscale=2, so the fudge factor would be
	 sqrt(2) = 1.41 so if this image got a score of 1000, then it's
	 "equivalent" to a 12000x800 image that got a score of 1410.  Just
	 because the bigger image had more pixels contributing to it's score.

	 This allows one to compare a 6000x4000 image (fudge factor = 1.41) with
	 a 3000x2000 image (fudge factor = 2) .  This is far from an exact
	 science, if you shink an image it actally gets sharper if you (blurred
	 grey line becomes a sharp black line) if you enlarge an image it gets
	 fuzzier; but if an image with 200 horizontal lines (at a certain level
	 of focus) were compared with a 400 line image (at same level of focus),
	 withot fudging, the latter would win because it had more lines
	 contributing.
	 
	 Depending on your planned use of a image, it may be better to consider the raw score
	 or the fudged one.

      */
      hfudge = sqrt(xscale);
      vfudge = sqrt(yscale);
      fudge  = sqrt(scale);
    }


  if (verbose)
    {
      fprintf(stderr, "Fudge Factors Horizontal=%1.3G, vertical=%1.3G, overall=%1.3G \n", hfudge, vfudge, fudge);
      fprintf(stderr, "Bits to right shift scores=%d\n", shift);
     }

  
  /* Use internal memory manager to give us a buffer ..sadly (and undocumented) it wants BYTES not components */
  buffer1 = (cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,
				       JPOOL_IMAGE,            /* just for this (jpeg) image */
				       (JDIMENSION)row_stride, /* Number of bytes (not samples) */
				       1);

  buffer2 = (cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,
				       JPOOL_IMAGE,         
				       (JDIMENSION)row_stride,	    
				       1);
         
  /* read the 1st row (special case) outside of loop */
  (void) jpeg_read_scanlines(&cinfo, buffer1, 1);
  
  if (debug && verbose)
    dump_1row(output_components, buffer1, output_width);

  prev_row = &buffer1;
  this_row = &buffer2;
  
  
  /* Do somthing clever with flip/flop (2 pointers) [avoid clever XOR trick, confuses non-SW engineers]*/

  row=1; /* done 1 already */
  
  if (debug)
      fprintf(stderr, "will process %d rows\n", output_height);
 
  while (cinfo.output_scanline < cinfo.output_height)
    {
      (void) jpeg_read_scanlines(&cinfo, *this_row, 1);
      ++row;

      calculate_contrast(row, output_width, output_components, focus_channel, *prev_row, *this_row, &this_box, result); 
      
      temp_row    = prev_row;
      prev_row    = this_row;   /* what was current, now becomes previous  */
      this_row    = temp_row;	/* the old "prev" is now free to be reused */
    }

  if (debug)
      fprintf(stderr, "ended with row %d\n", row);
       
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
 
  return 0;
}


static void usage(char prog[])
{
  fprintf(stderr, "Usage: %s [-v|--verbose][-d|--debug][-r|--red|-b|--blue|-g|--green]\n"
	          "[-B x:y-x:y|--box x:y-x:y][-f <filelistname>|--file <filelistname>] [-F[<rows:cols>] | --fudge=[<row:col>]\n"
	          "[-o<0-255>] | --overexposed=<0-255> [-u<0-255>] | --underxposed=<0-255> <list of filenames>\n"
		  "-s <bits>|--shift <bits>\n"
		  "\n"
                  "-v or --verbose produce more verbose output, can be repeated for more verbosity\n"
                  "-d or --debug produce debug output on stderr, can be repeated for more verbosity\n"
	          "[-r|--red|-b|--blue|-g|--green] base the calculations on reg/green or blue channels, default is green, ignored for greyscale\n"
		  "-B x:y-x:y|--box x:y-x-y, where 0,0 is top LH corner of image, x is colum, y is row. Defines a smaller 'box' to analyse'\n"
		  "-s <bits>|--shift <bits> shift the output scores <bits> right. Simply to reduce the scale of numbers to avoid breaking other tools using this data\n"
		  "[-f <filelistname>|--file <filelistname>] filelistname is a file which contains a list of files, one per line. These are processed before <list of filenames>\n"
		  "[-F[<rows:cols>] | --fudge=[<row:col>] {note the syntax} defines a fudgebox, adjusts score to match this\n"
		  "[-o<0-255>] | --overexposed=<0-255> default is 230 \n"
		  "[-u<0-255>] | --underxposed=<0-255> default is 25\n"

	  , prog);
}


static int process_1file(char * input_filename)
{
  
  FILE           *input_file;
  struct Scores result;

  if (verbose)
    fprintf(stderr, "Processing file: [%s]\n", input_filename);

  if ((input_file = fopen(input_filename, "rb")) == NULL)
    {
    fprintf(stderr, "can't open [%s]\n", input_filename );
    return -1;
    }

  read_jpeg_file(input_file, &result);

  printf("%s %llu %llu %llu %llu %llu %llu %d %d\n",
	 input_filename,
	 adjust(result.image.overall,         shift, fudge),  /* we only fudge the overall score, box is smaller (so fixed if required) */
	 adjust(result.image.horizontal,      shift, hfudge),
	 adjust(result.image.vertical,        shift, vfudge),	 
	 adjust(result.centre_box.overall,    shift, 0.0),
	 adjust(result.centre_box.horizontal, shift, 0.0),
	 adjust(result.centre_box.vertical,   shift, 0.0),
	 (100*result.overexposed)/result.samples,             /* Order of evaluation is important */
	 (100*result.underexposed)/result.samples             /* %age of cells at or below limit */
	 );
  
  if (verbose)
    fprintf(stderr, "\n");

  fclose(input_file);
  
  return 0;
}



int main(int argc, char **argv)
{
  char           *input_filename;
  struct Scores   result;

  char *filelist  = NULL;
  char *boxstring = NULL;
  char *fudgebox  = NULL;
  int   x;
  
  int   c;

  while (1)
    {
      int option_index = 0;
      static struct option long_options[] =
	{
	 /* name           has_arg,           flag, val */
	 {"debug",         no_argument,       0,    'd' },
	 {"verbose",       no_argument,       0,    'v' },
	 {"red",           no_argument,       0,    'r' },
	 {"blue",          no_argument,       0,    'b' },
	 {"green",         no_argument,       0,    'g' },
	 {"file",          required_argument, 0,    'f' },
	 {"box",           required_argument, 0,    'B' },
	 {"shift",         required_argument, 0,    's' },
	 {"fudge",         optional_argument, 0,    'F' },
	 {"overexposed",   required_argument, 0,    'o' },
	 {"underexposed",  required_argument, 0,    'u' },

	 {0,         0,                 0,    0 }
	};
      
      c = getopt_long(argc,
		      argv,
		      "dvrbgf:B:s:F::o:u:",
		      long_options,
		      &option_index);
      
      if (c == -1)
	break;

      switch (c)
	{

	case 'd':
	  debug+=1;
	  break;

	case 'v':
	  verbose+=1;
	  break;

	case 'r':
	  channel= RGB_RED;
	  break;

	case 'b':
	  channel= RGB_BLUE;
	  break;

	case 'g':
	  channel= RGB_GREEN;
	  break;

	case 'f':
	  filelist=optarg;
	  break;

	case 'B':
	  boxstring=optarg;
	  break;

	case 's':
	  shift=atoi(optarg);
	  break;

	case 'F':
	  if (optarg)
	    fudgebox=optarg;
	  else
	    fudgebox="";
	  break;

	case 'o':
	  x=atoi(optarg);
	  if (x < 0 || x > 255)
	      fprintf(stderr, "--overexposed must be in range 0-255, setting ignored");
	  else
	    overexposed_limit=(unsigned char)x;
	  break;

	case 'u':
	  x=atoi(optarg);
	  if (x < 0 || x > 255)
	      fprintf(stderr, "--underexposed must be in range 0-255, setting ignored");
	  else
	    underexposed_limit=(unsigned char)x;
	  break;

	default:
	  usage(argv[0]);
	  exit(1);
	}
    }

  if (verbose)
    {
    fprintf(stderr, "Package %s[%s] %s - check overall image focus\n", PACKAGE_NAME, PACKAGE_VERSION, argv[0]);
    fprintf(stderr, "Overexposure limit set to %d\n"
	            "Underexposure limit set to %d\n",
	            overexposed_limit,
	            underexposed_limit);
    }
  
  if (debug)
    fprintf(stderr, "Following getopt %d arguments remain\n", argc-optind);


  if (debug)
    {
      result.image.overall  = 0;
      result.image.overall -= 1;
      
      fprintf(stderr, "The size of stats is %ld\n"
	              "It's maximum value is %llu\n",
	      sizeof(result.image.overall),
	      result.image.overall);
    }


  if (boxstring)
    {
      Tcol topx,bottomx;
      Trow topy,bottomy;
      
      if (sscanf(boxstring, "%d:%d-%d:%d", &topx, &topy, &bottomx, &bottomy ) !=4)
	{
	  fprintf(stderr, "Syntax error in --box, should be x:y-x:y value is [%s]\n", boxstring);
	  exit(1);
	}
      box.first_column=topx;
      box.first_row=topy;
      
      box.last_column=bottomx;
      box.last_row=bottomy;

      if (verbose)
	{
	  fprintf(stderr, "Will use a fixed box of %d:%d-%d:%d\n", box.first_column, box.first_row, box.last_column, box.last_row);
	}
    }

  if (fudgebox)
    {
      int rows;
      int columns;

      if (fudgebox[0] == '\0')
	{
	  rows    = FUDGEROWS;
	  columns = FUDGECOLS;
	}
      else
	{
	  if (sscanf(fudgebox, "%d:%d", &rows, &columns ) !=2)
	    {
	      fprintf(stderr, "Syntax error in --fudge, should be --fudge=x:y value is [%s]\n", fudgebox);
	      exit(1);
	    }
	}
      
      fudge_rows    = rows;
      fudge_columns = columns;

      if (verbose)
	fprintf(stderr, "Scores will be fudged to match an image of %d:%d\n", fudge_rows, fudge_columns);
    }


  
#define MAX_FILENAME 4096
  
  if (filelist)
    {
      FILE * p_filelist;
      char   indirect_filename[MAX_FILENAME];  /* huge, to avoid buffer overflow risks */
      
      if (verbose)
	fprintf(stderr, "Processing filelist: %s\n", filelist);

      if ((p_filelist = fopen(filelist, "rb")) == NULL)
	{
	fprintf(stderr, "can't open [filelist] %s\n", filelist );
	return -1;
	}

      while (fgets(indirect_filename, MAX_FILENAME, p_filelist))
	{
	  current_filename = indirect_filename;
	  indirect_filename[strcspn(indirect_filename, "\n")] = 0;  /* splat out the /n, otherwise bad filename */
	  process_1file(indirect_filename);
	}
      fclose(p_filelist);
    }
  
  /* Trailing args are filename */

  for (;optind < argc; ++optind)
    {
      input_filename = argv[optind];
      current_filename = input_filename;

      process_1file(input_filename);
    }

  exit(0);
}
