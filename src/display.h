/* Ultra-hot screen management package
 *		James Gosling, January 1980
 */

#include "term.h"

#define ScreenLength (tt.t_length)
#define ScreenWidth (tt.t_width)

int	ScreenGarbaged,		/* true => screen content is uncertain. */
	DoHighlights,		/* true => hightlights should be done */
	cursX,			/* X and Y coordinates of the cursor */
	cursY,			/* between updates. */
	CurrentLine,		/* current line for writing to the virtual
				 * screen. */
	IDdebug,		/* line insertion/deletion debug switch */
	RDdebug,		/* line redraw debug switch */
	left;			/* number of columns left on the current
				 * line of the virtual screen. */
char
	*cursor;		/* pointer into a line object, indicates
				 * where to put the next character */

#define dputc(c) (--left>=0 ? *cursor++ = c : 0)
