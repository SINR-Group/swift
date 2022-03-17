
@bin\FixedQpEncoderStatic rc_cfg\City.cfg 1>log\City.txt

@bin\BitStreamExtractorStatic    str\City.264              substr\City_layer5.264      -l 5
@bin\BitStreamExtractorStatic substr\City_layer5.264       substr\City_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\City_layer4.264       substr\City_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\City_layer3.264       substr\City_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\City_layer2.264       substr\City_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\City_layer1.264       substr\City_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\City_layer5.264       rec\City_layer5.yuv
@bin\H264AVCDecoderLibTestStatic substr\City_layer4.264       rec\City_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\City_layer3.264       rec\City_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\City_layer2.264       rec\City_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\City_layer1.264       rec\City_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\City_layer0.264       rec\City_layer0.yuv

@bin\PSNRStatic 704 576 ..\orig\City_4CIF60.yuv       rec\City_layer5.yuv     0 0 0 substr\City_layer5.264     60  2> dat\City.txt
@bin\PSNRStatic 704 576 ..\orig\City_4CIF30.yuv       rec\City_layer4.yuv     0 0 0 substr\City_layer4.264     30  2>>dat\City.txt
@bin\PSNRStatic 352 288 ..\orig\City_CIF30.yuv        rec\City_layer3.yuv     0 0 0 substr\City_layer3.264     30  2>>dat\City.txt
@bin\PSNRStatic 352 288 ..\orig\City_CIF30.yuv        rec\City_layer2.yuv     0 0 0 substr\City_layer2.264     30  2>>dat\City.txt
@bin\PSNRStatic 176 144 ..\orig\City_QCIF15.yuv       rec\City_layer1.yuv     0 0 0 substr\City_layer1.264     15  2>>dat\City.txt
@bin\PSNRStatic 176 144 ..\orig\City_QCIF15.yuv       rec\City_layer0.yuv     0 0 0 substr\City_layer0.264     15  2>>dat\City.txt
