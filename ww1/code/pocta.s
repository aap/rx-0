/ LSR # OT1.1 t
/ print octal number, numeric sign, CR

pocta,	ta pocta1
	ts t1
	cp poctan
	ca flexn
pocta2,	si 225		/ print on flexowriter
	rc
	cs (4
	ts t2
pocta3,	ca t1
	srh 14
	ad (flexn
	td .+3
	slr 17
	ts t1
	ca .
	rc
	ao t2
	cp pocta3
	ca (122000	/ CR
	rc
pocta1,	sp .	/ return
poctan,	ad (077777
	ts t1
	ca flexn+1
	sp pocta2

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

