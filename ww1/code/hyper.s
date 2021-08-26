start,	ca	(0
	ts	sinh
	ca	(040000
	ts	cosh
	cs	(240
	ts	n
loop,	ca	sinh
	si	600
	ca	cosh
	rc	.


	cs	sinh
	si	600
	ca	cosh
	rc	.

	/ left part
	ca	sinh
	si	600
	cs	cosh
	rc	.

	cs	sinh
	si	600
	cs	cosh
	rc	.

	/ upper part
	ca	cosh
	si	600
	ca	sinh
	rc	.

	ca	cosh
	si	600
	cs	sinh
	rc	.

	/ lower part
	cs	cosh
	si	600
	ca	sinh
	rc	.

	cs	cosh
	si	600
	cs	sinh
	rc	.

	ca	cosh
	srr	7
	ad	sinh
	ts	sinh	/ new sine
	ca	sinh
	srr	7
	ad	cosh
	ts	cosh	/ new cosine

	ao	n
	cp	loop
	sp	start

sinh,	0
cosh,	040000
n,	0
