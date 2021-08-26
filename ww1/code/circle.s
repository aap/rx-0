/ sin(x+e) = sin(x) + cos(x)*0.00001
/ cos(x+e) = cos(x) - sin(x+e)*0.00001
start,	ca	cos
	srr	7
	ad	sin
	ts	sin	/ new sine
	si	600	/ vdefl
	cs	sin
	srr	7
	ad	cos
	ts	cos	/ new cosine
	rc	.	/ hdefl
	sp	start

sin,	0
cos,	040000
