
@bin\FixedQpEncoderStatic rc_cfg\Mobile.cfg 1>log\Mobile.txt

@bin\BitStreamExtractorStatic    str\Mobile.264              substr\Mobile_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Mobile_layer4.264       substr\Mobile_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Mobile_layer3.264       substr\Mobile_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Mobile_layer2.264       substr\Mobile_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Mobile_layer1.264       substr\Mobile_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Mobile_layer4.264       rec\Mobile_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Mobile_layer3.264       rec\Mobile_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Mobile_layer2.264       rec\Mobile_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Mobile_layer1.264       rec\Mobile_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Mobile_layer0.264       rec\Mobile_layer0.yuv

@bin\PSNRStatic 352 288 ..\orig\Mobile_CIF30.yuv         rec\Mobile_layer4.yuv      0 0 0 substr\Mobile_layer4.264      30  2> dat\Mobile.txt
@bin\PSNRStatic 352 288 ..\orig\Mobile_CIF15.yuv         rec\Mobile_layer3.yuv      0 0 0 substr\Mobile_layer3.264      15  2>>dat\Mobile.txt
@bin\PSNRStatic 352 288 ..\orig\Mobile_CIF15.yuv         rec\Mobile_layer2.yuv      0 0 0 substr\Mobile_layer2.264      15  2>>dat\Mobile.txt
@bin\PSNRStatic 176 144 ..\orig\Mobile_QCIF15.yuv        rec\Mobile_layer1.yuv      0 0 0 substr\Mobile_layer1.264      15  2>>dat\Mobile.txt
@bin\PSNRStatic 176 144 ..\orig\Mobile_QCIF7.5.yuv       rec\Mobile_layer0.yuv      0 0 0 substr\Mobile_layer0.264      7.5 2>>dat\Mobile.txt
