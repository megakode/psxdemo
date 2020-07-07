	
		section .text

		include inline_a.h
		include gtereg.h

		
		
		;
		; inputs in registers a0-a8
		; result in v0-v1
		;
		;
		;


		; a0 = poly
		; a1 = SVECTOR *cubeVert
		
		; optimization idea
		
		; 1. initial loop the transforms 8 vertices (3 * RTPT), storing results on the scratchpad
		; 
		; load vertices
		; RTPT
		; load vertices into temps
		; store results
		; copy tepmts to GTE
		; RTPT
		; 
		
		
		
		
		xdef	CubeAsm
		
CubeAsm:

		;li		t0, 4096/3
		;ctc2	t0, C2_ZSF3
		

		li		t2, 12				; iteration count
		li		t3, 0				; iterate normals
		
			
		
@loop:

		lwc2	C2_VXY0,(a1)
		lwc2	C2_VZ0,4(a1)
		lwc2	C2_VXY1,8(a1)
		lwc2	C2_VZ1,12(a1)
		lwc2	C2_VXY2,16(a1)
		lwc2	C2_VZ2,20(a1)
		
		addiu	a1, a1, 24					; cubeVertIndex += 3;
		addi	t2,t2,-1					; decrease loop count
		
		RTPT

		nop
		nop
		
		NCLIP
		
		nop
		nop
		
		mfc2	v0, C2_MAC0					; v0 = nclip result
		nop
		bltz	v0, @culled

		AVSZ3
		
		nop
		nop
		mfc2	v0, C2_OTZ					; v0 = average Z

		swc2	C2_SXY0,8(a0)
		swc2	C2_SXY1,12(a0)
		swc2	C2_SXY2,16(a0)
		
		lwc2	C2_VXY0,(a2)
		lwc2	C2_VZ0,4(a2)
		
		sll		v0,v0,2						; v0 = otz * 4
		addu	v0,v0,a3					; v0 = &ot[otz]
		
		NCCS

		; add primitive
		lw		t0,(v0)						; t0 = ot[otz] = pointer to old primitive (lower 24-bits)
		li		t1, 0xffffff				; t1 = 0xffffff = mask for lower 24 -bits of address
		and		t1,t1,a0					; t1 = (poly & 0xffffff), lower 24 bits of address of poly
		or		t0, 0x04000000				; t0 = (packet length << 24) | pointer to old primitive
		
		sw		t1,(v0)						; ot[otz] = poly & 0xffffff
		sw		t0,(a0)						; poly->tag = (packet length << 24) | pointer to old primitive
		
		; write out color result of lighting computation
		swc2	C2_RGB2, 4(a0)
		addiu	a0, a0, 20					; poly++
	

@culled:

		addu	a2, a2, t3
			
		bnez	t2,@loop
		xor		t3, t3, 8


		jr		ra
		addu	v0,a0,zero
		
		
		




		
