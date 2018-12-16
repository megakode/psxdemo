all:
	del main.exe
	ccpsx -c -O main.c -omain.obj -comments-c++ 
	dmpsx main.obj
	ccpsx -O -Xo$80010000 main.obj -omain.cpe
	cpe2x /ce main.cpe


	del main.cpe
