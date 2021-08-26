start,
	/ ya
	ca xb
	sa xa
	sp fixov
	srh 		7	/ sh0
	sa ya
	sp fixov
	ts ya
	si 600		/ vdefl

	/ xa
	cs yb
	sa ya
	sp fixov
	srh		10	/ sh1
	ts t
	cs t
	sa xa
	sp fixov
	ts xa
	rc .		/ hdefl

	/ yb
	cs xc
	sa xb
	sp fixov
	srh 		10	/ sh2
	sa yb
	sp fixov
	ts yb
	si 600		/ vdefl

	/ xb
	cs yc
	sa yb
	sp fixov
	srh		10	/ sh1
	ts t
	cs t
	sa xb
	sp fixov
	ts xb
	rc .		/ hdefl

	/ yc
	cs xa
	sa xc
	sp fixov
	srh 		3	/ sh2
	sa yc
	sp fixov
	ts yc
	si 600		/ vdefl

	/ xc
	cs ya
	sa yc
	sp fixov
	srh		2	/ sh1
	ts t
	cs t
	sa xc
	sp fixov
	ts xc
	rc .		/ hdefl
	sp start

xa,	167777
xb,	0
xc,	004000
ya,	0
yb,	014000
yc,	0
t,	0

fixov,	ta fixov1
	ts t
	ca (ovsgn	/ add with SAM
	td .+1
	ca .	/ load sign fixup
	ad t
fixov1,	sp .

	077777
ovsgn,	0
	100000
