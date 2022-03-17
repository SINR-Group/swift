
@echo QCIF15                                           >dat\Soccer.txt
@CALL batch\decode	Soccer	QCIF15	  96   176 144 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	QCIF15	  112  176 144 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	QCIF15	  128  176 144 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	QCIF15	  160  176 144 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	QCIF15	  192  176 144 15	2>>dat\Soccer.txt
					        
@echo CIF7.5                                                 >>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF7.5	  192  352 288 7.5	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF7.5	  224  352 288 7.5	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF7.5	  256  352 288 7.5	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF7.5	  320  352 288 7.5	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF7.5	  384  352 288 7.5	2>>dat\Soccer.txt
					        
@echo CIF15                                                  >>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF15	  256  352 288 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF15	  320  352 288 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF15	  384  352 288 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF15	  448  352 288 15	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF15	  512  352 288 15	2>>dat\Soccer.txt
					        
@echo CIF30                                                  >>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF30	  384  352 288 30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF30	  448  352 288 30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF30	  512  352 288 30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF30	  640  352 288 30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	CIF30	  768  352 288 30	2>>dat\Soccer.txt
					        
@echo 4CIF15                                                        >>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF15	  768     704 576   15       2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF15	  896     704 576   15       2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF15	  1024    704 576   15       2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF15	  1280    704 576   15       2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF15	  1536    704 576   15       2>>dat\Soccer.txt
					       
@echo 4CIF30                                             >>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF30	  1024  704 576   30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF30	  1280  704 576   30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF30	  1536  704 576   30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF30	  1792  704 576   30	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF30	  2048  704 576   30	2>>dat\Soccer.txt
					       
@echo 4CIF60                                             >>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF60	  1536  704 576   60	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF60	  1780  704 576   60	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF60	  2048  704 576   60	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF60	  2560  704 576   60	2>>dat\Soccer.txt
@CALL batch\decode	Soccer	4CIF60	  3072  704 576   60	2>>dat\Soccer.txt

