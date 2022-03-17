
@bin\FixedQpEncoderStatic rc_cfg\Bus.cfg 1>log\Bus.txt

@bin\BitStreamExtractorStatic    str\Bus.264              substr\Bus_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Bus_layer4.264       substr\Bus_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Bus_layer3.264       substr\Bus_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Bus_layer2.264       substr\Bus_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Bus_layer1.264       substr\Bus_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Bus_layer4.264       rec\Bus_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Bus_layer3.264       rec\Bus_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Bus_layer2.264       rec\Bus_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Bus_layer1.264       rec\Bus_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Bus_layer0.264       rec\Bus_layer0.yuv

@bin\PSNRStatic 352 288 ..\orig\Bus_CIF30.yuv         rec\Bus_layer4.yuv      0 0 0 substr\Bus_layer4.264      30  2> dat\Bus.txt
@bin\PSNRStatic 352 288 ..\orig\Bus_CIF15.yuv         rec\Bus_layer3.yuv      0 0 0 substr\Bus_layer3.264      15  2>>dat\Bus.txt
@bin\PSNRStatic 352 288 ..\orig\Bus_CIF15.yuv         rec\Bus_layer2.yuv      0 0 0 substr\Bus_layer2.264      15  2>>dat\Bus.txt
@bin\PSNRStatic 176 144 ..\orig\Bus_QCIF15.yuv        rec\Bus_layer1.yuv      0 0 0 substr\Bus_layer1.264      15  2>>dat\Bus.txt
@bin\PSNRStatic 176 144 ..\orig\Bus_QCIF7.5.yuv       rec\Bus_layer0.yuv      0 0 0 substr\Bus_layer0.264      7.5 2>>dat\Bus.txt
