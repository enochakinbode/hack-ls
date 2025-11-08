// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/06/add/Add.asm

// Computes R0 = 5 + 5  (R0 refers to RAM[0])

@5
D=A
@5
D=D+A
@R0
M=D

