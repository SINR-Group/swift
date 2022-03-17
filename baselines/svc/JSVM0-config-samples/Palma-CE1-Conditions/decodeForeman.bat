
@echo QCIF7.5                                                     >dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF7.5	 32       176 144 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF7.5	 40       176 144 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF7.5	 48       176 144 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF7.5	 56       176 144 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF7.5	 64       176 144 7.5	2>>dat\Foreman.txt
					
@echo QCIF15                                                  >>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF15	 48       176 144 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF15	 56       176 144 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF15	 64       176 144 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF15	 80       176 144 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	QCIF15	 96       176 144 15	2>>dat\Foreman.txt
					
@echo CIF7.5                                                  >>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF7.5	 64       352 288 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF7.5	 80       352 288 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF7.5	 96       352 288 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF7.5	 112      352 288 7.5	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF7.5	 128      352 288 7.5	2>>dat\Foreman.txt
					
@echo CIF15                                                   >>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF15	 96    	  352 288 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF15	 112      352 288 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF15	 128      352 288 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF15	 160      352 288 15	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF15	 192      352 288 15	2>>dat\Foreman.txt
					
@echo CIF30                                                   >>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF30	 128      352 288 30	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF30	 160      352 288 30	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF30	 192      352 288 30	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF30	 224      352 288 30	2>>dat\Foreman.txt
@CALL batch\decode	Foreman	CIF30	 256      352 288 30	2>>dat\Foreman.txt


