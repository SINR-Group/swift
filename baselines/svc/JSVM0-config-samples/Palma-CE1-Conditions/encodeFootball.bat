
@bin\H264AVCEncoderLibTestStatic -pf cfg\Football.cfg -numl 1 -mfile 0 2 mot\Football_layer0.mot -anafgs 0 3     tmp\Football_FGS0.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Football.cfg -numl 2 -mfile 1 2 mot\Football_layer1.mot -encfgs 0 256.0 tmp\Football_FGS0.dat -anafgs 1 3      tmp\Football_FGS1.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Football.cfg -numl 2                                    -encfgs 0 256.0 tmp\Football_FGS0.dat -encfgs 1 1024.0 tmp\Football_FGS1.dat

@CALL extractFootball.bat
@CALL decodeFootball.bat
