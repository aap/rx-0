# Collection of MIT computers

This project is mostly a toy I wrote for myself to play around
with old computers.

Currently it sort of simulates the Whirlwind I and the TX-0
with some peripherals, and the PDP-1 without any peripherals.

The PDP-1 should have some peripherals,
and a PDP-4 and -5 would also be nice to have.
Whirlwind II (aka AN/FSQ-7) would be nice as well,
TX-2 I don't know yet.

The emulators use a generic DDT-like frontend.

## Host programs

I have included assemblers `wwas` for Whirlwind I and `texas` for TX-0.
They read from stdin and produce a memory file `out.mem`
that the emulators read on startup, and a listing `out.lst`.

The program `crt` is a simulation of P7 phosphor CRT.
Run it before starting the emulator and it will connect to it.

To get flexowriter, run `mkptyfl /tmp/fl` before starting the
emulator. Ctrl-] to exit.

## Demo programs

See `ww1/code` and `tx0/code` for some example code.
Some of it written by me, some of it original.

## Interface

The DDT commands are the following:

    ◊ is escape
    ↑ is control

    addr◊g	start execution at addr
    addr◊0g	set PC to addr but don't start
    ↑N	single step instruction
    ◊N	single step memory
    ↑Z	stop instruction
    ◊Z	stop memory
    addr/	open memory addr
    reg◊/	open register reg
    addr\	open memory addr, don't set dot
    addr[	open memory in numeric mode
    addr]	open memory in symbolic mode
    word↑M	deposit in open location
    word↑J	deposit in open location, open next
    word^	deposit in open location, open previous
    =	type out quantity in numeric mode
    _	type out quantity in symbolic mode
    ◊q	last quantity
    .	last opened location
    ◊◊z	zero core
    lab:	define symbol at .
