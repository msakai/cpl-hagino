wcat: lcat wcathelp.o wdia.o wmlib.o wm
lcat: wcat.l
	liszt -o lcat -r wcat.l
wcathelp.o: wcathelp.l
	liszt -r wcathelp.l
wdia.o: wdia.l
	liszt -r wdia.l
wmlib.o: wmlib.l
	liszt wmlib.l
swm: swm.o winlib.o display.o term.o
	cc -o swm swm.o winlib.o display.o term.o -ltermlib
wm: wm.o winlib.o display.o term.o
	cc -o wm wm.o winlib.o display.o term.o -ltermlib
tconv: tconv.o term.o
	cc -o tconv tconv.o term.o -ltermlib
tconv.o: term.h tconv.c
	cc -c -O tconv.c
swm.o: winlib.h display.h term.h wm.c
	cc -c -O swm.c
wm.o: winlib.h display.h term.h wm.c
	cc -c -O wm.c
winlib.o: winlib.c winlib.h display.h term.h
	cc -c -O winlib.c
display.o: display.c display.h term.h
	cc -c -O display.c
term.o : term.c term.h
	cc -c -O term.c
