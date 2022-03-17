
@bin\FixedQpEncoderStatic rc_cfg\Foreman.cfg 1>log\Foreman.txt

@bin\BitStreamExtractorStatic    str\Foreman.264              substr\Foreman_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Foreman_layer4.264       substr\Foreman_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Foreman_layer3.264       substr\Foreman_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Foreman_layer2.264       substr\Foreman_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Foreman_layer1.264       substr\Foreman_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Foreman_layer4.264       rec\Foreman_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Foreman_layer3.264       rec\Foreman_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Foreman_layer2.264       rec\Foreman_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Foreman_layer1.264       rec\Foreman_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Foreman_layer0.264       rec\Foreman_layer0.yuv

@bin\PSNRStatic 352 288 ..\orig\Foreman_CIF30.yuv         rec\Foreman_layer4.yuv      0 0 0 substr\Foreman_layer4.264      30  2> dat\Foreman.txt
@bin\PSNRStatic 352 288 ..\orig\Foreman_CIF15.yuv         rec\Foreman_layer3.yuv      0 0 0 substr\Foreman_layer3.264      15  2>>dat\Foreman.txt
@bin\PSNRStatic 352 288 ..\orig\Foreman_CIF15.yuv         rec\Foreman_layer2.yuv      0 0 0 substr\Foreman_layer2.264      15  2>>dat\Foreman.txt
@bin\PSNRStatic 176 144 ..\orig\Foreman_QCIF15.yuv        rec\Foreman_layer1.yuv      0 0 0 substr\Foreman_layer1.264      15  2>>dat\Foreman.txt
@bin\PSNRStatic 176 144 ..\orig\Foreman_QCIF7.5.yuv       rec\Foreman_layer0.yuv      0 0 0 substr\Foreman_layer0.264      7.5 2>>dat\Foreman.txt
