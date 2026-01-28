/* terminal control module header file */

/*		Copyright (c) 1981,1980 James Gosling		*/

struct TrmControl {
    int     (*t_topos) (int, int);	/* move the cursor to the indicated
				   (row,column); (1,1) is the upper left */
    int     (*t_reset) (void);	/* reset terminal (screen is in unkown state,
				   convert it to a known one) */
    int     (*t_INSmode) (int);	/* set or reset character insert mode */
    int     (*t_HLmode) (int);	/* set or reset highlighting */
    int     (*t_inslines) (int);	/* insert n lines */
    int     (*t_dellines) (int);	/* delete n lines */
    int     (*t_blanks) (int);	/* print n blanks */
    int     (*t_init) (int);	/* initialize terminal settings */
    int     (*t_cleanup) (void);	/* clean up terminal settings */
    int     (*t_wipeline) (void);	/* erase to the end of the line */
    int     (*t_wipescreen) (void);	/* erase the entire screen */
    int     (*t_wipedisplay) (void);/* erase to the end of the display */
    int     (*t_delchars) (int);	/* delete n characters */
    int     (*t_writechars) (char *, char *);	/* write characters; either inserting or
				   overwriting according to the current
				   character insert mode. */
    int     (*t_window) (int);	/* set the screen window so that IDline
				   operations only affect the first n
				   lines of the screen */
    int     (*t_flash) (void);	/* Flash the screen -- not set if this
				   terminal type won't support it. */
/* costs are expressed as number_affected*mf + ov
	cost to insert/delete 1 line: (number of lines left)*ILmf+ILov
	cost to insert one character: (number of chars left on line)*ICmf+ICov
	cost to delete n characters:  n*DCmf+DCov */
    float   t_ILmf;		/* insert lines multiply factor */
    int     t_ILov;		/* insert lines overhead */
    float   t_ICmf;		/* insert character multiply factor */
    int     t_ICov;		/* insert character overhead */
    float   t_DCmf;		/* delete character multiply factor */
    int     t_DCov;		/* delete character overhead */
    int     t_length;		/* screen length */
    int     t_width;		/* screen width */
    int     t_needspaces;	/* set true iff the terminal needs to have
				   real spaces in the middle of lines in
				   order to have character insertion work --
				   this only matters on terminals that
				   distinguish between real and imaginary
				   blanks. */
};

#define MissingFeature 99999	/* IC and IL overheads should be set to this
				   value if the corresponding feature is
				   missing */
struct TrmControl tt;		/* terminal specific information for the
				   current display */

char  *HLBstr,*HLEstr,*KUstr,*KDstr,*KRstr,*KLstr,*KHstr,*KSstr,*KEstr,
      *CursStr,*Kstr[10];			/* function keys */

