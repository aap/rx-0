/ LSR # OT1.3 t
/ print octal number, sign, point, CR

poctc,	ta poctc1
	ts t1
	cp poctcn
	ca flexn
poctc2,	si 225		/ print on flexowriter
	rc
	ca (042000	/ .
	rc
	cs (4
	ts t2
poctc3,	ca t1
	srh 14
	ad (flexn
	td .+3
	slr 17
	ts t1
	ca .
	rc
	ao t2
	cp poctc3
	ca (122000	/ CR
	rc
poctc1,	sp .	/ return
poctcn,	ad (077777
	ts t1
	ca flexn+1
	sp poctc2

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

