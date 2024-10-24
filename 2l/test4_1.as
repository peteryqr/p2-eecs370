start	lw	0	1	0
        sw  0   1   Loc
        lw  0   6   FA
        beq 0   0   1
        lw  1   2   start
done	halt
Two .fill 2