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
	short cur_y,cur_x;
	short max_y,max_x;
	short beg_y,beg_x;
	char **scr;
	struct line_stack *pre_line;
	struct line_stack *next_line;
	struct line_stack *free_line;
	struct win_str *next_win;
	short flags;
	short scroll_start;
	int max_line;
} *wmmake(),*wmcreate(),*wmgetmap();

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
