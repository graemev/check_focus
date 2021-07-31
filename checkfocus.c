#include "checkfocus.h"
#include <getopt.h>

static int debug=0;
static int verbose=0;

/* defined by jmorecfg.h ... */
//#define RGB_RED         0       /* Offset of Red in an RGB scanline element */
//#define RGB_GREEN       1       /* Offset of Green */
//#define RGB_BLUE        2       /* Offset of Blue */
//#define RGB_PIXELSIZE   3       /* JSAMPLEs per RGB scanline element */


static int   channel = RGB_GREEN;

static char *filelist  = NULL;
static char *boxstring = NULL;

static struct box box = {0,0,0,0};

/*
 * Descr:   Collects status info on the focus of a given image
 * @param:  file open FD (e.g. jpeg) 
 * @param:  result struct of useful stats about the jpeg
 * @Author: Graeme Vetterlein
 * @return: Return 0 if successful, -1 if failed
 */


/* If a function has more than six argumensts, you missed one */
static void calculate_contrast(int               row,
			       JDIMENSION        output_width,
			       int               output_components,
			       int		 focus_channel,
			       JSAMPARRAY        prev_buffer,
			       JSAMPARRAY        this_buffer,
			       struct box      * this_box,
			       struct Cf_stats * result)
{
  int             col;
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
	  fprintf(stderr, "col=%u, row=%u, interrow=%llu, intercol=%llu\n",  col, row, interrow_value, intercol_value);
	}

      
      result->overall    += interrow_value + intercol_value;
      result->horizontal += intercol_value;
      result->vertical   += interrow_value;

      if (inbox(this_box, row, col))
	{
	  result->square            += interrow_value + intercol_value;
	  result->square_horizontal += intercol_value;
	  result->square_vertical   += interrow_value;
	}
	/* step to the next component */
      p+=output_components;
      q+=output_components;
    }
}


int read_jpeg_file(FILE * const infile, struct Cf_stats * result)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;

  JSAMPARRAY                    buffer1;
  JSAMPARRAY                    buffer2;

  JSAMPARRAY                  * this_row;
  JSAMPARRAY                  * prev_row;
  JSAMPARRAY                  * temp_row;

  
  J_COLOR_SPACE out_color_space;       /* colorspace for output */
  JDIMENSION    output_width;          /* scaled image width */
  JDIMENSION    output_height;         /* scaled image height */
  int           out_color_components;  /* # of color components in out_color_space */
  int           output_components;     /* # of color components returned */

  int		row;
  int	        row_stride;

  int		focus_channel;

  struct box	this_box;

  result->overall = 0;
  result->horizontal = 0;
  result->vertical = 0;

  result->square= 0;
  result->square_horizontal =0;
  result->square_vertical =0;

  
  cinfo.err = jpeg_std_error(&jerr);   /* standard error handling (writes to stderr) */
        
   
  jpeg_create_decompress(&cinfo);	/* init */
  jpeg_stdio_src(&cinfo, infile);	/* Set input */
 
  (void) jpeg_read_header(&cinfo, TRUE); /* From now we have the basic stats of the jpeg (e.g. width& height) */

  /* now we set any  "parameters for decompression" (we have none, defaults were set by jpeg_read_header) */

  (void) jpeg_start_decompress(&cinfo);  /* now we're live, using the parameters just set */


  /* mostly a documentation aid, to make it clear whch values we use from "cinfo" */
  out_color_space      = cinfo.out_color_space;       /* colorspace for output */
  output_width         = cinfo.output_width;          /* scaled image width */
  output_height        = cinfo.output_height;         /* scaled image height */
  out_color_components = cinfo.out_color_components;  /* # of color components in out_color_space */
  output_components    = cinfo.output_components;     /* # of color components returned */

  focus_channel        = (out_color_components == 1) ? 0 : channel; /* greyscale only has index 0 */
  
  if (box_defined(&box))
    copy_box(&this_box, &box);
  else
    {
      int box_half_width  = (output_width  * 5 / 100);  /* 5% */
      int box_half_height = (output_height * 5 / 100);  /* 5% */

      int mid_row         = output_height/2;
      int mid_column      = output_width/2;

      this_box.first_row      = mid_row - box_half_width;  /* so total box is 10% of image, in the centre */
      this_box.last_row      = mid_row + box_half_width;
      this_box.first_column  = mid_column - box_half_height;
      this_box.last_column   = mid_column + box_half_height;
    }

  if (verbose>1)
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
  
  row_stride = cinfo.output_width * cinfo.output_components; /* in bytes */

  /* Use internal memory manager to give us a buffer ..sadly (and undocumented) it wants BYTES not components */
  buffer1 = (cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,
				       JPOOL_IMAGE,            /* just for this (jpeg) image */
				       row_stride,	     /* Number of bytes (not samples) */
				       1);

  buffer2 = (cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo,
				       JPOOL_IMAGE,         
				       row_stride,	    
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
  fprintf(stderr, "Usage: %s [-v|--verbose][-d|--debug][-r|--red|-b|--blue|-g|--green][-B x:y-x:y|--box x:y-x:y][-f <filelistname>|--file <filelistname>] <list of filenames>\n"
                  "-v or --verbose produce more verbose output, can be repeated for more verbosity\n"
                  "-d or --debug produce debug output on stderr, can be repeated for more verbosity\n"
	          "[-r|--red|-b|--blue|-g|--green] base the calculations on reg/green or blue channels, default is green, ignored for greyscale\n"
		  "-B x:y-x:y|--box x:y-x-y, where 0,0 is top LH corner of image, x is colum, y is row. Defines a smaller 'box' to analyse'\n"
		  "[-f <filelistname>|--file <filelistname>] filelistname is a file which contains a list of files, one per line. These are processed before <list of filenames>\n"
	  , prog);
}


int main(int argc, char **argv)
{

  FILE           *input_file;
  char           *input_filename;
  struct Cf_stats result;

  int c;

  while (1)
    {
      int option_index = 0;
      static struct option long_options[] =
	{
	 /* name     has_arg,           flag, val */
	 {"debug",   no_argument,       0,    'd' },
	 {"verbose", no_argument,       0,    'v' },
	 {"red",     no_argument,       0,    'r' },
	 {"blue",    no_argument,       0,    'b' },
	 {"green",   no_argument,       0,    'g' },
	 {"file",    required_argument, 0,    'f' },
	 {"box",     required_argument, 0,    'B' },
	 {0,         0,                 0,    0 }
	};
      
      c = getopt_long(argc,
		      argv,
		      "dvrbgf:B:",
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

	default:
	  usage(argv[0]);
	  exit(1);
	}
    }

  if (debug)
    fprintf(stderr, "Following getopt %d arguments remain\n", argc-optind);


  if (debug)
    {
      result.overall  = 0;
      result.overall -= 1;
      
      fprintf(stderr, "The size of stats is %ld\n"
	              "It's maximum value is %llu\n",
	      sizeof(result.overall),
	      result.overall);
    }


  if (boxstring)
    {
      int topx,topy,bottomx,bottomy;
      
      if (sscanf(boxstring, "%d:%d-%d:%d", &topx, &topy, &bottomx, &bottomy ) !=4)
	{
	  fprintf(stderr, "Syntax error in --box, should be x:y-x:y value is [%s]\n", boxstring);
	  exit(1);
	}
      box.first_column=topx;
      box.first_row=topy;
      
      box.last_column=bottomx;
      box.last_row=bottomy;
    }
  
  
  for (;optind < argc; ++optind)
    {
      input_filename = argv[optind];

      if (verbose)
	fprintf(stderr, "Processing file: %s\n", input_filename);

      if ((input_file = fopen(input_filename, "rb")) == NULL) {
	fprintf(stderr, "can't open %s\n", input_filename );
	return -1;
      }
      read_jpeg_file(input_file, &result);

      printf("%s %llu %llu %llu %llu %llu %llu\n",
	     input_filename,
	     result.overall,
	     result.horizontal,
	     result.vertical,

	     result.square,
	     result.square_horizontal,
	     result.square_vertical);

      if (verbose)
	fprintf(stderr, "\n");



    }

  exit(0);
}
