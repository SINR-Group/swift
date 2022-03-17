#!/bin/sh
# (1:sequence) (2:label) (3:rate) (4:width) (5:height) (6:fps)

bin/H264AVCDecoderLibTestStatic str/$1_$2_$3.264 rec/$1_$2_$3.yuv
bin/PSNRStatic $4 $5 ../orig/$1_$2.yuv rec/$1_$2_$3.yuv 0 0 0 str/$1_$2_$3.264 $6
