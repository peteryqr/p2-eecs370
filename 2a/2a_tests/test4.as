        lw      0   6   addr
        lw      0   5   Addr
        lw      5   4   ADDR
        lw      6   3   zero
        lw      6   2   add
        sw      0   5   addr
        sw      0   6   Addr
        sw      4   4   zero
        sw      5   3   ADDR
        sw      6   2   add
end     halt
zero    .fill   0
add     .fill   zero
addr    .fill   Add1
ADDR    .fill   10