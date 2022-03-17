#!/bin/sh

bin/H264AVCEncoderLibTestStatic -pf cfg/FOOTBALL.cfg -numl 1 -mfile 0 2 mot/FOOTBALL_layer0.mot -anafgs 0 3     tmp/FOOTBALL_FGS0.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/FOOTBALL.cfg -numl 2 -mfile 1 2 mot/FOOTBALL_layer1.mot -encfgs 0 256.0 tmp/FOOTBALL_FGS0.dat -anafgs 1 3      tmp/FOOTBALL_FGS1.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/FOOTBALL.cfg -numl 2                                    -encfgs 0 256.0 tmp/FOOTBALL_FGS0.dat -encfgs 1 1024.0 tmp/FOOTBALL_FGS1.dat

./extractFOOTBALL.sh
./decodeFOOTBALL.sh
