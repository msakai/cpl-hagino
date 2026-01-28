/* window library by Tatsuya Hagino */
struct line_str {
	char *line_string;
	struct line_str *line_pre;
	struct line_str *line_next;
};

struct line_stack {
	struct line_str *line_top;
	int line_stack_length;
};

struct win_str {
	short cur_y, cur_x;
	short max_y, max_x;
	short beg_y, beg_x;
	char **scr;
	struct line_stack *pre_line;
	struct line_stack *next_line;
	struct line_stack *free_line;
	struct win_str *next_win;
	short flags;
	short scroll_start;
	int max_line;
};

/* Function prototypes */
struct win_str *wmmake(int lines, int cols, int begin_y, int begin_x,
                       int pages, int flag);
struct win_str *wmcreate(int lines, int cols, int pages, int flag);
struct win_str *wmgetmap(int y, int x);
void wminit(void);
void wmnewscreen(struct win_str *win);
void wmrefreshlines(int cy, int cx, int from, int to);
void wmrefresh0(int cy, int cx);
void wmrefresh(struct win_str *win);
void wmrefreshpart(struct win_str *win, int from, int to);
void wmredraw(struct win_str *win);
void wmredraw0(int y, int x);
void wmaddch(struct win_str *win, char ch, int flag);
void wmaddstr(struct win_str *win, const char *str, int flag);
void wmaddnum(struct win_str *win, const char *s, int i, int flag);
void wmheadstr(struct win_str *win, const char *str, int flag);
void wmdelchar(struct win_str *win, int c, int flag);
int wmget(struct win_str *win, int y, int x);
void wmscroll(struct win_str *win, int n, int flag);
void wmend(void);
void wmputwin(struct win_str *win, struct win_str *win2, int fflag);
void wmdel(struct win_str *win, int flag);
void wmrepos(struct win_str *win, struct win_str *win2, int y, int x, int flag);
void wmcupos(struct win_str *win, int y, int x, int flag);
void wmvect(struct win_str *win, int vy, int vx, int flag);
void wmcleol(struct win_str *win, int flag);
void wmcleos(struct win_str *win, int flag);
void wmcls(struct win_str *win, int flag);
void wmclp(struct win_str *win);
void wmfree(struct win_str *win);
void wminsline(struct win_str *win, int l, int flag);
void wmdelline(struct win_str *win, int l, int flag);
void wmcopy(FILE *file, int flag);

#define WM_AUTONL 1
#define WM_SCROLL 2
#define DM_REFRESH 4
#define DM_PAGE 8
#define WM_STANDOUT 16
#define WM_INSERT 32

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
