/*
 * super (?) window terminal driver
 *
 *	By Tatsuya Hagino   June 1984 (revised October 1984)
 */

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
#include <signal.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>

/* private include files */
#include "display.h"
#include "winlib.h"

/* types */
typedef ((*funcptr)());

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
    funcptr pro_sdo;
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
    char *mouse_msg;			/* message */
    } mouse_table[10+4+1];		/* mouse table */

/* function pre-definition */
char	*getenv();
int	finish();
int	doaddch();
int	doesc();

/* data */
char	*shell;				/* default shell name */
int	is_shell;			/* is it really a shell */
int	is_cshell;			/* is it a c shell */

extern struct win_str *top_win;		/* top window */

char	i_buf[BUFSIZ];			/* input buffer */
char	o_buf[BUFSIZ];			/* output buffer */

struct pro_str *current_process;	/* current input process */
int	current_process_number;		/* current input process number */

char	fn_key_buf[10];			/* function key buffer */
char	*fn_key_buf_pointer;		/* function key buffer pointer */

int kill_character = 28;		/* kill character (default ^\) */

funcptr ansitable[0200];		/* ansi escape sequence */
funcptr vi200table[0200];		/* vi200 escape sequnce */
funcptr prltable[0200];			/* window escape sequence */
funcptr specialtable[0200];		/* special escape sequence */

struct win_str *mousewin;		/* mouse window */
struct win_str *mousemark;		/* mouse mark */
int mouse_win_number;			/* mouse window number */
int mouse_page_number;			/* mouse page number */
int mouse_process_number;
int mouse_increment;			/* cursor increment */
int mouse_input_window;
int mouse_input_page;
int mouseY,mouseX;			/* mouse position */
char *mouse_copy_file;			/* hardcopy file name */

struct win_str *statuswin;		/* status window */

int input_freq;				/* input service frequency */

char fn_key_char;			/* function key common starter */

/* flags */
int statusmode;				/* status mode */

int dumpmode;				/* dump mode */

int mousemode;				/* mouse mode */
int mousemark_set;			/* mouse mark set/reset */
int mouse_input_number;			/* mouse input mode */
int mouse_flag;				/* mouse capability */

int fn_key_partial_match;

int new_mask;				/* input mask recalculation */

char **nenvp;				/* new environment */
int new_envp;

int stoppable;				/* stop */
char stopch;

/* program */

char **sub_argv;
char *sub_init_str;
int sub_init_lf;

main(argc,argv,envp)
int argc;
char **argv,**envp;
{
	char *cp;
	int help,l,sub_argc;

	mouse_copy_file = NULL;
	statusmode = FALSE;
	help = FALSE;
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
	sub_init_lf = FALSE;
	while (argc > 1) {
	    if (*argv[1] == '-') {
		cp = argv[1];
		while (*++cp) {
		    switch(*cp) {
		    case 'a': sub_argv = &(argv[1]);
			      sub_argc = 1;
			      argc = 0;
			      break;
		    case 's': statusmode = TRUE;
			      break;
		    case 'h': help = TRUE;
			      break;
		    case 'q': stoppable = TRUE;
			      break;
		    case 'Q': stoppable = FALSE;
			      break;
		    case 'I': sub_init_lf = TRUE;
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

writehelp()
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

initialize()
{
#ifdef WELCOM
	FILE *f;
#endif
	int p,ch,l;
	char *s,*term,*termcap,**envp,**envq;

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
	if (term = getenv("WTERM")) {
	    l++;
	    s = (char *) calloc(strlen(term)+6,sizeof(char));
	    strcpy(s,"TERM=");
	    strcat(s,term);
	    term = s;
	}
	if (termcap = getenv("WTERMCAP")) {
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
	    new_envp = TRUE;
	}
	else new_envp = FALSE;

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

	(void) signal(SIGCHLD, finish);
}

gettty()
{
	ioctl(0, TIOCGETP, (char *)&(ttychar.tty_b));
	ioctl(0, TIOCGETC, (char *)&(ttychar.tty_tc));
	ioctl(0, TIOCGETD, (char *)&(ttychar.tty_l));
	ioctl(0, TIOCGLTC, (char *)&(ttychar.tty_lc));
	ioctl(0, TIOCLGET, (char *)&(ttychar.tty_lb));
	stopch = ttychar.tty_lc.t_suspc;
}

fixtty()
{
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
}

static dumpchar(c)
{
	putchar(c);
}

initwin()
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
	winlist[0] = wmmake(ScreenLength,ScreenWidth,0,0,0,FALSE);
	if (statusmode) {
	    statuswin = wmmake(ScreenLength/4,
				ScreenWidth/2,
				1,
				ScreenWidth-ScreenWidth/2-1,
				3,
				FALSE);
	    wmheadstr(statuswin,"WM Status Window",FALSE);
	    statuswin->flags |= WM_SCROLL;
	}
	else statuswin = NULL;
	dumpmode = FALSE;
}

initfnkey()
{
	int f,flg;
	char *p,ch;

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

	flg = FALSE;
	for (f = 0; f < 10+4+1; f++) {
	    if (ch = *fn_table[f].fn_def) {
		if (!flg) { fn_key_char = ch; flg = TRUE; }
		else if (ch != fn_key_char) fn_key_char = 0;
	    }
	}
	if (!flg) fn_key_char = 0;
}

startprocess(p,w)
int p,w;
{
	struct pro_str *proc;
	char line[11];
	char c;
	int i,master,slave,child;
	struct stat stb;
	struct tchars tbuf;

	if (p < 0 || p >= MAX_PROCESSES || prolist[p]) return;
	if (w < 0 || w >= MAX_WINDOWS || winlist[w] == NULL) return;
	prolist[p] = proc =
		(struct pro_str *) calloc(1, sizeof(struct pro_str));

	strcpy(line,"/dev/ptyXX");
	for(c = 'p'; c <= 's'; c++) {
	    line[strlen("/dev/pty")] = c;
	    line[strlen("/dev/ptyp")] = '0';
	    if (stat(line, &stb) < 0) break;
	    for (i = 15; i >= 0; i--) {
		line[strlen("/dev/ptyp")] = "0123456789abcdef"[i];
		master = open(line, 2);
		if (master >= 0) break;
	    }
	    if (master >= 0) break;
	}
	if (master < 0) {
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Out of pty's\r\n",FALSE);
		dorefresh();
	    }
	    return;
	}
	proc->pro_master = master;
	line[strlen("/dev/")] = 't';
	proc->pro_slave = slave = open(line,2);
	if (slave < 0) {
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Can't open slave\r\n",FALSE);
		dorefresh();
	    }
	    return;
	}
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

	proc->pro_id = child = fork();
	if (child < 0) {
	    free((char *)proc);
	    prolist[p] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Can't get process\r\n",FALSE);
		dorefresh();
	    }
	    return;
	}
	if (child == 0) {
	    int t;
	    t = open("/dev/tty",2);
	    if (t >= 0) {
		ioctl(t, TIOCNOTTY, (char *)0);
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
	    execle("/bin/sh","sh","-i",0,nenvp);
	    printf("Can't run sh either\r\n");
	    exit(1);
	}
	proc->pro_cur_win_num = w;
	proc->pro_base_win_num = w;
	proc->pro_cur_win = winlist[w];
	proc->pro_ibuf_pointer = proc->pro_ibuf;
	proc->pro_ibuf_counter = 0;
	proc->change_sg_flags = FALSE;
	proc->change_to_cooked = FALSE;
	proc->sg_flags = ttychar.tty_b.sg_flags;
	proc->pro_sts = 0;
	proc->pro_do = doaddch;
	if (sub_init_str) {
	    sendstringtoprocess(proc,sub_init_str);
	    if (sub_init_lf) sendtoprocess(proc,'\n');
	}
	else if (!new_envp && is_cshell)
	    sendstringtoprocess(proc,"setenv TERM $wterm\n");
	new_mask = TRUE;
	if (statusmode) {
	    wmaddstr(statuswin,"Start process ",FALSE);
	    wmaddnum(statuswin,"%d",p,FALSE);
	    wmaddstr(statuswin," in window ",FALSE);
	    wmaddnum(statuswin,"%d\r\n",w,TRUE);
	}
}

finish()
{
	union wait status;
	int pid,p;

	(void) signal(SIGCHLD, SIG_IGN);
	pid = wait3(&status, WNOHANG | WUNTRACED, 0);
	for (p = 0; p < MAX_PROCESSES; p++) {
	    if (prolist[p]->pro_id == pid) break;
	}
	if (p >= MAX_PROCESSES) {
	    if (statusmode)
		wmaddstr(statuswin,"Strange Signal Received\r\n",FALSE);
		dorefresh();
	}
	else if (WIFEXITED(status)) killedprocess(p);
	else if (WIFSTOPPED(status)) {
		prolist[p]->pro_sts = 1;
	}
	(void) signal(SIGCHLD, finish);
}

killedprocess(p)
int p;
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
	    wmaddstr(statuswin,"Process ",FALSE);
	    wmaddnum(statuswin,"%d",kp,FALSE);
	    wmaddstr(statuswin," killed\r\n",FALSE);
	    dorefresh();
	}
	new_mask = TRUE;
}

done()
{
	union wait status;
	int p,i;

	(void) signal(SIGCHLD, SIG_IGN);
	for (p = 0; p < MAX_PROCESSES; p++)
	    if (prolist[p]) {
		kill(prolist[p]->pro_id,SIGKILL);
	    }

	if (KEstr) tputs(KEstr,0,dumpchar);
	wmend();
	fflush(stdout);
#ifdef NOFLOWCTL
	ioctl(0, TIOCSETC, &(ttychar.tty_tc));
#endif
	ioctl(0, TIOCSETP, (char *)&(ttychar.tty_b));
	if (is_shell) printf("\nWindow Manager end\n");
	else printf("\n%s end\n",shell);
	(void) signal(SIGCHLD, SIG_IGN);
	exit(0);
}

stopprocess(p)
{
	if (p < 0 || p >= MAX_PROCESSES || prolist[p] == NULL) return;
	if (prolist[p]->pro_sts == 0) kill(prolist[p]->pro_id,SIGSTOP);
}

process()
{
	int nfound,maxfd,readfds,writefds,execptfds;
	int p,bp[MAX_PROCESSES],master,stop;
	int pn[MAX_PROCESSES+1],q;
	struct pro_str *proc;
	struct timeval timeout;
	struct sgttyb sb;

	input_freq = INPUT_FREQ;
	new_mask = TRUE;
	for(;;) {
	    maxfd = readfds = writefds = execptfds = 0;
	    stop = FALSE;
	    if (new_mask) {
		q = 0;
		for (p = 0; p < MAX_PROCESSES; p++)
		    if (prolist[p]) pn[q++] = p;
		pn[q] = -1;
		new_mask = FALSE;
	    }
	    for (q = 0; pn[q] >= 0; q++) {
		p = pn[q];
		if ((proc = prolist[p]) == NULL) continue;
		master = proc->pro_master;
		if (master > maxfd) maxfd = master;
		readfds |= bp[p] = 1<<(proc->pro_master);
		if (proc->pro_ibuf_counter != 0) writefds |= bp[p];
		stop = stop || proc->change_sg_flags;
	    }
	    readfds |= 1<<0;
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
	    nfound = select(maxfd+1,&readfds,&writefds,&execptfds,&timeout);
	    if (readfds & (1<<0)) processinput();
	    for (p = 0; p < MAX_PROCESSES; p++) {
		if ((proc = prolist[p]) == NULL) continue;
		master = proc->pro_master;
		if (readfds & bp[p]) processoutput(p);
		else if (proc->change_sg_flags && proc->pro_sts == 1) {
		    sb = ttychar.tty_b;
		    sb.sg_flags = proc->sg_flags;
		    ioctl(proc->pro_slave,TIOCSETP,&sb);
		    kill(proc->pro_id,SIGCONT);
		    proc->pro_sts = 0;
		    proc->change_sg_flags = FALSE;
		    proc->change_to_cooked = FALSE;
		}
		if (writefds & bp[p]) {
		    write(master,proc->pro_ibuf,proc->pro_ibuf_counter);
		    proc->pro_ibuf_pointer = proc->pro_ibuf;
		    proc->pro_ibuf_counter = 0;
		}
	    }
	}
}

processoutput(p)
int p;
{
	int cc;
	char *cp;
	struct pro_str *proc;
	int nfound,readfds,writefds,execptfds;
	struct timeval timeout;

	proc = prolist[p];
	cc = read(proc->pro_master,o_buf,sizeof(o_buf));
	cp = o_buf;
	while (cc--) {
	    (proc->pro_do)(*cp++ & 0177,p,proc);
	    if (--input_freq <= 0) {
		input_freq = INPUT_FREQ;
		readfds = 1<<0;
		writefds = execptfds = 0;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;
		nfound = select(0+1,&readfds,&writefds,&execptfds,&timeout);
		if (readfds & (1<<0)) processinput();
	    }
	}
}

processinput()
{
	int cc,f,stop;
	char *cp,*p,ch;
	struct pro_str *proc;

	input_freq = INPUT_FREQ;
	proc = current_process;
	if (!proc) proc = prolist[0];

	stop = FALSE;
	cc = read(0,i_buf,sizeof(i_buf));
	for(cp = i_buf; cp < i_buf+cc; cp++) {
	    ch = *cp & 0177;
	    if (ch == kill_character) done();
	    if (stoppable && ch == stopch) {
		stop = TRUE;
		continue;
	    }
	    if (mouse_input_number && '0' <= ch && ch <= '9') {
		if (mouse_input_number == 1)
		    mouse_input_window = mouse_input_window * 10 + (ch - '0');
		else
		    mouse_input_page = mouse_input_page * 10 + (ch - '0');
		wmaddch(mousewin,ch,FALSE);
		continue;
	    }
	    if (mouse_input_number && (ch == '\n' || ch == '\r')) {
		mouse_input_number = 2;
		wmcupos(mousewin,1,4,FALSE);
		continue;
	    }
	    fn_key_partial_match = FALSE;
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
		    (*mouse_table[f].do_mouse)();
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
#ifdef NOFLOWCTL
	    ioctl(0, TIOCSETC, &(ttychar.tty_tc));
#endif
	    ioctl(0, TIOCSETP, (char *)&(ttychar.tty_b));
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

funkeycheck(fk)
char *fk;
{
	register char *p;

	if (!*fk) return FALSE;

	for (p = fn_key_buf; p < fn_key_buf_pointer && *fk;)
		if (*fk++ != *p++) return FALSE;
	if (*fk) {
	    fn_key_partial_match = TRUE;
	    return FALSE;
	}
	return TRUE;
}

sendtoprocess(proc,ch)
struct pro_str *proc;
char ch;
{
	if (proc->pro_ibuf_counter >= BUFSIZ) return;
	*proc->pro_ibuf_pointer++ = ch;
	proc->pro_ibuf_counter++;
}

sendstringtoprocess(proc,str)
struct pro_str *proc;
char *str;
{
	while(*str) sendtoprocess(proc,*str++);
}

doaddch(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
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
		return;
	    }
	    if (ch == '\n') {
		if (proc->change_to_cooked) wmaddch(current,'\r',FALSE);
		if (!dumpmode &&
		    current->cur_x == current->max_x-1 &&
		    (current->flags & DM_PAGE))
			dorefresh();
		wmaddch(current,'\n',FALSE);
		if (!dumpmode && (current->flags & DM_REFRESH)) dorefresh();
	    }
	    else wmaddch(current,ch,FALSE);
	}
}

nowindow(p)
int p;
{
	if (statuswin) {
	    wmaddstr(statuswin,"No window to process ",FALSE);
	    wmaddnum(statuswin,"%d\r\n",p,FALSE);
	    dorefresh();
	}
}

dorefresh()
{
	if (mousemode) wmrefresh0(mouseY,mouseX);
	else if (current_process && current_process->pro_cur_win)
		wmrefresh(current_process->pro_cur_win);
	else wmrefresh(winlist[0]);
}

doesc(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_top = 0;
	dopushzero(ch,p,proc);
	if ((*vi200table[ch])(ch,p,proc)) proc->pro_do = doaddch;
}

dopushzero(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
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

dodigit(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_SET;
	proc->pro_stk[proc->pro_stk_top] =
	    proc->pro_stk[proc->pro_stk_top]*10+(ch-'0');
	return CONT;
}

dosetplus(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_PLUS;
	return CONT;
}

dosetminus(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_MINUS;
	return CONT;
}

doseton(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_ON;
	return CONT;
}

dosetoff(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_stk_flg[proc->pro_stk_top] |= STK_OFF;
	return CONT;
}

fin(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if (statusmode) {
	    wmaddstr(statuswin,"Illegal esc seq from process ",FALSE);
	    wmaddnum(statuswin,"%d\r\n",p,FALSE);
	    dorefresh();
	}
	proc->pro_do = doaddch;
	return END;
}

doansiesc(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if ((*ansitable[ch])(ch,p,proc)) proc->pro_do = doaddch;
}

doansi(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = doansiesc;
	return CONT;
}

docursor(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int x,y;
	struct win_str *current;

	current = proc->pro_cur_win;
	if (proc->pro_stk_top < 2) wmcupos(current,0,0,FALSE);
	else {
		y = evalarg(proc,1,1,current->cur_y+1,1,current->max_y);
		x = evalarg(proc,2,1,current->cur_x+1,1,current->max_x);
		wmcupos(current,y-1,x-1,FALSE);
	}
	return END;
}

evalarg(proc,n,d,p,min,max)
struct pro_str *proc;
int n,d,p,min,max;
{
	int flg,x;

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

docursorv2(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if (ch < ' ') fin();
	wmcupos(proc->pro_cur_win,proc->pro_stk[1],ch-' ',FALSE);
	proc->pro_do = doaddch;
}

docursorv1(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = docursorv2;
	if (ch < ' ') fin();
	else proc->pro_stk[1] = ch-' ';
}

docursorv(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = docursorv1;
	return CONT;
}

doeeol(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmcleol(proc->pro_cur_win,FALSE);
	return END;
}

doeeos(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int arg = proc->pro_stk[1];

	switch (arg) {
		case 3: wmclp(proc->pro_cur_win);
		case 0: wmcleos(proc->pro_cur_win,FALSE);
			break;
		case 4: wmscroll(proc->pro_cur_win,
			    proc->pro_cur_win->pre_line->line_stack_length,
			    FALSE);
			wmclp(proc->pro_cur_win);
		case 2: wmcls(proc->pro_cur_win,FALSE);
		default: break;
	}
	return END;
}

doeeosv(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmcleos(proc->pro_cur_win,FALSE);
	return END;
}

doup(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmvect(proc->pro_cur_win,-1,0,FALSE);
	return END;
}

dodown(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmvect(proc->pro_cur_win,1,0,FALSE);
	return END;
}

doright(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmvect(proc->pro_cur_win,0,1,FALSE);
	return END;
}

doleft(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmvect(proc->pro_cur_win,0,-1,FALSE);
	return END;
}

dohome(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmcupos(proc->pro_cur_win,0,0,FALSE);
	return END;
}

docld(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmcls(proc->pro_cur_win,FALSE);
	return END;
}

dolinsert(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wminsline(proc->pro_cur_win,proc->pro_cur_win->cur_y,FALSE);
	return END;
}

doldelete(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmdelline(proc->pro_cur_win,proc->pro_cur_win->cur_y,FALSE);
	return END;
}

dosetins(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_cur_win->flags |= WM_INSERT;
	return END;
}

doresetins(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_cur_win->flags &= ~WM_INSERT;
	return END;
}

dodelchar(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	wmdelchar(proc->pro_cur_win,1,FALSE);
	return END;
}

dodelchars(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int cc;

	cc = evalarg(proc,1,1,0,1,proc->pro_cur_win->max_x);
	wmdelchar(proc->pro_cur_win,cc,FALSE);
	return END;
}

doredraw(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if (mousemode) wmredraw0(mouseY,mouseX);
	else if (current_process && current_process->pro_cur_win)
		wmredraw(current_process->pro_cur_win);
	else wmredraw(winlist[0]);
	return END;
}

doreverselinefeed(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	struct win_str *current;

	current = proc->pro_cur_win;
	if (current->scroll_start < current->cur_y) {
	    --current->cur_y;
	}
	else if (current->flags & WM_SCROLL) {
	    wmscroll(current,-1,FALSE);
	    current->cur_y = current->scroll_start;
	}
	else if (current->flags & DM_PAGE) {
	    wmscroll(current,current->scroll_start-current->max_y,FALSE);
	    current->cur_y = current->max_y-1;
	}
	return END;
}

dosetscroll(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_cur_win->scroll_start = proc->pro_cur_win->cur_y;
	return END;
}

dostandout(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if (proc->pro_stk[1]) proc->pro_cur_win->flags |= WM_STANDOUT;
	else proc->pro_cur_win->flags &= ~WM_STANDOUT;
	return END;
}

dospecialesc(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if ((*specialtable[ch])(ch,p,proc)) proc->pro_do = doaddch;
}

dospecial(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = dospecialesc;
	return CONT;
}

dosetmode(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw,flg,dfl;
	struct win_str *win;

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

dosetecho(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
    int old_sg_flags;

	old_sg_flags = proc->sg_flags;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		proc->sg_flags |= ECHO;
	else proc->sg_flags &= ~ECHO;
	if (old_sg_flags == proc->sg_flags) return END;
	if (!proc->change_sg_flags) {
	    proc->change_sg_flags = TRUE;
	    stopprocess(p);
	}
	return END;
}

dosetraw(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
    int old_sg_flags;

	old_sg_flags = proc->sg_flags;
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON))) {
	    proc->sg_flags |= RAW;
	    proc->change_to_cooked = FALSE;
	}
	else {
	    if (proc->sg_flags & RAW) proc->change_to_cooked = TRUE;
	    proc->sg_flags &= ~RAW;
	}
	if (proc->sg_flags == old_sg_flags) return END;
	if (!proc->change_sg_flags) {
	    proc->change_sg_flags = TRUE;
	    stopprocess(p);
	}
	return END;
}

dosetdump(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		dumpmode = TRUE;
	else dumpmode = FALSE;
	dorefresh();
	return END;
}

dosetkill(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	kill_character = evalarg(proc,1,0,kill_character,0,127);
	return END;
}

dosetstop(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	stopch = evalarg(proc,1,0,stopch,0,127);
	return END;
}

dosetmouse(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int new_flag;

	new_flag = evalarg(proc,1,01777,mouse_flag,-INF,INF);
	if (new_flag != mouse_flag) {
	    if (!(new_flag & 1) && mousemode) {
		domouseend();
		dorefresh();
	    }
	    mouse_flag = new_flag;
	    if (mousemode) wmdel(mousewin,FALSE);
	    if (mousewin) wmfree(mousewin);
	    mousewin = NULL;
	    if (mousemode) {
		createmouse();
		wmputwin(mousewin,top_win,TRUE);
		mouse_win_number = -1;
		mouse_page_number = -1;
		mouse_process_number = -1;
	    }
	}
	return END;
}

dostring(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
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

docopys(p,proc)
int p;
struct pro_str *proc;
{
	char *copy_mode;
	FILE *copy_file;

	if (proc->pro_stk[1] || (proc->pro_stk_flg[1] & (STK_PLUS | STK_ON)))
		copy_mode = "w";
	else copy_mode = "a";
	copy_file = fopen(proc->pro_sbuf,copy_mode);
	if (copy_file) {
		if ((long) ftell(copy_file)) {
			fputc(12,copy_file);
		}
		wmcopy(copy_file,TRUE);
		fclose(copy_file);
	}
	return END;
}

docopy(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = dostring;
	proc->pro_sdo = docopys;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

dosetfnkeys(p,proc)
int p;
struct pro_str *proc;
{
	int f,m;

	f = evalarg(proc,1,0,0,0,10+4+1-1);
	m = evalarg(proc,2,0,0,-INF,INF);
	fn_table[f].fn_mode = m;
	free((char *)fn_table[f].fn_tra);
	fn_table[f].fn_tra =
	    (char *) calloc(strlen(proc->pro_sbuf)+1, sizeof(char));
	strcpy(fn_table[f].fn_tra,proc->pro_sbuf);
	return END;
}

dosetfnkey(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = dostring;
	proc->pro_sdo = dosetfnkeys;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

dopsdins(p,proc)
int p;
struct pro_str *proc;
{
	int pp;

	pp = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (prolist[pp] == NULL) return END;
	sendstringtoprocess(prolist[pp],proc->pro_sbuf);
	return END;
}

dopsdin(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = dostring;
	proc->pro_sdo = dopsdins;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

dosetbase(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int pn,wn;

	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	wn = evalarg(proc,2,proc->pro_base_win_num,proc->pro_base_win_num,
			0,MAX_WINDOWS-1);
	if (prolist[pn] == NULL || winlist[wn] == NULL) return END;
	prolist[pn]->pro_base_win_num = wn;
	return END;
}

dogivekeyboard(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int pn,wn;

	if (current_process_number != p) return END;
	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (prolist[pn] == NULL) return END;
	current_process_number = pn;
	current_process = prolist[pn];
	return END;
}

dokillprocess(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int pn;

	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	if (pn == 0 || prolist[pn] == NULL) return END;
	kill(prolist[pn]->pro_id,SIGKILL);
	return END;
}

dostartprocess(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int pn,w,bw;

	pn = evalarg(proc,1,p,p,0,MAX_PROCESSES-1);
	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,2,w,w,0,MAX_WINDOWS-1-bw)+bw;

	if (prolist[pn] || winlist[w] == NULL) return END;
	startprocess(pn,w);
	return END;
}

doprlesc(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	if ((*prltable[ch])(ch,p,proc)) proc->pro_do = doaddch;
}

doprl(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = doprlesc;
	return CONT;
}

docreate(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,x1,x2,y1,y2,pages,bw;

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
	    wmdel(winlist[w],FALSE);
	    wmfree(winlist[w]);
	}
	proc->pro_cur_win = winlist[w] =
		wmmake(y2-y1+1,x2-x1+1,y1,x1,pages,FALSE);
	proc->pro_cur_win_num = w;
	if (statusmode) {
	    wmaddstr(statuswin,"Process ",FALSE);
	    wmaddnum(statuswin,"%d",p,FALSE);
	    wmaddstr(statuswin," create ",FALSE);
	    dowinfo(w);
	}
	return END;
}

dowinfo(w)
{
	struct win_str *win;

	if (!statusmode || w < 0 || w >= MAX_WINDOWS) return;
	if (win = winlist[w]) {
	    wmaddnum(statuswin,"window %d ",w,FALSE);
	    wmaddnum(statuswin,"at (%d,",win->beg_x,FALSE);
	    wmaddnum(statuswin,"%d) ",win->beg_y,FALSE);
	    wmaddnum(statuswin,"c=%d ",win->max_x,FALSE);
	    wmaddnum(statuswin,"l=%d\r\n",win->max_y,TRUE);
	}
}

dodestroy(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;

	if (w == 0 || winlist[w] == NULL) return END;
	wmdel(winlist[w],FALSE);
	wmfree(winlist[w]);
	winlist[w] = NULL;
	if (statusmode) {
	    wmaddnum(statuswin,"Process %d ",p,FALSE);
	    wmaddstr(statuswin,"delete window ",FALSE);
	    wmaddnum(statuswin,"%d\r\n",w,TRUE);
	}

	for(p = 0; p < MAX_PROCESSES; p++) {
	   proc = prolist[p];
	   if (proc->pro_cur_win_num == w) {
		if (winlist[proc->pro_base_win_num])
			proc->pro_cur_win_num = proc->pro_base_win_num;
		else proc->pro_cur_win_num = 0;
		if (statusmode) {
		    wmaddnum(statuswin,"Process %d",p,FALSE);
		    wmaddstr(statuswin,"'s current window is forced to",FALSE);
		    wmaddnum(statuswin," %d\r\n",proc->pro_cur_win_num,TRUE);
		}
		proc->pro_cur_win = winlist[proc->pro_cur_win_num];
	    }
	}
	return END;
}

doenquire(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int bw,w,f;
	struct win_str *win;
	char buf[100];

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if (proc->pro_stk_top < 2) f = FALSE;
	else f = evalarg(proc,2,1,0,-INF,INF);

	if (win = winlist[w]) {
	    if (f) {
		sprintf(buf,"\033{%d;%d;%d;%d;%d;%d;%d;%d;%d;%de",
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
		sprintf(buf,"\033{%d;%d;%d;%d;%d;%d;%de",
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

doselect(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,0,w,0,MAX_WINDOWS-1-bw)+bw;

	if (winlist[w]) {
	    proc->pro_cur_win = winlist[w];
	    proc->pro_cur_win_num = w;
	}
	return END;
}

dopopwindow(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int bw,w,w2;
	struct win_str *win,*win2;

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

	wmrepos(win,win2,win->beg_y,win->beg_x,FALSE);

	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

doreposwindow(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw,x,y;
	struct win_str *win;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((win = winlist[w]) == NULL) return END;
	x = evalarg(proc,2,win->beg_x,win->beg_x,-INF,INF);
	x = evalarg(proc,3,win->beg_y,win->beg_y,-INF,INF);
	wmrepos(win,win->next_win,y,x,FALSE);
	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

dopageselect(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw,l;
	struct win_str *wp;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	if (proc->pro_stk_top < 2) l = wp->next_line->line_stack_length;
	else l = proc->pro_stk[2] * (wp->max_y - wp->scroll_start) -
			wp->pre_line->line_stack_length;
	wmscroll(wp,l,FALSE);
	return END;
}

dopageforward(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw,page;
	struct win_str *wp;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	page = evalarg(proc,2,1,0,0,INF);
	wmscroll(wp,page*(wp->max_y-wp->scroll_start),FALSE);
	return END;
}

dopagebackward(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int w,bw,page;
	struct win_str *wp;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((wp = winlist[w]) == NULL) return END;
	page = evalarg(proc,2,1,0,0,INF);
	wmscroll(wp,-page*(wp->max_y-wp->scroll_start),FALSE);
	return END;
}

doscroll(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	int bw,w,n;
	struct win_str *win;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if ((win = winlist[w]) == NULL) return END;
	if (proc->pro_stk_top < 2) {
	    wmscroll(win,win->next_line->line_stack_length,FALSE);
	}
	else if (!(proc->pro_stk_flg[2] & STK_SET)) {
	    wmscroll(win,-win->pre_line->line_stack_length,FALSE);
	}
	else if (proc->pro_stk_top < 3) {
	    n = evalarg(proc,2,0,0,-INF,INF);
	    wmscroll(win,n,FALSE);
	}
	else {
	    n = evalarg(proc,2,0,0,-win->pre_line->line_stack_length,
	    			   win->next_line->line_stack_length);
	    wmscroll(win,n,FALSE);
	}

	proc->pro_cur_win_num = w;
	proc->pro_cur_win = win;
	return END;
}

doheadings(p,proc)
int p;
struct pro_str *proc;
{
	int w,bw;

	bw = proc->pro_base_win_num;
	w = proc->pro_cur_win_num-bw;
	w = evalarg(proc,1,w,w,0,MAX_WINDOWS-1-bw)+bw;
	if (winlist[w] == NULL) return END;
	wmheadstr(winlist[w],proc->pro_sbuf,FALSE);
	return END;
}

doheading(ch,p,proc)
char ch;
int p;
struct pro_str *proc;
{
	proc->pro_do = dostring;
	proc->pro_sdo = doheadings;
	proc->pro_sbuf_pointer = proc->pro_sbuf;
	proc->pro_mch = 0;
	return CONT;
}

/* mouse support routines */

domouseend()
{
	if (mousemode) {
	    wmdel(mousewin,FALSE);
	    wmdel(mousemark,FALSE);
	    mousemark_set = FALSE;
	    mouse_input_number = 0;
	    mousemode = 0;
	}
}

domouse()
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
	wmputwin(mousewin,top_win,TRUE);

	if (!mousemark) {
	    mousemark = wmcreate(1,1,0,0);
	    mousemark->flags |= WM_STANDOUT;
	    wmaddstr(mousemark,"*");
	}
}

createmouse()
{
	int mwlen,f;
	char tbuf[16];

	mwlen = 3;
	for (f = 0; f < 10; f++) {
	    if ((mouse_flag>>f)&1 && mouse_table[f].mouse_msg && Kstr[f])
		mwlen++;
	}
	mousewin = wmcreate(mwlen,10,1,0);
	mousewin->beg_y = 1;
	mousewin->beg_x = ScreenWidth-11;
	wmcupos(mousewin,0,0,FALSE);
	wmheadstr(mousewin,"Mouse Mode",FALSE);
	wmaddstr(mousewin,"w = ",FALSE);
	wmaddstr(mousewin,"\r\np = ",FALSE);
	wmaddstr(mousewin,"\r\ncp= ",FALSE);
	for (f = 0; f < 10; f++) {
	    if ((mouse_flag>>f)&1 && mouse_table[f].mouse_msg && Kstr[f]) {
		sprintf(tbuf,"\r\nF%d %s",f,mouse_table[f].mouse_msg);
		wmaddstr(mousewin,tbuf,FALSE);
	    }
	}
}

wmsetwnum() /* display current window number */
{
	int w,cp,pl;
	struct win_str *wp;

	if (mousemode != 1) return;

	if (mouse_input_number == 0) {
	    wp = wmgetmap(mouseY,mouseX);
	    w = dosearchwin(wp);
	    if (w < 0) w = -2;
	    if (w != mouse_win_number) {
		wmcupos(mousewin,0,4,FALSE);
		if (w >= 0) wmaddnum(mousewin,"%d",w,FALSE);
		else wmaddstr(mousewin,"????",FALSE);
		wmcleol(mousewin,FALSE);
	    }
	    mouse_win_number = w;
	    if (wp) {
		pl = wp->max_y - wp->scroll_start;
		cp = (wp->pre_line->line_stack_length + pl - 1)/pl;
		if (cp != mouse_page_number) {
	 	    wmcupos(mousewin,1,4,FALSE);
		    wmaddnum(mousewin,"%d",cp,FALSE);
		    wmcleol(mousewin,FALSE);
		    mouse_page_number = cp;
		}
	    }
	    else if (mouse_page_number != -2) {
	 	    wmcupos(mousewin,1,4,FALSE);
		    wmaddstr(mousewin,"????",FALSE);
		    wmcleol(mousewin,FALSE);
		    mouse_page_number = -2;
	    }
	}
	if (current_process_number != mouse_process_number) {
	    wmcupos(mousewin,2,4,FALSE);
	    wmaddnum(mousewin,"%d",current_process_number,FALSE);
	    wmcleol(mousewin,FALSE);
	    mouse_process_number = current_process_number;
	}
}

domousehome()
{
	mouse_increment *= 4;
}

domouseup()
{
	mouseY -= mouse_increment;
	mouse_increment = 1;
	if (mouseY < 0) mouseY = 0;
}

domousedown()
{
	mouseY += mouse_increment;
	mouse_increment = 1;
	if (mouseY >= ScreenLength) mouseY = ScreenLength-1;
}

domouseright()
{
	mouseX += mouse_increment;
	mouse_increment = 1;
	if (mouseX >= ScreenWidth) mouseX = ScreenWidth-1;
}

domouseleft()
{
	mouseX -= mouse_increment;
	mouse_increment = 1;
	if (mouseX < 0) mouseX = 0;
}

dosetmark()
{
	mousemark->beg_x = mouseX;
	mousemark->beg_y = mouseY;
	wmputwin(mousemark,top_win,FALSE);
	mousemark_set = TRUE;
}

doresetinput()
{
	if (!mouse_input_number) return;
	mouse_input_number = 0;
	mouse_win_number = -1;
	mouse_page_number = -1;
	mouse_process_number = -1;
}

domousecreate()
{
	int lines,cols,begin_x,begin_y,w,p;
	struct win_str *win;

	mouse_increment = 1;
	if (!mousemark_set) {
		dosetmark();
		wmcupos(mousewin,1,4,FALSE);
		wmcleol(mousewin,FALSE);
		wmcupos(mousewin,0,4,FALSE);
		wmcleol(mousewin,FALSE);
		mouse_input_number = 1;
		mouse_input_window = 0;
		mouse_input_page = 0;
		return;
	}
	w = mouse_input_window;
	p = mouse_input_page;
	wmdel(mousemark,FALSE);
	mousemark_set = FALSE;
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
		wmdel(winlist[w],FALSE);
		wmfree(winlist[w]);
	}
	winlist[w] = wmmake(lines,cols,begin_y,begin_x,p,FALSE);
	if (statusmode) {
	    wmaddstr(statuswin,"Create ",FALSE);
	    dowinfo(w);
	}
}

dosearchwin(win)
struct win_str *win;
{
	register int w;

	if (win == mousewin || win == mousemark) return -1;
	for (w = 0; w < MAX_WINDOWS; w++)
		if (win == winlist[w]) return w;
	return -1;
}

domousedelete()
{
	int w,p;
	struct win_str *win;
	struct pro_str *proc;

	mouse_increment = 1;
	doresetinput();

	win = wmgetmap(mouseY,mouseX);
	if (win == mousewin || win == mousemark || win == statuswin) return;
	w = dosearchwin(win);
	if (w == 0) return;
	wmdel(win,FALSE);
	wmfree(win);
	if (w > 0) {
	    winlist[w] = NULL;
	    if (statusmode) {
		wmaddstr(statuswin,"Delete window ",FALSE);
		wmaddnum(statuswin,"%d\r\n",w,TRUE);
	    }
	    for(p = 0; p < MAX_PROCESSES; p++) {
		proc = prolist[p];
		if (proc->pro_cur_win_num == w) {
		    if (winlist[proc->pro_base_win_num])
			proc->pro_cur_win_num = proc->pro_base_win_num;
		    else proc->pro_cur_win_num = 0;
		    if (statusmode) {
			wmaddnum(statuswin,"Process %d",p,FALSE);
			wmaddstr(statuswin,"'s current window is forced to",
					FALSE);
			wmaddnum(statuswin," %d\r\n",proc->pro_cur_win_num,
					TRUE);
		    }
		    proc->pro_cur_win = winlist[proc->pro_cur_win_num];
	        }
	    }
	}
}

domousemove()
{
	struct win_str *win;
	int x,y;

	mouse_increment = 1;
	doresetinput();

	if (!mousemark_set) {
		dosetmark();
		return;
	}
	wmdel(mousemark,FALSE);
	mousemark_set = FALSE;

	win = wmgetmap(mousemark->beg_y,mousemark->beg_x);
	if (win != winlist[0]) {
		x = win->beg_x + (mouseX - mousemark->beg_x);
		y = win->beg_y + (mouseY - mousemark->beg_y);
	}
	else { x = win->beg_x; y = win->beg_y;}

	wmrepos(win,top_win,y,x,FALSE);
}

domousecopy()
{
	FILE *copy_file;

	copy_file = fopen(mouse_copy_file,"a");
	if (copy_file) {
		if ((long) ftell(copy_file)) {
			fputc(12,copy_file);
		}
		wmdel(mousewin,FALSE);
		if (mousemark_set) {
			wmdel(mousemark,FALSE);
			mousemark_set = FALSE;
			doresetinput();
		}
		wmdel(mousemark,FALSE);
		wmcopy(copy_file,TRUE);
		fclose(copy_file);
		wmputwin(mousewin,top_win,TRUE);
	}
}

domousenext()
{
	struct win_str *wp;
	int nls,ls;

	wp = wmgetmap(mouseY,mouseX);
	if (!wp) return;
	if ((nls = wp->next_line->line_stack_length) == 0) return;
	ls = (wp->max_y-wp->scroll_start) * mouse_increment;
	if (ls > nls) ls = nls;
	wmscroll(wp,ls);
	mouse_increment = 1;
}

domouseback()
{
	struct win_str *wp;
	int ls,pls;

	wp = wmgetmap(mouseY,mouseX);
	if (!wp) return;
	if ((pls = wp->pre_line->line_stack_length) == 0) return;
	ls = (wp->max_y-wp->scroll_start) * mouse_increment;
	if (ls > pls) ls = pls;
	wmscroll(wp,-ls);
	mouse_increment = 1;
}

domousepush()
{
	struct win_str *win;
	int x,y;

	mouse_increment = 1;
	doresetinput();

	win = wmgetmap(mouseY,mouseX);
	if (!win) return;
	if (win == winlist[0]) wmrepos(win,NULL,win->beg_y,win->beg_x,FALSE);
	else wmrepos(win,winlist[0],win->beg_y,win->beg_x,FALSE);
}

domouseprocessselect()
{
	int p,w;
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

domouseprocessnext()
{
	int p;

	for (p = current_process_number+1; p < MAX_PROCESSES; p++)
	    if (prolist[p]) break;
	if (p >= MAX_PROCESSES) p = 0;
	current_process_number = p;
	current_process = prolist[p];
}

initmouse()
{
	int f;

	for (f = 0; f < 10+4+1; f++) {
	    mouse_table[f].mouse_msg = NULL;
	}
	mouse_table[1].do_mouse = domousecreate;
	mouse_table[1].mouse_msg = "Create";
	mouse_table[2].do_mouse = domousedelete;
	mouse_table[2].mouse_msg = "Delete";
	mouse_table[3].do_mouse = domousemove;
	mouse_table[3].mouse_msg = "Move";
	mouse_table[4].do_mouse = domouseback;
	mouse_table[4].mouse_msg = "Back";
	mouse_table[5].do_mouse = domousenext;
	mouse_table[5].mouse_msg = "Next";
	mouse_table[6].do_mouse = domousepush;
	mouse_table[6].mouse_msg = "Push";
	mouse_table[7].do_mouse = domouseprocessselect;
	mouse_table[7].mouse_msg = "PSelect";
	mouse_table[8].do_mouse = domouseprocessnext;
	mouse_table[8].mouse_msg = "PNext";
	mouse_table[9].do_mouse = domousecopy;
	mouse_table[9].mouse_msg = "Copy";
	mouse_table[10].do_mouse = domouseup;
	mouse_table[11].do_mouse = domousedown;
	mouse_table[12].do_mouse = domouseright;
	mouse_table[13].do_mouse = domouseleft;
	mouse_table[14].do_mouse = domousehome;
	mouse_flag = 01777;
}

initeachtbl(tbl)
funcptr *tbl;
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

inittbl()
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

