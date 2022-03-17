
@bin\H264AVCEncoderLibTestStatic -pf cfg\Soccer.cfg -numl 1 -mfile 0 2 mot\Soccer_layer0.mot -anafgs 0 3     tmp\Soccer_FGS0.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Soccer.cfg -numl 2 -mfile 1 2 mot\Soccer_layer1.mot -encfgs 0 192.0 tmp\Soccer_FGS0.dat -anafgs 1 3     tmp\Soccer_FGS1.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Soccer.cfg -numl 3 -mfile 2 2 mot\Soccer_layer2.mot -encfgs 0 192.0 tmp\Soccer_FGS0.dat -encfgs 1 768.0 tmp\Soccer_FGS1.dat -anafgs 2 3      tmp\Soccer_FGS2.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Soccer.cfg -numl 3                                  -encfgs 0 192.0 tmp\Soccer_FGS0.dat -encfgs 1 768.0 tmp\Soccer_FGS1.dat -encfgs 2 3072.0 tmp\Soccer_FGS2.dat

@CALL extractSoccer.bat
@CALL decodeSoccer.bat
