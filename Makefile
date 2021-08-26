all: fe_ww1 fe_tx0 fe_pdp1 crt mkptyfl wwas texas

fe_ww1: common/fe.c common/util.c common/threading.c ww1/mach.c
	$(CC) -g -lpthread -o fe_ww1 -Iww1 common/fe.c common/util.c common/threading.c ww1/mach.c

fe_tx0: common/fe.c common/util.c common/threading.c tx0/mach.c
	$(CC) -g -lpthread -o fe_tx0 -Itx0 common/fe.c common/util.c common/threading.c tx0/mach.c

fe_pdp1: common/fe.c common/util.c common/threading.c pdp1/mach.c
	$(CC) -g -lpthread -o fe_pdp1 -Ipdp1 common/fe.c common/util.c common/threading.c pdp1/mach.c

crt: misc/crt.c
	cc -o $@ $^ common/threading.c common/util.c -Icommon -lm -lpthread `sdl2-config --libs`

mkptyfl: misc/mkptyfl.c
	cc -o $@ $^

wwas: misc/wwas.c
	cc -o $@ $^

texas: misc/texas.c
	cc -o $@ $^
