 *** TB-TRACE - TIMA 2013 ***
==============================

Qemu translation block trace tool

How to use:

1- First run make: genrate a.out executable file

2- Run ./a.out <Qemu Log file>
	<the Qemu Log file> is path for log file using qemu monitor command: log exec

	Default config is:
		Algo: 	A-2VQ
		Quota: 	1/2
		Size:		512
		Stop after: 1 000 000  Translations

3- Use one of the following commandes:
	1 - Run Cache policy simulation					:		Run simulation, until a cache flush occurs
	2 - Modify number of executions into loop		:		Number of translations to stop after
	3 - Modify Quota of simulated cache policy	:		Quota is a value between 1 and 31, (Quota=16 -> 1/2)
	4 - Modify Trace Size								:		Max Value is 1024
	5 - Modify Fth of A-2Q Algo						:		Fth is the threshold above which TB are added to HST (depend on cache size)
	6 - Change Simulated Algo							:		Available algo: Qemu basic, A-LRU, A-LFU, A-2VQ
	7 - Dump Coldspot cache								:		store current cache value to file
	8 - Plot hit ratio									:		Run gnuplot script
	9 - Display this menu								:		Display this menu again
	0 - Exit													:		Exit application

	