
all:
	del main.exe
	ccpsx -c -O2 main.c -omain.obj -comments-c++ 
	dmpsx main.obj -b
	ccpsx -c -O2 land.c -oland.obj -comments-c++ 
	ccpsx -c -O2 stars.c -ostars.obj -comments-c++ 
	ccpsx -c -O2 model.c -omodel.obj -comments-c++ 
	ccpsx -c -O2 picture.c -opicture.obj -comments-c++ 
	ccpsx -c -O2 dsrlib.c -odsrlib.obj -comments-c++ 
	ccpsx -c -O2 cubescroll.c -ocubesc.obj -comments-c++ 
	asmpsx /g /l /oc+ /j c:\psyq\include cubeasm.asm,cubeasm.obj
	dmpsx cubeasm.obj -b
	dmpsx cubesc.obj -b
	# ccpsx -O -Xo$80010000 -Xm main.obj dsrlib.obj cubeasm.obj stars.obj cubesc.obj hitmod.obj -omain.cpe,main.sym,main.map
	ccpsx @main.lnk
	cpe2x main.cpe


	del main.cpe


