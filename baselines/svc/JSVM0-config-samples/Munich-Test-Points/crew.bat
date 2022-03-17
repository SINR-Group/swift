
@bin\FixedQpEncoderStatic rc_cfg\Crew.cfg 1>log\Crew.txt

@bin\BitStreamExtractorStatic    str\Crew.264              substr\Crew_layer5.264      -l 5
@bin\BitStreamExtractorStatic substr\Crew_layer5.264       substr\Crew_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Crew_layer4.264       substr\Crew_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Crew_layer3.264       substr\Crew_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Crew_layer2.264       substr\Crew_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Crew_layer1.264       substr\Crew_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Crew_layer5.264       rec\Crew_layer5.yuv
@bin\H264AVCDecoderLibTestStatic substr\Crew_layer4.264       rec\Crew_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Crew_layer3.264       rec\Crew_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Crew_layer2.264       rec\Crew_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Crew_layer1.264       rec\Crew_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Crew_layer0.264       rec\Crew_layer0.yuv

@bin\PSNRStatic 704 576 ..\orig\Crew_4CIF60.yuv       rec\Crew_layer5.yuv     0 0 0 substr\Crew_layer5.264     60  2> dat\Crew.txt
@bin\PSNRStatic 704 576 ..\orig\Crew_4CIF30.yuv       rec\Crew_layer4.yuv     0 0 0 substr\Crew_layer4.264     30  2>>dat\Crew.txt
@bin\PSNRStatic 352 288 ..\orig\Crew_CIF30.yuv        rec\Crew_layer3.yuv     0 0 0 substr\Crew_layer3.264     30  2>>dat\Crew.txt
@bin\PSNRStatic 352 288 ..\orig\Crew_CIF30.yuv        rec\Crew_layer2.yuv     0 0 0 substr\Crew_layer2.264     30  2>>dat\Crew.txt
@bin\PSNRStatic 176 144 ..\orig\Crew_QCIF15.yuv       rec\Crew_layer1.yuv     0 0 0 substr\Crew_layer1.264     15  2>>dat\Crew.txt
@bin\PSNRStatic 176 144 ..\orig\Crew_QCIF15.yuv       rec\Crew_layer0.yuv     0 0 0 substr\Crew_layer0.264     15  2>>dat\Crew.txt
