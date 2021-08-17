#include "checkfocus.h"
#include <getopt.h>
#include <string.h>
#include <time.h>

/*
 * Find the "box" with the "best focus".
 * As an attempt to avoid an edge case, where the image just happens to line up on predefined boxes
 * we define overlapping boxes as follows:
 *
 * Assume an image is 100 pixels square (0 to 99) and we define 10 frameboxes (0-9)(10-19)...(90-99)
 * So box1 is:  "0:0-9:9" box2 is "10:0-19:9" box3 is "20:0-29:9"  (top LH corner to bottom right)
 *
 * Then we define an overlapping set of "alternate " boxes, at half way points
 *
 * box101 is 5:5-14:14 , box102 is 15:5-24:14 ... box109 is 85:5-94:14
 *
 *
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
 * | box001 | box002 | box003 | box004 | box005 | box006 | box007 | box008 | box009 | box010 |
 * |        |        |        |        |        |        |        |        |        |        |
 * |        |        |        |        |        |        |        |        |        |        |
 * |    .========.========.========.========.========.========.========.========.========.=========.
 * |    : BOX101 : BOX102 : BOX103 : BOX104 : BOX105 : BOX106 : BOX107 : BOX108 : BOX109 : BOX110  :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * +----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----+---+-----+
 * | box011 | box012 | box013 | box014 | box015 | box016 | box017 | box018 | box019 | box020 |	   :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    :   |    :   | X  :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    .========.========.========.========.========.========.========.========.========.=========.
 * |    : BOX111 : BOX112 : BOX113 :  BOX114: BOX115 :  BOX116: BOX117 : BOX118 : BOX119 : BOX120  :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |	   :
 * +----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----+---+-----+
 *
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ similar ~~~~~~~~~~~~~~~~~~~~~~~
 *
 * |    .========.========.========.========.========.========.========.========.========.========.
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * +----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----:---+----+---+----+
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :   |    :
 * |    : BOX181 : BOX182 : BOX183 : BOX184 : BOX185 : BOX186 : BOX187 : BOX188 : BOX189 : BOX190 :
 * |    .========.========.========.========.========.========.========.========.========.========.
 * |        |        |        |        |        |        |        |        |        |        |
 * |        |        |        |        |        |        |        |        |        |        |
 * |        |        |        |        |        |        |        |        |        |        |
 * | box091 | box092 | box093 | box094 | box095 | box096 | box097 | box098 | box099 | box100 |
 * +--------+--------+--------+--------+--------+--------+--------+--------+--------+--------+
 *

 * To be clear, we ALWAYS get box1 ... box189 (190 boxes). If we had an image
 * 105 by 108 pixels it would still define the above 190 boxes so the right hand
 * edge would still be 99, the lower bound also 99. This means we would not spot
 * good focus near the edges. For real world photography it's unlikely these are
 * the areas where the "intended subject" lay.
 *
 * If we had an image 2000 by 1000, the "boxes" would start with 0:0-199:99
 *
 * So overall, each box holds 1% of the image. 190 boxes means most of the image
 * is seen in two boxes. In the above diagram the "X" is in boxes box013 and BOX102.
 *

 * TBD, change the model to have "frames" (the above frameboxes) , then in each
 * frame we define a "box" , e.g. we might have a 50x50 box (realworld images are
 * 4-9000 pixel wide) and we define a 12x12 "box" inside the frame. So we end up
 * with many 12x12 boxes separated by 50-12=38 pixels. Why do this? Well imagine
 * an image (like a focus test pattern) with a big black&white circle in the
 * middle maybe 200 pixels wide. Elsewhere there is some text, consider the
 * letter "E" going vertically down we get white, black, white, black, white,
 * black, white assuming the "E" was "goodish" focus, we might get
 * 000,009,250,255 (4 lines to go from black to white) and the big circle was
 * perfect 000,255 We'd actually choose a block of text containing the "E"
 * because while 000,255 (B&W circle) gets a score of 65,025 , 000,009 gets 81 +
 * 009,250 gets 58081 + 250,255 gets 25 (total 58187), then the next E gets
 * another 58187, already we're ahead of the 65,025. Why did this happen? Well
 * our definition of "good focus" means lots of "vertical/horizontal lines"
 * outscores a single big black/white transition. So to "bias" the mechanism to
 * choose the "big circle" we'd need to choose boxes so small that a single edge
 * of an "E" would fill a box (so the box would be about 6 times smaller than an
 * "E") the the 65,025 would beat the 58081. However this is very
 * computationally intensive (might be worth a go) so we approximate this
 * behavior by using a very small but skipped through the image. This has the
 * downside that in the exact case of the "focus chart" we might not end up with
 * a box covering the Black/White line in the big circle. In a, real world,
 * image these big monochrome area are unlikely and we should find "something"
 * in the scattered small boxes. Which works best for your image? Who knows?
 * I'm a landscape photographer and images are always large areas of good focus
 * at infinity with small out-of-focus images in the foreground. I'm a Portrait
 * photographer and images are significant in-focus foreground and large fuzzy
 * background. I'm an insect photographer and a good image has a small bit in
 * perfect focus and it's mostly out of focus.
 *
 *
 */
/* eg 10 boxes horizontal and 10 boxes vertical */

/* 500x500 frames would be 250,000 frames, hence similar number of focus boxes ... lots of Maths */
#define MAX_HFRAMES (500)

#define HFRAMES (100)
#define VFRAMES (100)

/* eg 100 primary (and 100 secondary) so 200 total boxes (ie on paper) */
#define PBOXES (HFRAMES*VFRAMES)

/* eg 10 primary (and 10 secondary) boxes at any one time (ie in memory)*/

#define MAX_CURSORS (MAX_HFRAMES*2)
#define CURSORS (HFRAMES*2)  /* TBD chnage to MAX */

int vframes = VFRAMES;
int hframes = HFRAMES;
int cursors = CURSORS;


static int channel = RGB_GREEN;
static int opt_box_height = 0;
static int opt_box_width  = 0;

static char *filelist  = NULL;


static struct numbered_box
{
  int	     box_no;
  Cf_stat    stat;
  struct box box;
} nbox [MAX_CURSORS];   /* e.g. 10 primary, 10 secondary */

static struct numbered_box best_box = {0,0,{0,0,0,0}};

static void dump_nbox(struct numbered_box *p)
{
  fprintf(stderr,"box%03d: Stat=%llu (%d:%d-%d:%d)",
	  p->box_no,
	  p->stat,
	  p->box.first_column, p->box.first_row, p->box.last_column, p->box.last_row);
}


static CF_BOOL update_box(struct numbered_box *p, int row, int col, int frame_height, Cf_stat interrow_value, Cf_stat intercol_value)
{
  CF_BOOL ended = CF_FALSE;

  if (inbox(&(p->box), col, row))    /* considering just this one box, are we inside it? */
    {
      p->stat                += interrow_value + intercol_value;  /* TBD use a flag to limit to just horizontal or vertical */
    }

  ended = row >= p->box.last_row;	/* have we reached the final row in this box? */

  if (ended)	/* We've reached the end of this BOX ...shuffle down, we go off the "end" , but code stops at end of image */
    {
    if (verbose>1)
      {
	fprintf(stderr, "box stat complete: ");
	dump_nbox(p);
	fprintf(stderr, "\n");
      }

      if (p->stat > best_box.stat)	/* Horra, we are now the "best box" */
	{
	  best_box.box_no = p->box_no;
	  best_box.stat   = p->stat;
	  copy_box(&best_box.box, &(p->box));

	  if (verbose)
	    {
	      fprintf(stderr, "New BESTBOX found: ");
	      dump_nbox(p);
	      fprintf(stderr, "\n");
	    }
	}

      p->box.first_row += frame_height;
      p->box.last_row  += frame_height;
      p->stat           = 0; /* start again*/
      p->box_no        += hframes;   /* e.g 10 on each row, so same place in next row */
    }
  return(ended);
}


/* If a function has more than six argumensts, you missed one */
static void box_contrast(int               row,
			 JDIMENSION        output_width,
			 int               output_components,
			 int		   focus_channel,
			 JSAMPARRAY        this_buffer,
			 JSAMPARRAY        prev_buffer,
			 int		   frame_height)
{
  int             col;
  int		  i;
  unsigned char * p;
  unsigned char * q;

  p = *this_buffer;
  q = *prev_buffer;

  for (col=0; col<output_width-1; ++col)  /* NB, stop one component short */
    {
      Cf_stat interrow_value = diff(p[focus_channel],q[focus_channel]);                      /* for greyscale [0] for green [1] etc */
      Cf_stat intercol_value = diff(p[focus_channel],(p+output_components)[focus_channel]);

      if (debug > 2)
	{
	  fprintf(stderr, "col=%u, row=%u, interrow=%llu, intercol=%llu\n",  col, row, interrow_value, intercol_value);
	}

      /* we've got 2 numbers (inter column and inter row) contrast .... these may be in 2 box (a primary and an alternate) */

      for (i=0; i<cursors; ++i)
	{
	  if (debug>2)
	    fprintf(stderr, "box_contrast: nbox[%d] is at %p\n", i, &nbox[i]);
	  
	  update_box(&nbox[i], row, col, frame_height, interrow_value, intercol_value);   /* NB we move down and entire FRAME (not box height) */
	}
      
      /* step to the next (jpeg) component */
      p+=output_components;
      q+=output_components;
    }
}



static int read_jpeg_file(FILE * const infile)
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

  int		frame_height;
  int		frame_width;
  int		half_height;
  int		half_width;

  int		box_height;
  int		box_width;

  int		i;

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

  frame_height           = output_height/vframes;	      /* size of full box  */
  frame_width            = output_width/hframes;
  half_height          = frame_height/2;                /* Offset of alternate boxes */
  half_width           = frame_width/2;

  best_box.box_no           = 0;
  best_box.stat             = 0;
  best_box.box.first_column = 0;
  best_box.box.first_row    = 0;
  best_box.box.last_column  = 0;
  best_box.box.last_row     = 0;

  box_width  = frame_width;
  box_height = frame_height;

  if (opt_box_height)
    {
    if (opt_box_height > frame_height)
      fprintf(stderr, "requested box height %d exceeds height of frame [%d], capped to frame height\n", opt_box_height, frame_height);
    else
      box_height = opt_box_height;
    }

  if (opt_box_width)
    {
    if (opt_box_width > frame_width)
      fprintf(stderr, "requested box width %d exceeds width of frame [%d], capped to frame width\n", opt_box_width, frame_width);
    else
      box_width = opt_box_width;
    }
  
  
  /* Create inital boxes (1st row has hframes, next alternate row of hframes */

  if (verbose)
    {
      fprintf(stderr, "Image will be divided into %d horizontal frames by %d vertical frames (plus as many again alternate frames)\n",  hframes,      vframes);
      fprintf(stderr, "horizontal frames are %d pixels wide by %d high\n",       frame_width,  frame_height);
      fprintf(stderr, "Each frame contains a single focus box of %d:%d high\n",  box_width,    box_height);
      
    }


  
  int column;
  column = 0;

  /* primary boxes ... note we space out as FRAME, but box sizes are size of BOX*/
  for (i=0; i<hframes; ++i)			/* e.g. 0 ... 9 */
    {
      nbox[i].box_no           = 1+i;           /* e.g. 1 to 10 */
      nbox[i].stat             = 0;
      nbox[i].box.first_row    = 0;             /* e.g. rows, 0 to 9 [in example] */
      nbox[i].box.last_row     = box_height-1;
      nbox[i].box.first_column = column;
      nbox[i].box.last_column  = column+box_width-1;
      column += frame_width;
    }

  column = half_width;

  /* alternate boxes  ... note we space out as FRAME, but box sizes are size of BOX*/
  for (i=0; i<hframes; ++i)                              /* eg 0 ... 9 */
    {
      nbox[hframes+i].box_no           = PBOXES+1+i;     /* e.g. 10 .. 19 get 101 to 110 */
      nbox[hframes+i].stat             = 0;
      nbox[hframes+i].box.first_row    = half_height;    /* e.g. rows, e.g. 5 to 14 [in example] */
      nbox[hframes+i].box.last_row     = box_height+half_height-1;
      nbox[hframes+i].box.first_column = column;
      nbox[hframes+i].box.last_column  = column+box_width-1;
      column += frame_width;
    }

  if (debug)
    {
      fprintf(stderr, "initial Boxes defined\n");

      for (i=0; i<(cursors); ++i)
	{
	  fprintf(stderr, "%0d:", i);
	  dump_nbox(&nbox[i]);
	  fprintf(stderr, "\n");
	}
    }


  if (verbose)
    {
      fprintf(stderr, "Colour Space     : %s\n", colorspace_string(out_color_space));
      fprintf(stderr, "Output Width     : %d\n", output_width);
      fprintf(stderr, "Output Height    : %d\n", output_height);
      fprintf(stderr, "Output Colour Components: %d Number of components in colour Space)\n", out_color_components);
      fprintf(stderr, "Output Components       : %d (actual number per Pixel [1 iff indexed])\n", output_components);

      fprintf(stderr, "Vertical Frames  : %d\n", vframes);
      fprintf(stderr, "Horizontal Frames: %d\n", hframes);

      fprintf(stderr, "Frame Height     : %d\n", frame_height);
      fprintf(stderr, "Frame Width      : %d\n", frame_width);
      fprintf(stderr, "Box Height       : %d\n", box_height);
      fprintf(stderr, "Box Width        : %d\n", box_width);
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

  if (debug>2)
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

      box_contrast(row, output_width, output_components, focus_channel, *this_row, *prev_row, frame_height);

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
  fprintf(stderr, "Usage: %s [-v|--verbose][-d|--debug][-r|--red|-b|--blue|-g|--green][-f <filelistname>|--file <filelistname>] <list of filenames>\n"
                  "-v or --verbose produce more verbose output, can be repeated for more verbosity\n"
                  "-d or --debug produce debug output on stderr, can be repeated for more verbosity\n"
	          "[-r|--red|-b|--blue|-g|--green] base the calculations on reg/green or blue channels, default is green, ignored for greyscale\n"
		  "[-f <filelistname>|--file <filelistname>] filelistname is a file which contains a list of files, one per line. These are processed before <list of filenames>\n"
		  "[-b <cols>:<rows>|--box <cols>:<rows>] [-H <hframes> | -hframes <hframes>] [-V <vframes> | -vframes <vframes>]\n"
		  "\n"
		  "The Image is divided into <hframe> Horizontal frames by <vframe> Vertical frames\n"
		  "A 2nd set of alternate frames is defined midways (horizontally & vertically) between frames\n"
		  "In each frame a focus box is defined <cols>:<rows>\n"
		  "The focus box with the highest contrast store is output\n"
		  "\n"
		  "<hframes> defaults to: HFRAMES <vframes> defaults to: VFRAMES \n"
		  "The box size defaults to the entire frame\n"
		  "\n"
	  
	  , prog);
}


static int process_1file(char * input_filename)
{
  FILE           *input_file;

  time_t start = time(NULL);
  time_t end;
  
  if (verbose)
    fprintf(stderr, "Processing file: %s\n", input_filename);

  if ((input_file = fopen(input_filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", input_filename );
    return -1;
  }

  read_jpeg_file(input_file);

  if (verbose)
    {
      fprintf(stderr, "Final BESTBOX: ");
      dump_nbox(&best_box);
      fprintf(stderr, "\n");
    }
  
  printf("%s %llu (%d:%d-%d:%d)\n",
	 input_filename,	 
	 best_box.stat,
	 best_box.box.first_column,  best_box.box.first_row,
	 best_box.box.last_column,   best_box.box.last_row);


  end = time(NULL);

  if (verbose)
    fprintf(stderr, "Processing took %0.0f seconds\n", difftime(end, start));

  
  return 0;
}


int main(int argc, char **argv)
{
  char           *input_filename;
  int		  c;
  char           *boxstring = NULL;

  
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
	 {"hframes", required_argument, 0,    'H' },   /* -H number of Horizontal frames */
	 {"vframes", required_argument, 0,    'V' },   /* -V number of Vertical frames   */
	 {"box",     required_argument, 0,    'B' },   /* -B cols:rows */
	 {0,         0,                 0,    0 }
	};

      c = getopt_long(argc,
		      argv,
		      "dvrbgf:H:V:B:",
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

	case 'H':
	  hframes = atoi(optarg);

	  if (hframes > MAX_HFRAMES)
	    {
	      fprintf(stderr, "-hframes setting of %d exceeds the maximum of %d, reset to maximum\n", hframes, MAX_HFRAMES);
	      hframes = MAX_HFRAMES;
	    }
	  
	  break;

	case 'V':
	  vframes = atoi(optarg);
	  break;

	case 'B':
	  boxstring=optarg;
	  break;

	default:
	  usage(argv[0]);
	  exit(1);
	}
    }
      
  if (verbose)
    fprintf(stderr, "Package %s[%s] %s - find best area of focus\n", PACKAGE_NAME, PACKAGE_VERSION, argv[0]);
  
  cursors = hframes*2;

  if (debug)
    fprintf(stderr, "Number of cursors: %d\n", cursors);

  if (boxstring)
    {
      if (sscanf(boxstring, "%d:%d", &opt_box_width, &opt_box_height ) !=2)
	{
	  fprintf(stderr, "Syntax error in --box, should be cols:rows value is [%s]\n", boxstring);
	  exit(1);
	}
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
	  indirect_filename[strcspn(indirect_filename, "\n")] = 0;  /* splat out the /n, otherwise bad filename */
	  process_1file(indirect_filename);
	}
    }

  
  /* Trailing args are filename */

  if (debug)
    fprintf(stderr, "Following getopt %d arguments remain\n", argc-optind);

  for (;optind < argc; ++optind)
    {
      input_filename = argv[optind];

      process_1file(input_filename);
    }
  exit(0);
}
