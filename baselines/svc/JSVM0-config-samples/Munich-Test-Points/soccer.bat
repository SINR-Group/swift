
@bin\FixedQpEncoderStatic rc_cfg\Soccer.cfg 1>log\Soccer.txt

@bin\BitStreamExtractorStatic    str\Soccer.264              substr\Soccer_layer5.264      -l 5
@bin\BitStreamExtractorStatic substr\Soccer_layer5.264       substr\Soccer_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Soccer_layer4.264       substr\Soccer_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Soccer_layer3.264       substr\Soccer_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Soccer_layer2.264       substr\Soccer_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Soccer_layer1.264       substr\Soccer_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer5.264       rec\Soccer_layer5.yuv
@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer4.264       rec\Soccer_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer3.264       rec\Soccer_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer2.264       rec\Soccer_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer1.264       rec\Soccer_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Soccer_layer0.264       rec\Soccer_layer0.yuv

@bin\PSNRStatic 704 576 ..\orig\Soccer_4CIF60.yuv       rec\Soccer_layer5.yuv     0 0 0 substr\Soccer_layer5.264     60  2> dat\Soccer.txt
@bin\PSNRStatic 704 576 ..\orig\Soccer_4CIF30.yuv       rec\Soccer_layer4.yuv     0 0 0 substr\Soccer_layer4.264     30  2>>dat\Soccer.txt
@bin\PSNRStatic 352 288 ..\orig\Soccer_CIF30.yuv        rec\Soccer_layer3.yuv     0 0 0 substr\Soccer_layer3.264     30  2>>dat\Soccer.txt
@bin\PSNRStatic 352 288 ..\orig\Soccer_CIF30.yuv        rec\Soccer_layer2.yuv     0 0 0 substr\Soccer_layer2.264     30  2>>dat\Soccer.txt
@bin\PSNRStatic 176 144 ..\orig\Soccer_QCIF15.yuv       rec\Soccer_layer1.yuv     0 0 0 substr\Soccer_layer1.264     15  2>>dat\Soccer.txt
@bin\PSNRStatic 176 144 ..\orig\Soccer_QCIF15.yuv       rec\Soccer_layer0.yuv     0 0 0 substr\Soccer_layer0.264     15  2>>dat\Soccer.txt
