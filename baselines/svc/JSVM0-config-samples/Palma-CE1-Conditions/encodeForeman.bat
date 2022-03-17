
@bin\H264AVCEncoderLibTestStatic -pf cfg\Foreman.cfg -numl 1 -mfile 0 2 mot\Foreman_layer0.mot -anafgs 0 3    tmp\Foreman_FGS0.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Foreman.cfg -numl 2 -mfile 1 2 mot\Foreman_layer1.mot -encfgs 0 80.0 tmp\Foreman_FGS0.dat -anafgs 1 3     tmp\Foreman_FGS1.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Foreman.cfg -numl 2                                   -encfgs 0 80.0 tmp\Foreman_FGS0.dat -encfgs 1 256.0 tmp\Foreman_FGS1.dat

@CALL extractForeman.bat
@CALL decodeForeman.bat
