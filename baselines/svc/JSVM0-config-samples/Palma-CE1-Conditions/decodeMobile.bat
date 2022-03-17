
@echo QCIF7.5                                                     >dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF7.5	  48      176 144 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF7.5	  56      176 144 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF7.5	  64      176 144 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF7.5	  80      176 144 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF7.5	  96      176 144 7.5	2>>dat\Mobile.txt
					 
@echo QCIF15                                               >>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF15	  64      176 144 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF15	  80      176 144 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF15	  96      176 144 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF15	  112     176 144 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	QCIF15	  128     176 144 15	2>>dat\Mobile.txt
					 
@echo CIF7.5                                               >>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF7.5	  96      352 288 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF7.5	  112     352 288 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF7.5	  128     352 288 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF7.5	  160     352 288 7.5	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF7.5	  192     352 288 7.5	2>>dat\Mobile.txt
					 
@echo CIF15                                                >>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF15	  128  	  352 288 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF15	  160     352 288 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF15	  192     352 288 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF15	  224     352 288 15	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF15	  256     352 288 15	2>>dat\Mobile.txt
					 
@echo CIF30                                                >>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF30	  192     352 288 30	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF30	  224     352 288 30	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF30	  256     352 288 30	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF30	  320     352 288 30	2>>dat\Mobile.txt
@CALL batch\decode	Mobile	CIF30	  384     352 288 30	2>>dat\Mobile.txt

