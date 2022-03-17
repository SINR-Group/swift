
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <memory.h>
#include <limits.h>

#ifndef _RATE_CTL_H_
#define _RATE_CTL_H_

#define RC_MODE_1              1
#define RC_MODE_2              2
#define MB_BLOCK_SIZE         16

extern bool bRateControlEnable;

#if defined(WIN32) || (__STDC_VERSION__ >= 199901L)

static inline int imin(int a, int b)
{
  return ((a) < (b)) ? (a) : (b);
}

static inline int imax(int a, int b)
{
  return ((a) > (b)) ? (a) : (b);
}

static inline int iClip3(int low, int high, int x)
{
  x = imax(x, low);
  x = imin(x, high);

  return x;
}

static inline int iabs(int x)
{
  return ((x) < 0) ? -(x) : (x);
}

#else

#define imin(a, b)                  (((a) < (b)) ? (a) : (b))
#define imax(a, b)                  (((a) > (b)) ? (a) : (b))
#define iClip3(low, high, x)        (imax( imin(x, high), low))
#define iabs(x)                     (((x) < 0)   ? -(x) : (x))

#endif

typedef enum
{
  FRAME_CODING,
  FIELD_CODING,
  ADAPTIVE_CODING
} CodingType;


class jsvm_parameters {
public:
  float FrameRate;
	float bit_rate;

	int current_mb_nr;
	int current_frame_number;
	int successive_Bframe;
	int jumpd;
	int Frame_Total_Number_MB;
	int basicunit;
	int height;
	int width;
	int FrameSizeInMbs;
	int PicInterlace;
	int MbInterlace;
	int channel_type;
	int type;
	int BasicUnit;
	int frame_mbs_only_flag;
	int intra_period;
	int SetInitialQP;
	int NumberofCodedMacroBlocks;
  int qp;
  int RCMaxQP;
  int RCMinQP;
  int RCUpdateMode;
  int number;
  int PicWidthInMbs;
  int size;
  int HierarchicalCoding;
  int no_frames;
  int CurrGopLevel;
  int nal_reference_idc;
  int HierarchicalLevels;

  unsigned int m_uiLayerId;
  unsigned int m_uiIntraPeriod;
};

class rc_generic {
public:
  // RC flags
  int   m_iTopFieldFlag;
  int   m_iFieldControl;
  int   m_iFieldFrame;
  int   m_iNoGranularFieldRC;
  // bits stats
  int   m_iNumberofHeaderBits;
  int   m_iNumberofTextureBits;
  int   m_iNumberofBasicUnitHeaderBits;
  int   m_iNumberofBasicUnitTextureBits;
  // frame stats
  int   m_iNumberofGOP;
  int   m_iNumberofCodedBFrame;
  // MAD stats
  Int64 m_i64TotalMADBasicUnit;
  int   *m_piMADofMB;
  // buffer and budget
  Int64 m_i64CurrentBufferFullness; //LIZG 25/10/2002
  int   m_iRemainingBits;

  int   m_iRCTextureBits;
  int   m_iRCHeaderBits;
  int   m_iRCTotalBits;

  jsvm_parameters *m_pcJSVMParams;

  rc_generic( jsvm_parameters *jsvm_params );
  ~rc_generic( void );

  int    Qstep2QP( double dQstep );
  double QP2Qstep( int iQP );
  double ComputeFrameMAD( void );
  void   generic_alloc( void );
  void   generic_free( void );

  void   update_rc( unsigned int currentMAD );
  int    getCurrGopLevel( int frame_no );
  void   adaptInitialQP( void );
};

#endif
