/* terminal control module for terminals described by TERMCAP */

/*		Copyright (c) 1981,1980 James Gosling		*/

/*	Modified 1-Dec-80 by Dan Hoey (DJH) to understand C100 underlines */
/*	Modified 2-Dec-80 (DJH) to turn off highlighting on insertline */
/*	Modified 4 Aug 81 by JQ Johnson:  use "dm","ei","pc","mi" */
/*	Modified 24-Aug-81 by Jeff Mogul (JCM) at Stanford
 *		- uses "nl" instead of \n in case \n is destructive
 *	Modified 8-Sept-81 by JCM @ Stanford
 *		- re-integrated changes from Gosling since July '81
 *	Modified 22 March 84 by mkc at paisley to get arrow keys from termcap
 */
/*	Addabted 31-May-84 to window manager by Tatsuya Hagino
 *		 at Edinburgh University
*/
/* #define DEBUG */

#include <stdio.h>
#include "term.h"

static
int	curX, curY;

static int full_width;

char *tgetstr ();
char *UP;
char *BC;
char PC;
short ospeed;

static char *ILstr, *DLstr, *ICstr, *DCstr, *ELstr, *ESstr,
	*ICPstr, *ICPDstr, *BEGINstr, *ENDstr,
	*TIstr, *TEstr,*CDstr,
	*ICEstr, *NDstr, *VBstr, *EDstr, *DMstr, *NLstr;
/* extern char *KUstr, *KDstr, *KRstr, *KLstr;*/
	/* MKC - see arrows.c */
static int ULflag;	/* DJH -- 1 if terminal has underline */
static int MIflag;	/* JQJ -- 1 if safe to move while in insert mode */

static
dumpchar (c) {
    putchar (c);
}

static
enum IDmode { m_insert = 1, m_overwrite = 0 }
	CurMode, DesMode;

static
INSmode (new)
enum IDmode new; {
	DesMode = new;
	if(DesMode==m_insert && ICstr==0) abort();
};

static curHL, desHL;
static
HLmode (on) {
    desHL = on;
}

static
setHL () {
    register char *com;
    if (curHL == desHL)
	return;
    if(com = desHL ? HLBstr : HLEstr)
	tputs (com, 0, dumpchar);
    curHL = desHL;
}

static
clearHL () {
    if (curHL) {
	register oldes = desHL;
	desHL = 0;
	setHL ();
	desHL = oldes;
    }
}

static
setmode () {
    if (DesMode == CurMode)
	return;
    tputs(DesMode==m_insert ? ICstr : ICEstr, 0, dumpchar);
    CurMode = DesMode;
};

#ifdef DEBUG
static int windowl;

static window (n) /* set the window */
register int n;
{
    windowl = n;
};
#endif

static
inslines (n) {
#ifdef DEBUG
    register int m,y;
#endif
    HLmode (0);		/* DJH -- Don't highlight the inserted line */
    setHL ();
#ifdef DEBUG
    if (tt.t_length > windowl) {
	m = n;
	y = curY;
	topos (windowl-m+1,1);
	while (--m >= 0)
	    tputs (DLstr, tt.t_length-curY, dumpchar);
	topos (y,1);
    }
#endif
    while (--n >= 0)
	tputs (ILstr, tt.t_length-curY, dumpchar);
};

static
dellines (n) {
#ifdef DEBUG
    register int m,y;
    m = n;
#endif
    while (--n >= 0)
	tputs(DLstr, tt.t_length-curY, dumpchar);
#ifdef DEBUG
    if (tt.t_length > windowl) {
	y = curY;
	topos(windowl-m+1,1);
	while (--m >=0)
	    tputs(ILstr, tt.t_length-curY, dumpchar);
	topos(y,1);
    }
#endif
};

static
writechars (start, end)
register char	*start,
		*end; {
    register int standout;
    setmode ();
    setHL();
    standout = 0;
    while (start <= end) {
	if(CurMode == m_insert && ICPstr) tputs(ICPstr, tt.t_width-curX, dumpchar);
	if(*start == '_' && CurMode != m_insert && ULflag != 0) {
		putchar (' ');
		putchar (*BC);
	}
	if ((*start & 0200) && !standout) {
		tputs(HLBstr,0,dumpchar);
		standout = 1;
	}
	else if (!(*start & 0200) && standout) {
		tputs(HLEstr,0,dumpchar);
		standout = 0;
	}
	putchar (*start++ & 0177);
	if(CurMode == m_insert && ICPDstr) tputs(ICPDstr, tt.t_width-curX, dumpchar);
	curX++;
    }
    if (standout) tputs(HLEstr,0,dumpchar);
    if (full_width && curX > tt.t_width) curX = tt.t_width;
};

static
blanks (n) {
    setmode ();
    setHL ();
    while (--n >= 0) {
	if (CurMode == m_insert && ICPstr)
	    tputs (ICPstr, tt.t_width - curX, dumpchar);
	putchar (' ');
	if (CurMode == m_insert && ICPDstr)
	    tputs (ICPDstr, 1 /* tt.t_width - curX */, dumpchar);
	curX++;
    }
    if (full_width && curX > tt.t_width) curX = tt.t_width;
};

static float BaudFactor;

static pad(n,f)
float   f; {
    register    k = n * f * BaudFactor;
    while (--k >= 0)
	putchar (PC);
};

static
topos (row, column) {
    clearHL ();			/* many terminals can't hack highlighting
				   around cursor positioning.  Silly twits! */
    if (CurMode==m_insert && ! MIflag) {
	tputs(ICEstr, 0, dumpchar);	/* some terminals can't move in */
	CurMode = m_overwrite;		/* insert mode -- JQJ */
    }
    if (curY == row) {
	if (curX == column)
	    return;
	if (curX == column + 1 && (CurMode != m_insert)) {
	    tputs (BC, 0, dumpchar);
	    goto done;
	}
	if (curX == column - 1 && NDstr && CurMode != m_insert){
	    tputs (NDstr, 0, dumpchar);
	    goto done;
	}
    }
    if (curY - 1 == row && curX == column && UP != 0
		&& CurMode != m_insert){
	tputs (UP, 0, dumpchar);
	goto done;
    }
    if ( (curY + 1 == row && (column == 1 || column==curX))
					&& (CurMode != m_insert) ){
	if(column!=curX) putchar (015);
	tputs (NLstr, 0, dumpchar);
/*	putchar (012);		JCM */
	goto done;
    }
    tputs(tgoto (CursStr, column-1, row-1), 0, dumpchar);
done:
    curX = column;
    curY = row;
};

static
flash () {			/* dump a visible bell */
    tputs (VBstr, 0, dumpchar);
}

static
init (BaudRate) {
    static char tbuf[1024];
    static char combuf[1024];
    extern  old;
    char   *fill = combuf;
    static  inited;
    if (!inited)
	if (tgetent (tbuf, getenv ("TERM")) <= 0) {
	    fprintf(stderr,"\rNo environment-specified terminal type.\r\n");
	    done();
	}
    inited = 1;
    ILstr = tgetstr ("al", &fill);
    DLstr = tgetstr ("dl", &fill);
    ICstr = tgetstr ("im", &fill);
    ICEstr = tgetstr ("ei", &fill);
    MIflag = tgetflag ("mi");	/* can move in insert mode */
    DCstr = tgetstr ("dc", &fill);
    ELstr = tgetstr ("ce", &fill);
    ESstr = tgetstr ("cl", &fill);
    CDstr = tgetstr ("cd", &fill);
    HLBstr = tgetstr ("so", &fill);
    HLEstr = tgetstr ("se", &fill);
    ICPstr = tgetstr ("ic", &fill);
    ICPDstr = tgetstr ("ip", &fill);
    CursStr = tgetstr ("cm", &fill);
    UP = tgetstr ("up", &fill);
    NDstr = tgetstr ("nd", &fill);
    VBstr = tgetstr ("vb", &fill);
    TIstr = tgetstr ("ti", &fill);
    TEstr = tgetstr ("te", &fill);
    DMstr = tgetstr ("dm", &fill);/* start delete mode */
    EDstr = tgetstr ("ed", &fill);/* end delete mode */
    BC = tgetstr ("bc", &fill);
    if (BC == 0)
	BC = "\b";
    ULflag = tgetflag ("ul");	/* DJH -- Find out about underline */
    NLstr = tgetstr ("nl", &fill);/* JCM -- find out about newline */
    if (NLstr == 0)
	NLstr = "\n";		/*   use default if none specified */
    BEGINstr = tgetstr ("ti", &fill);
    KUstr = tgetstr("ku", &fill);
    KDstr = tgetstr("kd", &fill);
    KRstr = tgetstr("kr", &fill);
    KLstr = tgetstr("kl", &fill);
    KHstr = tgetstr("kh", &fill);
    KSstr = tgetstr("ks", &fill);
    KEstr = tgetstr("ke", &fill);
    Kstr[0] = tgetstr("k0", &fill);
    Kstr[1] = tgetstr("k1", &fill);
    Kstr[2] = tgetstr("k2", &fill);
    Kstr[3] = tgetstr("k3", &fill);
    Kstr[4] = tgetstr("k4", &fill);
    Kstr[5] = tgetstr("k5", &fill);
    Kstr[6] = tgetstr("k6", &fill);
    Kstr[7] = tgetstr("k7", &fill);
    Kstr[8] = tgetstr("k8", &fill);
    Kstr[9] = tgetstr("k9", &fill);
/*    PC = (tgetstr("pc", &fill)!=0) ? *tgetstr("pc", &fill) : 0; JQJ */
    PC = 0;
    BaudFactor = 1 / (1 - (.45 +.3 * BaudRate / 9600.))
	* (BaudRate / 10000.);
    if (CursStr == 0 || UP == 0 || ELstr == 0 || ESstr == 0) {
	stty (1, &old);
	fprintf (stderr, "\rSorry, this terminal isn't powerful enough.\r\n");
	done();
    }
    tt.t_ILmf = BaudFactor * 0.75;
    tt.t_ILov = ILstr ? 2 : MissingFeature;
    if (!ILstr)
	tt.t_inslines = tt.t_dellines = (int (*) ()) - 1;
    if (VBstr)
	tt.t_flash = flash;
    if (ICstr && DCstr) {
	tt.t_ICov = 1;
	tt.t_ICov = 4;
	tt.t_DCmf = 2;
	tt.t_DCov = 0;
    }
    else {
	tt.t_ICov = MissingFeature;
	tt.t_ICov = MissingFeature;
	tt.t_DCmf = MissingFeature;
	tt.t_DCov = MissingFeature;
    }
    tt.t_length = tgetnum ("li");
    full_width = !(tgetflag ("in")||tgetflag("am"));
    tt.t_width = tgetnum ("co") - (full_width ? 0 : 1);
/*  tt.t_width = tgetnum ("co") - 1; */
};


static
reset () {
    curX = -1;
    curY = -1;
    if (BEGINstr) tputs(BEGINstr, 0, dumpchar);
    if (TIstr) tputs(TIstr, 0, dumpchar);
    topos(1,1);
    tputs(ESstr, 0, dumpchar);
    CurMode = m_insert;
    DesMode = m_overwrite;
#ifdef DEBUG
    windowl = tt.t_length;
#endif
};

static
cleanup () {
    HLmode (0);
    DesMode = m_overwrite;
    setmode();
    if (TEstr) tputs(TEstr, 0, dumpchar);
};

static
wipeline () {
    setHL ();
    tputs (ELstr, tt.t_width-curX, dumpchar);
};

static
wipescreen () {
    tputs(ESstr, 0, dumpchar);
    curX = curY = -1;
};

static
wipedisplay () {
    setHL ();
    tputs(CDstr, tt.t_length-curY, dumpchar);
};

static
delchars (n) {
    if (DMstr) {	/* we may have delete mode, or delete == insert */
	if (strcmp(DMstr,ICstr)) {
	    if (CurMode  == m_overwrite) {
	        tputs(ICstr,0,dumpchar);
		CurMode = m_insert;	/* we're now in both */
	    }
	}
	else {
	    if (CurMode == m_insert) {
		tputs(ICEstr, 0, dumpchar);
		CurMode = m_overwrite;
	    }
	    tputs(DMstr,0,dumpchar);
        }
    }
    while (--n >= 0) {
	tputs(DCstr, tt.t_width-curX, dumpchar);
    }
    if (EDstr) {		/* for some, insert mode == delete mode */
		/* bug!  /etc/termcap pads ICEstr but not EDstr */
        if (strcmp(DMstr,ICstr))
	    CurMode = m_insert;
	else
	    tputs(EDstr,0,dumpchar);
    }
};

TrmTERM () {
	tt.t_INSmode = INSmode;
	tt.t_HLmode = HLmode;
	tt.t_inslines = inslines;
	tt.t_dellines = dellines;
	tt.t_blanks = blanks;
	tt.t_init = init;
	tt.t_cleanup = cleanup;
	tt.t_wipeline = wipeline;
	tt.t_wipescreen = wipescreen;
	tt.t_wipedisplay = wipedisplay;
	tt.t_topos = topos;
	tt.t_reset = reset;
	tt.t_delchars = delchars;
	tt.t_writechars = writechars;
#ifdef DEBUG
	tt.t_window = window;
#else
	tt.t_window = 0;
#endif
	tt.t_ILmf = 0;
	tt.t_ILov = 0;
	tt.t_ICmf = 0;
	tt.t_ICov = 0;
	tt.t_length = 24;
	tt.t_width = 80;
};
