#!/bin/sh

bin/H264AVCEncoderLibTestStatic -pf cfg/CREW.cfg -numl 1 -mfile 0 2 mot/CREW_layer0.mot -anafgs 0 3     tmp/CREW_FGS0.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/CREW.cfg -numl 2 -mfile 1 2 mot/CREW_layer1.mot -encfgs 0 192.0 tmp/CREW_FGS0.dat -anafgs 1 3     tmp/CREW_FGS1.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/CREW.cfg -numl 3 -mfile 2 2 mot/CREW_layer2.mot -encfgs 0 192.0 tmp/CREW_FGS0.dat -encfgs 1 768.0 tmp/CREW_FGS1.dat -anafgs 2 3      tmp/CREW_FGS2.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/CREW.cfg -numl 3                                -encfgs 0 192.0 tmp/CREW_FGS0.dat -encfgs 1 768.0 tmp/CREW_FGS1.dat -encfgs 2 3072.0 tmp/CREW_FGS2.dat

./extractCREW.sh
./decodeCREW.sh
