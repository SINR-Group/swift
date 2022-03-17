
@echo QCIF7.5                                                     >dat\Football.txt
@CALL batch\decode	Football	QCIF7.5	128     176 144 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF7.5	160     176 144 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF7.5	192     176 144 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF7.5	224	176 144 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF7.5	256	176 144 7.5	2>>dat\Football.txt

@echo QCIF15                                                     >>dat\Football.txt
@CALL batch\decode	Football	QCIF15	192     176 144 15	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF15	224	176 144 15	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF15	256	176 144 15	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF15	320	176 144 15	2>>dat\Football.txt
@CALL batch\decode	Football	QCIF15	384	176 144 15	2>>dat\Football.txt

@echo CIF7.5                                                     >>dat\Football.txt
@CALL batch\decode	Football	CIF7.5	256	352 288 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	CIF7.5	320	352 288 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	CIF7.5	384	352 288 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	CIF7.5	448	352 288 7.5	2>>dat\Football.txt
@CALL batch\decode	Football	CIF7.5	512	352 288 7.5	2>>dat\Football.txt

@echo CIF15                                                      >>dat\Football.txt
@CALL batch\decode	Football	CIF15	384	352 288 15	2>>dat\Football.txt
@CALL batch\decode	Football	CIF15	448	352 288 15	2>>dat\Football.txt
@CALL batch\decode	Football	CIF15	512	352 288 15	2>>dat\Football.txt
@CALL batch\decode	Football	CIF15	640	352 288 15	2>>dat\Football.txt
@CALL batch\decode	Football	CIF15	768	352 288 15	2>>dat\Football.txt

@echo CIF30                                                      >>dat\Football.txt
@CALL batch\decode	Football	CIF30	512	352 288 30	2>>dat\Football.txt
@CALL batch\decode	Football	CIF30	640	352 288 30	2>>dat\Football.txt
@CALL batch\decode	Football	CIF30	768	352 288 30	2>>dat\Football.txt
@CALL batch\decode	Football	CIF30	896	352 288 30	2>>dat\Football.txt
@CALL batch\decode	Football	CIF30	1024	352 288 30	2>>dat\Football.txt
