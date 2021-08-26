start,	cla
	add cos
	shr
	shr
	shr
	shr
	shr
	shr
	shr
	add sin
	sto sin
	shr
	shr
	shr
	shr
	shr
	shr
	shr
	shr
	shr
	cll
	sto t	/ y in right half

	cla
	add sin
	shr
	shr
	shr
	shr
	shr
	shr
	shr
	com
	add cos
	sto cos

	clr
	add t
	dis
	clc
	trn start
	hlt

sin,	0
cos,	200000
t,	0
