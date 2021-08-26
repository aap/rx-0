|7773	1

*5540
k1,	wordstore-tn-h6
adm2,	ad-2
limit,	-7737
k2,	tn+sp5b-2
k3,	srpt-sp5b
comma,	add wordstore
cm1,	trn owbp
	cla
	add cm1
	trn s3
owbp,	p6s
	add ca
	sto sp2
	sto 7777
	p6s
	add h2a
	trn nir
startat,	add (tn+st1
	trn nsrch
st1,	p6s
	add comtable+3
	add 7777
	p7h
	p7h
	p7a
feed,	cla
	add char+20
blkt,	sto fcc1a
	p6s
	add fcc1a
	add 7773
	trn blkt

|entry
hark,	cla
	add fnc6-1	|trn error
listen,	sto lsxt
ls5a,	cla
	add char+45
	pna		|switch color
ls5,	cal
ls4,	add cr6		|add wordstore
	sto ns1
	sto i2a
	add char+4	|sto wordstore
	sto ls1
	sto e3
	add (-wordstore+dr+ad	|add dr
	sto cr1
	sto e2
e1,	clc		|empty wordstore buffer
e3,	0		|sto wordstore+n, also plus/minus, also trn command
	add e3
	add 7773
	sto e3
	add char+77	|-7741
	trn e1
	clc
e2,	0		|???
ls2,	lac
	add (401776
	trn ls2		|wait for char from flexo
ls2b,	cyr
	trn ls2b	|get code in the low bits
	add (ad+char-377600	|add from char table
	sto ls2c
	cal
ls2c,	0	|load entry from char table
	sto ls2e
	trn ls2d	|negative entries
ls1,	0		|sto wordstore+n, also current value
ls3,	cla
ls6,	add ls1
	add 7773
	sto ls1		|inc ptr
	clc
	trn ls2
ls2d,	cyl		|negative entries
	trn ls2		|only allow trn
	cyl
	trn carret	|5xxxxx
	cla		|4xxxxx
	add char+45	|switch color
	pnc
ls2e,	0		|do trn

carret,	cyl		|cr or x
	trn ls5		|x, delete input
	cla
cr6,	add wordstore
	trn ls5		|no input
	cla
	add char+45	|switch color
	pna
cr5,	cla
	add wordstore+3
	cyl
	add wordstore+2
cr1,	0		|add dr+2n
	trn cr2		|possible match
cr3,	cla		|no match
	add cr1
	add char+41	|2
	sto cr1
	add (-ad-st
	trn cr5		|loop through all commands
	clc
lsxt,	0		|nothing found, do default
cr2,	add 7773
	trn cr3		|still negative, no match
	add cr1		|found match, AC=1 now
	add comtable+3	|add->trn
	sto e3
	trn e1		|jump indirectly to command handler

slash,	add char+55	|trn s1
interpret,	sto ixt
i1,	cla
	sto e3		|plus
	sto 7777	|sum
i2a,	0		|get next char, also number to be printed
ixt,	0		|end is -0
	cyl
	trn ipm		|plus/minus
	cla
	add char+47	|add comtable
	sto fcc2
	cla
	add i2a
	sto fcc1c
	sto fnc1
	add 7773
	sto fcc1b
	add 7773
	sto fcc1a
fcc4r,	cla
fcc1a,	0		|third char
	cyl
fcc1b,	0		|second char
	cyl
fcc1c,	0		|first char
	com
fcc2,	0		|add symbol
	trn fcc3
fcc4,	cla		|no match
	add fcc2
	add char+41
	sto fcc2	|advance by 2
	add char+57
	trn fcc4r	|loop through whole comtable
	cla		|not found in table, try number
	sto ls1
fnc,	add pnt0	|add numstore
	sto fnc2
	cla
fnc1,	0		|next digit
	trn fnc6	|end
	com
fnc2,	0
	trn fnc3	|equal if AC = -0
fnc4,	cla		|not equal, try next
	add fnc2
	add 7773
	sto fnc2
	add (-numstore-7-ad
	trn fnc1-1	|loop through digits
	cla		|not a digit
	add i2a
	com
	add fnc1
	trn error
fnc6,	cla
	add fnc1	|add nextchar
	sto i2a
fnc5,	cla		|combine values
	add e3
	trn fncm
	add ls1
	add 7777
	sto 7777	|add cur val to sum
	clc
	trn i2a
ipm,	cyl		|plus/minus
	trn im
	cla
im,	sto e3
	cla
ip2,	add i2a		|next character
	add 7773
	sto i2a
	clc
	trn i2a
fcc3,	add 7773
	trn fcc4	|still negative, no match
	add fcc2	|found a match, AC=1 now
	sto fcc3a
	cla
	add fcc1a
	add 7773
	sto i2a		|fetch next char at i2a
	cla
fcc3a,	0		|load value
	sto ls1
	clc
	trn fnc5
fnc3,	add 7773
	trn fnc4	|still negative no match
	add fnc1	|found a match, AC=1 now
	sto fnc1
	cla
	add ls1		|shift current value
	cyl
	cyl
	cyl
	sto ls1
	cla
	add fnc2
	add (-ad-numstore	| get digit
	trn fnc		|??
	add ls1
	sto ls1		|update value
	clc
	trn fnc
fncm,	cla		|subtract from sum
	add ls1
	com
	add 7777
	sto 7777
	clc
	trn i2a

s1,	cla		|slash, address in 7777
	add 7777
	sto ca
	cla
equal,	sto owbp1
	add char+67	|trn s2
	sto s4xt
	trn as2
s2,	add char+66	|here after printing
	pnc		|tab
	add h6+3	|trn hark
	sto s4xt
	add (s3a-hark	|trn s3a
	trn listen
s3,	sto s4xt
s3a,	cla		|here after cr to store new value
	add char+63	|trn s4
	trn interpret	|get number first
s4,	cla
	add 7777	|load number
ca,	0		|sto addr
	clc
s4xt,	0
period,	add wordstore
p2,	trn p1		|empty buffer
	cla
	add p2
	trn s3
p1,	add ca		|here after storing number
	add 7773
	sto ca		|increment address
	cla
	add char+3
	pna		|cr
	add char+67	|trn s2
autoslash,	sto s4xt
	cla
	sto owbp1
	add ca		|address
	sto i2a
	cla
as6,	add char+53	|trn as1
	trn npr
as1,	add char+36	|here after printing address
	pna		|print |
as2,	cla
	add ca
	add comtable+3	|add
	sto as3
	cla
as3,	0		|load value at address
	sto i2a
	cla
	add owbp1
	trn as4		|taken for =, print raw number
	add s4xt
printer,	sto nprxt
	cla
	add comtable+41	|pna
	sto mpr7
	add (4-op-24021	|4
	sto fnc2
mpr0,	cla
	add (ad+comtable+1
	sto mpr1
mpr3,	cla
	add i2a
	opr 241		|alc
mpr1,	0		|table value
	trn mpr2
mpr4,	cla		|not equal
	add mpr1
	add char+41	|2
	sto mpr1	|advance table ptr
	add char+57
	trn mpr3	|loop through comtable
	lac		|nothing found
	opr 40031	|clr, cyl
	cyl
	opr 100231	|cll, alr, cyl
	add char+47	|add comtable
	sto mpr5
	lac
	cyr
	opr 640
	add i2a
	sto i2a		|subtract order bits
	cla
mpr5,	0		|get name
	pnt
	pnt
mpr7,	0		|pna
	trn npr7
	add char+56
	pnc		|space
	trn npr7	|print number
mpr2,	add 7773
	trn mpr4	|still negative no match
	add mpr1	|found a match, AC=1 now
	add char+61	|-2
	sto mpr6
	cla
mpr6,	0		|get name
	pnt
	pnt
	pnc		|print
	trn nprxt
as4,	add s4xt
npr,	sto nprxt
	clc
	sto fnc2
npr7,	sto fcc3a	|-0 initially, no zeroes
	add char+73	|-5
npr5,	sto fnc1	|digit counter
	cla
	sto fcc2
	add char+61	|-2, bit counter
npr2,	opr 221		|alr, cla
	add fcc2
	cyl
	sto fcc2
	cla
	add i2a
	trn npr1	|top bit set
	add char+10	|-0
	trn pnt0	|number is +0
npr6,	cyl
	sto i2a		|advance number
	lac
	add 7773
	trn npr2	|loop through bits in digit
	cla		|got one octal digit
	add fcc3a	|decide whether to print zeroes
	add fcc2	|get digit
	trn npr3	|skip leading zero
	add pnt0	|add numstore
	sto npr4
	cla
npr4,	0		|load flexo digit
	pna		|print it
	sto fcc3a	|start printing zeroes
	add 7773
	add fnc2
	sto fnc2	|increment fnc2 (number of chars printed?)
	cla
npr3,	add fnc1
	add 7773
	trn npr5	|loop through digits
	clc
nprxt,	0		|return from printing
npr1,	cla		|top bit was 1
	add fcc2
	add 7773
	sto fcc2	|set bit
	cla
	add i2a
	trn npr6
pnt0,	add numstore	|print 0
	pnc
	trn nprxt

gt2,	add 7777
	add comtable+5
	sto s4xt
	trn s4xt-1
goto,	add (gt2+tn
nsrch,	sto ixt
	cla
ns1,	0
	cyr
	trn ns2
	cla
	add ns1
	add 7773
	sto ns1
	clc
	trn ns1
ns2,	cla
	add ns1
	sto i2a
	clc
	trn i1
surprise,	add sp00
	sto e2
srpt,	cla
	add h2a
	sto spdn
	add k1
	sto sp4a
	sto e3
	add 7773

	sto sp4b
	add adm2
	sto sp7b
	clc
	trn e1
sp0,	r3c
	trn sp1
	opr 163061
sp00,	trn sp0
sp5a,	add 7773
	trn se
sp1,	opr 163201
	sto sp4d
	add comtable+3
	trn sp3m
	sto sp2
	sto sp4c
	r3c
	sto fcc1b
	lad
	sto fcc1c
sp2a,	opr 163201
	add fcc1c
	sto fcc1c
	lac
	com
sp2,	0
	trn sp5
sp4,	cla
sp4c,	0
sp4b,	0
	lac
sp4d,	0
	cla
	add sp4d
sp4a,	0
	cla
	add char+41
	add sp4a
	sto sp4a
	add 7773
	sto sp4b
	add limit
	trn sp5b
	add k2
	sto e2
	add k3
	sto spdn
	trn sp3m
sp5b,	cla
sp5,	add 7773
	trn sp4
	add sp2
	sto sp2
	sto sp4c
	add char+4
	sto sp4d
	add fcc1b
	trn sp2a
	r3c
	add fcc1c
	trn sp5a
se,	cla
	add 7757

	sto se3
	sto se1
se2,	cla
se1,	0
	trn se5
	sto se4
	cla
se3,	0
se4,	0
	cla
se5,	add se3
	add char+61
	sto se3
	add char+71
	sto se1
	com
	add cr6
	trn se2
error,	cla
	add (460210
	pnt
	prt
	pnt
	opr 24031
	pnc
	trn ls5a
sp7d,	add char+45
	opr 24031
	pna
sp3m,	cla
sp3c,	add 7773
	add sp7b
	sto sp3a
	add 7773
	sto sp7b
sp3,	cla
sp3a,	0
spdn,	0
	sto i2a
	add comtable+3
	sto sp7e
	cla
	add (tn+sp7a
	trn npr
sp7a,	add char+36
	pna
sp7b,	0
	sto i2a
	cla
	add (tn+sp7c
	trn printer
sp7c,	add char+45
	pnt
	pna
sp7e,	0
	sto i2a
	cla
	add (tn+sp7d
	trn printer
word,	sto cr1
	add (ad+wordstore+4
	sto i2a
	add (ad+f2a-wordstore-4
	trn interpret

address,	cla
	sto cr1
	add (tn+f2a
	trn nsrch
f2a,	sto as3
	sto ca
	sto ls1
	add comtable+3
	sto finder
f2,	cla
finder,	0
	opr 221
	add cr1
	trn f3
	lac
	opr 40600
	cyr
	cyr
	opr 40031
	cyl
	cyl
f3,	add 7777
	opr 72
	add char+10
	trn f4
f6,	cla
	add finder
	add 7773
	sto finder
	add char+1
	trn f2
h6,	cla
	add char+3
	pnc
	trn hark
f4,	add 7773
	trn f6
	add finder
	add (-ad-1
	sto ca
	cla
	add (tn+f5
	trn autoslash
f5,	add char+3
	pnc
	sto i2a
	trn f6
pntv,	cla
pnth,	sto spdn
	add char+37
	trn nsrch
pt1,	add char
	pnt
	pna
	add 7777
	sto sp2
	cla
	add (tn+last
	trn listen
last,	add char+65
	trn nsrch
lt1,	add char+42
	opr 24640
	pna

	add 7777
	sto sp4b
	clc
	trn hark
instructions,	cla
constants,	sto owbp1
	cla
	add spdn
	trn h
v,	add sp2
	cyl
	cyl
	cyl
	opr 40600
	cyr
	cyr
	sto sp3a
page,	cla
	add char+56
	pnt
	opr 24031
	pnt
	pna
	add sp3a
	sto i2a
	cla
	add (tn+pg1
	trn npr
pg1,	add char+66
	prt
	prt
	prt
	pna
	add sp3a
	add (100
	sto nir1
	sto i2a
	sto sp7b
	cla
	add (tn+v2
	trn npr
v2,	cla
	add char+3
	pna
	add sp3a
	sto ca
	com
	add sp2
	sto finder
	trn v2a
v3,	cla
	add sp4b
	com
	add nir1
	trn rtp
v4,	cla
	add sp3a
	com
	add sp4b
	trn h6
v5,	cla
	add nir1
	add 7773
	sto nir1

	add char+20
	sto sp3a
	com
	add sp7b
	com
	trn v2
v7,	add nir1
	sto sp3a
	opr 221
	add sp4b
	opr 72
	trn page
	clc
	trn h6
v2a,	cla
	add (tn+v1
	trn as5
v1,	add fnc2
	add char+73
	com
	trn v3
	cla
	add char+66
	pnc
	trn v3
rtp,	cla
	add char+66
	prt
	opr 24221
	add finder
	trn rtp1
	lac
	prt
	prt
rtp1,	cla
	add nir1
	sto ca
	cla
	add (tn+v5
as5,	sto s4xt
	cla
	add char+66
	pna
	add ca
	opr 100031
	cyl
	cyl
	opr 100600
	cyr
	cyr
	sto i2a
	add char+75
	trn asp
	clc
	trn as6
asp,	cla
	add char+56
	pnc
	trn as6
h,	cla
	add sp2
	shr
	shr
	shr

	cyl
	cyl
	cyl
	sto sp3a
line,	cla
	add char+3
	pna
	add sp3a
	sto i2a
	add char+2
	sto sp7e
	cla
	add (tn+h4
	trn npr
h4,	add char+36
	pna
h1,	add char+66
	pna
	add sp3a
	com
	add sp2
	trn hzpt
h2,	cla
	add sp3a
	opr 241
	add sp4b
h2a,	trn h6
	lac
	add 7773
	sto sp3a
	com
	add sp7e
	trn line
	clc
	trn h1
hzpt,	cla
	add hzpt2
	sto mpr7
	add (tn+h2-op-140040
	sto nprxt
	add (ad-tn-h2
	add sp3a
	sto hzpt1
	cla
hzpt1,	0
	sto i2a
	cla
	add owbp1
	trn npr7
hzpt2,	clc
	trn mpr0
punch,	add (tn+pc1
	trn nsrch
pc1,	add char
	pnt
	pna
	add 7777
	sto sp2
	cla
	add (tn+pc
	trn listen
pc,	add (tn+pc2
	trn nsrch
pc2,	add ip4

nir,	sto nirxt
	p6s
	add sp2
	p7h
	p7h
	p7a
	add 7777
	com
	sto sp4b
	p7h
	p7h
	opr 27031
	cyl
	add sp2
	sto fcc1c
nir3,	add 7777
	add comtable+3
	sto nir1
nir2,	cla
nir1,	0
	p7h
	p7h
	opr 27031
	cyl
	add fcc1c
	sto fcc1c
	cla
	add nir1
	add 7773
	add char+4
	add sp4b
	trn nir3
	p6s
	add fcc1c
	com
	p7h
	p7h
	opr 27061
nirxt,	0
input,	add (7742+ad
	sto owbp1
	add char+1
ip3,	add char+26
	p7h
	p7h
	p7a
owbp1,	0
	p7h
	p7h
	p7a
	add owbp1
	add 7773
	sto owbp1
	add char+1
	trn ip3
	add (tn+7743
	p7h
	p7h
	opr 27061
ip4,	trn feed
numstore,	111110	|0
	10101		|1
	1111		|2
	111		|3

	1011		|4
	10011		|5
	11011		|6
	10111		|7

|commands, flexo chars are chars 23, all other chars are ignored
dr,	-30210		|addr
	trn address
rd,	-30120		|word
	trn word
rp,	-212300		|surp
	trn surprise
to,	-320000		|goto
	trn goto
ar,	-20310		|star
	trn startat
tv,	-322200		|pntv
	trn pntv
th,	-302000		|pnth
	trn pnth
ns,	-13020		|cons
	trn constants
nc,	-33200		|punc
	trn punch
pu,	-103320		|inpu
	trn input
st,	-201010		|inst
	trn instructions
char,	320000		|00 nu
	-ad-7776	|01 nu
	10		|02 e
	101001		|03 8	cr
	-ad		|04 nu
	trn slash	|05 |
	110		|06 a
	111		|07 3
	-0		|10 space
	trn equal	|11 =:
	1010		|12 s
	1011		|13 4
	1100		|14 i
	200001		|15 +/
	1110		|16 u
	1111		|17 2
	-100		|20 color shift
	trn period	|21 .)
	10010		|22 d
	10011		|23 5
	10100		|24 r
	10101		|25 1
	7776		|26 j		what?
	10111		|27 7
	11000		|30 n
	trn comma	|31 ,(
	11010		|32 f
	11011		|33 6
	11100		|34 c
	300001		|35 -
	101		|36 k		what?
	tn+pt1		|37 nu
	100000		|40 t
	2		|41 nu
	664756		|42 z		what?
	trn feed	|43 back space
	100100		|44 l
	650602		|45 tab		color,tab,g?
	100110		|46 w
	ad+comtable	|47 nu
	101000		|50 h
	500000		|51 carriage return
	101010		|52 y
	tn+as1		|53 nu
	101100		|54 p
	tn+s1		|55 nu
	203002		|56 q		space,cr
	-ad-comtable-115	|57 nu
	110000		|60 o
	-2		|61 stop
	110010		|62 b
	tn+s4		|63 nu
	110100		|64 g
	tn+lt1		|65 nu
	100101		|66 9	tab
	tn+s2		|67 nu
	111000		|70 m
	-1		|71 upper case
	540000		|72 x		delete input
	-5		|73 nu
	111100		|74 v
	-7		|75 lower case
	111110		|76 0
	-7741		|77 delete	end of wordstore
|first word is three chars in flexo format, second is value
comtable,	641010
	sto
	60170
	add
	164200
	trn
	352600
	opr
	211740
	cla
	613520
	cyl
	253520
	cyr
	255700
	clc
	16250
	dis
	225140
	ios
	525322
	p7h
	527122
	p6h
	127162
	p6s
	523100
	pnt
	121762
	p7a
	123562
	p6a
	123540
	pna
	32560
	rfa
	432520
	rfl
	72520
	rfr
	54722
	r3c
	74702
	r1c
	430702
	r1l
	70702
	r1r
	144720
	lac
	240710
	alr
	342340
	lpd
	560300
	lro
	140360
	lad
	144620
	tac
	360420
	tbr
	675100
	com
	243410
	shr
	701200
	hlt
	611700
	cll
	251700
	clr
	167500
	pnc
	411720
	cal
	521300
	prt

wordstore=7622
ad=200000
tn=400000
op=600000
