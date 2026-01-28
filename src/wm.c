/*
 * super (?) window terminal driver
 *
 *	By Tatsuya Hagino   June 1984 (revised October 1984)
 *	Modernized to ANSI C (C11) 2026
 */

/* include config first */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* define */
#define NOFLOWCTL
#ifndef HELPFILE
#define HELPFILE "wm.hlp"
#endif
/* #define WELCOME "/mnt/th/th/bin/swm.msg" */
#define COPY_FILE "hardcopy"
#define MAX_WINDOWS 256
#define MAX_PROCESSES 20
#define MAX_STR 256
#define STK_LEN 10
#define MOUSE_MODE "\033\033"
#define INPUT_FREQ 10

/* stack info */
#define STK_SET 1
#define STK_PLUS 2
#define STK_MINUS 4
#define STK_ON 8
#define STK_OFF 16

/* other define */
#define ESC '\033'
#define FF 12
#define NL 10
#define BELL 7
#define END 1
#define CONT 0
#define INF 32767

/* public include files */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <limits.h>
#include <errno.h>

/* Termcap function declarations */
extern int tputs(const char *str, int affcnt, int (*putc)(int));
extern char *tgetstr(const char *id, char **area);
extern int tgetent(char *bp, const char *name);
extern int tgetnum(const char *id);
extern int tgetflag(const char *id);
extern char *tgoto(const char *cap, int col, int row);

/* PTY handling - try various headers */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <util.h>
#elif defined(__linux__)
#include <pty.h>
#else
/* Fall back to BSD-style PTY on other systems */
#define USE_BSD_PTY
#endif

/* Compatibility for old terminal interface */
#ifndef HAVE_TERMIOS
#include <sgtty.h>
#else
/* Define sgttyb structure in terms of termios for compatibility */
struct sgttyb {
    char sg_ispeed;
    char sg_ospeed;
    char sg_erase;
    char sg_kill;
    int sg_flags;
};
struct tchars {
    char t_intrc;
    char t_quitc;
    char t_startc;
    char t_stopc;
    char t_eofc;
    char t_brkc;
};
struct ltchars {
    char t_suspc;
    char t_dsuspc;
    char t_rprntc;
    char t_flushc;
    char t_werasc;
    char t_lnextc;
};
/* Emulate old flags */
#define RAW 0040
#define ECHO 010
#define TANDEM 0
#endif

/* private include files */
#include "display.h"
#include "winlib.h"

/* Forward declarations */
struct pro_str;

/* types */
typedef int (*funcptr)(char, int, struct pro_str *);
typedef int (*sfuncptr)(int, struct pro_str *);  /* string processing function */

/* structures */
struct win_str *winlist[MAX_WINDOWS];	/* window list */

struct pro_str {
    int pro_sts;	/* process status, -1:dead, 0:alive, 1:sleep */
    int pro_id;		/* process id */
    int pro_master;	/* master half of pty */
    int pro_slave;	/* slave half of pty */
    struct win_str *pro_cur_win;
			/* current window */
    int pro_cur_win_num;/* current window number */
    int pro_base_win_num;
			/* base window number */
    short change_sg_flags;
			/* remember that just chaged to cooked mode */
    short change_to_cooked;
    int sg_flags;
    char pro_ibuf[BUFSIZ];
			/* input buffer */
    char *pro_ibuf_pointer;
			/* input buffer pointer */
    int pro_ibuf_counter;
			/* input buffer counter */
    funcptr pro_do;	/* sequence processor */
    int pro_stk[STK_LEN];
    short pro_stk_flg[STK_LEN];
    int pro_stk_top;
    char pro_sbuf[MAX_STR];
    char *pro_sbuf_pointer;
    char pro_mch;
    sfuncptr pro_sdo;
    } *prolist[MAX_PROCESSES];		/* process list */

struct {
    struct sgttyb tty_b;
    struct tchars tty_tc;
    struct ltchars tty_lc;
    int tty_lb;
    int tty_l;
    } ttychar;				/* terminal characteristic */

struct {
    int fn_mode;			/* mode -1:dead,0:transmit,1:local */
    char *fn_def;			/* definition */
    char *fn_tra;			/* translation */
    } fn_table[10+4+1];			/* function key table */

struct {
    funcptr do_mouse;			/* mouse driver */
    const char *mouse_msg;		/* message */
    } mouse_table[10+4+1];		/* mouse table */

/* function prototypes */
static void writehelp(void);
static void initialize(void);
static void gettty(void);
static void fixtty(void);
static int dumpchar(int c);
static void initwin(void);
static void initfnkey(void);
static void startprocess(int p, int w);
static void finish(int sig);
static void killedprocess(int p);
void done(void);  /* non-static, called from term.c */
static void stopprocess(int p);
static void process(void);
static void processoutput(int p);
static void processinput(void);
static int funkeycheck(const char *fk);
static void sendtoprocess(struct pro_str *proc, char ch);
static void sendstringtoprocess(struct pro_str *proc, const char *str);
static int doaddch(char ch, int p, struct pro_str *proc);
static void nowindow(int p);
static void dorefresh(void);
static int doesc(char ch, int p, struct pro_str *proc);
static int dopushzero(char ch, int p, struct pro_str *proc);
static int dodigit(char ch, int p, struct pro_str *proc);
static int dosetplus(char ch, int p, struct pro_str *proc);
static int dosetminus(char ch, int p, struct pro_str *proc);
static int doseton(char ch, int p, struct pro_str *proc);
static int dosetoff(char ch, int p, struct pro_str *proc);
static int fin(char ch, int p, struct pro_str *proc);
static int doansiesc(char ch, int p, struct pro_str *proc);
static int doansi(char ch, int p, struct pro_str *proc);
static int docursor(char ch, int p, struct pro_str *proc);
static int evalarg(struct pro_str *proc, int n, int d, int p, int min, int max);
static int docursorv2(char ch, int p, struct pro_str *proc);
static int docursorv1(char ch, int p, struct pro_str *proc);
static int docursorv(char ch, int p, struct pro_str *proc);
static int doeeol(char ch, int p, struct pro_str *proc);
static int doeeos(char ch, int p, struct pro_str *proc);
static int doeeosv(char ch, int p, struct pro_str *proc);
static int doup(char ch, int p, struct pro_str *proc);
static int dodown(char ch, int p, struct pro_str *proc);
static int doright(char ch, int p, struct pro_str *proc);
static int doleft(char ch, int p, struct pro_str *proc);
static int dohome(char ch, int p, struct pro_str *proc);
static int docld(char ch, int p, struct pro_str *proc);
static int dolinsert(char ch, int p, struct pro_str *proc);
static int doldelete(char ch, int p, struct pro_str *proc);
static int dosetins(char ch, int p, struct pro_str *proc);
static int doresetins(char ch, int p, struct pro_str *proc);
static int dodelchar(char ch, int p, struct pro_str *proc);
static int dodelchars(char ch, int p, struct pro_str *proc);
static int doredraw(char ch, int p, struct pro_str *proc);
static int doreverselinefeed(char ch, int p, struct pro_str *proc);
static int dosetscroll(char ch, int p, struct pro_str *proc);
static int dostandout(char ch, int p, struct pro_str *proc);
static int dospecialesc(char ch, int p, struct pro_str *proc);
static int dospecial(char ch, int p, struct pro_str *proc);
static int dosetmode(char ch, int p, struct pro_str *proc);
static int dosetecho(char ch, int p, struct pro_str *proc);
static int dosetraw(char ch, int p, struct pro_str *proc);
static int dosetdump(char ch, int p, struct pro_str *proc);
static int dosetkill(char ch, int p, struct pro_str *proc);
static int dosetstop(char ch, int p, struct pro_str *proc);
static int dosetmouse(char ch, int p, struct pro_str *proc);
static int dostring(char ch, int p, struct pro_str *proc);
static int docopys(int p, struct pro_str *proc);
static int docopy(char ch, int p, struct pro_str *proc);
static int dosetfnkeys(int p, struct pro_str *proc);
static int dosetfnkey(char ch, int p, struct pro_str *proc);
static int dopsdins(int p, struct pro_str *proc);
static int dopsdin(char ch, int p, struct pro_str *proc);
static int dosetbase(char ch, int p, struct pro_str *proc);
static int dogivekeyboard(char ch, int p, struct pro_str *proc);
static int dokillprocess(char ch, int p, struct pro_str *proc);
static int dostartprocess(char ch, int p, struct pro_str *proc);
static int doprlesc(char ch, int p, struct pro_str *proc);
static int doprl(char ch, int p, struct pro_str *proc);
static int docreate(char ch, int p, struct pro_str *proc);
static void dowinfo(int w);
static int dodestroy(char ch, int p, struct pro_str *proc);
static int doenquire(char ch, int p, struct pro_str *proc);
static int doselect(char ch, int p, struct pro_str *proc);
static int dopopwindow(char ch, int p, struct pro_str *proc);
static int doreposwindow(char ch, int p, struct pro_str *proc);
static int dopageselect(char ch, int p, struct pro_str *proc);
static int dopageforward(char ch, int p, struct pro_str *proc);
static int dopagebackward(char ch, int p, struct pro_str *proc);
static int doscroll(char ch, int p, struct pro_str *proc);
static int doheadings(int p, struct pro_str *proc);
static int doheading(char ch, int p, struct pro_str *proc);
static void domouseend(void);
static void domouse(void);
static void createmouse(void);
static void wmsetwnum(void);
static void domousehome(void);
static void domouseup(void);
static void domousedown(void);
static void domouseright(void);
static void domouseleft(void);
static void dosetmark(void);
static void doresetinput(void);
static void domousecreate(void);
static int dosearchwin(struct win_str *win);
static void domousedelete(void);
static void domousemove(void);
static void domousecopy(void);
static void domousenext(void);
static void domouseback(void);
static void domousepush(void);
static void domouseprocessselect(void);
static void domouseprocessnext(void);
static void initmouse(void);
static void initeachtbl(funcptr *tbl);
static void inittbl(void);

/* data */
static char *shell;			/* default shell name */
static int is_shell;			/* is it really a shell */
static int is_cshell;			/* is it a c shell */

extern struct win_str *top_win;		/* top window */

static char i_buf[BUFSIZ];		/* input buffer */
static char o_buf[BUFSIZ];		/* output buffer */

static struct pro_str *current_process;	/* current input process */
static int current_process_number;	/* current input process number */

static char fn_key_buf[10];		/* function key buffer */
static char *fn_key_buf_pointer;	/* function key buffer pointer */

static int kill_character = 28;		/* kill character (default ^\) */

static funcptr ansitable[0200];		/* ansi escape sequence */
static funcptr vi200table[0200];	/* vi200 escape sequnce */
static funcptr prltable[0200];		/* window escape sequence */
static funcptr specialtable[0200];	/* special escape sequence */

static struct win_str *mousewin;	/* mouse window */
static struct win_str *mousemark;	/* mouse mark */
static int mouse_win_number;		/* mouse window number */
static int mouse_page_number;		/* mouse page number */
static int mouse_process_number;
static int mouse_increment;		/* cursor increment */
static int mouse_input_window;
static int mouse_input_page;
static int mouseY, mouseX;		/* mouse position */
static char *mouse_copy_file;		/* hardcopy file name */

static struct win_str *statuswin;	/* status window */

static int input_freq;			/* input service frequency */

static char fn_key_char;		/* function key common starter */

/* flags */
static int statusmode;			/* status mode */

static int dumpmode;			/* dump mode */

static int mousemode;			/* mouse mode */
static int mousemark_set;		/* mouse mark set/reset */
static int mouse_input_number;		/* mouse input mode */
static int mouse_flag;			/* mouse capability */

static int fn_key_partial_match;

static int new_mask;			/* input mask recalculation */

static char **nenvp;			/* new environment */
static int new_envp;

static int stoppable;			/* stop */
static char stopch;

/* program */

static char **sub_argv;
static char *sub_init_str;
static int sub_init_lf;

int
main(int argc, char **argv, char **envp)
{
	char *cp;
	int help, l, sub_argc;

	mouse_copy_file = NULL;
	statusmode = 0;
	help = 0;
	nenvp = envp;

	if (!(shell = getenv("WSHELL")))
	    if (!(shell = getenv("SHELL")))
		shell = "/bin/sh";
	is_cshell = ((l = strlen(shell)) >= 3 && strcmp(shell+l-3,"csh") == 0);
	is_shell = ((l = strlen(shell)) >= 2 && strcmp(shell+l-2,"sh") == 0);
	stoppable = !is_shell;

	sub_argc = 0;
	sub_argv = argv;
	sub_init_str = 0;
	sub_init_lf = 0;
	while (argc > 1) {
	    if (*argv[1] == '-') {
		cp = argv[1];
		while (*++cp) {
		    switch(*cp) {
		    case 'a': sub_argv = &(argv[1]);
			      sub_argc = 1;
			      argc = 0;
			      break;
		    case 's': statusmode = 1;
			      break;
		    case 'h': help = 1;
			      break;
		    case 'q': stoppable = 1;
			      break;
		    case 'Q': stoppable = 0;
			      break;
		    case 'I': sub_init_lf = 1;
		    case 'i': if (argc > 2) {
		    		sub_init_str = argv[2];
				argc--;
				argv++;
			      }
			      break;
		    default : fprintf(stderr,"illegal flag '-%c'\n",*cp);
			      break;
		    }
		}
	    }
	    else if (!mouse_copy_file) mouse_copy_file = argv[1];
	    else if (*(argv[1]))
		fprintf(stderr,"illegal argument %s\n",argv[1]);
	    argv++;
	    argc--;
	}
	if (!mouse_copy_file) mouse_copy_file = COPY_FILE;

	if (sub_argc == 0) {
	   sub_argv[1] = 0;
	}

	if (help) {
	    writehelp();
	    exit(0);
	}
	initialize();
	process();
	exit(1);
}

static void
writehelp(void)
{
	FILE *help;
	int c;

	help = fopen(HELPFILE,"r");
	if (!help) {
	    printf("Sorry!! No help!!\n");
	    return;
	}
	while ((c = fgetc(help)) > 0) fputc(c,stdout);
	fclose(help);
}

static void
initialize(void)
{
#ifdef WELCOM
	FILE *f;
#endif
	int p, ch, l;
	char *s, *term, *termcap, **envp, **envq;

	if (is_shell) {
#ifdef WELCOM
	    f = fopen(WELCOME,"r");
	    if (f) {
		while ((ch = fgetc(f)) >= 0) fputc(ch,stdout);
		fclose(f);
	    }
	    else
#endif
		printf("Welcome to Window Manager (version 5)\n");
	}
	else {
	    printf("Running %s under Window Manager (version 5)\n",shell);
	}

	/* setup the new environment */
	l = 0;
	envp = nenvp;
	while (*envp++) l++;
	if ((term = getenv("WTERM"))) {
	    l++;
	    s = (char *) calloc(strlen(term)+6,sizeof(char));
	    strcpy(s,"TERM=");
	    strcat(s,term);
	    term = s;
	}
	if ((termcap = getenv("WTERMCAP"))) {
	    l++;
	    s = (char *) calloc(strlen(termcap)+9,sizeof(char));
	    strcpy(s,"TERMCAP=");
	    strcat(s,termcap);
	    termcap = s;
	}
	if (term || termcap) {
	    envp = nenvp;
	    nenvp = envq = (char **) calloc(l+1,sizeof(char *));
	    if (term) *envq++ = term;
	    if (termcap) *envq++ = termcap;
	    while (*envp) {
		if ((term && (strncmp(*envp,"TERM=",5) == 0)) ||
		    (termcap && (strncmp(*envp,"TERMCAP=",9) == 0)))
			envp++;
		else *envq++ = *envp++;
	    }
	    *envq = (char *)0;
	    new_envp = 1;
	}
	else new_envp = 0;

	gettty();
	fixtty();

	initwin();
	inittbl();
	initmouse();
	initfnkey();

	for (p = 0; p < MAX_PROCESSES; p++) prolist[p] = NULL;
	startprocess(0,0);
	current_process_number = 0;
	current_process = prolist[0];

	/* Use sigaction instead of signal */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = finish;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);
}

static void
gettty(void)
{
#ifndef HAVE_TERMIOS
	ioctl(0, TIOCGETP, (char *)&(ttychar.tty_b));
	ioctl(0, TIOCGETC, (char *)&(ttychar.tty_tc));
	ioctl(0, TIOCGETD, (char *)&(ttychar.tty_l));
	ioctl(0, TIOCGLTC, (char *)&(ttychar.tty_lc));
	ioctl(0, TIOCLGET, (char *)&(ttychar.tty_lb));
	stopch = ttychar.tty_lc.t_suspc;
#else
	/* Use termios for modern systems */
	struct termios tio;
	tcgetattr(0, &tio);
	/* Convert termios to sgttyb for compatibility */
	ttychar.tty_b.sg_ispeed = cfgetispeed(&tio);
	ttychar.tty_b.sg_ospeed = cfgetospeed(&tio);
	ttychar.tty_b.sg_erase = tio.c_cc[VERASE];
	ttychar.tty_b.sg_kill = tio.c_cc[VKILL];
	ttychar.tty_b.sg_flags = 0;
	if (tio.c_lflag & ICANON) ttychar.tty_b.sg_flags &= ~RAW;
	else ttychar.tty_b.sg_flags |= RAW;
	if (tio.c_lflag & ECHO) ttychar.tty_b.sg_flags |= ECHO;

	ttychar.tty_tc.t_intrc = tio.c_cc[VINTR];
	ttychar.tty_tc.t_quitc = tio.c_cc[VQUIT];
	ttychar.tty_tc.t_startc = tio.c_cc[VSTART];
	ttychar.tty_tc.t_stopc = tio.c_cc[VSTOP];
	ttychar.tty_tc.t_eofc = tio.c_cc[VEOF];

	ttychar.tty_lc.t_suspc = tio.c_cc[VSUSP];
	stopch = ttychar.tty_lc.t_suspc;
#endif
}

static void
fixtty(void)
{
#ifndef HAVE_TERMIOS
	struct sgttyb sbuf;
#ifdef NOFLOWCTL
	struct tchars tbuf;

	tbuf = ttychar.tty_tc;
	tbuf.t_startc = -1;
	tbuf.t_stopc = -1;
	ioctl(0, TIOCSETC, &tbuf);
#endif

	sbuf = ttychar.tty_b;
	sbuf.sg_flags |= RAW;
	sbuf.sg_flags &= ~ECHO;
#ifdef NOFLOWCTL
	sbuf.sg_flags &= ~TANDEM;
#endif
	ioctl(0, TIOCSETP, (char *)&sbuf);
#else
	/* Use termios for modern systems */
	struct termios tio;
	tcgetattr(0, &tio);

	/* Set raw mode */
	tio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG | IEXTEN);
	tio.c_iflag &= ~(ICRNL | INLCR | IGNCR | IXON | IXOFF);
	tio.c_oflag &= ~OPOST;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &tio);
#endif
}

static int
dumpchar(int c)
{
	putchar(c);
	return 0;
}

static void
initwin(void)
{
	int i;

	wminit();	/* initialize the window package */
	if (KSstr) {
		tputs(KSstr, 0, dumpchar); /* initialize function key */
		fflush(stdout);
	}
	if (HLEstr) {
		tputs(HLEstr, 0, dumpchar);
		fflush(stdout);
	}
	mouseX = ScreenWidth / 2;
	mouseY = ScreenLength / 2;

	for (i = 1; i<MAX_WINDOWS; i++) winlist[i] = NULL;
	winlist[0] = wmmake(ScreenLength,ScreenWidth,0,0,0,0);
	if (statusmode) {
	    statuswin = wmmake(ScreenLength/4,
				ScreenWidth/2,
				1,
				ScreenWidth-ScreenWidth/2-1,
				3,
				0);
	    wmheadstr(statuswin,"WM Status Window",0);
	    statuswin->flags |= WM_SCROLL;
	}
	else statuswin = NULL;
	dumpmode = 0;
}

static void
initfnkey(void)
{
	int f, flg;
	char *p, ch;

	fn_key_buf_pointer = fn_key_buf;

	for (f = 0; f < 10; f++) {
		fn_table[f].fn_mode = 0;
		fn_table[f].fn_def = Kstr[f];
		p = (char *) calloc(3, sizeof(char));
		fn_table[f].fn_tra = p;
		*p++ = ESC;
		*p++ = '0' + f;
		*p = 0;
	}
	for(ch = 'A'; f < 10+4+1; f++,ch++) {
		fn_table[f].fn_mode = 0;
		p = (char *) calloc(3, sizeof(char));
		fn_table[f].fn_tra = p;
		*p++ = ESC;
		*p++ = ch;
		*p = 0;
	}
	fn_table[10+0].fn_def = KUstr;
	fn_table[10+1].fn_def = KDstr;
	fn_table[10+2].fn_def = KRstr;
	fn_table[10+3].fn_def = KLstr;
	fn_table[10+4].fn_def = KHstr;
	fn_table[10+4].fn_tra[1] = 'H';

	flg = 0;
	for (f = 0; f < 10+4+1; f++) {
	    if ((ch = *fn_table[f].fn_def)) {
		if (!flg) { fn_key_char = ch; flg = 1; }
		else if (ch != fn_key_char) fn_key_char = 0;
	    }
	}
	if (!flg) fn_key_char = 0;
}

static void
startprocess(int p, int w)
{
	struct pro_str *proc;
	int master, slave, child;
#ifdef USE_BSD_PTY
	char line[PATH_MAX];
	char c;
	int i;
	struct stat stb;
#else
	char slave_name[PATH_MAX];
#endif
#ifndef HAVE_TERMIOS
	struct tchars tbuf;
#else
	struct termios tio;
#endif

	if (p < 0 || p >= MAX_PROCESSES || prolist[p]) return;
	if (w < 0 || w >= MAX_WINDOWS || winlist[w] == NULL) return;
	prolist[p] = proc =
		(struct pro_str *) calloc(1, sizeof(struct pro_str));

#ifdef USE_BSD_PTY
	/* BSD-style PTY allocation */
	strncpy(line,"/dev/ptyXX", sizeof(line)-1);
	line[sizeof(line)-1] = '\0';
	for(c = 'p'; c <= 's'; c++) {
	    line[strlen("/dev/pty")] = c;
	    line[strlen("/dev/ptyp")] = '0';
	    if (stat(line, &stb) < 0) break;
	    for (i = 15; i >= 0; i--) {
		line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
		master = open(line, O_RDWR);
		if (master >= 0) break;
	    }
	    if (master >= 0) break;
	}
	if (master < 0) {
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Out of pty's\r\n",0);
		dorefresh();
	    }
	    return;
	}
	proc->pro_master = master;
	line[strlen("/dev/")] = 't';
	proc->pro_slave = slave = open(line, O_RDWR);
	if (slave < 0) {
	    close(master);
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Can't open slave\r\n",0);
		dorefresh();
	    }
	    return;
	}
#else
	/* POSIX openpty() */
	if (openpty(&master, &slave, slave_name, NULL, NULL) < 0) {
	    perror("openpty");
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Out of pty's\r\n",0);
		dorefresh();
	    }
	    return;
	}
	proc->pro_master = master;
	proc->pro_slave = slave;
#endif

#ifndef HAVE_TERMIOS
	ioctl(slave, TIOCSETP, (char *)&(ttychar.tty_b));
	tbuf = ttychar.tty_tc;
#ifdef NOFLOWCTL
	tbuf.t_startc = -1;
	tbuf.t_stopc = -1;
#endif
	ioctl(slave, TIOCSETC, (char *)&tbuf);
	ioctl(slave, TIOCSLTC, (char *)&(ttychar.tty_lc));
	ioctl(slave, TIOCLSET, (char *)&(ttychar.tty_lb));
	ioctl(slave, TIOCSETD, (char *)&(ttychar.tty_l));
#else
	/* Setup termios for slave */
	tcgetattr(slave, &tio);
	tio.c_cc[VINTR] = ttychar.tty_tc.t_intrc;
	tio.c_cc[VQUIT] = ttychar.tty_tc.t_quitc;
	tio.c_cc[VSTART] = ttychar.tty_tc.t_startc;
	tio.c_cc[VSTOP] = ttychar.tty_tc.t_stopc;
	tio.c_cc[VEOF] = ttychar.tty_tc.t_eofc;
	tio.c_cc[VSUSP] = ttychar.tty_lc.t_suspc;
	tcsetattr(slave, TCSANOW, &tio);
#endif

	proc->pro_id = child = fork();
	if (child < 0) {
	    close(master);
	    close(slave);
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Can't get process\r\n",0);
		dorefresh();
	    }
	    return;
	}
	if (child == 0) {
	    int t;
	    t = open("/dev/tty",O_RDWR);
	    if (t >= 0) {
#ifndef HAVE_TERMIOS
		ioctl(t, TIOCNOTTY, (char *)0);
#else
		/* Modern way to detach from controlling terminal */
		setsid();
#endif
		close(t);
	    }
	    close(master);
	    dup2(slave,0);
	    dup2(slave,1);
	    dup2(slave,2);
	    close(slave);
	    for(t = 3; t < 20; t++) close(t); /* try to close all files */
	    sub_argv[0] = shell;
	    execve(shell,sub_argv,nenvp);
	    printf("Can't run %s\r\n",shell);
	    execle("/bin/sh","sh","-i",(char *)0,nenvp);
	    printf("Can't run sh either\r\n");
	    exit(1);
	}
	proc->pro_cur_win_num = w;
	proc->pro_base_win_num = w;
	proc->pro_cur_win = winlist[w];
	proc->pro_ibuf_pointer = proc->pro_ibuf;
	proc->pro_ibuf_counter = 0;
	proc->change_sg_flags = 0;
	proc->change_to_cooked = 0;
	proc->sg_flags = ttychar.tty_b.sg_flags;
	proc->pro_sts = 0;
	proc->pro_do = doaddch;
	if (sub_init_str) {
	    sendstringtoprocess(proc,sub_init_str);
	    if (sub_init_lf) sendtoprocess(proc,'\n');
	}
	else if (!new_envp && is_cshell)
	    sendstringtoprocess(proc,"setenv TERM $wterm\n");
	new_mask = 1;
	if (statusmode) {
	    wmaddstr(statuswin,"Start process ",0);
	    wmaddnum(statuswin,"%d",p,0);
	    wmaddstr(statuswin," in window ",0);
	    wmaddnum(statuswin,"%d\r\n",w,1);
	}
}

static void
finish(int sig)
{
	int status;
	int pid, p;

	/* Re-establish handler using sigaction */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);

	pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
	for (p = 0; p < MAX_PROCESSES; p++) {
	    if (prolist[p] && prolist[p]->pro_id == pid) break;
	}
	if (p >= MAX_PROCESSES) {
	    if (statusmode) {
		wmaddstr(statuswin,"Strange Signal Received\r\n",0);
		dorefresh();
	    }
	}
	else if (WIFEXITED(status)) killedprocess(p);
	else if (WIFSTOPPED(status)) {
		prolist[p]->pro_sts = 1;
	}

	/* Re-establish handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = finish;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);
}

static void
killedprocess(int p)
{
	int kp;
	struct pro_str *proc;

	if (p == 0) done();
	if (prolist[p] == NULL) return;
	proc = prolist[p];
	close(proc->pro_master);
	close(proc->pro_slave);
	free((char *)proc);
	prolist[p] = NULL;
	kp = p;
	if (current_process_number == p) {
	    while (--p >= 0) if (prolist[p]) break;
	    if (p < 0) done();
	    current_process_number = p;
	    current_process = prolist[p];
	}
	if (statusmode) {
	    wmaddstr(statuswin,"Process ",0);
	    wmaddnum(statuswin,"%d",kp,0);
	    wmaddstr(statuswin," killed\r\n",0);
	    dorefresh();
	}
	new_mask = 1;
}

void
done(void)
{
	int p;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGCHLD, &sa, NULL);

	for (p = 0; p < MAX_PROCESSES; p++)
	    if (prolist[p]) {
		kill(prolist[p]->pro_id,SIGKILL);
	    }

	if (KEstr) tputs(KEstr,0,dumpchar);
	wmend();
	fflush(stdout);
#ifndef HAVE_TERMIOS
#ifdef NOFLOWCTL
	ioctl(0, TIOCSETC, &(ttychar.tty_tc));
#endif
	ioctl(0, TIOCSETP, (char *)&(ttychar.tty_b));
#else
	/* Restore original terminal settings */
	struct termios tio;
	tcgetattr(0, &tio);
	tio.c_lflag |= ICANON | ECHO;
	tio.c_iflag |= ICRNL;
	tio.c_oflag |= OPOST;
	tcsetattr(0, TCSANOW, &tio);
#endif
	if (is_shell) printf("\nWindow Manager end\n");
	else printf("\n%s end\n",shell);

	sigaction(SIGCHLD, &sa, NULL);
	exit(0);
}

static void
stopprocess(int p)
{
	if (p < 0 || p >= MAX_PROCESSES || prolist[p] == NULL) return;
	if (prolist[p]->pro_sts == 0) kill(prolist[p]->pro_id,SIGSTOP);
}

static void
process(void)
{
	fd_set readfds_set, writefds_set, exceptfds_set;
	int nfound, maxfd;
	int p, bp[MAX_PROCESSES], master, stop;
	int pn[MAX_PROCESSES+1], q;
	struct pro_str *proc;
	struct timeval timeout;
#ifndef HAVE_TERMIOS
	struct sgttyb sb;
#else
	struct termios tio;
#endif

	input_freq = INPUT_FREQ;
	new_mask = 1;
	for(;;) {
	    FD_ZERO(&readfds_set);
	    FD_ZERO(&writefds_set);
	    FD_ZERO(&exceptfds_set);
	    maxfd = 0;
	    stop = 0;
	    if (new_mask) {
		q = 0;
		for (p = 0; p < MAX_PROCESSES; p++)
		    if (prolist[p]) pn[q++] = p;
		pn[q] = -1;
		new_mask = 0;
	    }
	    for (q = 0; pn[q] >= 0; q++) {
		p = pn[q];
		if ((proc = prolist[p]) == NULL) continue;
		master = proc->pro_master;
		if (master > maxfd) maxfd = master;
		FD_SET(master, &readfds_set);
		bp[p] = master;
		if (proc->pro_ibuf_counter != 0) FD_SET(master, &writefds_set);
		stop = stop || proc->change_sg_flags;
	    }
	    FD_SET(0, &readfds_set);
	    if (stop) {
		timeout.tv_sec = 0;
	        timeout.tv_usec = 1;
	    }
	    else {
		timeout.tv_sec = 10;
	        timeout.tv_usec = 0;
	    }
	    if (mousemode == 1) {
		wmsetwnum();
		wmrefresh0(mouseY,mouseX);
	    }
	    else if (!dumpmode) dorefresh();
	    nfound = select(maxfd+1, &readfds_set, &writefds_set, &exceptfds_set, &timeout);
	    if (FD_ISSET(0, &readfds_set)) processinput();
	    for (p = 0; p < MAX_PROCESSES; p++) {
		if ((proc = prolist[p]) == NULL) continue;
		master = proc->pro_master;
		if (FD_ISSET(master, &readfds_set)) processoutput(p);
		else if (proc->change_sg_flags && proc->pro_sts == 1) {
#ifndef HAVE_TERMIOS
		    sb = ttychar.tty_b;
		    sb.sg_flags = proc->sg_flags;
		    ioctl(proc->pro_slave,TIOCSETP,&sb);
#else
		    tcgetattr(proc->pro_slave, &tio);
		    if (proc->sg_flags & RAW) {
			tio.c_lflag &= ~ICANON;
		    } else {
			tio.c_lflag |= ICANON;
		    }
		    if (proc->sg_flags & ECHO) {
			tio.c_lflag |= ECHO;
		    } else {
			tio.c_lflag &= ~ECHO;
		    }
		    tcsetattr(proc->pro_slave, TCSANOW, &tio);
#endif
		    kill(proc->pro_id,SIGCONT);
		    proc->pro_sts = 0;
		    proc->change_sg_flags = 0;
		    proc->change_to_cooked = 0;
		}
		if (FD_ISSET(master, &writefds_set)) {
		    write(master,proc->pro_ibuf,proc->pro_ibuf_counter);
		    proc->pro_ibuf_pointer = proc->pro_ibuf;
		    proc->pro_ibuf_counter = 0;
		}
	    }
	}
}

static void
processoutput(int p)
{
	ssize_t cc;
	char *cp;
	struct pro_str *proc;
	fd_set readfds_set, writefds_set, exceptfds_set;
	struct timeval timeout;

	proc = prolist[p];
	cc = read(proc->pro_master,o_buf,sizeof(o_buf));
	cp = o_buf;
	while (cc--) {
	    (proc->pro_do)(*cp++ & 0177,p,proc);
	    if (--input_freq <= 0) {
		input_freq = INPUT_FREQ;
		FD_ZERO(&readfds_set);
		FD_ZERO(&writefds_set);
		FD_ZERO(&exceptfds_set);
		FD_SET(0, &readfds_set);
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;
		select(1, &readfds_set, &writefds_set, &exceptfds_set, &timeout);
		if (FD_ISSET(0, &readfds_set)) processinput();
	    }
	}
}

static void
processinput(void)
{
	ssize_t cc;
	int f, stop;
	char *cp, *p, ch;
	struct pro_str *proc;

	input_freq = INPUT_FREQ;
	proc = current_process;
	if (!proc) proc = prolist[0];

	stop = 0;
	cc = read(0,i_buf,sizeof(i_buf));
	for(cp = i_buf; cp < i_buf+cc; cp++) {
	    ch = *cp & 0177;
	    if (ch == kill_character) done();
	    if (stoppable && ch == stopch) {
		stop = 1;
		continue;
	    }
	    if (mouse_input_number && '0' <= ch && ch <= '9') {
		if (mouse_input_number == 1)
		    mouse_input_window = mouse_input_window * 10 + (ch - '0');
		else
		    mouse_input_page = mouse_input_page * 10 + (ch - '0');
		wmaddch(mousewin,ch,0);
		continue;
	    }
	    if (mouse_input_number && (ch == '\n' || ch == '\r')) {
		mouse_input_number = 2;
		wmcupos(mousewin,1,4,0);
		continue;
	    }
	    fn_key_partial_match = 0;
	    *fn_key_buf_pointer++ = ch;
	    if (fn_key_char == 0 || *fn_key_buf == fn_key_char) {
		for(f = 0; f < 10+4+1; f++)
		    if (fn_table[f].fn_mode >= 0 &&
			funkeycheck(fn_table[f].fn_def)) break;
	    }
	    else f = 10+4+1;
	    if (f < 10+4+1) {
		if (mousemode &&
		    (f >= 10 ||
		     (((mouse_flag>>f)&1) && mouse_table[f].mouse_msg))) {
		    (*mouse_table[f].do_mouse)(0,0,NULL);
		    (*tt.t_topos)(mouseY+1,mouseX+1);
		    fflush(stdout);
		}
		else if (mousemode && f == 0) domousehome();
		else if (fn_table[f].fn_mode == 0)
		    sendstringtoprocess(proc,fn_table[f].fn_tra);
		else if (fn_table[f].fn_mode == 1) {
		    for (p = fn_table[f].fn_tra; *p;)
			(prolist[0]->pro_do)(*p++,0,prolist[0]);
		}
		fn_key_buf_pointer = fn_key_buf;
	    }
	    if ((mouse_flag & 1) && funkeycheck(MOUSE_MODE)) {
		domouse();
		fn_key_buf_pointer = fn_key_buf;
	    }
	    else if(!fn_key_partial_match) {
		for(p = fn_key_buf; p < fn_key_buf_pointer;)
		    sendtoprocess(proc,*p++);
		fn_key_buf_pointer = fn_key_buf;
	    }
	}

	if (stop) {
	    if (KEstr) tputs(KEstr,0,dumpchar);
	    wmend();
	    fflush(stdout);
#ifndef HAVE_TERMIOS
#ifdef NOFLOWCTL
	    ioctl(0, TIOCSETC, &(ttychar.tty_tc));
#endif
	    ioctl(0, TIOCSETP, (char *)&(ttychar.tty_b));
#else
	    struct termios tio;
	    tcgetattr(0, &tio);
	    tio.c_lflag |= ICANON | ECHO;
	    tcsetattr(0, TCSANOW, &tio);
#endif
	    kill(0, SIGTSTP); /* stop itself */
	    fixtty();
	    wminit();
	    if (KSstr) {
		tputs(KSstr, 0, dumpchar); /* initialize function key */
		fflush(stdout);
	    }
	    if (HLEstr) {
		tputs(HLEstr, 0, dumpchar);
		fflush(stdout);
	    }
	    if (mousemode) wmredraw0(mouseY,mouseX);
	    else if (current_process && current_process->pro_cur_win)
		wmredraw(current_process->pro_cur_win);
	    else wmredraw(winlist[0]);
	}
}

static int
funkeycheck(const char *fk)
{
	char *p;

	if (!*fk) return 0;

	for (p = fn_key_buf; p < fn_key_buf_pointer && *fk;)
		if (*fk++ != *p++) return 0;
	if (*fk) {
	    fn_key_partial_match = 1;
	    return 0;
	}
	return 1;
}

static void
sendtoprocess(struct pro_str *proc, char ch)
{
	if (proc->pro_ibuf_counter >= BUFSIZ) return;
	*proc->pro_ibuf_pointer++ = ch;
	proc->pro_ibuf_counter++;
}

static void
sendstringtoprocess(struct pro_str *proc, const char *str)
{
	while(*str) sendtoprocess(proc,*str++);
}

static int
doaddch(char ch, int p, struct pro_str *proc)
{
	struct win_str *current;

	if (ch == ESC) {
	    proc->pro_do = doesc;
	}
	else if (ch == BELL) {
	    putchar(BELL);
	    fflush(stdout);
	}
	else {
	    current = proc->pro_cur_win;
	    if (!current) {
		nowindow(p);
		return 0;
	    }
	    if (ch == '\n') {
		if (proc->change_to_cooked) wmaddch(current,'\r',0);
		if (!dumpmode &&
		    current->cur_x == current->max_x-1 &&
		    (current->flags & DM_PAGE))
			dorefresh();
		wmaddch(current,'\n',0);
		if (!dumpmode && (current->flags & DM_REFRESH)) dorefresh();
	    }
	    else wmaddch(current,ch,0);
	}
	return 0;
}

static void
nowindow(int p)
{
	if (statuswin) {
	    wmaddstr(statuswin,"No window to process ",0);
	    wmaddnum(statuswin,"%d\r\n",p,0);
	    dorefresh();
	}
}

static void
dorefresh(void)
{
	if (mousemode) wmrefresh0(mouseY,mouseX);
	else if (current_process && current_process->pro_cur_win)
		wmrefresh(current_process->pro_cur_win);
	else wmrefresh(winlist[0]);
}

static int
doesc(char ch, int p, struct pro_str *proc)
{
	proc->pro_stk_top = 0;
	dopushzero(ch,p,proc);
	if ((*vi200table[ch])(ch,p,proc)) proc->pro_do = doaddch;
	return 0;
}

static int
dopushzero(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	if (proc->pro_stk_top >= STK_LEN) {
	    fin(ch,p,proc);
	    return END;
	}
	else {
	    proc->pro_stk[++proc->pro_stk_top] = 0;
	    proc->pro_stk_flg[proc->pro_stk_top] = 0;
	    return CONT;
	}
}

static int
dodigit(char ch, int p, struct pro_str *proc)
{
	(void)p;
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_SET;
	proc->pro_stk[proc->pro_stk_top] =
	    proc->pro_stk[proc->pro_stk_top]*10+(ch-'0');
	return CONT;
}

static int
dosetplus(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_PLUS;
	return CONT;
}

static int
dosetminus(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_MINUS;
	return CONT;
}

static int
doseton(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_ON;
	return CONT;
}

static int
dosetoff(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_OFF;
	return CONT;
}

static int
fin(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	if (statusmode) {
	    wmaddstr(statuswin,"Illegal esc seq from process ",0);
	    wmaddnum(statuswin,"%d\r\n",p,0);
	    dorefresh();
	}
	proc->pro_do = doaddch;
	return END;
}

static int
doansiesc(char ch, int p, struct pro_str *proc)
{
	if ((*ansitable[ch])(ch,p,proc)) proc->pro_do = doaddch;
	return 0;
}

static int
doansi(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = doansiesc;
	return CONT;
}

static int
docursor(char ch, int p, struct pro_str *proc)
{
	int x, y;
	struct win_str *current;

	(void)ch;
	(void)p;
	current = proc->pro_cur_win;
	if (proc->pro_stk_top < 2) wmcupos(current,0,0,0);
	else {
		y = evalarg(proc,1,1,current->cur_y+1,1,current->max_y);
		x = evalarg(proc,2,1,current->cur_x+1,1,current->max_x);
		wmcupos(current,y-1,x-1,0);
	}
	return END;
}

static int
evalarg(struct pro_str *proc, int n, int d, int p, int min, int max)
{
	int flg, x;

	if (n > proc->pro_stk_top) return d;
	flg = proc->pro_stk_flg[n];
	if (!(flg & STK_SET)) return d;
	x = proc->pro_stk[n];
	if (flg & STK_PLUS) x += p;
	else if (flg & STK_MINUS) x = p-x;
	else if (flg & STK_ON) x |= p;
	else if (flg & STK_OFF) x = p & ~x;
	if (x < min) x = min;
	else if (min <= max && x > max) x = max;
	return x;
}

static int
docursorv2(char ch, int p, struct pro_str *proc)
{
	(void)p;
	if (ch < ' ') {
		fin(ch,p,proc);
		return 0;
	}
	wmcupos(proc->pro_cur_win,proc->pro_stk[1],ch-' ',0);
	proc->pro_do = doaddch;
	return 0;
}

static int
docursorv1(char ch, int p, struct pro_str *proc)
{
	proc->pro_do = docursorv2;
	if (ch < ' ') {
		fin(ch,p,proc);
		return 0;
	}
	else proc->pro_stk[1] = ch-' ';
	return 0;
}

static int
docursorv(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = docursorv1;
	return CONT;
}

static int
doeeol(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmcleol(proc->pro_cur_win,0);
	return END;
}

static int
doeeos(char ch, int p, struct pro_str *proc)
{
	int arg = proc->pro_stk[1];

	(void)ch;
	(void)p;
	switch (arg) {
		case 3: wmclp(proc->pro_cur_win);
		case 0: wmcleos(proc->pro_cur_win,0);
			break;
		case 4: wmscroll(proc->pro_cur_win,
			    proc->pro_cur_win->pre_line->line_stack_length,
			    0);
			wmclp(proc->pro_cur_win);
		case 2: wmcls(proc->pro_cur_win,0);
		default: break;
	}
	return END;
}

static int
doeeosv(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmcleos(proc->pro_cur_win,0);
	return END;
}

static int
doup(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmvect(proc->pro_cur_win,-1,0,0);
	return END;
}

static int
dodown(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmvect(proc->pro_cur_win,1,0,0);
	return END;
}

static int
doright(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmvect(proc->pro_cur_win,0,1,0);
	return END;
}

static int
doleft(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmvect(proc->pro_cur_win,0,-1,0);
	return END;
}

static int
dohome(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmcupos(proc->pro_cur_win,0,0,0);
	return END;
}

static int
docld(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmcls(proc->pro_cur_win,0);
	return END;
}

static int
dolinsert(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wminsline(proc->pro_cur_win,proc->pro_cur_win->cur_y,0);
	return END;
}

static int
doldelete(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmdelline(proc->pro_cur_win,proc->pro_cur_win->cur_y,0);
	return END;
}

static int
dosetins(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_cur_win->flags |= WM_INSERT;
	return END;
}

static int
doresetins(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_cur_win->flags &= ~WM_INSERT;
	return END;
}

static int
dodelchar(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	wmdelchar(proc->pro_cur_win,1,0);
	return END;
}

static int
dodelchars(char ch, int p, struct pro_str *proc)
{
	int cc;

	(void)ch;
	(void)p;
	cc = evalarg(proc,1,1,0,1,proc->pro_cur_win->max_x);
	wmdelchar(proc->pro_cur_win,cc,0);
	return END;
}

static int
doredraw(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	(void)proc;
	if (mousemode) wmredraw0(mouseY,mouseX);
	else if (current_process && current_process->pro_cur_win)
		wmredraw(current_process->pro_cur_win);
	else wmredraw(winlist[0]);
	return END;
}

static int
doreverselinefeed(char ch, int p, struct pro_str *proc)
{
	struct win_str *current;

	(void)ch;
	(void)p;
	current = proc->pro_cur_win;
	if (current->scroll_start < current->cur_y) {
	    --current->cur_y;
	}
	else if (current->flags & WM_SCROLL) {
	    wmscroll(current,-1,0);
	    current->cur_y = current->scroll_start;
	}
	else if (current->flags & DM_PAGE) {
	    wmscroll(current,current->scroll_start-current->max_y,0);
	    current->cur_y = current->max_y-1;
	}
	return END;
}

static int
dosetscroll(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_cur_win->scroll_start = proc->pro_cur_win->cur_y;
	return END;
}

static int
dostandout(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	if (proc->pro_stk[1]) proc->pro_cur_win->flags |= WM_STANDOUT;
	else proc->pro_cur_win->flags &= ~WM_STANDOUT;
	return END;
}

static int
dospecialesc(char ch, int p, struct pro_str *proc)
{
	if ((*specialtable[ch])(ch,p,proc)) proc->pro_do = doaddch;
	return 0;
}

static int
dospecial(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = dospecialesc;
	return CONT;
}

static int
dosetmode(char ch, int p, struct pro_str *proc)
{
	int w, bw, flg, dfl;

	(void)ch;
	if (proc->pro_stk_top < 2) {
	    w = proc->pro_cur_win_num;
	    flg = 1;
	}
	else {
	    flg = 2;
	    bw = proc->pro_base_win_num;
	    w = proc->pro_cur_win_num-bw;
	    w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	}
	if (winlist[w] == NULL) return END;
	if (winlist[w]->max_line) dfl = WM_AUTONL | DM_PAGE;
	else dfl = WM_AUTONL | WM_SCROLL;
	winlist[w]->flags = evalarg(proc,flg,dfl,winlist[w]->flags,-INF,INF);
	return END;
}

static int
dosetecho(char ch, int p, struct pro_str *proc)
{
    int old_sg_flags;

	(void)ch;
	old_sg_flags = proc->sg_flags;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		proc->sg_flags |= ECHO;
	else proc->sg_flags &= ~ECHO;
	if (old_sg_flags == proc->sg_flags) return END;
	if (!proc->change_sg_flags) {
	    proc->change_sg_flags = 1;
	    stopprocess(p);
	}
	return END;
}

static int
dosetraw(char ch, int p, struct pro_str *proc)
{
    int old_sg_flags;

	(void)ch;
	old_sg_flags = proc->sg_flags;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON))) {
	    proc->sg_flags |= RAW;
	    proc->change_to_cooked = 0;
	}
	else {
	    if (proc->sg_flags & RAW) proc->change_to_cooked = 1;
	    proc->sg_flags &= ~RAW;
	}
	if (proc->sg_flags == old_sg_flags) return END;
	if (!proc->change_sg_flags) {
	    proc->change_sg_flags = 1;
	    stopprocess(p);
	}
	return END;
}

static int
dosetdump(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		dumpmode = 1;
	else dumpmode = 0;
	dorefresh();
	return END;
}

static int
dosetkill(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	kill_character = evalarg(proc,1,0,kill_character,0,127);
	return END;
}

static int
dosetstop(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	stopch = evalarg(proc,1,0,stopch,0,127);
	return END;
}

static int
dosetmouse(char ch, int p, struct pro_str *proc)
{
	int new_flag;

	(void)ch;
	(void)p;
	new_flag = evalarg(proc,1,01777,mouse_flag,-INF,INF);
	if (new_flag != mouse_flag) {
	    if (!(new_flag & 1) && mousemode) {
		domouseend();
		dorefresh();
	    }
	    mouse_flag = new_flag;
	    if (mousemode) wmdel(mousewin,0);
	    if (mousewin) wmfree(mousewin);
	    mousewin = NULL;
	    if (mousemode) {
		createmouse();
		wmputwin(mousewin,top_win,1);
		mouse_win_number = -1;
		mouse_page_number = -1;
		mouse_process_number = -1;
	    }
	}
	return END;
}

static int
dostring(char ch, int p, struct pro_str *proc)
{
	(void)p;
	if (proc->pro_mch == 0) {
	    if (ch == '[') proc->pro_mch = ']';
	    else if (ch == '(') proc->pro_mch = ')';
	    else if (ch == '<') proc->pro_mch = '>';
	    else if (ch == '{') proc->pro_mch = '}';
	    else proc->pro_mch = ch;
	    return CONT;
	}
	if (ch == proc->pro_mch) {
	    *(proc->pro_sbuf_pointer) = 0;
	    proc->pro_do = doaddch;
	    (proc->pro_sdo)(p,proc);
	    return END;
	}
	if (proc->pro_sbuf_pointer < proc->pro_sbuf+MAX_STR-1) {
	    *proc->pro_sbuf_pointer++ = ch;
	}
	return CONT;
}

static int
docopys(int p, struct pro_str *proc)
{
	const char *copy_mode;
	FILE *copy_file;

	(void)p;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		copy_mode = "w";
	else copy_mode = "a";
	copy_file = fopen(proc->pro_sbuf,copy_mode);
	if (copy_file) {
		if (ftell(copy_file)) {
			fputc(12,copy_file);
		}
		wmcopy(copy_file,1);
		fclose(copy_file);
	}
	return END;
}

static int
docopy(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = dostring;
	proc->pro_sdo = docopys;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

static int
dosetfnkeys(int p, struct pro_str *proc)
{
	int f, m;

	(void)p;
	f = evalarg(proc,1,0,0,0,10+4+1-1);
	m = evalarg(proc,2,0,0,-INF,INF);
	fn_table[f].fn_mode = m;
	free((char *)fn_table[f].fn_tra);
	fn_table[f].fn_tra =
	    (char *) calloc(strlen(proc->pro_sbuf)+1, sizeof(char));
	strcpy(fn_table[f].fn_tra,proc->pro_sbuf);
	return END;
}

static int
dosetfnkey(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = dostring;
	proc->pro_sdo = dosetfnkeys;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

static int
dopsdins(int p, struct pro_str *proc)
{
	int pp;

	pp = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (prolist[pp] == NULL) return END;
	sendstringtoprocess(prolist[pp],proc->pro_sbuf);
	return END;
}

static int
dopsdin(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = dostring;
	proc->pro_sdo = dopsdins;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

static int
dosetbase(char ch, int p, struct pro_str *proc)
{
	int pn, wn;

	(void)ch;
	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	wn = evalarg(proc,2,proc->pro_base_win_num,proc->pro_base_win_num,
			0,MAX_WINDOWS-1);
	if (prolist[pn] == NULL || winlist[wn] == NULL) return END;
	prolist[pn]->pro_base_win_num = wn;
	return END;
}

static int
dogivekeyboard(char ch, int p, struct pro_str *proc)
{
	int pn;

	(void)ch;
	(void)proc;
	if (current_process_number != p) return END;
	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (prolist[pn] == NULL) return END;
	current_process_number = pn;
	current_process = prolist[pn];
	return END;
}

static int
dokillprocess(char ch, int p, struct pro_str *proc)
{
	int pn;

	(void)ch;
	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (pn == 0 || prolist[pn] == NULL) return END;
	kill(prolist[pn]->pro_id,SIGKILL);
	return END;
}

static int
dostartprocess(char ch, int p, struct pro_str *proc)
{
	int pn, w, bw;

	(void)ch;
	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,2,w,w,0,MAX_WINDOWS-1-bw)+bw;

	if (prolist[pn] || winlist[w] == NULL) return END;
	startprocess(pn,w);
	return END;
}

static int
doprlesc(char ch, int p, struct pro_str *proc)
{
	if ((*prltable[ch])(ch,p,proc)) proc->pro_do = doaddch;
	return 0;
}

static int
doprl(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = doprlesc;
	return CONT;
}

static int
docreate(char ch, int p, struct pro_str *proc)
{
	int w, x1, x2, y1, y2, pages, bw;

	(void)ch;
	bw = proc->pro_base_win_num;
	for(w = bw; w < MAX_WINDOWS; w++) if (winlist[w] == NULL) break;
	if (w >= MAX_WINDOWS) w = bw;
	else w = w-bw;
	w = evalarg(proc,1,w,proc->pro_cur_win_num-bw,0,MAX_WINDOWS-1-bw)+bw;
	x1 = evalarg(proc,2,0,0,-INF,INF);
	x2 = evalarg(proc,3,x1+ScreenWidth-1,x1,x1,INF);
	y1 = evalarg(proc,4,0,0,-INF,INF);
	y2 = evalarg(proc,5,y1+ScreenLength-1,y1,y1,INF);
	pages = evalarg(proc,6,0,0,0,INF);

	if (winlist[w]) {
	    wmdel(winlist[w],0);
	    wmfree(winlist[w]);
	}
	proc->pro_cur_win = winlist[w] =
		wmmake(y2-y1+1,x2-x1+1,y1,x1,pages,0);
	proc->pro_cur_win_num = w;
	if (statusmode) {
	    wmaddstr(statuswin,"Process ",0);
	    wmaddnum(statuswin,"%d",p,0);
	    wmaddstr(statuswin," create ",0);
	    dowinfo(w);
	}
	return END;
}

static void
dowinfo(int w)
{
	struct win_str *win;

	if (!statusmode || w < 0 || w >= MAX_WINDOWS) return;
	if ((win = winlist[w])) {
	    wmaddnum(statuswin,"window %d ",w,0);
	    wmaddnum(statuswin,"at (%d,",win->beg_x,0);
	    wmaddnum(statuswin,"%d) ",win->beg_y,0);
	    wmaddnum(statuswin,"c=%d ",win->max_x,0);
	    wmaddnum(statuswin,"l=%d\r\n",win->max_y,1);
	}
}

static int
dodestroy(char ch, int p, struct pro_str *proc)
{
	int w, bw;
	struct pro_str *proc2;

	(void)ch;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;

	if (w == 0 || winlist[w] == NULL) return END;
	wmdel(winlist[w],0);
	wmfree(winlist[w]);
	winlist[w] = NULL;
	if (statusmode) {
	    wmaddnum(statuswin,"Process %d ",p,0);
	    wmaddstr(statuswin,"delete window ",0);
	    wmaddnum(statuswin,"%d\r\n",w,1);
	}

	for(p = 0; p < MAX_PROCESSES; p++) {
	   proc2 = prolist[p];
	   if (proc2 && proc2->pro_cur_win_num == w) {
		if (winlist[proc2->pro_base_win_num])
			proc2->pro_cur_win_num = proc2->pro_base_win_num;
		else proc2->pro_cur_win_num = 0;
		if (statusmode) {
		    wmaddnum(statuswin,"Process %d",p,0);
		    wmaddstr(statuswin,"'s current window is forced to",0);
		    wmaddnum(statuswin," %d\r\n",proc2->pro_cur_win_num,1);
		}
		proc2->pro_cur_win = winlist[proc2->pro_cur_win_num];
	    }
	}
	return END;
}

static int
doenquire(char ch, int p, struct pro_str *proc)
{
	int bw, w, f;
	struct win_str *win;
	char buf[100];

	(void)ch;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if (proc->pro_stk_top < 2) f = 0;
	else f = evalarg(proc,2,1,0,-INF,INF);

	if ((win = winlist[w])) {
	    if (f) {
		snprintf(buf, sizeof(buf),"\033{%d;%d;%d;%d;%d;%d;%d;%d;%d;%de",
			w,
			win->cur_x,
			win->cur_y,
			win->beg_x,
			win->beg_y,
			win->max_x,
			win->max_y,
			win->pre_line->line_stack_length,
			win->next_line->line_stack_length,
			win->max_line);
	    }
	    else {
		snprintf(buf, sizeof(buf),"\033{%d;%d;%d;%d;%d;%d;%de",
			w,
			win->cur_x,
			win->cur_y,
			win->beg_x,
			win->beg_y,
			win->max_x,
			win->max_y);
	    }
	    sendstringtoprocess(proc,buf);
	}
	else if (f) {
	    sendstringtoprocess(proc,"\033{0;0;0;0;0;0;0;0;0;0e");
	}
	else {
	    sendstringtoprocess(proc,"\033{0;0;0;0;0;0;0e");
	}
	return END;
}

static int
doselect(char ch, int p, struct pro_str *proc)
{
	int w, bw;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,0,w,0,MAX_WINDOWS-1-bw)+bw;

	if (winlist[w]) {
	    proc->pro_cur_win = winlist[w];
	    proc->pro_cur_win_num = w;
	}
	return END;
}

static int
dopopwindow(char ch, int p, struct pro_str *proc)
{
	int bw, w, w2;
	struct win_str *win, *win2;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((win = winlist[w]) == NULL) return END;
	if (proc->pro_stk_top < 2) win2 = top_win;
	else if (!(proc->pro_stk_flg[2] & STK_SET)) win2 = NULL;
	else {
	    w2 = proc->pro_cur_win_num-bw;
	    w2 = evalarg(proc,2,w2,w2,0,MAX_WINDOWS-1-bw)+bw;
	    if ((win2 = winlist[w2]) == NULL) return END;
	}

	wmrepos(win,win2,win->beg_y,win->beg_x,0);

	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

static int
doreposwindow(char ch, int p, struct pro_str *proc)
{
	int w, bw, x, y;
	struct win_str *win;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((win = winlist[w]) == NULL) return END;
	x = evalarg(proc,2,win->beg_x,win->beg_x,-INF,INF);
	y = evalarg(proc,3,win->beg_y,win->beg_y,-INF,INF);
	wmrepos(win,win->next_win,y,x,0);
	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

static int
dopageselect(char ch, int p, struct pro_str *proc)
{
	int w, bw, l;
	struct win_str *wp;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	if (proc->pro_stk_top < 2) l = wp->next_line->line_stack_length;
	else l = proc->pro_stk[2] * (wp->max_y - wp->scroll_start) -
			wp->pre_line->line_stack_length;
	wmscroll(wp,l,0);
	return END;
}

static int
dopageforward(char ch, int p, struct pro_str *proc)
{
	int w, bw, page;
	struct win_str *wp;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	page = evalarg(proc,2,1,0,0,INF);
	wmscroll(wp,page*(wp->max_y-wp->scroll_start),0);
	return END;
}

static int
dopagebackward(char ch, int p, struct pro_str *proc)
{
	int w, bw, page;
	struct win_str *wp;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	page = evalarg(proc,2,1,0,0,INF);
	wmscroll(wp,-page*(wp->max_y-wp->scroll_start),0);
	return END;
}

static int
doscroll(char ch, int p, struct pro_str *proc)
{
	int bw, w, n;
	struct win_str *win;

	(void)ch;
	(void)p;
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((win = winlist[w]) == NULL) return END;
	if (proc->pro_stk_top < 2) {
	    wmscroll(win,win->next_line->line_stack_length,0);
	}
	else if (!(proc->pro_stk_flg[2] & STK_SET)) {
	    wmscroll(win,-win->pre_line->line_stack_length,0);
	}
	else if (proc->pro_stk_top < 3) {
	    n = evalarg(proc,2,0,0,-INF,INF);
	    wmscroll(win,n,0);
	}
	else {
	    n = evalarg(proc,2,0,0,-win->pre_line->line_stack_length,
	    			   win->next_line->line_stack_length);
	    wmscroll(win,n,0);
	}

	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

static int
doheadings(int p, struct pro_str *proc)
{
	int w, bw;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if (winlist[w] == NULL) return END;
	wmheadstr(winlist[w],proc->pro_sbuf,0);
	return END;
}

static int
doheading(char ch, int p, struct pro_str *proc)
{
	(void)ch;
	(void)p;
	proc->pro_do = dostring;
	proc->pro_sdo = doheadings;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

/* mouse support routines */

static void
domouseend(void)
{
	if (mousemode) {
	    wmdel(mousewin,0);
	    wmdel(mousemark,0);
	    mousemark_set = 0;
	    mouse_input_number = 0;
	    mousemode = 0;
	}
}

static void
domouse(void)
{
	if (mousemode) { /* reset mouse mode */
	    domouseend();
	    dorefresh();
	    return;
	}

	/* set mouse mode */

	mousemode = 1;
	mouse_input_number = 0;
	mouse_win_number = -1;
	mouse_page_number = -1;
	mouse_process_number = -1;
	mouse_increment = 1;

	if (!mousewin) createmouse();
	wmputwin(mousewin,top_win,1);

	if (!mousemark) {
	    mousemark = wmcreate(1,1,0,0);
	    mousemark->flags |= WM_STANDOUT;
	    wmaddstr(mousemark,"*",0);
	}
}

static void
createmouse(void)
{
	int mwlen, f;
	char tbuf[16];

	mwlen = 3;
	for (f = 0; f < 10; f++) {
	    if ((mouse_flag>>f)&1 && mouse_table[f].mouse_msg && Kstr[f])
		mwlen++;
	}
	mousewin = wmcreate(mwlen,10,1,0);
	mousewin->beg_y = 1;
	mousewin->beg_x = ScreenWidth-11;
	wmcupos(mousewin,0,0,0);
	wmheadstr(mousewin,"Mouse Mode",0);
	wmaddstr(mousewin,"w = ",0);
	wmaddstr(mousewin,"\r\np = ",0);
	wmaddstr(mousewin,"\r\ncp= ",0);
	for (f = 0; f < 10; f++) {
	    if ((mouse_flag>>f)&1 && mouse_table[f].mouse_msg && Kstr[f]) {
		snprintf(tbuf,sizeof(tbuf),"\r\nF%d %s",f,mouse_table[f].mouse_msg);
		wmaddstr(mousewin,tbuf,0);
	    }
	}
}

static void
wmsetwnum(void) /* display current window number */
{
	int w, cp, pl;
	struct win_str *wp;

	if (mousemode != 1) return;

	if (mouse_input_number == 0) {
	    wp = wmgetmap(mouseY,mouseX);
	    w = dosearchwin(wp);
	    if (w < 0) w = -2;
	    if (w != mouse_win_number) {
		wmcupos(mousewin,0,4,0);
		if (w >= 0) wmaddnum(mousewin,"%d",w,0);
		else wmaddstr(mousewin,"????",0);
		wmcleol(mousewin,0);
	    }
	    mouse_win_number = w;
	    if (wp) {
		pl = wp->max_y - wp->scroll_start;
		cp = (wp->pre_line->line_stack_length + pl - 1)/pl;
		if (cp != mouse_page_number) {
	 	    wmcupos(mousewin,1,4,0);
		    wmaddnum(mousewin,"%d",cp,0);
		    wmcleol(mousewin,0);
		    mouse_page_number = cp;
		}
	    }
	    else if (mouse_page_number != -2) {
	 	    wmcupos(mousewin,1,4,0);
		    wmaddstr(mousewin,"????",0);
		    wmcleol(mousewin,0);
		    mouse_page_number = -2;
	    }
	}
	if (current_process_number != mouse_process_number) {
	    wmcupos(mousewin,2,4,0);
	    wmaddnum(mousewin,"%d",current_process_number,0);
	    wmcleol(mousewin,0);
	    mouse_process_number = current_process_number;
	}
}

static void
domousehome(void)
{
	mouse_increment *= 4;
}

static void
domouseup(void)
{
	mouseY -= mouse_increment;
	mouse_increment = 1;
	if (mouseY < 0) mouseY = 0;
}

static void
domousedown(void)
{
	mouseY += mouse_increment;
	mouse_increment = 1;
	if (mouseY >= ScreenLength) mouseY = ScreenLength-1;
}

static void
domouseright(void)
{
	mouseX += mouse_increment;
	mouse_increment = 1;
	if (mouseX >= ScreenWidth) mouseX = ScreenWidth-1;
}

static void
domouseleft(void)
{
	mouseX -= mouse_increment;
	mouse_increment = 1;
	if (mouseX < 0) mouseX = 0;
}

static void
dosetmark(void)
{
	mousemark->beg_x = mouseX;
	mousemark->beg_y = mouseY;
	wmputwin(mousemark,top_win,0);
	mousemark_set = 1;
}

static void
doresetinput(void)
{
	if (!mouse_input_number) return;
	mouse_input_number = 0;
	mouse_win_number = -1;
	mouse_page_number = -1;
	mouse_process_number = -1;
}

static void
domousecreate(void)
{
	int lines, cols, begin_x, begin_y, w, p;

	mouse_increment = 1;
	if (!mousemark_set) {
		dosetmark();
		wmcupos(mousewin,1,4,0);
		wmcleol(mousewin,0);
		wmcupos(mousewin,0,4,0);
		wmcleol(mousewin,0);
		mouse_input_number = 1;
		mouse_input_window = 0;
		mouse_input_page = 0;
		return;
	}
	w = mouse_input_window;
	p = mouse_input_page;
	wmdel(mousemark,0);
	mousemark_set = 0;
	doresetinput();

	if (w < 0 || w >= MAX_WINDOWS) return;
	if (w == 0) {
		for (w = 1; w < MAX_WINDOWS && winlist[w]; w++);
		if (w >= MAX_WINDOWS) return;
	}

	if (mouseX >= mousemark->beg_x) {
		cols = mouseX - mousemark->beg_x + 1;
		begin_x = mousemark->beg_x;
	}
	else {
		cols = mousemark->beg_x - mouseX + 1;
		begin_x = mouseX;
	}
	if (mouseY >= mousemark->beg_y) {
		lines = mouseY - mousemark->beg_y + 1;
		begin_y = mousemark->beg_y;
	}
	else {
		lines = mousemark->beg_y - mouseY + 1;
		begin_y = mouseY;
	}
	if (winlist[w]) {
		wmdel(winlist[w],0);
		wmfree(winlist[w]);
	}
	winlist[w] = wmmake(lines,cols,begin_y,begin_x,p,0);
	if (statusmode) {
	    wmaddstr(statuswin,"Create ",0);
	    dowinfo(w);
	}
}

static int
dosearchwin(struct win_str *win)
{
	int w;

	if (win == mousewin || win == mousemark) return -1;
	for (w = 0; w < MAX_WINDOWS; w++)
		if (win == winlist[w]) return w;
	return -1;
}

static void
domousedelete(void)
{
	int w, p;
	struct win_str *win;
	struct pro_str *proc;

	mouse_increment = 1;
	doresetinput();

	win = wmgetmap(mouseY,mouseX);
	if (win == mousewin || win == mousemark || win == statuswin) return;
	w = dosearchwin(win);
	if (w == 0) return;
	wmdel(win,0);
	wmfree(win);
	if (w > 0) {
	    winlist[w] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Delete window ",0);
		wmaddnum(statuswin,"%d\r\n",w,1);
	    }
	    for(p = 0; p < MAX_PROCESSES; p++) {
		proc = prolist[p];
		if (proc && proc->pro_cur_win_num == w) {
		    if (winlist[proc->pro_base_win_num])
			proc->pro_cur_win_num = proc->pro_base_win_num;
		    else proc->pro_cur_win_num = 0;
		    if (statusmode) {
			wmaddnum(statuswin,"Process %d",p,0);
			wmaddstr(statuswin,"'s current window is forced to",
					0);
			wmaddnum(statuswin," %d\r\n",proc->pro_cur_win_num,
					1);
		    }
		    proc->pro_cur_win = winlist[proc->pro_cur_win_num];
	        }
	    }
	}
}

static void
domousemove(void)
{
	struct win_str *win;
	int x, y;

	mouse_increment = 1;
	doresetinput();

	if (!mousemark_set) {
		dosetmark();
		return;
	}
	wmdel(mousemark,0);
	mousemark_set = 0;

	win = wmgetmap(mousemark->beg_y,mousemark->beg_x);
	if (win != winlist[0]) {
		x = win->beg_x + (mouseX - mousemark->beg_x);
		y = win->beg_y + (mouseY - mousemark->beg_y);
	}
	else { x = win->beg_x; y = win->beg_y;}

	wmrepos(win,top_win,y,x,0);
}

static void
domousecopy(void)
{
	FILE *copy_file;

	copy_file = fopen(mouse_copy_file,"a");
	if (copy_file) {
		if (ftell(copy_file)) {
			fputc(12,copy_file);
		}
		wmdel(mousewin,0);
		if (mousemark_set) {
			wmdel(mousemark,0);
			mousemark_set = 0;
			doresetinput();
		}
		wmdel(mousemark,0);
		wmcopy(copy_file,1);
		fclose(copy_file);
		wmputwin(mousewin,top_win,1);
	}
}

static void
domousenext(void)
{
	struct win_str *wp;
	int nls, ls;

	wp = wmgetmap(mouseY,mouseX);
	if (!wp) return;
	if ((nls = wp->next_line->line_stack_length) == 0) return;
	ls = (wp->max_y-wp->scroll_start) * mouse_increment;
	if (ls > nls) ls = nls;
	wmscroll(wp,ls,0);
	mouse_increment = 1;
}

static void
domouseback(void)
{
	struct win_str *wp;
	int ls, pls;

	wp = wmgetmap(mouseY,mouseX);
	if (!wp) return;
	if ((pls = wp->pre_line->line_stack_length) == 0) return;
	ls = (wp->max_y-wp->scroll_start) * mouse_increment;
	if (ls > pls) ls = pls;
	wmscroll(wp,-ls,0);
	mouse_increment = 1;
}

static void
domousepush(void)
{
	struct win_str *win;

	mouse_increment = 1;
	doresetinput();

	win = wmgetmap(mouseY,mouseX);
	if (!win) return;
	if (win == winlist[0]) wmrepos(win,NULL,win->beg_y,win->beg_x,0);
	else wmrepos(win,winlist[0],win->beg_y,win->beg_x,0);
}

static void
domouseprocessselect(void)
{
	int p, w;
	struct win_str *win;

	win = wmgetmap(mouseY,mouseX);
	if (win == mousewin || win == mousemark || win == statuswin) return;
	w = dosearchwin(win);

	for (p = current_process_number+1; p < MAX_PROCESSES; p++)
	    if (prolist[p] && prolist[p]->pro_cur_win_num == w) break;
	if (p >= MAX_PROCESSES) {
	    for (p = 0; p < current_process_number; p++)
		if (prolist[p] && prolist[p]->pro_cur_win_num == w) break;
	    if (p >= current_process_number) return;
	}
	current_process = prolist[p];
	current_process_number = p;
}

static void
domouseprocessnext(void)
{
	int p;

	for (p = current_process_number+1; p < MAX_PROCESSES; p++)
	    if (prolist[p]) break;
	if (p >= MAX_PROCESSES) p = 0;
	current_process_number = p;
	current_process = prolist[p];
}

static void
initmouse(void)
{
	int f;

	for (f = 0; f < 10+4+1; f++) {
	    mouse_table[f].mouse_msg = NULL;
	}
	mouse_table[1].do_mouse = (funcptr)domousecreate;
	mouse_table[1].mouse_msg = "Create";
	mouse_table[2].do_mouse = (funcptr)domousedelete;
	mouse_table[2].mouse_msg = "Delete";
	mouse_table[3].do_mouse = (funcptr)domousemove;
	mouse_table[3].mouse_msg = "Move";
	mouse_table[4].do_mouse = (funcptr)domouseback;
	mouse_table[4].mouse_msg = "Back";
	mouse_table[5].do_mouse = (funcptr)domousenext;
	mouse_table[5].mouse_msg = "Next";
	mouse_table[6].do_mouse = (funcptr)domousepush;
	mouse_table[6].mouse_msg = "Push";
	mouse_table[7].do_mouse = (funcptr)domouseprocessselect;
	mouse_table[7].mouse_msg = "PSelect";
	mouse_table[8].do_mouse = (funcptr)domouseprocessnext;
	mouse_table[8].mouse_msg = "PNext";
	mouse_table[9].do_mouse = (funcptr)domousecopy;
	mouse_table[9].mouse_msg = "Copy";
	mouse_table[10].do_mouse = (funcptr)domouseup;
	mouse_table[11].do_mouse = (funcptr)domousedown;
	mouse_table[12].do_mouse = (funcptr)domouseright;
	mouse_table[13].do_mouse = (funcptr)domouseleft;
	mouse_table[14].do_mouse = (funcptr)domousehome;
	mouse_flag = 01777;
}

static void
initeachtbl(funcptr *tbl)
{
	int i;
	for (i = 0; i <= 0177; i++) {
	    tbl[i] = fin;
	}
	for (i = '0'; i <= '9'; i++) {
	    tbl[i] = dodigit;
	}
	tbl[';'] = dopushzero;
	tbl['+'] = dosetplus;
	tbl['-'] = dosetminus;
	tbl['|'] = doseton;
	tbl['&'] = dosetoff;
}

static void
inittbl(void)
{
	int i;
	for (i = 0; i <= 0177; i++) vi200table[i] = fin;
	vi200table['K'] = doeeol;
	vi200table['x'] = doeeol;
	vi200table['J'] = doeeosv;
	vi200table['y'] = doeeosv;
	vi200table['H'] = dohome;
	vi200table['v'] = docld;
	vi200table['Y'] = docursorv;
	vi200table['A'] = doup;
	vi200table['B'] = dodown;
	vi200table['C'] = doright;
	vi200table['D'] = doleft;
	vi200table['L'] = dolinsert;
	vi200table['M'] = doldelete;
	vi200table['i'] = dosetins;
	vi200table['j'] = doresetins;
	vi200table['O'] = dodelchar;
	vi200table['R'] = doredraw;
	vi200table[10] = doreverselinefeed;
	vi200table['r'] = dosetscroll;
	vi200table['['] = doansi;
	vi200table['('] = dospecial;
	vi200table['{'] = doprl;

	initeachtbl(ansitable);
	ansitable['f'] = docursor;
	ansitable['H'] = docursor;
	ansitable['K'] = doeeol;
	ansitable['J'] = doeeos;
	ansitable['A'] = doup;
	ansitable['B'] = dodown;
	ansitable['C'] = doright;
	ansitable['D'] = doleft;
	ansitable['L'] = dolinsert;
	ansitable['M'] = doldelete;
	ansitable['h'] = dosetins;
	ansitable['l'] = doresetins;
	ansitable['P'] = dodelchars;
	ansitable['m'] = dostandout;

	initeachtbl(prltable);
	prltable['C'] = docreate;
	prltable['D'] = dodestroy;
	prltable['E'] = doenquire;
	prltable['S'] = doselect;
	prltable['P'] = dopopwindow;
	prltable['R'] = doreposwindow;
	prltable['p'] = dopageselect;
	prltable['f'] = dopageforward;
	prltable['b'] = dopagebackward;
	prltable['H'] = doheading;
	prltable['A'] = dosetmode;
	prltable['s'] = doscroll;

	initeachtbl(specialtable);
	specialtable['A'] = dosetmode;
	specialtable['E'] = dosetecho;
	specialtable['R'] = dosetraw;
	specialtable['D'] = dosetdump;
	specialtable['K'] = dosetkill;
	specialtable['S'] = dosetstop;
	specialtable['M'] = dosetmouse;
	specialtable['C'] = docopy;
	specialtable['F'] = dosetfnkey;
	specialtable['I'] = dopsdin;
	specialtable['b'] = dosetbase;
	specialtable['g'] = dogivekeyboard;
	specialtable['k'] = dokillprocess;
	specialtable['s'] = dostartprocess;
}
