/ LSR # OT1.2 t
/ print octal number, sign, no CR

poctb,	ta poctb1
	ts t1
	cp poctbn
	ca (032000
poctb2,	si 225		/ print on flexowriter
	rc
	cs (4
	ts t2
poctb3,	ca t1
	srh 14
	ad (flexn
	td .+3
	slr 17
	ts t1
	ca .
	rc
	ao t2
	cp poctb3
poctb1,	sp .	/ return
poctbn,	cm t1
	ts t1
	ca (072000	/ -
	sp poctb2

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

