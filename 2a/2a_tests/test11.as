		add		0	0	0
		add		1	1	1
		add		1	2	3
		nor		2	2	5
		nor		6	6	6
ONE		nor		1	1	1
		lw		0	2	4
		lw		2	3	EIGHT
		sw		0	2	3
		sw		1	2	NONE
		beq 		0	0	1
		beq 		1	4	EIGHT
		beq 		2	7	ONE
		jalr		3	2
		noop
		halt
EIGHT	.fill	8
NONE	.fill	-1
