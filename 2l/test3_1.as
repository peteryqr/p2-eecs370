sub4n   lw          0       6       pos1        r6 = 1
        sw          5       7       Stack       save return address on stack
        add         5       6       5           increment stack pointer
        sw          5       1       Stack       save input on stack
        add         5       6       5           increment stack pointer
        add         1       1       1           compute 2*input
        add         1       1       3           compute 4*input into return value
        lw          0       6       neg1        r6 = -1
        add         5       6       5           decrement stack pointer
        lw          5       1       Stack       recover original input
        add         5       6       5           decrement stack pointer
        lw          5       7       Stack       recover original return address
        jalr        7       4                   return.  r4 is not restored.
pos1    .fill       1
neg1    .fill       -1
SubAdr  .fill       sub4n                         contains the address of sub4n