
@bin\FixedQpEncoderStatic rc_cfg\Harbour.cfg 1>log\Harbour.txt

@bin\BitStreamExtractorStatic    str\Harbour.264              substr\Harbour_layer5.264      -l 5
@bin\BitStreamExtractorStatic substr\Harbour_layer5.264       substr\Harbour_layer4.264      -l 4
@bin\BitStreamExtractorStatic substr\Harbour_layer4.264       substr\Harbour_layer3.264      -l 3
@bin\BitStreamExtractorStatic substr\Harbour_layer3.264       substr\Harbour_layer2.264      -l 2
@bin\BitStreamExtractorStatic substr\Harbour_layer2.264       substr\Harbour_layer1.264      -l 1
@bin\BitStreamExtractorStatic substr\Harbour_layer1.264       substr\Harbour_layer0.264      -l 0

@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer5.264       rec\Harbour_layer5.yuv
@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer4.264       rec\Harbour_layer4.yuv
@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer3.264       rec\Harbour_layer3.yuv
@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer2.264       rec\Harbour_layer2.yuv
@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer1.264       rec\Harbour_layer1.yuv
@bin\H264AVCDecoderLibTestStatic substr\Harbour_layer0.264       rec\Harbour_layer0.yuv

@bin\PSNRStatic 704 576 ..\orig\Harbour_4CIF60.yuv       rec\Harbour_layer5.yuv     0 0 0 substr\Harbour_layer5.264     60  2> dat\Harbour.txt
@bin\PSNRStatic 704 576 ..\orig\Harbour_4CIF30.yuv       rec\Harbour_layer4.yuv     0 0 0 substr\Harbour_layer4.264     30  2>>dat\Harbour.txt
@bin\PSNRStatic 352 288 ..\orig\Harbour_CIF30.yuv        rec\Harbour_layer3.yuv     0 0 0 substr\Harbour_layer3.264     30  2>>dat\Harbour.txt
@bin\PSNRStatic 352 288 ..\orig\Harbour_CIF30.yuv        rec\Harbour_layer2.yuv     0 0 0 substr\Harbour_layer2.264     30  2>>dat\Harbour.txt
@bin\PSNRStatic 176 144 ..\orig\Harbour_QCIF15.yuv       rec\Harbour_layer1.yuv     0 0 0 substr\Harbour_layer1.264     15  2>>dat\Harbour.txt
@bin\PSNRStatic 176 144 ..\orig\Harbour_QCIF15.yuv       rec\Harbour_layer0.yuv     0 0 0 substr\Harbour_layer0.264     15  2>>dat\Harbour.txt
