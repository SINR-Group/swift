
#ifndef __IMG_PROCESS_MUX_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
#define __IMG_PROCESS_MUX_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79

#include "H264AVCEncoderLibTest.h"

#if DOLBY_ENCMUX_ENABLE
typedef UChar imgpel;
typedef struct img_process_filter_1D
{
  short coef1[16];
  short *c_coef1;
  int c_tap;
  int c_normal1;
  int c_taps_div2;
  int max_pel_value;
  int c_offset;
  int symmetric;
} ImgProcessFilter1D;

static inline int iClip1(int high, int x)
{
  x = gMax(x, 0);
  x = gMin(x, high);

  return x;
}

static inline int shift_off_sf(int x, int o, int a)
{
  return ((x + o) >> a);
}

ImgProcessFilter1D * create_img_process_filter_1D( int filter_type);
void destroy_img_process_filter_1D( ImgProcessFilter1D * ptr );

#endif

#endif //__H264AVCENCODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
