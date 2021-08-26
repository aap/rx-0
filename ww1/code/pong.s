vdefl=si 600
hdefl=rc

start,	ca 0
	ts ballx
	ts bally
	cs (200
	ts bwait

draw,
	ca p1y
	ts y1
	ts y2
	ca p2y
	ts y3
	ts y4

	cs (10
	ts i
plup,	
	/ draw paddle 1
	ca y1
	vdefl
	ad yinc
	ts y1
	cs pdlx
	hdefl

	ca y2
	vdefl
	su yinc
	ts y2
	cs pdlx
	hdefl

	/ draw paddle 2
	ca y3
	vdefl
	ad yinc
	ts y3
	ca pdlx
	hdefl
	ca y4
	vdefl
	su yinc
	ts y4
	ca pdlx
	hdefl
	ao i
	cp plup

	/ see if we want to update the ball
	ao bwait
	cp bend		/ nope
	ca 0
	ts bwait	/ so we don't overflow

	/ update ball y
	ca bally
	ts t		/ original pos
	vdefl
	sa ballvy
	ts bally
	ca (ovcond
	td .+1
	cs .
	cp byend
	ca t		/ bounce
	ts bally	/ restore y pos
	cs ballvy
	ts ballvy	/ flip y-speed
byend,

	/ update ball x
	ca ballx
	ts t		/ original pos
	hdefl
	sa ballvx
	ts ballx
	ca (ovcond
	td .+1
	ca .
	cp start	/ game over
	/ if ball passes pdlx, check for hit
	/ i.e. |bx0| - |pdlx| < 0  and
	/      |bx1| - |pdlx| > 0
	/ NB: this doesn't work if the ball hits
	/ the paddle exactly, maybe should fix this
	ca pdlx
	dm t
	cp bend		/ already beyond paddle x

	cm ballx
	dm pdlx
	cp bend		/ not yet reached paddle yet

	/ see which side to check
	ca ballx
	cp cklt
	/ ckrt
	ca (p2y
	td ppdl
	sp ppdl
cklt,	ca (p1y
	td ppdl

	/ if |by - pdly| < pdlen, hit
ppdl,	cs .		/ p1y or p2y
	sa bally
	ad 0		/ clear SAM
	ts disty
	cm disty	/ abs dist
	su pdlen
	cp xhit		/ hit the paddle
	sp bend
xhit,	ca t		/ bounce
	ts ballx	/ restore x pos
	cs ballvx
	ts ballvx	/ flip x-speed
	ca disty	/ set new y speed
	srr 4
	ts ballvy
bend,

	/ update paddles
	ca (p1y
	td pmv
	td pts
	ca 2		/ p1 input
	sp pdl
pdl,	ta pdlsp
	srh 16
	ad (pinp
	td .+1
	sp .
	sp pup
pinp,	sp pend
	cs ymv		/ pdn
	sp pmv
pup,	ca ymv
pmv,	ad .		/ pdl y
	ts t		/ temp position
	ca (77777
	dm t		/ distance to edge
	su pdlen
	cp pend		/ too close to edge
	ca t
pts,	ts .		/ pdl y, position ok
pend,	ca (p2y
	td pmv
	td pts
	ca 2
	clc 2		/ p2 input
pdlsp,	sp .

	sp draw


i,	0
t,	0
disty,	0
p1y,	0
p2y,	0
pdlx,	072000	/ paddle x position
pdlen,	011000	/ half length of paddle
yinc,	001000
ymv,	000600
ballx,	0
bally,	0
ballvx,	000177
ballvy,	000400
bwait,	0

/ temp for drawing paddles
y1,	0
y2,	0
y3,	0
y4,	0

	-0
ovcond,	0
	-0
