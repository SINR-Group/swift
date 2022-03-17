
@echo QCIF15                                                      >dat\City.txt
@CALL batch\decode	City	QCIF15	64      176 144 15	2>>dat\City.txt
@CALL batch\decode	City	QCIF15	80 	176 144 15	2>>dat\City.txt
@CALL batch\decode	City	QCIF15	96 	176 144 15	2>>dat\City.txt
@CALL batch\decode	City	QCIF15	112	176 144 15	2>>dat\City.txt
@CALL batch\decode	City	QCIF15	128	176 144 15	2>>dat\City.txt

@echo CIF7.5                                                     >>dat\City.txt
@CALL batch\decode	City	CIF7.5	128	352 288 7.5	2>>dat\City.txt
@CALL batch\decode	City	CIF7.5	160	352 288 7.5	2>>dat\City.txt
@CALL batch\decode	City	CIF7.5	192	352 288 7.5	2>>dat\City.txt
@CALL batch\decode	City	CIF7.5	224	352 288 7.5	2>>dat\City.txt
@CALL batch\decode	City	CIF7.5	256	352 288 7.5	2>>dat\City.txt

@echo CIF15                                                      >>dat\City.txt
@CALL batch\decode	City	CIF15	192	352 288 15	2>>dat\City.txt
@CALL batch\decode	City	CIF15	224	352 288 15	2>>dat\City.txt
@CALL batch\decode	City	CIF15	256	352 288 15	2>>dat\City.txt
@CALL batch\decode	City	CIF15	320	352 288 15	2>>dat\City.txt
@CALL batch\decode	City	CIF15	384	352 288 15	2>>dat\City.txt

@echo CIF30                                                      >>dat\City.txt
@CALL batch\decode	City	CIF30	256	352 288 30	2>>dat\City.txt
@CALL batch\decode	City	CIF30	320	352 288 30	2>>dat\City.txt
@CALL batch\decode	City	CIF30	384	352 288 30	2>>dat\City.txt
@CALL batch\decode	City	CIF30	448	352 288 30	2>>dat\City.txt
@CALL batch\decode	City	CIF30	512	352 288 30	2>>dat\City.txt

@echo 4CIF15                                                      >>dat\City.txt
@CALL batch\decode	City	4CIF15	512     704 576   15       2>>dat\City.txt
@CALL batch\decode	City	4CIF15	640     704 576   15       2>>dat\City.txt
@CALL batch\decode	City	4CIF15	768     704 576   15       2>>dat\City.txt
@CALL batch\decode	City	4CIF15	896     704 576   15       2>>dat\City.txt
@CALL batch\decode	City	4CIF15	1024    704 576   15       2>>dat\City.txt
					           
@echo 4CIF30                                                 >>dat\City.txt
@CALL batch\decode	City	4CIF30	768     704 576   30	2>>dat\City.txt
@CALL batch\decode	City	4CIF30	896     704 576   30	2>>dat\City.txt
@CALL batch\decode	City	4CIF30	1024    704 576   30	2>>dat\City.txt
@CALL batch\decode	City	4CIF30	1280    704 576   30	2>>dat\City.txt
@CALL batch\decode	City	4CIF30	1536    704 576   30	2>>dat\City.txt
					           
@echo 4CIF60                                                 >>dat\City.txt
@CALL batch\decode	City	4CIF60	1024    704 576   60	2>>dat\City.txt
@CALL batch\decode	City	4CIF60	1280    704 576   60	2>>dat\City.txt
@CALL batch\decode	City	4CIF60	1536    704 576   60	2>>dat\City.txt
@CALL batch\decode	City	4CIF60	1792    704 576   60	2>>dat\City.txt
@CALL batch\decode	City	4CIF60	2048    704 576   60	2>>dat\City.txt


