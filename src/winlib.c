/*
 * window library
 *	By Tatsuya Hagino (28,May,1984)
 */

#include <stdio.h>
#include <sgtty.h>
#include "display.h"
#include "winlib.h"
#include <sys/types.h>
#include <sys/stat.h>

#define min(a,b) (a<b ? a : b)
#define max(a,b) (a>b ? a : b)
#define NULL 0

short *calloc();

struct win_str *top_win;

static struct win_str ***win_map;

static short *win_change;

struct sgttyb old;

static struct line_str *free_line_frame;

static struct stat stb;
static char *tty;

wminit() /* initialization */
{
	struct win_str *wmmake();
	int y,x,f;
	static int been_here = 0;

	tty=(char *)ttyname(fileno(stdout));
	if (tty == 0) {
	   fprintf(stderr,"You are not at a terminal.\r\n");
	   exit(1);
	}
	gtty(fileno(stdout),&old);
	f = old.sg_flags;
	old.sg_flags |= RAW;
	old.sg_flags &= ~ECHO;
	stty(fileno(stdout),&old);
	old.sg_flags = f;
	stat(tty,&stb);
	chmod(tty, stb.st_mode&~0122);

	if (been_here) {
	    (*tt.t_reset)();
	    return;
	}
	been_here = 1;
	term_init();
	ScreenGarbaged = 1;
	top_win = NULL;
	free_line_frame = NULL;

	win_map = (struct win_str ***)
			calloc(ScreenLength,sizeof(struct win_str **));
	win_change = (short *) calloc(ScreenLength,sizeof(short));

	for (y = 0; y < ScreenLength; y++) {
		win_map[y] = (struct win_str **)
				calloc(ScreenWidth,sizeof(struct win_str *));
		for (x = 0; x < ScreenWidth; x++)
			win_map[y][x] = NULL;
		win_change[y] = TRUE;
	}
}

static char *wmnewline(cols)
int cols;
{
	char *p,*q;

	p = q = (char *) calloc(cols+2, sizeof(char));

	*q++ = '|';
	while (cols--) *q++ = ' ';
	*q = '|';

	return p;
}

wmnewscreen(win)
struct win_str *win;
{
	int y;
	char *p,*q;

	win->scr = (char **) calloc(win->max_y+2, sizeof(char *));
	for (y = 0; y<win->max_y+2; y++) {
		win->scr[y] = wmnewline(win->max_x);
	}
	
	p = win->scr[0];
	q = win->scr[win->max_y+1];
	*p++ = ',';
	*q++ = '`';
	while (p <= win->scr[0]+win->max_x) *p++ = *q++ = '-';
	*p = ',';
	*q = '\'';
}

static struct line_str *wmlsget(l)
char *l;
{
	struct line_str *lsp;

	if (free_line_frame) {
	    lsp = free_line_frame;
	    free_line_frame = free_line_frame->line_next;
	}
	else {
	    lsp = (struct line_str *) calloc(1, sizeof(struct line_str));
	}
	lsp->line_pre = NULL;
	lsp->line_next = NULL;
	lsp->line_string = l;

	return lsp;
}

static wmlsfree(lp)
struct line_str *lp;
{
	lp->line_next = free_line_frame;
	free_line_frame = lp;
}

static struct line_stack *wmlsempty()
{
	struct line_stack *ls;

	ls = (struct line_stack *) calloc(1, sizeof(struct line_stack));
	ls->line_top = wmlsget(NULL);
	ls->line_top->line_next = ls->line_top;
	ls->line_top->line_pre = ls->line_top;
	ls->line_stack_length = 0;

	return ls;
}

static wmlsallfree(ls)
struct line_stack *ls;
{
	char *l;
	struct line_str *lp,*lpn;

	lp = ls->line_top;
	lp->line_pre->line_next = NULL;
	while (lp) {
	    lpn = lp->line_next;
	    free(lp->line_string);
	    free((char *)lp);
	    lp = lpn;
	}
	free((char *)ls);
}

static wmlspush(ls,l)
struct line_stack *ls;
char *l;
{
	struct line_str *lp;

	lp = wmlsget(l);
	lp->line_next = ls->line_top->line_next;
	ls->line_top->line_next = lp;
	lp->line_pre = ls->line_top;
	lp->line_next->line_pre = lp;
	++ls->line_stack_length;
}

static char *wmlspop(ls)
struct line_stack *ls;
{
	char *p;
	struct line_str *lp;

	if (ls->line_stack_length == 0) return NULL;
	lp = ls->line_top->line_next;
	p = lp->line_string;
	ls->line_top->line_next = lp->line_next;
	ls->line_top->line_next->line_pre = ls->line_top;
	wmlsfree(lp);
	--ls->line_stack_length;
	return p;
}

static char *wmlspoplast(ls)
struct line_stack *ls;
{
	char *p;
	struct line_str *lp;

	if (ls->line_stack_length == 0) return NULL;
	lp = ls->line_top->line_pre;
	p = lp->line_string;
	ls->line_top->line_pre = lp->line_pre;
	ls->line_top->line_pre->line_next = ls->line_top;
	--ls->line_stack_length;
	wmlsfree(lp);
	return p;
}

struct win_str *wmcreate(lines,cols,pages,flag)
int lines,cols,pages,flag;
{
	struct win_str *win;

	win = (struct win_str *) calloc(1, sizeof(struct win_str));
	win -> cur_y = 0;
	win -> cur_x = 0;
	win -> max_y = lines;
	win -> max_x = cols;
	win -> beg_y = 0;
	win -> beg_x = 0;
	win -> flags = flag;
	win -> scroll_start = 0;
	wmnewscreen(win);
	win->pre_line =	wmlsempty();
	win->next_line = wmlsempty();
	win->free_line = wmlsempty();
	if (pages < 0) pages = 0;
	win -> max_line = pages * lines;

	return win;
}

struct win_str *wmmake(lines,cols,begin_y,begin_x,pages,flag)
					 /* make a new screen */
int lines,cols,begin_y,begin_x,pages,flag;
{
	struct win_str *win;
	int wflag;

	wflag = WM_AUTONL;
	if (pages < 1) wflag |= WM_SCROLL; else wflag |= DM_PAGE;

	win = wmcreate(lines,cols,pages,wflag);
	win -> beg_y = begin_y;
	win -> beg_x = begin_x;

	wmputwin(win,top_win,TRUE);

	if (flag) wmrefresh(win);

	return win;
}

wmrefreshlines(cy,cx,from,to)
int cy,cx;
int from,to;
{
	register int y,x;
	register struct win_str *w,**p;

	if (ScreenLength < to) to = ScreenLength;
	if (from < 0 || to <= from) return;

	for(y = from; y < to; y++)
	    if (win_change[y]) {
		win_change[y] = FALSE;
		clearline(y+1);
		for (x = 1, p = win_map[y]; x <= ScreenWidth; x++, p++)
		    if (*p)
			dputc((*p)->scr[y-(*p)->beg_y+1]
						[x-(*p)->beg_x]);
		    else dputc(' ');
	    }

	if (0 <= cx && cx < ScreenWidth) cursX = cx+1;
	if (0 <= cy && cy < ScreenLength) cursY = cy+1;

	UpdateScreen(1);
	fflush(stdout);
}

wmrefresh0(cy,cx)
int cy,cx;
{
  wmrefreshlines(cy,cx,0,ScreenLength);
}

wmrefresh(win)
struct win_str *win;
{
  wmrefreshlines(win->cur_y + win->beg_y,win->cur_x + win->beg_x,
  		 0,ScreenLength);
}

wmrefreshpart(win,from,to)
struct win_str *win;
int from,to;
{
  wmrefreshlines(win->cur_y + win->beg_y,win->cur_x + win->beg_x,from,to);
}

wmredraw(win) /* redraw the whole screen */
struct win_str *win;
{
	int y;

	ScreenGarbaged = 1;

	for ( y = 0; y < ScreenLength; y++)
		win_change[y] = TRUE;
	wmrefresh(win);
}

wmredraw0(y,x) /* redraw the whole screen */
int y,x;
{
	int yy;

	ScreenGarbaged = 1;

	for ( yy = 0; yy < ScreenLength; yy++)
		win_change[yy] = TRUE;
	wmrefresh0(y,x);
}

#define CR '\r'
#define LF '\n'
#define BS 8
#define HT '\t'
#define FF 12

wmaddch(win,ch,flag) /* output a character onto the screen */
struct win_str *win;
char ch;
int flag;
{
	switch(ch) {

	case CR :
		win->cur_x=0;
		break;

	case LF :
		if (win->cur_y == win->max_y-1) {
		    if (win->flags & WM_SCROLL) {
			wmscroll(win,1,FALSE);
		    }
		    else if (win->flags & DM_PAGE) {
			wmscroll(win,win->max_y-win->scroll_start,FALSE);
			win->cur_y = win->scroll_start;
		    }
		}
		else win->cur_y ++;
		break;

	case BS :
		if (--win->cur_x < 0) {
		    if (win->flags & WM_AUTONL) {
			win->cur_x = win->max_x-1;
			if (--win->cur_y < win->scroll_start) {
			    if (win->flags & WM_SCROLL) {
				wmscroll(win,-1,FALSE);
				win->cur_y = win->scroll_start;
			    }
			    else if (win->flags & DM_PAGE) {
				wmscroll(win,win->scroll_start-win->max_y,
						FALSE);
				win->cur_y = win->max_y-1;
			    }
			    else win->cur_y = win->scroll_start;
			}
		    }
		    else win->cur_x = 0;
		}
		break;

	case HT :
		do ++win->cur_x;
			while (win->cur_x % 8 && win->cur_x < win->max_x);
		if (win->cur_x >= win->max_x) {
		    if (win->flags & WM_AUTONL) {
			win->cur_x = 0;
			if(win->cur_y == win->max_y-1) {
			    if (win->flags & WM_SCROLL) {
				wmscroll(win,1,FALSE);
			    }
			    else if (win->flags & DM_PAGE) {
				wmscroll(win,win->max_y-win->scroll_start,
						FALSE);
				win->cur_y = win->scroll_start;
			    }
			}
			else win->cur_y++;
		    }
		    else win->cur_x = win->max_x-1;
		}
		break;

	case FF :
		if (win->flags & DM_PAGE) {
			win->cur_x = 0;
			wmscroll(win,win->max_y-win->scroll_start,FALSE);
			win->cur_y = win->scroll_start;
		}
		break;

	default :
		if (ch < ' ' || ch == '\177') break;
		if (win->flags & WM_STANDOUT) ch |= 0200;
		if (win->flags & WM_INSERT) {
		    char *p;
		    int x;
		    p = win->scr[win->cur_y+1];
		    for (x = win->max_x-1; x > win->cur_x; x--) {
			p[x+1] = p[x];
			wmupdate(win,win->cur_y,x);
		    }
		}
		win->scr[win->cur_y+1][win->cur_x+1] = ch;
		wmupdate(win,win->cur_y,win->cur_x);
		if (++win->cur_x == win->max_x)
		    if (win->flags & WM_AUTONL) {
			win->cur_x = 0;
			if(win->cur_y == win->max_y-1) {
			    if (win->flags & WM_SCROLL) {
				wmscroll(win,1,FALSE);
			    }
			    else if (win->flags & DM_PAGE) {
				wmscroll(win,win->max_y-win->scroll_start,
						FALSE);
				win->cur_y = win->scroll_start;
			    }
			}
			else win->cur_y++;
		    }
		    else win->cur_x = win->max_x-1;
	}

	if (flag) wmrefresh(win); /* actual update of the screen */
}

wmaddstr(win,str,flag)
struct win_str *win;
char *str;
int flag;
{
	while (*str) wmaddch(win,*str++,FALSE);

	if (flag) wmrefresh(win);
}

wmaddnum(win,s,i,flag)
struct win_str *win;
char *s;
int i,flag;
{
	char buf[20];
	sprintf(buf,s,i);
	wmaddstr(win,buf,flag);
}

wmheadstr(win,str,flag)
struct win_str *win;
char *str;
int flag;
{
	int x;

	x = 0;
	while (*str && x < win->max_x) {
	    win->scr[0][x+1] = *str++ | 0200; /* highlight */
	    x++;
	}
	while (x < win->max_x) {
	    win->scr[0][x+1] = '-';
	    x++;
	}
	wmupdatel(win,-1);

	if (flag) wmrefresh(win);
}

wmdelchar(win,c,flag)
struct win_str *win;
int c,flag;
{
	int x;
	char *p;

	p = win->scr[win->cur_y+1];
	for (x = win->cur_x; x < win->max_x-c; x++) p[x+1] = p[x+c+1];
	for (; x < win->max_x; x++) p[x+1] = ' ';
	wmupdatel(win,win->cur_y);
	if (flag) wmrefresh(win);
}

wmget(win,y,x) /* get a character on the screen */
struct win_str *win;
int y,x;
{
	if (y < -1 || y > win->max_y ||
	    x < -1 || x > win->max_x) return 0;
	return win->scr[y+1][x+1];
}

static char *wmgetnewline(win,flag)
struct win_str *win;
{
	register char *p,*q,*r;
	register struct line_stack *ls;

	if (p = wmlspop(win->free_line)) {
		q = p;
		r = (++q)+win->max_x;
		while (q < r) *q++ = ' ';
		return p;
	}
	if (win->pre_line->line_stack_length +
		win->next_line->line_stack_length < win->max_line)
		return wmnewline(win->max_x);
	if (flag) ls = win->pre_line; else ls = win->next_line;
	if (p = wmlspoplast(ls)) {
		q = p;
		r = (++q)+win->max_x;
		while (q < r) *q++ = ' ';
		return p;
	}
	return wmnewline(win->max_x);
}

wmscroll(win,n,flag) /* scroll the window */
register struct win_str *win;
{
	register int y;
	register char *p;

	if (n == 0) return;
	for(y = win->max_y; win->scroll_start < y; y--) {
	    wmlspush(win->next_line,win->scr[y]);
	}
	if (0 < n) {
	    while (n != 0) {
		if (!(p = wmlspop(win->next_line))) p = wmgetnewline(win,TRUE);
		wmlspush(win->pre_line,p);
		n--;
	    }
	}
	else {
	    while (n != 0) {
		if (!(p = wmlspop(win->pre_line))) p = wmgetnewline(win,FALSE);
		wmlspush(win->next_line,p);
		n++;
	    }
	}
	for(y = win->scroll_start; y < win->max_y; y++) {
	    p = wmlspop(win->next_line);
	    if(!p) break;
	    win->scr[y+1] = p;
	}
	for(; y < win->max_y; y++) {
	    win->scr[y+1] = wmgetnewline(win,TRUE);
	}
	for(y = win->scroll_start; y < win->max_y; y++)
		wmupdatel(win,y);

	if (flag) wmrefresh(win);
}

static wmupdate(win,y,x) /* update the screen */
register struct win_str *win;
register int y,x;
{
	register struct win_str *wp;
	register int ay,ax;

	ay = y + win->beg_y;
	ax = x + win->beg_x;
	if (ay < 0 || ay>=ScreenLength || ax<0 || ax>=ScreenWidth) return;
	if (win_map[ay][ax] == win) win_change[ay] = TRUE;
}

static wmupdatel(win,y)
register struct win_str *win;
register int y;
{
	y += win->beg_y;
	if (0 <= y && y < ScreenLength) win_change[y] = TRUE;
}

wmend() /* end up routine */
{
	(*tt.t_topos)(ScreenLength,1);
	(*tt.t_wipeline)();
	stty(1,&old);
	chmod(tty, stb.st_mode);
}

struct win_str *wmgetmap(y,x)
int y,x;
{
	if (y < 0 || y >= ScreenLength || x < 0 || x >= ScreenWidth)
		return 0;
	return win_map[y][x];
}

static struct win_str *wmgetwin(y,x) /* returrn the window at (y,x) */
int y,x;
{
	register struct win_str *wp;

	for(wp = top_win; wp; wp = wp->next_win)
		if (wp->beg_x-1 <= x &&
			wp->beg_y-1 <=y &&
			x <= wp->beg_x+wp->max_x &&
			y <= wp->beg_y+wp->max_y)
				break;
	return wp;
}

wmputwin(win,win2,fflag)   /* put the window win on win2 */
struct win_str *win,*win2;
int fflag;
{
	register int y,x,my,mx,i;
	struct win_str **wp,*w;

	i = fflag ? 1 : 0;

	for (wp = &top_win; *wp && *wp != win2; wp = &((*wp)->next_win));

	win->next_win = *wp;
	*wp = win;

	my = min(win->max_y+win->beg_y+i,ScreenLength);
	mx = min(win->max_x+win->beg_x+i,ScreenWidth);

	for (y = max(win->beg_y-i,0); y < my; y++) {
		for (x = max(win->beg_x-i,0); x < mx; x++) {
			w = wmgetwin(y,x);
			if (!(win_map[y][x] == w)) {
				win_map[y][x] = w;
				win_change[y] = TRUE;
			}
		}
	}
}


wmdel(win,flag) /* delete a window */
struct win_str *win;
int flag;
{
	register int y,x,mx,my;
	struct win_str *wp;

	if (win == top_win) top_win = win->next_win;
	else
		for (wp = top_win; wp->next_win; wp = wp->next_win)
			if (wp->next_win == win) {
				wp->next_win = win->next_win;
				break;
			}

	my = min(win->max_y+win->beg_y+1,ScreenLength);
	mx = min(win->max_x+win->beg_x+1,ScreenWidth);

	for (y = max(win->beg_y-1,0); y < my; y++) {
		win_change[y] = TRUE;
		for (x = max(win->beg_x-1,0); x < mx; x++) {
			win_map[y][x] = wmgetwin(y,x);
		}
	}

	if (flag) wmrefresh(win);
}

wmrepos(win,win2,y,x,flag) /* reposition the window */
struct win_str *win,*win2;
int y,x,flag;
{
	if (win == win2 && y == win->beg_y && x == win->beg_x)
		return;

	if (win == win2) win2 = win->next_win;

	wmdel(win,FALSE);

	win->beg_y = y;
	win->beg_x = x;

	wmputwin(win,win2,TRUE);

	if (flag) wmrefresh(win);
}

wmcupos(win,y,x,flag)
struct win_str *win;
int y,x,flag;
{
	if (y < 0 || y >= win->max_y ||
		x < 0 || x >= win->max_x) return;
	win->cur_y = y;
	win->cur_x = x;

	if (flag) wmrefresh(win);
}

wmvect(win,vy,vx,flag)
struct win_str *win;
int vy,vx,flag;
{
	int x,y;

	x = win->cur_x+vx;
	y = win->cur_y+vy;

	if (x < 0 || y >= win->max_y ||
		x < 0 || x >= win->max_x) return;
	win->cur_y = y;
	win->cur_x = x;

	if (flag) wmrefresh(win);
}

wmcleol(win,flag) /* clear to end of line */
struct win_str *win;
int flag;
{
	register char *p,*q;

	q = win->scr[win->cur_y+1] + win->max_x+1;
	p = win->scr[win->cur_y+1] + win->cur_x+1;

	while (p < q) *p++ = ' ';

	wmupdatel(win,win->cur_y);

	if (flag) wmrefresh(win);
}

wmcleos(win,flag) /* clear to end of screen */
struct win_str *win;
int flag;
{
	int y;
	register char *p,*q;

	wmcleol(win,FALSE);

	for (y = win->cur_y+1; y<win->max_y; y++) {
		p = win->scr[y+1]+1;
		q = win->scr[y+1]+win->max_x+1;
		while (p < q) *p++ = ' ';
		wmupdatel(win,y);
	}

	if (flag) wmrefresh(win);
}

wmcls(win,flag) /* clear the whole screen */
struct win_str *win;
int flag;
{
	win->cur_x = 0;
	win->cur_y = 0;

	wmcleos(win,flag);
}

wmclp(win) /* clear the following pages */
struct win_str *win;
{
	char *p;

	while(p = wmlspop(win->next_line)) wmlspush(win->free_line,p);
}

wmfree(win) /* free the area of the window */
struct win_str *win;
{
	int y;
	char *p;

	for (y = 0; y <= win->max_y+1; ++y)
	    free(win->scr[y]);
	free((char *) win->scr);
	wmlsallfree(win->free_line);
	wmlsallfree(win->next_line);
	wmlsallfree(win->pre_line);
	free((char *) win);
	while (free_line_frame) {
	    p = (char *) free_line_frame;
	    free_line_frame = free_line_frame->line_next;
	    free(p);
	}
}

wminsline(win,l,flag) /* insert a line at l */
register struct win_str *win;
int l,flag;
{
	register int y;

	if (l < win->scroll_start || l >= win->max_y) return;

	wmlspush(win->next_line,win->scr[win->max_y]);

	for (y = win->max_y-1; y > l; y--) {
		win->scr[y+1] = win->scr[y];
		wmupdatel(win,y);
	}

	win->scr[l+1] = wmgetnewline(win,FALSE);
	wmupdatel(win,l);

	if (flag) wmrefresh(win);
}

wmdelline(win,l,flag) /* delete a line at l */
register struct win_str *win;
int l,flag;
{
	register int y;

	if (l < 0 || l >= win->max_y) return;

	wmlspush(win->free_line,win->scr[l+1]);

	for (y = l; y < win->max_y-1; y++) {
		win->scr[y+1] = win->scr[y+2];
		wmupdatel(win,y);
	}

	win->scr[win->max_y] = wmgetnewline(win,TRUE);
	wmupdatel(win,win->max_y-1);

	if (flag) wmrefresh(win);
}

wmcopy(file,flag)
FILE *file;
int flag;
{
	register int x,y,xx,ch;
	register struct win_str *wp;

	for(y = 0; y < ScreenLength; y++) {
		for(xx = ScreenWidth-1; 0 <= xx; xx--) {
			wp = win_map[y][xx];
			if (wp &&
			    wp->scr[y-wp->beg_y+1][xx-wp->beg_x+1]
					!= ' ')
				break;
		}
		for(x = 0; x <= xx; x++) {
			wp = win_map[y][x];
			if (wp) {
				ch = wp->scr[y-wp->beg_y+1]
						     [x-wp->beg_x+1];
					if (flag && (ch & 0200)) {
						fputc('_',file);
						fputc(8,file);
					}
				fputc(ch & 0177,file);
			}
			else fputc(' ',file);
		}
		fputc('\n',file);
	}
}

