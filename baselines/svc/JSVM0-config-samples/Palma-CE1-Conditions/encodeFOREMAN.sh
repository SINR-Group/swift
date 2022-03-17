#!/bin/sh

bin/H264AVCEncoderLibTestStatic -pf cfg/FOREMAN.cfg -numl 1 -mfile 0 2 mot/FOREMAN_layer0.mot -anafgs 0 3    tmp/FOREMAN_FGS0.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/FOREMAN.cfg -numl 2 -mfile 1 2 mot/FOREMAN_layer1.mot -encfgs 0 80.0 tmp/FOREMAN_FGS0.dat -anafgs 1 3     tmp/FOREMAN_FGS1.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/FOREMAN.cfg -numl 2                                   -encfgs 0 80.0 tmp/FOREMAN_FGS0.dat -encfgs 1 256.0 tmp/FOREMAN_FGS1.dat

./extractFOREMAN.sh
./decodeFOREMAN.sh
