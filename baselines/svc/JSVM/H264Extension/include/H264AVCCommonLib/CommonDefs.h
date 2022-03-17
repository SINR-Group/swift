
#if !defined(AFX_COMMONDEFS_H__4CE634CE_B48D_4812_8098_9CAEA258BAA2__INCLUDED_)
#define AFX_COMMONDEFS_H__4CE634CE_B48D_4812_8098_9CAEA258BAA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define _JSVM_VERSION_  "9.19.15"


#define MB_BUFFER_WIDTH 24
#define MB_BASE_WIDTH   16

#define DC_PRED         2
#define OUTSIDE         -1
#define DOUBLE_MAX      1.7e+308

#define ESS_NONE      0
#define ESS_SEQ       1
#define ESS_PICT      2

#define NO_LEFT_REF        1
#define NO_ABOVE_REF       2
#define NO_ABOVELEFT_REF   4
#define NO_ABOVERIGHT_REF  8

typedef UChar CoefMap;

H264AVC_NAMESPACE_BEGIN

enum PicType
{
  NOT_SPECIFIED   = 0x00,
  TOP_FIELD       = 0x01,
  BOT_FIELD       = 0x02,
  FRAME           = 0x03,
	MAX_FRAME_TYPE  = 0x04
};

enum ParIdx16x16
{
  PART_16x16   = 0x00
};
enum ParIdx16x8
{
  PART_16x8_0   = 0x00,
  PART_16x8_1   = 0x08
};
enum ParIdx8x16
{
  PART_8x16_0   = 0x00,
  PART_8x16_1   = 0x02
};
enum Par8x8
{
  B_8x8_0    = 0x00,
  B_8x8_1    = 0x01,
  B_8x8_2    = 0x02,
  B_8x8_3    = 0x03
};
enum ParIdx8x8
{
  PART_8x8_0    = 0x00,
  PART_8x8_1    = 0x02,
  PART_8x8_2    = 0x08,
  PART_8x8_3    = 0x0A
};
enum SParIdx8x8
{
  SPART_8x8   = 0x00
};
enum SParIdx8x4
{
  SPART_8x4_0   = 0x00,
  SPART_8x4_1   = 0x04
};
enum SParIdx4x8
{
  SPART_4x8_0   = 0x00,
  SPART_4x8_1   = 0x01
};
enum SParIdx4x4
{
  SPART_4x4_0   = 0x00,
  SPART_4x4_1   = 0x01,
  SPART_4x4_2   = 0x04,
  SPART_4x4_3   = 0x05
};

enum NeighbourBlock
{
  CURR_MB_LEFT_NEIGHBOUR   = -1,
  LEFT_MB_LEFT_NEIGHBOUR   = +3,
  CURR_MB_ABOVE_NEIGHBOUR  = -4,
  ABOVE_MB_ABOVE_NEIGHBOUR = +12,
  CURR_MB_RIGHT_NEIGHBOUR  = +1,
  RIGHT_MB_RIGHT_NEIGHBOUR = -3,
};
enum ListIdx
{
  LIST_0 = 0x00,
  LIST_1 = 0x01
};

enum ProcessingState
{
  PRE_PROCESS     = 0,
  PARSE_PROCESS   = 1,
  DECODE_PROCESS  = 2,
  ENCODE_PROCESS  = 3,
  POST_PROCESS    = 4
};


enum SliceType
{
  P_SLICE             = 0,
  B_SLICE             = 1,
  I_SLICE             = 2,
  SP_SLICE            = 3,
  SI_SLICE            = 4,
  NOT_SPECIFIED_SLICE = 5
};


enum NalRefIdc
{
  NAL_REF_IDC_PRIORITY_LOWEST  = 0,
  NAL_REF_IDC_PRIORITY_LOW     = 1,
  NAL_REF_IDC_PRIORITY_HIGH    = 2,
  NAL_REF_IDC_PRIORITY_HIGHEST = 3
};

enum NalUnitType
{
  NAL_UNIT_UNSPECIFIED_0            =  0,
  NAL_UNIT_CODED_SLICE              =  1,
  NAL_UNIT_CODED_SLICE_DATAPART_A   =  2,
  NAL_UNIT_CODED_SLICE_DATAPART_B   =  3,
  NAL_UNIT_CODED_SLICE_DATAPART_C   =  4,
  NAL_UNIT_CODED_SLICE_IDR          =  5,
  NAL_UNIT_SEI                      =  6,
  NAL_UNIT_SPS                      =  7,
  NAL_UNIT_PPS                      =  8,
  NAL_UNIT_ACCESS_UNIT_DELIMITER    =  9,
  NAL_UNIT_END_OF_SEQUENCE          = 10,
  NAL_UNIT_END_OF_STREAM            = 11,
  NAL_UNIT_FILLER_DATA              = 12,
  NAL_UNIT_SPS_EXTENSION            = 13,
  NAL_UNIT_PREFIX										= 14,
  NAL_UNIT_SUBSET_SPS               = 15,
  NAL_UNIT_RESERVED_16              = 16,
  NAL_UNIT_RESERVED_17              = 17,
  NAL_UNIT_RESERVED_18              = 18,
  NAL_UNIT_AUX_CODED_SLICE          = 19,
  NAL_UNIT_CODED_SLICE_SCALABLE     = 20,
  NAL_UNIT_RESERVED_21              = 21,
  NAL_UNIT_RESERVED_22              = 22,
  NAL_UNIT_RESERVED_23              = 23
};

#define NAL_UNIT_HEADER_SVC_EXTENSION_BYTES      3
#define NUM_SPS_IDS                             32

enum MbMode
{
  MODE_SKIP         = 0,
  MODE_16x16        = 1,
  MODE_16x8         = 2,
  MODE_8x16         = 3,
  MODE_8x8          = 4,
  MODE_8x8ref0      = 5,
  INTRA_4X4         = 6,
  MODE_PCM          = 25+6,
  INTRA_BL          = 36,
  NOT_AVAILABLE     = 99 //TMM
};

enum BlkMode
{
  BLK_8x8   = 8,
  BLK_8x4   = 9,
  BLK_4x8   = 10,
  BLK_4x4   = 11,
  BLK_SKIP  = 0
};

enum Profile
{
  BASELINE_PROFILE  = 66,
  MAIN_PROFILE      = 77,
  EXTENDED_PROFILE  = 88,

  HIGH_PROFILE      = 100,
  HIGH_10_PROFILE   = 110,
  HIGH_422_PROFILE  = 122,
  HIGH_444_PROFILE  = 144,
  CAVLC_444_PROFILE = 244,

  SCALABLE_BASELINE_PROFILE = 83,
  SCALABLE_HIGH_PROFILE     = 86
};

enum DFunc
{
  DF_SAD      = 0,
  DF_SSD      = 1,
  DF_HADAMARD = 2,
  DF_YUV_SAD  = 3
};

enum SearchMode
{
  BLOCK_SEARCH  = 0,
  SPIRAL_SEARCH = 1,
  LOG_SEARCH    = 2,
  FAST_SEARCH   = 3,
  TZ_SEARCH     = 4
};

enum ResidualMode
{
  LUMA_I16_DC  = 0,
  LUMA_I16_AC     ,
  LUMA_SCAN       ,
  CHROMA_DC       ,
  CHROMA_AC       ,
  LUMA_8X8
};


H264AVC_NAMESPACE_END



#define MIN_QP              0
#define MAX_QP              51
#define QP_BITS             15
#define QP_SHIFT1           12
#define MAX_FRAME_NUM_LOG2  9

#define YUV_X_MARGIN        32
#define YUV_Y_MARGIN        64

#define MAX_LAYERS          8
#define MAX_TEMP_LEVELS     8

#define MAX_QUALITY_LEVELS  16

#define MAX_DSTAGES         6
#define LOG2_GOP_ID_WRAP    4
#define PRI_ID_BITS         6
#define MAX_SCALABLE_LAYERS MAX_LAYERS * MAX_TEMP_LEVELS * MAX_QUALITY_LEVELS
//JVT-W049 {
#define MAX_REDUNDANT_PICTURES_NUM          128
//JVT-W049 }

//{{Quality level estimation and modified truncation- JVTO044 and m12007
//France Telecom R&D-(nathalie.cammas@francetelecom.com)
#define MAX_NUM_RD_LEVELS      50
//}}Quality level estimation and modified truncation- JVTO044 and m12007
#define MAX_SIZE_PID 64

#define MAX_NUM_INFO_ENTRIES 8
#define MAX_NUM_NON_REQUIRED_PICS 32

#define WEIGHTED_PRED_FLAG                            0                   // (0:no weighted prediction, 1:random weights)
#define WEIGHTED_BIPRED_IDC                           0                   // (0:no weighted bi-prediction, 1:random weights, 2:implicit weights)
#define INFER_ELAYER_PRED_WEIGHTS                     0                   // (0:BL weights are not used, 1:infer enhancement layer prediction weights)

//TMM_EC {{
typedef	enum
{
	EC_NONE												=	100,
  EC_BLSKIP,
	EC_FRAME_COPY,
	EC_TEMPORAL_DIRECT,
	EC_INTRA_COPY                // =200
}	ERROR_CONCEAL;
//TMM_EC }}

#define MAX_NUM_PD_FRAGMENTS                          12


//--- deblocking filter version ---
#define PROPOSED_DEBLOCKING_APRIL2010                 1   // deblocking according to the proposal in April 2010 (cp. VCEG_AN10_r3)


#endif // !defined(AFX_COMMONDEFS_H__4CE634CE_B48D_4812_8098_9CAEA258BAA2__INCLUDED_)
