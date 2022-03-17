
@set DIR=..\jsvm\bin\
@set ENC=%DIR%\H264AVCEncoderLibTestStatic.exe
@set DEC=%DIR%\H264AVCDecoderLibTestStatic.exe
@set RES=%DIR%\DownConvertStatic.exe
@set EXT=%DIR%\BitStreamExtractorStatic.exe
@set SNR=%DIR%\PSNRStatic.exe
@set CMP=%DIR%\YUVCompareStatic.exe
@set SFL=snrSVC.txt

::===== resample =====
@%RES% 704 576 org_704x576_60.yuv 352 288 org_352x288_30.yuv 0 1
@%RES% 352 288 org_352x288_30.yuv 176 144 org_176x144_15.yuv  0 1

::===== encode =====
@%ENC% -pf encoder.cfg

::===== extract =====
@%EXT% test.264 str_D0_Q0.264 -l 0 -f 0 -keepf
@%EXT% test.264 str_D0_Q1.264 -l 0 -f 1 -keepf
@%EXT% test.264 str_D1_Q0.264 -l 1 -f 0 -keepf
@%EXT% test.264 str_D1_Q1.264 -l 1 -f 1 -keepf
@%EXT% test.264 str_D2_Q0.264 -l 2 -f 0 -keepf
@%EXT% test.264 str_D2_Q1.264 -l 2 -f 1 -keepf
@%EXT% test.264 str_D2_Q2.264 -l 2 -f 2 -keepf

::===== decode =====
@%DEC% str_D0_Q0.264 dec_D0_Q0.yuv
@%DEC% str_D0_Q1.264 dec_D0_Q1.yuv
@%DEC% str_D1_Q0.264 dec_D1_Q0.yuv
@%DEC% str_D1_Q1.264 dec_D1_Q1.yuv
@%DEC% str_D2_Q0.264 dec_D2_Q0.yuv
@%DEC% str_D2_Q1.264 dec_D2_Q1.yuv
@%DEC% str_D2_Q2.264 dec_D2_Q2.yuv

::===== get PSNR =====
@%SNR% 176 144 org_176x144_15.yuv dec_D0_Q0.yuv 0 0 str_D0_Q0.264 15 D0Q0 2>  %SFL%
@%SNR% 176 144 org_176x144_15.yuv dec_D0_Q1.yuv 0 0 str_D0_Q1.264 15 D0Q1 2>> %SFL%
@%SNR% 352 288 org_352x288_30.yuv dec_D1_Q0.yuv 0 0 str_D1_Q0.264 30 D1Q0 2>> %SFL%
@%SNR% 352 288 org_352x288_30.yuv dec_D1_Q1.yuv 0 0 str_D1_Q1.264 30 D1Q1 2>> %SFL%
@%SNR% 704 576 org_704x576_60.yuv dec_D2_Q0.yuv 0 0 str_D2_Q0.264 60 D2Q0 2>> %SFL%
@%SNR% 704 576 org_704x576_60.yuv dec_D2_Q1.yuv 0 0 str_D2_Q1.264 60 D2Q1 2>> %SFL%
@%SNR% 704 576 org_704x576_60.yuv dec_D2_Q2.yuv 0 0 str_D2_Q2.264 60 D2Q2 2>> %SFL%

::===== check encoder/decoder match
@%CMP% 176 144 dec_D0_Q0.yuv enc_D0_Q0.yuv 1
@%CMP% 176 144 dec_D0_Q1.yuv enc_D0_Q1.yuv 1
@%CMP% 352 288 dec_D1_Q0.yuv enc_D1_Q0.yuv 1
@%CMP% 352 288 dec_D1_Q1.yuv enc_D1_Q1.yuv 1
@%CMP% 704 576 dec_D2_Q0.yuv enc_D2_Q0.yuv 1
@%CMP% 704 576 dec_D2_Q2.yuv enc_D2_Q2.yuv 1

::===== output PSNR file =====
@type %SFL%
