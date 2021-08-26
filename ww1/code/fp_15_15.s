/ (15,15) floating point system as described in "LSR PA 1.1 t"
/ 0 must be 0

ax,	ta fpc		/ store address of 1st
	sp fpc		/ instruction to be interpreted
fx1,	0		/ x1 multiple register accumulator
fy1,	0		/ y1
fcaax,	ca ax		/ "ca" entry
	sp fcs		/ "cs" entry
	sp fad		/ "ad" entry
	sp fsu		/ "su" entry
fcs,	ca fcay2	/ "cm" entry. Pick up and
	ts finsty	/  store cs/ca/cm n+k
	sp fex2		/  .
c16,	20
	mr fx1		/ "mr" entry. Form x1*x2
	sp fpo
	ex fx1		/ "dv" entry. Form and
	srr 1		/  Store
	dv fx1		/  x*2^-1/x2 * 2^sf
	sf fexp		/  .
	ex fy1
fbase,	su ftmp		/ Form y1 - y2 + 16
	ad c16		/  .
	sp fsusf	/  .
fcp,	cs fx1		/ Is x1 < 0?
	cp fnxt		/  .
	sp fsp		/ Go to sp routine

fexp,	.		/ digit storage
fsep,	1		/ separation parameter
	sp fex2		/ "ts" entry
fspbas,	sp fbase
	ca far		/ "ta" entry
ftax,	td .		/ Store digits in
	sp fnxt		/  A-Register
	sp fex2		/ "ex" entry
	sp fcp		/ "cp" entry
fsp,	ao fpc		/ "sp" entry
	td far		/  Set A-Register
	ca fcax2	/ Set pick-up
	td fpc		/  instruction's address
	su fcaax	/ Does address of cp
	ts ftmp		/  or sp instruction
	cm ftmp		/  differ from ax?
	su 0		/  .
far,	cp .		/  .
	sp fpc
fsu,	ts finstx	/ Complement x2
	cs finstx	/  .
fad,	ex ftmp		/ Form and store
	su fy1		/  y2 - y1.
	ts finstx	/  .
	cp fyok		/ Is y2 - y1 > 0?

	ad fy1		/ Interchange
	ts fy1		/  (x1,y1) and
	ca fx1		/  (x2,y2)
	ex ftmp		/  .
	ts fx1		/  .
fyok,	cm finstx	/ Store |y2-y1|
	td fdsc		/  .
	su c15		/ Is |y2-y1| > 15?
	cp .+2		/  .
	sp fnxt		/ Addition unnecessary
	ca c15		/ Set y2 = 15
	ex ftmp		/  .
fdsc,	srr .		/ Form x2*2^(-y2-y1)
	sa fx1		/ Add
	ts fx1		/  .
	ca 0		/ Add overflow into
	ex fx1		/  2^-15 times the
	srh 17		/  sum
	ad fx1		/  .
fpo,	sf fexp		/ Scale factor
	ex fy1		/  and store result
	ad ftmp		/ Form y2+y1
fsusf,	su fexp		/ Subtract scale factor

fswp,	ex fy1		/ Store result in
	ts fx1		/  proper registers
fnxt,	ao fpc		/ Increase address of pick-up instruction
fpc,	ca .		/ Pick-up instruction
	ts finstx	/ Store instruction
	td ftax		/  and digits
	td fcax2	/  where
	ad fsep		/  necessary
	ts finsty	/  .
	td fcay2	/  .
	srh 13		/ Form sp to proper
	ad fspbas	/  part of subroutine
	td fdsp		/  .
fcay2,	ca .		/ Pick up and
	ts ftmp		/  store y2
fcax2,	ca .		/ Pick up x2
fdsp,	sp .		/ Go to part of subroutine for particular instruction
fex2,	ca fx1		/ execute two instructions
finstx,	.		/ inst x
	ex fy1
finsty,	.		/ inst y
	sp fswp
ftmp,	.		/ Temporary storage
c15,	17

