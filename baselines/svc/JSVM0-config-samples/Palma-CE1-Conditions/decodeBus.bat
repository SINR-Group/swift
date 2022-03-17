
@echo QCIF7.5                                                     >dat\Bus.txt
@CALL batch\decode	Bus	QCIF7.5	64	176 144 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF7.5	80	176 144 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF7.5	96	176 144 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF7.5	112	176 144 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF7.5	128	176 144 7.5	2>>dat\Bus.txt

@echo QCIF15                                                     >>dat\Bus.txt
@CALL batch\decode	Bus	QCIF15	96	176 144 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF15	112	176 144 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF15	128	176 144 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF15	160	176 144 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	QCIF15	192	176 144 15	2>>dat\Bus.txt

@echo CIF7.5                                                     >>dat\Bus.txt
@CALL batch\decode	Bus	CIF7.5	128	352 288 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF7.5	160	352 288 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF7.5	192	352 288 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF7.5	224	352 288 7.5	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF7.5	256	352 288 7.5	2>>dat\Bus.txt

@echo CIF15                                                      >>dat\Bus.txt
@CALL batch\decode	Bus	CIF15	192	352 288 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF15	224	352 288 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF15	256	352 288 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF15	320	352 288 15	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF15	384	352 288 15	2>>dat\Bus.txt

@echo CIF30                                                      >>dat\Bus.txt
@CALL batch\decode	Bus	CIF30	256	352 288 30	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF30	320	352 288 30	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF30	384	352 288 30	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF30	448	352 288 30	2>>dat\Bus.txt
@CALL batch\decode	Bus	CIF30	512	352 288 30	2>>dat\Bus.txt
