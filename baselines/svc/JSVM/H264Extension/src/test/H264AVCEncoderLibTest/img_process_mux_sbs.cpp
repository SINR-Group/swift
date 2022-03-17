/*!
*************************************************************************************
* \file img_process_mux_sbs.c
*
* \brief
*    Side by Side frame compatible format generation functions
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

static const int sbs_filter_taps[17][14] = {
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

static const int sbs_filter_length[17] = { 
  1/*2*/, 3, 5, 3, 5, 7, 13, 13, 13, 9, 7, 13, 8, 12, 14, 8, 12
};

static const int sbs_filter_normal[17] = { 
  0/*5*/,  3,  3,  2, 5, 6,  10,  10,  6,  10, 10/*6*/, 10, 7,  7,  6,  7, 10
};


static const int sbs_filter_offset[17] = { 
  0/*16*/,  4,  4,  (512+2), 16, 32,  512, 512, 32, 512, (128*1024+512)/*512*/, 512, 64, 64, 32, 64, 512
};


//-----------------------------------------------------------------------------
// Private methods
//-----------------------------------------------------------------------------
ImgProcessFilter1D * create_img_process_filter_1D( int filter_type)
{
  int idx;
  ImgProcessFilter1D * ptr;

  ptr = (ImgProcessFilter1D *)malloc( sizeof( ImgProcessFilter1D ) );
  if ( ptr == NULL )
  {
    fprintf(stderr, "Cannot allocate memory for RPUFilter1D structure.\n" );
    exit(100);
  }

  ptr->c_tap       = sbs_filter_length[filter_type];  
  ptr->c_taps_div2 = ptr->c_tap >> 1;

  for (idx = 0; idx < ptr->c_tap; idx++)
  {
    ptr->coef1[idx] = sbs_filter_taps[filter_type][idx];
  }
  ptr->c_coef1 = &ptr->coef1[ptr->c_taps_div2 - 1];

  ptr->c_normal1  = sbs_filter_normal[filter_type];
  ptr->c_offset   = sbs_filter_offset[filter_type];

  ptr->max_pel_value = 255;
  return ptr;
}

void destroy_img_process_filter_1D( ImgProcessFilter1D * ptr )
{
  if ( ptr != NULL )
  {
    free ( ptr );
  }
}

static inline imgpel filter_safe_positions( const ImgProcessFilter1D *flt, const imgpel *p_in )
{
  const short *c_coef = flt->coef1;
  int htap = flt->c_tap;
  const imgpel *p1 = p_in - flt->c_taps_div2;// + 1; //bug;

  int val = 0, k;

  for (k = 0; k < htap ; k++)
  { 
    val += *(p1++) * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_unsafe_left_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int left_samples  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2; // + 1;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - left_samples; // + 1; //bug???

  int val = 0, k;

  for (k = hmin; k < -left_samples ; k++)
  { 
    val +=  *p1 * *(c_coef++);
  }
  for (k = -left_samples; k < hmax ; k++)
  { 
    val += *(p1++) * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_unsafe_right_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int right_samples  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2;// + 1;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - flt->c_taps_div2 -1;

  int val = 0, k;

  for (k = hmin; k <= right_samples ; k++)
  { 
    val +=  *(++p1) * *(c_coef++);
  }
  for (k = right_samples + 1; k < hmax ; k++)
  { 
    val += *p1 * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static inline imgpel filter_even_unsafe_right_positions( const ImgProcessFilter1D *flt, const imgpel *p_in, int right_samples  )
{
  const short *c_coef = flt->coef1;
  int hmin = -flt->c_taps_div2;
  int hmax = flt->c_tap + hmin;
  const imgpel *p1 = p_in - flt->c_taps_div2 -1;

  int val = 0, k;

  for (k = hmin; k < right_samples ; k++)
  { 
    val +=  *(++p1) * *(c_coef++);
  }
  for (k = right_samples; k < hmax ; k++)
  { 
    val += *p1 * *(c_coef++);
  }
  return (imgpel)iClip1( flt->max_pel_value, shift_off_sf( val, flt->c_offset, flt->c_normal1 ) );
}

static void img_process_filter_even_hor_line(const ImgProcessFilter1D *flt, const imgpel *p_in, imgpel *p_out, int origin_x, int end_x, int center_start_x, 
                                 int center_end_x, int step)
{
  int i;  

  // left columns
  for ( i = origin_x; i < center_start_x; i += step)
  {
    *(p_out++) = filter_unsafe_left_positions( flt, p_in, i );
    p_in += step;
  }
  // center columns
  for ( /*i = center_start_x*/; i < center_end_x; i += step)
  {
    *(p_out++) = filter_safe_positions( flt, p_in );
    p_in += step;
  }
  // right columns
  for ( /*i = center_end_x*/; i < end_x; i += step )
  {
    *(p_out++) = filter_even_unsafe_right_positions( flt, p_in, end_x - i - 1 );
    p_in += step;
  }
}

void img_process_filter_hor_line(const ImgProcessFilter1D *flt, const imgpel *p_in, imgpel *p_out, int origin_x, int end_x, int center_start_x, 
                                 int center_end_x, int step)
{
  int i;  

  // left columns
  for ( i = origin_x; i < center_start_x; i += step)
  {
    *(p_out++) = filter_unsafe_left_positions( flt, p_in, i );
    p_in += step;
  }
  // center columns
  for ( /*i = center_start_x*/; i < center_end_x; i += step)
  {
    *(p_out++) = filter_safe_positions( flt, p_in );
    p_in += step;
  }
  // right columns
  for ( /*i = center_end_x*/; i < end_x; i += step )
  {
    *(p_out++) = filter_unsafe_right_positions( flt, p_in, end_x-i-1 );
    p_in += step;
  }
}

static void SbSBasic(imgpel *output, int iStrideOut, imgpel *input0, imgpel *input1, int iStrideIn, int width, int height, int offset0, int offset1)
{
  int i, j;
  int hwidth = (width >> 1);
  imgpel *p_out;
  imgpel *p_in0, *p_in1;

  for (j = 0; j < height; j++)
  {     
    p_out = output + j*iStrideOut;
    p_in0 = input0 + j*iStrideIn + offset0;
    for (i = 0; i < hwidth; i++)
    {
      *(p_out++) = *p_in0;
      p_in0 += 2;
    }
    p_in1 = input1 + j*iStrideIn +offset1;
    for (i = hwidth; i < width; i++)
    {
      *(p_out++) = *p_in1;
      p_in1 += 2;
    }
  }
}

void H264AVCEncoderTest::sbsMux(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height, Int offset0, Int offset1, Int iFilterIdx)
{
  int j;
  int hwidth = (width >> 1);
  if ( iFilterIdx > 11 )
  {
    ImgProcessFilter1D *flt = create_img_process_filter_1D(iFilterIdx);
    for (j = 0; j < height; j++)
    {
      img_process_filter_even_hor_line( flt, input0+j*iStrideIn +offset0, output+j*iStrideOut,        offset0, width + offset0, flt->c_taps_div2, width - flt->c_taps_div2 + offset0, 2 );
      img_process_filter_even_hor_line( flt, input1+j*iStrideIn +offset1, output+j*iStrideOut+hwidth, offset1, width + offset1, flt->c_taps_div2, width - flt->c_taps_div2 + offset1, 2 );
    }
    destroy_img_process_filter_1D(flt);
  }
  else if ( iFilterIdx > 0 )
  {
    ImgProcessFilter1D *flt = create_img_process_filter_1D(iFilterIdx);
    for (j = 0; j < height; j++)
    {
      img_process_filter_hor_line(flt, input0+j*iStrideIn +offset0, output+j*iStrideOut, offset0, width, flt->c_taps_div2, width-flt->c_taps_div2, 2);
      img_process_filter_hor_line(flt, input1+j*iStrideIn +offset1, output+j*iStrideOut+hwidth, offset1, width, flt->c_taps_div2, width-flt->c_taps_div2, 2);
    }
    destroy_img_process_filter_1D(flt);
  }
  else
  {
    SbSBasic(output, iStrideOut, input0, input1, iStrideIn, width, height, offset0, offset1);
  }
}

void H264AVCEncoderTest::sbsMuxFR(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height)
{
  for (int j = 0; j < height; j++)
  {
    memcpy(output +j*iStrideOut, input0 +j*iStrideIn, width*sizeof(UChar));
    memcpy(output +j*iStrideOut +width, input1 +j*iStrideIn, width*sizeof(UChar));
  }  
}

Int H264AVCEncoderTest::padBuf(UChar *output, Int iStrideOut, Int width, Int height, Int width_out, Int height_out, Int fillMode)
{
  int j;
  UChar *pDst;
  enum FillMode
  {
    FILL_CLEAR = 0,
    FILL_FRAME,
    FILL_FIELD
  };
  //horizontal;
  if(width < width_out)
  {
    pDst = output;
    for(j=0; j<height; j++)
    {
      memset(pDst+width, 0, (width_out-width)*sizeof(UChar));
      pDst += iStrideOut;
    }
  }
  if(height < height_out)
  {
    if(FILL_CLEAR == fillMode)
    {
      pDst = output +height*iStrideOut;
      for(j=height; j<height_out; j++)
      {
        memset(pDst, 0, width_out*sizeof(UChar));
        pDst += iStrideOut;
      }
    }
    else if(FILL_FRAME == fillMode )
    {
      pDst = output +height*iStrideOut;
      for(j=height; j<height_out; j++)
      {
        memcpy(pDst, pDst-iStrideOut, iStrideOut*sizeof(UChar));
        pDst += iStrideOut;
      }
    }
    else if(FILL_FIELD == fillMode)
    {
      ROT( (height_out - height) & 1 );
      pDst = output +height*iStrideOut;
      for(j=height; j<height_out; j+=2)
      {
        memcpy(pDst, pDst-2*iStrideOut, 2*iStrideOut*sizeof(UChar));
        pDst += iStrideOut*2;
      }
    }
    else
      ROT(!"Not supported yet!\n");
  }

  return Err::m_nOK;
}

#endif
