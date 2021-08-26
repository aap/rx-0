/ LSR # OT2.2 t
/ print decimal number, numeric sign, CR

pdeca,	ta pdeca1
	ts t1
	cp pdecan
	ca flexn
pdeca2,	si 225		/ print on flexowriter
	rc
	ca (042000	/ .
	rc
	cs (4
	ts t2
pdeca3,	cm t1
	mh (12
	ad (flexn
	td .+3
	slr 17
	ts t1
	ca .
	rc
	ao t2
	cp pdeca3
	ca (122000	/ CR
	rc
pdeca1,	sp .	/ return
pdecan,	ca (072000	/ -
	sp pdeca2

flexn,	174000
	052000
	036000
	016000
	026000
	046000
	066000
	056000
	006000
	154000

t1,	0
t2,	0

