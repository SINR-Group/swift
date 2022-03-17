/*!
*************************************************************************************
* \file img_process_mux_tab.c
*
* \brief
*    Input data Image Processing functions
*
* \author
*    Main contributors
*      - Yuwen He                <yhe@dolby.com>
*      - Alexis Michael Tourapis <alexismt@ieee.org> 
*      - Athanasios Leontaris    <aleon@dolby.com>
*      - Peshala Pahalawatta     <ppaha@dolby.com>
*************************************************************************************
*/

#include <stdio.h>
#include "H264AVCEncoderLibTest.h"
#include "H264AVCEncoderTest.h"
#include "img_process_mux.h"


#if DOLBY_ENCMUX_ENABLE

static const int tab_filter_taps[17][14] = {
  {  1}, //{  16,  16},                   //Bilinear
  {  1,    6,    1},              // horizontal
  { -1,    2,    6,    2,   -1},  // horizontal_lp
  { -1,    2,   -1},              //hp;
  { -1,    4,   26,    4,   -1},  //mp;
  {  1,   -2,    5,   56,    5,   -2,    1},  //gp;
  {-10,  -18,   45,  -12, -118,  286,  678,  286,  -118,   -12,    45,   -18,   -10},  //HF1;
  { -5,   19,   29,  -68,  -47,  305,  558,  305,   -47,   -68,    29,    19,    -5},  //HF0;
  {  2,    0,   -4,   -3,    5,   19,   26,   19,     5,    -3,    -4,     0,     2},  //SVC;
  { 27,  -17,  -80,  273,  618,  273,  -80,  -17,    27},    //CDF9;
  { 93,  -59, -605, 1142, -605,  -59, 93},  //CDF7;
  {-8,   -14,   34,   -9,  -89,  215,  765,  215,   -89,    -9,    34,    -14,   -8}, // /1024
  {  5,    8,   13,   38,   38,   13,    8,    5},                                         //bilinear;
  {  1,   -6,   -5,    7,   26,   41,   41,   26,     7,    -5,    -6,     1},             //JVT-R006;
  {  1,    1,   -2,   -4,    1,   12,   23,   23,    12,     1,    -4,    -2,     1,    1}, //JVT-R070;
  { -8,    0,   24,   48,   48,   24,    0,   -8}, // SVC-fractional
  {  7,   24,  -19,  -57,  128,  429,  429,  128,   -57,   -19,    24,     7} // HF0-fractional
};

static const int tab_filter_length[17] = { 
  1/*2*/, 3, 5, 3, 5, 7, 13, 13, 13, 9, 7, 13, 8, 12, 14, 8, 12
};

static const int tab_filter_normal[17] = { 
  0/*5*/,  3,  3,  2, 5, 6,  10,  10,  6,  10, 10/*6*/, 10, 7,  7,  6,  7, 10
};


static const int tab_filter_offset[17] = { 
  0/*16*/,  4,  4,  (512+2), 16, 32,  512, 512, 32, 512, (128*1024+512)/*512*/, 512, 64, 64, 32, 64, 512
};


//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
static inline imgpel filter_safe_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int stride_in_imgpel )
{
  const short *c_coef = flt->coef1;
  int htap = flt->c_tap;
  const imgpel *p1 = p_in - flt->c_taps_div2*stride_in_imgpel;

  int val = 0, k;

  for (k = 0; k < htap ; k++)
  { 
    val += *p1 * *(c_coef++);
    p1 += stride_in_imgpel;
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_unsafe_up_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int up_samples, int stride_in_imgpel  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - up_samples*stride_in_imgpel;

  int val = 0, k;

  for (k = hmin; k < -up_samples ; k++)
  { 
    val +=  *p1 * *(c_coef++);
  }
  for (k = -up_samples; k < hmax ; k++)
  { 
    val += *p1 * *(c_coef++);
    p1 += stride_in_imgpel;
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_unsafe_down_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int down_samples, int stride_in_imgpel  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2;// + 1;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - (flt->c_taps_div2 +1)*stride_in_imgpel;

  int val = 0, k;

  for (k = hmin; k <= down_samples ; k++)
  { 
    p1 += stride_in_imgpel;
    val +=  *p1 * *(c_coef++);
  }
  for (k = down_samples + 1; k < hmax ; k++)
  { 
    val += *p1 * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_even_unsafe_down_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int down_samples, int stride_in_imgpel  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2;// + 1;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - (flt->c_taps_div2 +1)*stride_in_imgpel;

  int val = 0, k;

  for (k = hmin; k < down_samples ; k++)
  { 
    p1 += stride_in_imgpel;
    val +=  *p1 * *(c_coef++);
  }
  for (k = down_samples; k < hmax ; k++)
  { 
    val += *p1 * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

void img_process_filter_vert_line(const ImgProcessFilter1D *flt, const imgpel *p_in, imgpel *p_out, int origin_x, int end_x, int center_start_y, 
                                 int center_end_y, int step, int y, int stride_in, int height_in)
{
  int i;  

  if(y < center_start_y)
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_unsafe_up_positions( flt, p_in, y, stride_in);
      p_in += step;
    }
  }
  else if(y < center_end_y)
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_safe_positions( flt, p_in, stride_in);
      p_in += step;
    }
  }
  else
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_unsafe_down_positions( flt, p_in, height_in-y-1, stride_in);
      p_in += step;
    }
  }
}

static void img_process_filter_even_vert_line(const ImgProcessFilter1D *flt, const imgpel *p_in, imgpel *p_out, int origin_x, int end_x, int center_start_y, 
                                 int center_end_y, int step, int y, int stride_in, int height_in)
{
  int i;  

  if(y < center_start_y)
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_unsafe_up_positions( flt, p_in, y, stride_in);
      p_in += step;
    }
  }
  else if(y < center_end_y)
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_safe_positions( flt, p_in, stride_in);
      p_in += step;
    }
  }
  else
  {
    for(i = origin_x; i < end_x; i += step)
    {
      *(p_out++) = filter_even_unsafe_down_positions( flt, p_in, height_in-y-1, stride_in);
      p_in += step;
    }
  }
}

//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
static void TaBBasic(imgpel *output, int iStrideOut, imgpel *input0, imgpel *input1, int iStrideIn, int width, int height, int offset0, int offset1)
{
  int j, jj = offset0;
  int hheight = (height >> 1);

  for (j = 0; j < hheight; j++)
  {    
    memcpy(output+j*iStrideOut, input0+jj*iStrideIn, width * sizeof(imgpel));
    jj+=2;
  }
  jj = offset1;
  for (j = hheight; j < height; j++)
  {    
    memcpy(output+j*iStrideOut, input1+jj*iStrideIn, width * sizeof(imgpel));
    jj+=2;
  }

}

void H264AVCEncoderTest::tabMux(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height, Int offset0, Int offset1, Int iFilterIdx)
{
  int j, jj = offset0;
  int hheight = (height >> 1);  

  if ( iFilterIdx > 11 )
  {
    ImgProcessFilter1D *flt = create_img_process_filter_1D(iFilterIdx);
    for (j = 0; j < hheight; j++)
    { 
      img_process_filter_even_vert_line( flt, input0+jj*iStrideIn, output+j*iStrideOut, 0, width, flt->c_taps_div2, height - flt->c_taps_div2 + offset0, 1, jj, iStrideIn, height + offset0 );
      jj += 2;
    }
    jj = offset1;
    for (j = hheight; j < (hheight<<1); j++)
    { 
      img_process_filter_even_vert_line( flt, input1+jj*iStrideIn, output+j*iStrideOut, 0, width, flt->c_taps_div2, height - flt->c_taps_div2 + offset1, 1, jj, iStrideIn, height + offset1 );
      jj += 2;
    }
    for(j=(hheight<<1); j<height; j++)
    {
      memcpy(output+j*iStrideOut, output+((hheight<<1)-1)*iStrideOut, width*sizeof(imgpel));
    }
    destroy_img_process_filter_1D(flt);
  }
  else if (iFilterIdx > 0)
  {
    ImgProcessFilter1D *flt = create_img_process_filter_1D(iFilterIdx);
    for (j = 0; j < hheight; j++)
    { 
      img_process_filter_vert_line(flt, input0+jj*iStrideIn, output+j*iStrideOut, 0, width, flt->c_taps_div2, height-flt->c_taps_div2, 1, jj, iStrideIn, height);
      jj += 2;
    }
    jj = offset1;
    for (j = hheight; j < (hheight<<1); j++)
    { 
      img_process_filter_vert_line(flt, input1+jj*iStrideIn, output+j*iStrideOut, 0, width, flt->c_taps_div2, height-flt->c_taps_div2, 1, jj, iStrideIn, height);
      jj += 2;
    }
    for(j=(hheight<<1); j<height; j++)
    {
      memcpy(output+j*iStrideOut, output+((hheight<<1)-1)*iStrideOut, width*sizeof(imgpel));
    }
    destroy_img_process_filter_1D(flt);
  }
  else
  {
    TaBBasic(output, iStrideOut, input0, input1, iStrideIn, width, height, offset0, offset1);
  }
}

void H264AVCEncoderTest::tabMuxFR(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height)
{
  int j;
  UChar *pDst = output;
  for (j = 0; j < height; j++)
  {
    memcpy(pDst, input0 +j*iStrideIn, width*sizeof(UChar));
    pDst += iStrideOut;
  }
  for (j = 0; j < height; j++)
  {
    memcpy(pDst, input1 +j*iStrideIn, width*sizeof(UChar));
    pDst += iStrideOut;
  }  
}



#endif
