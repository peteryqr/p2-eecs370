	lw	0	1	100
	sw	1	2	-500
	lw	1	2	3
loop	beq	2	3	end
	add	1	1	3
	beq	0	0	loop
end	lw	1	3	900
	halt
	noop
	noop
	noop
