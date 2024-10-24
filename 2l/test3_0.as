        lw         0        1      input        $1 = memory[input]
        lw         0        4      SubAdr       prepare to call sub4n. $4 = addr(sub4n)
        jalr       4        7                   call sub4n; $7 = return address; $3 = answer
        halt
input   .fill      10     