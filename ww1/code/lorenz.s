start,
	/ make fixed point numbers
	ca rsh
	su y+1
	cp lose
	td .+2
	ca y
	srr .
	slr 1
	si 600	/ vdefl

	ca rsh
	su x+1
	cp lose
	td .+2
	ca x
	srr .
	slr 1
	rc .	/ hdefl

	/ lorenz attractor
	sp ax
	 cs x
	 ad y
	 mr c10f
	 ts dx

	 cs z
	 ad c28f
	 mr x
	 su y
	 ts dy

	 ca z
	 mr c2.66f
	 ts t
	 ca x
	 mr y
	 su t
	 ts dz
	 sp ax

	/ right shift dxyz
	ca dx
	ts ddx
	ca dx+1
	su c7
	ts ddx+1

	ca dy
	ts ddy
	ca dy+1
	su c7
	ts ddy+1

	ca dz
	ts ddz
	ca dz+1
	su c7
	ts ddz+1

	sp ax
	 ca ddx
	 ad x
	 ts x
	 ca ddy
	 ad y
	 ts y
	 ca ddz
	 ad z
	 ts z
	 sp ax
	sp start

lose,	si 0	/ too big for fixed point
	si 0

rsh,	6	/ -1 because we're left shifting again, avoids -0
x,	040000; 1
y,	040000; 1
z,	040000; 1
t,	0; 0
dx,	0; 0
dy,	0; 0
dz,	0; 0
ddx,	0; 0
ddy,	0; 0
ddz,	0; 0

c7,	7
c10f,	050000; 4
c28f,	070000; 5
c2.66f,	052525; 2

	/ NB:
	/ include fp_15_15.s here
