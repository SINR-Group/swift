
@bin\H264AVCEncoderLibTestStatic -pf cfg\Bus.cfg -numl 1 -mfile 0 2 mot\Bus_layer0.mot -anafgs 0 3     tmp\Bus_FGS0.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Bus.cfg -numl 2 -mfile 1 2 mot\Bus_layer1.mot -encfgs 0 128.0 tmp\Bus_FGS0.dat -anafgs 1 3     tmp\Bus_FGS1.dat
@bin\H264AVCEncoderLibTestStatic -pf cfg\Bus.cfg -numl 2                               -encfgs 0 128.0 tmp\Bus_FGS0.dat -encfgs 1 512.0 tmp\Bus_FGS1.dat

@CALL extractBus.bat
@CALL decodeBus.bat
