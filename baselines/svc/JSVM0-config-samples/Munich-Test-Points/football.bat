
@bin\FixedQpEncoderStatic rc_cfg\Football.cfg 1>log\Football.txt

@bin\BitStreamExtractorStatic    str\Football.264              substr\Football_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Football_layer4.264       substr\Football_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Football_layer3.264       substr\Football_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Football_layer2.264       substr\Football_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Football_layer1.264       substr\Football_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Football_layer4.264       rec\Football_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Football_layer3.264       rec\Football_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Football_layer2.264       rec\Football_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Football_layer1.264       rec\Football_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Football_layer0.264       rec\Football_layer0.yuv

@bin\PSNRStatic 352 288 ..\orig\Football_CIF30.yuv         rec\Football_layer4.yuv      0 0 0 substr\Football_layer4.264      30  2> dat\Football.txt
@bin\PSNRStatic 352 288 ..\orig\Football_CIF15.yuv         rec\Football_layer3.yuv      0 0 0 substr\Football_layer3.264      15  2>>dat\Football.txt
@bin\PSNRStatic 352 288 ..\orig\Football_CIF15.yuv         rec\Football_layer2.yuv      0 0 0 substr\Football_layer2.264      15  2>>dat\Football.txt
@bin\PSNRStatic 176 144 ..\orig\Football_QCIF15.yuv        rec\Football_layer1.yuv      0 0 0 substr\Football_layer1.264      15  2>>dat\Football.txt
@bin\PSNRStatic 176 144 ..\orig\Football_QCIF7.5.yuv       rec\Football_layer0.yuv      0 0 0 substr\Football_layer0.264      7.5 2>>dat\Football.txt
