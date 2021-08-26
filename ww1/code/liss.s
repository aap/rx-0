start,
	ca	c1
	srr	5
	ad	s1
	ts	s1	/ new sine
/	rc	.	/ hdefl
	cs	s1
	srr	5
	ad	c1
	ts	c1	/ new cosine
/	si	600	/ vdefl
	rc	.	/ hdefl

	ca	c2
	srr	3
	ad	s2
	ts	s2	/ new sine
/	rc	.	/ hdefl
	si	600	/ vdefl
	cs	s2
	srr	3
	ad	c2
	ts	c2	/ new cosine
/	si	600	/ vdefl

	sp	start

s1,	0
c1,	040000
s2,	0
c2,	040000
