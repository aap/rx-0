start,	ca x0
	ts xi
	ca y0
	ts yi
	ca vy0
	ts vy

loop,	ca xi
	ad vxdt
	ts xi	/ qh

	ca yha
	/ts yha	/ qd
	 si 600
	 ca xi
	 rc .
	cs yi
	cp cont	/ not bouncing
	ca vy	/ bounce
	mr r
	ts vy
cont,	ca vy
	su gdt
	ts vy
	mr dt
	ad yi
	ts yi	/ qd
	 si 600
	 ca xi
	 rc .
	cm xi
	su xul
	cp loop
	sp start

foo,	si 1

.=1000
xi,	0
yi,	0
vy,	0

r,	107777
/gdt,	001000
/gdt,	002000
gdt,	000100
dt,	040000
x0,	117777
y0,	060000
vy0,	000000
/vxdt,	001000
vxdt,	000240
xul,	060000
yha,	000000
