
@set DIR=..\jsvm\bin\
@set ENC=%DIR%\H264AVCEncoderLibTestStatic.exe
@set DEC=%DIR%\H264AVCDecoderLibTestStatic.exe
@set CMP=%DIR%\YUVCompareStatic.exe

::===== encode =====
@%ENC% -pf MVC.cfg

::===== decode =====
@%DEC% test.264 dec.yuv

::===== check encoder/decoder match
@%CMP% 176 144 dec.yuv enc.yuv 1
