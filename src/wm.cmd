Quick Reference Table of Window Manager Escape Sequences

Window Manipulation
    ESC { #w ; #f A	- change the optional switches of window #w
    ESC { #w ; #x1 ; #x2 ; #y1 ; #y2 ; #p C
		- create window (#x1,#y1)-(#x2,#y2) with #p pages
    ESC { #w D	- delete the window #w
    ESC { #w ; #f E
		- enquire about the window #w
	ESC { #w ; #x ; #y ; #bx ; #by ; #c ; #l e
	ESC { #w ; #x ; #y ; #bx ; #by ; #c ; #l ; #pl ; #nl ; #ml e
    ESC { #w H " $s "
		- heading of the window #w
    ESC { #w P	- put the window #w on top of all the windows
    ESC { #w ; P
		- move the window #w to bottom
    ESC { #w1 ; #w2 P
		- put the window #w1 on top of #w2
    ESC { #w ; #x ; #y R
		- move the window #w so that the top left corner becomes
		  (#x,#y)
    ESC { #w S	- select the window #w
    ESC { #w ; #p b
		- backward #p pages
    ESC { #w ; #p f
		- forward #p pages
    ESC { #w ; #p p
		- select the page #p of the window #w
    ESC { #w s  - show the first page
    ESC { #w ; s
		- show the last page
    ESC { #w ; #n s
		- scroll #n line
    ESC { #w ; #n ; s
		- restricted scroll #n line

Screen Manipulation
    LF		- cursor down or scroll up
    ESC LF	- cursor up or scroll down
    ESC A	- cursor up
    ESC [ A	- cursor up
    ESC B	- cursor down
    ESC [ B	- cursor down
    ESC C	- cursor right
    ESC [ C	- cursor right
    ESC D	- cursor left
    ESC [ D	- cursor left
    ESC H	- cursor home, i.e. (0,0)
    ESC [ #y ; #x H
		- cursor to (#x-1,#y-1)
    ESC J	- clear to the end of current page
    ESC [ J	- clear to the end of current page
    ESC [ 2 J	- clear the current page
    ESC [ 3 J	- clear to the end of last page
    ESC [ 4 J	- clear all pages
    ESC K	- clear to the end of line
    ESC [ K	- clear to the end of line
    ESC L	- insert a line
    ESC [ L	- insert a line
    ESC M	- delete a line
    ESC [ M	- delete a line
    ESC O	- delete one character
    ESC [ #p P	- delete #p characters
    ESC R	- redraw the screen
    ESC Y $y $x
		- cursor to ($x-' ',$y-' ')
    ESC [ #y ; #x f
		- cursor to (#x-1,#y-1)
    ESC [ h	- insert mode on
    ESC i	- insert mode on
    ESC j	- insert mode off
    ESC [ l	- insert mode off
    ESC [ m	- finish highlighting
    ESC [ 1 m	- start highlighting
    ESC r	- set the scroll start line at the current cursor position
    ESC v	- clear the current page
    ESC x	- clear to the end of line
    ESC y	- clear to the end of current page

Auxiliary Command
    ESC ( #f A	- change the optional switches of the current window
    ESC ( #a C " $f "
		- hardcopy
    ESC ( D	- reset dump mode
    ESC ( 1 D	- set dump mode
    ESC ( E	- terminal echo off
    ESC ( 1 E	- terminal echo on
    ESC ( #f ; #m F " $s "
		- set function key
    ESC ( K	- reset kill character
    ESC ( #c K	- set #c to kill character (default control-\, ascii 28)
    ESC ( #f M	- set mouse capability
    ESC ( R	- terminal cooked mode
    ESC ( 1 R	- terminal raw mode

Process Control
    ESC ( #p ; #w b
		- change base window to #w
    ESC ( #p g	- give the keyborad to process #p
    ESC ( #p k	- kill process #p
    ESC ( #p ; #w s
		- start process #p in #w
    ESC ( #p I " $s "
		- send string $s to process #I
