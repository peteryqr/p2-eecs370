start lw   0  2  Five
      lw   0  1  Glob1
      beq  0  1  GlobB
next  add  2  2  2
      beq  0  0  start
      sw   0  1  GlobD
GlobB nor  1  2  1
      jalr 4  7
      halt
Five  .fill    5
One   .fill    Glob1
      .fill    1
      .fill    next