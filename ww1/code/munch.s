start,
	ca a
	sa t

	ts a
	ts x
	ca (0	/ get overflow
//	srr 1
	su (0
	cp nov
	/ there was overflow, flip sign
	ca a
	sd sgn
	ts a
	ts x
nov,
	ca x
	clh 20
	ad y
	clh 10
	ts y
	si 600	/ vdefl
	clc 20
	ts x
	sd a
	rc .	/ hdefl

	sp start

a,	0
x,	0
y,	0
t,	1200	/2002
tt,	0
sgn,	100000
