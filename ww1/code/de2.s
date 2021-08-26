/ solve   ydd - 2zyd + y = 0
start,	ca t0
	ts t
	ca y0
	ts y
	ca yd0
	ts yd
loop,
	ca t
	rc .	/ hdefl
	sa dt
	ts t	/ advance t
	cs (0
	cp .+2
	 sp start	/ x-axis overflow, restart

	ca z
	mh yd
	slr 2
	ad y
	ts ydd

	ca yd
	srr 6
	ad y
	ts y
	slr 1
	si 600	/ vdefl

	cs ydd
	ts ydd
	srr 6
	ad yd
	ts yd
	sp loop
	

t,	0
t0,	100000
y,	0
y0,	0
yd,	0
yd0,	40000	/ 1
ydd,	0

z,	4000	/ 1/8
dt,	40
