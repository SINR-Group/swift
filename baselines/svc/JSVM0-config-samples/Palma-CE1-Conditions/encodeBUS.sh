#!/bin/sh

bin/H264AVCEncoderLibTestStatic -pf cfg/BUS.cfg -numl 1 -mfile 0 2 mot/BUS_layer0.mot -anafgs 0 3     tmp/BUS_FGS0.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/BUS.cfg -numl 2 -mfile 1 2 mot/BUS_layer1.mot -encfgs 0 128.0 tmp/BUS_FGS0.dat -anafgs 1 3     tmp/BUS_FGS1.dat
bin/H264AVCEncoderLibTestStatic -pf cfg/BUS.cfg -numl 2                               -encfgs 0 128.0 tmp/BUS_FGS0.dat -encfgs 1 512.0 tmp/BUS_FGS1.dat

./extractBUS.sh
./decodeBUS.sh
