TAppEncoder -c cfg/encoder_randomaccess_main.cfg -c cfg/per-sequence-svc/BasketballDrive-2x.cfg -c cfg/layers.cfg -q0 22 -q1 22 -b str/BasketballDrive.bin -o0 rec/BasketballDrive_l0_rec.yuv -o1 rec/BasketballDrive_l1_rec.yuv

TAppDecoder -b str/BasketballDrive.bin -ls 2 -o0 rec/BasketballDrive_l0_drec.yuv -o1 rec/BasketballDrive_l1_drec.yuv

For AVC base layer tests the following should be used:
 cfg/layers_avcbase.cfg configuration file
 -ibl <BLrecon.yuv> option to specify the reconstructed base layer input
