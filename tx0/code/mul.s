mb,	123
ac,	0
mq,	321
t,	0

mul,	cla
	add (-21	|16 iterations
	sto t
mul5,	cla 		|check whether to add
	add mq
	cyr
	trn mul3	|add
	clc		|don't add
	trn mul4
mul3,	cla		|add multiplicand
	add mb
	add ac
	sto ac
	cla
mul4,	add (400000	|combined right shift
	alo		|upper bit to LR
	add ac
	cyr
	trn mul1
	sto ac		|shift zero into mq
	cla
	add mq
	cyr
	trn mul2	|bit is set, flip it
	sto mq
	clc
	trn mul6
mul1,	lpd		|shift one into mq, first clear it
	sto ac
	cla
	add mq
	cyr
	trn .+2		|bit is set as it should be
mul2,	lpd		|flip it
	sto mq
	clc
mul6,	add t
	add (1
	sto t
	trn mul5	|loop

	cla
	add mq
	hlt		|done
