
#include "H264AVCEncoderLib.h"
#include "UvlcWriter.h"
#include "H264AVCCommonLib/Tables.h"
#include "H264AVCCommonLib/TraceFile.h"

#define MAX_VALUE  0xdead
#define TOTRUN_NUM    15
#define RUNBEFORE_NUM  7

// h264 namepace begin
H264AVC_NAMESPACE_BEGIN

const UInt g_auiIncVlc[] = {0,3,6,12,24,48,32768};	// maximum vlc = 6

const UChar g_aucLenTableTZ16[TOTRUN_NUM][16] =
{

  { 1,3,3,4,4,5,5,6,6,7,7,8,8,9,9,9},
  { 3,3,3,3,3,4,4,4,4,5,5,6,6,6,6},
  { 4,3,3,3,4,4,3,3,4,5,5,6,5,6},
  { 5,3,4,4,3,3,3,4,3,4,5,5,5},
  { 4,4,4,3,3,3,3,3,4,5,4,5},
  { 6,5,3,3,3,3,3,3,4,3,6},
  { 6,5,3,3,3,2,3,4,3,6},
  { 6,4,5,3,2,2,3,3,6},
  { 6,6,4,2,2,3,2,5},
  { 5,5,3,2,2,2,4},
  { 4,4,3,3,1,3},
  { 4,4,2,1,3},
  { 3,3,1,2},
  { 2,2,1},
  { 1,1},
};

const UChar g_aucCodeTableTZ16[TOTRUN_NUM][16] =
{
  {1,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1},
  {7,6,5,4,3,5,4,3,2,3,2,3,2,1,0},
  {5,7,6,5,4,3,4,3,2,3,2,1,1,0},
  {3,7,5,4,6,5,4,3,3,2,2,1,0},
  {5,4,3,7,6,5,4,3,2,1,1,0},
  {1,1,7,6,5,4,3,2,1,1,0},
  {1,1,5,4,3,3,2,1,1,0},
  {1,1,1,3,3,2,2,1,0},
  {1,0,1,3,2,1,1,1,},
  {1,0,1,3,2,1,1,},
  {0,1,1,2,1,3},
  {0,1,1,1,1},
  {0,1,1,1},
  {0,1,1},
  {0,1},
};


const UChar g_aucLenTableTZ4[3][4] =
{
  { 1, 2, 3, 3,},
  { 1, 2, 2, 0,},
  { 1, 1, 0, 0,},
};

const UChar g_aucCodeTableTZ4[3][4] =
{
  { 1, 1, 1, 0,},
  { 1, 1, 0, 0,},
  { 1, 0, 0, 0,},
};

const UChar g_aucLenTableTZ8[7][8] =
{
  { 1, 3, 3, 4, 4, 4, 5, 5 },
  { 3, 2, 3, 3, 3, 3, 3    },
  { 3, 3, 2, 2, 3, 3       },
  { 3, 2, 2, 2, 3          },
  { 2, 2, 2, 2             },
  { 2, 2, 1                },
  { 1, 1                   }
};

const UChar g_aucCodeTableTZ8[7][8] =
{
  { 1, 2, 3, 2, 3, 1, 1, 0 },
  { 0, 1, 1, 4, 5, 6, 7    },
  { 0, 1, 1, 2, 6, 7       },
  { 6, 0, 1, 2, 7          },
  { 0, 1, 2, 3             },
  { 0, 1, 1                },
  { 0, 1                   }
};

const UChar g_aucLenTable3[7][15] =
{
  {1,1},
  {1,2,2},
  {2,2,2,2},
  {2,2,2,3,3},
  {2,2,3,3,3,3},
  {2,3,3,3,3,3,3},
  {3,3,3,3,3,3,3,4,5,6,7,8,9,10,11},
};

const UChar g_aucCodeTable3[7][15] =
{
  {1,0},
  {1,1,0},
  {3,2,1,0},
  {3,2,1,1,0},
  {3,2,3,2,1,0},
  {3,0,1,3,2,5,4},
  {7,6,5,4,3,2,1,1,1,1,1,1,1,1,1},
};

const UChar g_aucLenTableTO4[4][5] =
{
  { 2, 6, 6, 6, 6,},
  { 0, 1, 6, 7, 8,},
  { 0, 0, 3, 7, 8,},
  { 0, 0, 0, 6, 7,},
};

const UChar g_aucCodeTableTO4[4][5] =
{
  {1,7,4,3,2},
  {0,1,6,3,3},
  {0,0,1,2,2},
  {0,0,0,5,0},
};


const UChar g_aucCwLenVectTO16[4][62] =
{
  { 1, 2, 3, 5, 6, 6, 6, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15,16,16,16,16,16,16,16,16,16,16,16,16 },
  { 2, 2, 3, 4, 4, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,13,13,13,13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14 },
  { 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6 }
};
const UChar g_aucCwCodeVectTO16[4][62] =
{
  { 1, 1, 1, 3, 5, 4, 3, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8, 1,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4 },
  { 3, 2, 3, 5, 4, 7, 6,11,10, 9, 8, 7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4, 7, 6, 5, 4,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8, 7, 6, 1,11, 9, 8,10, 4, 7, 6, 5 },
  {15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8,15,14,13,12,11,10, 9, 8, 7,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1 },
  { 3, 1, 6,11,15,19,23,27, 5,10,31, 9,14,13,18,17, 0,21,22,35, 4,25,26, 8,12,29,30,39,16,34,20,24,28,33,38,43,32,37,42,47,36,41,46,51,40,45,50,44,49,48,53,54,55,52,57,58,59,56,61,62,63,60 }
};


const UChar g_aucCwMapTO16[3][4][17] =
{
  {
    { 0, 4, 9,13,17,21,25,29,32,33,37,41,45,50,54,58,61},
    { 0, 1, 5,10,14,18,22,26,30,34,38,42,46,49,51,55,59},
    { 0, 0, 2, 7,11,15,19,23,27,31,35,39,43,47,52,56,60},
    { 0, 0, 0, 3, 6, 8,12,16,20,24,28,36,40,44,48,53,57},
  },
  {
    { 0, 7,11,15,19,22,23,27,31,35,39,42,43,47,51,55,59},
    { 0, 1, 5, 8,12,16,20,24,28,32,36,40,44,48,54,56,60},
    { 0, 0, 2, 9,13,17,21,25,29,33,37,41,45,49,52,57,61},
    { 0, 0, 0, 3, 4, 6,10,14,18,26,30,34,38,46,50,53,58},
  },
  {
    { 0,16,20,23,24,28,30,31,32,36,40,44,47,49,53,57,61},
    { 0, 1, 8,11,13,15,17,21,25,33,37,41,45,48,50,54,58},
    { 0, 0, 2, 9,12,14,18,22,26,29,34,38,42,46,51,55,59},
    { 0, 0, 0, 3, 4, 5, 6, 7,10,19,27,35,39,43,52,56,60},
  },
};

const UChar g_aucLenTableTO16[3][4][17] =
{
  {   // 0702
    { 1, 6, 8, 9,10,11,13,13,13,14,14,15,15,16,16,16,16},
    { 0, 2, 6, 8, 9,10,11,13,13,14,14,15,15,15,16,16,16},
    { 0, 0, 3, 7, 8, 9,10,11,13,13,14,14,15,15,16,16,16},
    { 0, 0, 0, 5, 6, 7, 8, 9,10,11,13,14,14,15,15,16,16},
  },
  {
    { 2, 6, 6, 7, 8, 8, 9,11,11,12,12,12,13,13,13,14,14},
    { 0, 2, 5, 6, 6, 7, 8, 9,11,11,12,12,13,13,14,14,14},
    { 0, 0, 3, 6, 6, 7, 8, 9,11,11,12,12,13,13,13,14,14},
    { 0, 0, 0, 4, 4, 5, 6, 6, 7, 9,11,11,12,13,13,13,14},
  },
  {
    { 4, 6, 6, 6, 7, 7, 7, 7, 8, 8, 9, 9, 9,10,10,10,10},
    { 0, 4, 5, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 9,10,10,10},
    { 0, 0, 4, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9,10,10,10},
    { 0, 0, 0, 4, 4, 4, 4, 4, 5, 6, 7, 8, 8, 9,10,10,10},
  },
};

const UChar g_aucCodeTableTO16[3][4][17] =
{
  {
    { 1, 5, 7, 7, 7, 7,15,11, 8,15,11,15,11,15,11, 7,4},
    { 0, 1, 4, 6, 6, 6, 6,14,10,14,10,14,10, 1,14,10,6},
    { 0, 0, 1, 5, 5, 5, 5, 5,13, 9,13, 9,13, 9,13, 9,5},
    { 0, 0, 0, 3, 3, 4, 4, 4, 4, 4,12,12, 8,12, 8,12,8},
  },
  {
    { 3,11, 7, 7, 7, 4, 7,15,11,15,11, 8,15,11, 7, 9,7},
    { 0, 2, 7,10, 6, 6, 6, 6,14,10,14,10,14,10,11, 8,6},
    { 0, 0, 3, 9, 5, 5, 5, 5,13, 9,13, 9,13, 9, 6,10,5},
    { 0, 0, 0, 5, 4, 6, 8, 4, 4, 4,12, 8,12,12, 8, 1,4},
  },
  {
    {15,15,11, 8,15,11, 9, 8,15,11,15,11, 8,13, 9, 5,1},
    { 0,14,15,12,10, 8,14,10,14,14,10,14,10, 7,12, 8,4},
    { 0, 0,13,14,11, 9,13, 9,13,10,13, 9,13, 9,11, 7,3},
    { 0, 0, 0,12,11,10, 9, 8,13,12,12,12, 8,12,10, 6,2},
  },
};

const UChar COEFF_COST[16] =
{
  3, 2,2,1, 1,1,0,0,0,0,0,0,0,0,0,0
};

const UChar COEFF_COST8x8[64] =
{
  3,3,3,3,2,2,2,2,2,2,2,2,1,1,1,1,
  1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

const UChar g_aucCbpIntra[48] =
{
   3, 29, 30, 17,
  31, 18, 37,  8,
  32, 38, 19,  9,
  20, 10, 11,  2,
  16, 33, 34, 21,
  35, 22, 39,  4,
  36, 40, 23,  5,
  24,  6,  7,  1,
  41, 42, 43, 25,
  44, 26, 46, 12,
  45, 47, 27, 13,
  28, 14, 15,  0
};


const UChar g_aucCbpInter[48] =
{
   0,  2,  3,  7,
   4,  8, 17, 13,
   5, 18,  9, 14,
  10, 15, 16, 11,
   1, 32, 33, 36,
  34, 37, 44, 40,
  35, 45, 38, 41,
  39, 42, 43, 19,
   6, 24, 25, 20,
  26, 21, 46, 28,
  27, 47, 22, 29,
  23, 30, 31, 12
};

const UInt g_auiISymCode[3][16] =
{ {  0,  1                                                        },
  {  0,  4,  5, 28,  6, 29, 30, 31                                },
  {  0,  4,  5, 28, 12, 29, 60,252, 13, 61,124,253,125,254,510,511}
};
const UInt g_auiISymLen[3][16] =
{ { 1, 1                                          },
  { 1, 3, 3, 5, 3, 5, 5, 5                        },
  { 1, 3, 3, 5, 4, 5, 6, 8, 4, 6, 7, 8, 7, 8, 9, 9}
};

const UInt g_auiRefSymCode[2][27] =
{
  {
    0x1, 0x3, 0x5, 0x3, 0x5, 0x5, 0x4, 0x5, 0x5,
    0x2, 0x4, 0x4, 0x3, 0x4, 0x3, 0x4, 0x2, 0x3,
    0x3, 0x3, 0x3, 0x3, 0x1, 0x2, 0x2, 0x1, 0x0
  },
  {
    0x1, 0x7, 0x6, 0x7,	0x9, 0x8, 0x6, 0x7,	0x9,
    0x5, 0x6,	0x5, 0x8,	0x7, 0x6,	0x7, 0x5,	0x4,
    0x4, 0x4,	0x6, 0x5,	0x3, 0x2,	0x5, 0x1,	0x0
  }
};

const UInt g_auiRefSymLen[2][27] =
{
  {
    1, 4, 5, 3, 6, 8, 5, 7, 9,
    3, 6, 8, 6, 9,10, 7,10,12,
    5, 7, 9, 8,10,12, 9,12,12
  },
  {
    1, 5, 5, 4, 7, 7, 4, 7, 6,
    4, 7, 7, 6, 8, 8, 6, 8, 8,
    4, 7, 6, 6, 8, 8, 5, 8, 8
  }
};

UvlcWriter::UvlcWriter( Bool bTraceEnable ) :
  m_pcBitWriteBufferIf( NULL ),
  m_uiBitCounter( 0 ),
  m_uiPosCounter( 0 ),
  m_uiCoeffCost( 0 ),
  m_bTraceEnable( bTraceEnable ),
  m_bRunLengthCoding( false ),
  m_uiRun( 0 ),
  m_pcNextUvlcWriter( NULL )
{
}


UvlcWriter::~UvlcWriter()
{
}


ErrVal UvlcWriter::create( UvlcWriter*& rpcUvlcWriter, Bool bTraceEnable )
{
  rpcUvlcWriter = new UvlcWriter( bTraceEnable );

  ROT( NULL == rpcUvlcWriter );

  return Err::m_nOK;
}


ErrVal UvlcWriter::destroy()
{
  if( m_pcNextUvlcWriter )
  {
    m_pcNextUvlcWriter->destroy();
  }
  delete this;
  return Err::m_nOK;
}

__inline ErrVal UvlcWriter::xWriteCode( UInt uiCode, UInt uiLength )
{
  AOT_DBG(uiLength<1);

  ErrVal retVal = m_pcBitWriteBufferIf->write( uiCode, uiLength );

  ETRACE_TY( " u(v)" );
  ETRACE_BITS( uiCode, uiLength );
  ETRACE_POS;

  ETRACE_CODE( uiCode );
  ETRACE_COUNT (uiLength);
  return retVal;
}

__inline ErrVal UvlcWriter::xWriteCodeNT( UInt uiCode, UInt uiLength )
{
  AOT_DBG(uiLength<1);
  ErrVal retVal = m_pcBitWriteBufferIf->write( uiCode, uiLength );
  return retVal;
}


__inline ErrVal UvlcWriter::xWriteFlag( UInt uiCode )
{
  ErrVal retVal = m_pcBitWriteBufferIf->write( uiCode, 1 );

  ETRACE_TY( " u(1)" );
  ETRACE_BITS( uiCode, 1 );
  ETRACE_POS;

  ETRACE_CODE( uiCode );
  ETRACE_COUNT (1);
  return retVal;
}

ErrVal UvlcWriter::init(  BitWriteBufferIf* pcBitWriteBufferIf )
{
  ROT( NULL == pcBitWriteBufferIf );

  m_pcBitWriteBufferIf= pcBitWriteBufferIf;
  m_bRunLengthCoding  = false;
  m_uiRun             = 0;

  return Err::m_nOK;
}

ErrVal UvlcWriter::uninit()
{
  if( m_pcNextUvlcWriter )
  {
    m_pcNextUvlcWriter->uninit();
  }
  m_pcBitWriteBufferIf = NULL;
  return Err::m_nOK;
}

UInt UvlcWriter::getNumberOfWrittenBits()
{
  return m_pcBitWriteBufferIf->getNumberOfWrittenBits();
}


ErrVal UvlcWriter::startSlice( const SliceHeader& rcSliceHeader )
{
  m_bRunLengthCoding  = ! rcSliceHeader.isIntraSlice();
  m_uiRun             = 0;

  if( m_pcNextUvlcWriter )
  {
    m_pcNextUvlcWriter->startSlice( rcSliceHeader );
  }
  return Err::m_nOK;
}

//FIX_FRAG_CAVLC
ErrVal UvlcWriter::getLastByte(UChar &uiLastByte, UInt &uiLastBitPos)
{
  RNOK(m_pcBitWriteBufferIf->getLastByte(uiLastByte, uiLastBitPos));
  return Err::m_nOK;
}
ErrVal UvlcWriter::setFirstBits(UChar ucByte,UInt uiLastBitPos)
{
  RNOK( m_pcBitWriteBufferIf->write(ucByte,uiLastBitPos));
  return Err::m_nOK;
}
//~FIX_FRAG_CAVLC
ErrVal UvlcWriter::xWriteUvlcCode( UInt uiVal)
{
  UInt uiLength = 1;
  UInt uiTemp = ++uiVal;

  AOF_DBG( uiTemp );

  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }

  RNOK( m_pcBitWriteBufferIf->write( uiVal, uiLength ) );

  ETRACE_TY( "ue(v)" );
  ETRACE_BITS( uiVal, uiLength );
  ETRACE_POS;

  ETRACE_DO( uiVal-- );

  ETRACE_CODE( uiVal );
  ETRACE_COUNT (uiLength);

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteSvlcCode( Int iVal)
{
  UInt uiVal = xConvertToUInt( iVal );
  UInt uiLength = 1;
  UInt uiTemp = ++uiVal;

  AOF_DBG( uiTemp );

  while( 1 != uiTemp )
  {
    uiTemp >>= 1;
    uiLength += 2;
  }

  RNOK( m_pcBitWriteBufferIf->write( uiVal, uiLength ) );

  ETRACE_TY( "ue(v)" );
  ETRACE_BITS( uiVal, uiLength );
  ETRACE_POS;

  ETRACE_CODE( iVal );
  ETRACE_COUNT (uiLength);

  return Err::m_nOK;
}

ErrVal UvlcWriter::writeUvlc( UInt uiCode, const Char* pcTraceString )
{
  ETRACE_TH( pcTraceString );

  RNOK( xWriteUvlcCode( uiCode ) );

  ETRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcWriter::writeSvlc( Int iCode, const Char* pcTraceString )
{
  UInt uiCode;

  ETRACE_TH( pcTraceString );

  uiCode = xConvertToUInt( iCode );
  RNOK( xWriteUvlcCode( uiCode ) );

  ETRACE_TY( "se(v)" );
  ETRACE_CODE( iCode );

  ETRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcWriter::writeFlag( Bool bFlag, const Char* pcTraceString )
{
  ETRACE_TH( pcTraceString );
  ETRACE_TY( " u(1)" );

  RNOK( xWriteFlag( bFlag ? 1:0) );

  ETRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcWriter::writeSCode( Int iCode, UInt uiLength, const Char* pcTraceString )
{
  AOT_DBG(uiLength<1);
  ETRACE_TH( pcTraceString );
  ETRACE_TY( " i(v)" );

  UInt uiShift = 32 - uiLength;
  UInt uiCode = ((UInt)(iCode << uiShift)) >> uiShift;
  RNOK( m_pcBitWriteBufferIf->write( uiCode, uiLength ) );

  ETRACE_POS;
  ETRACE_CODE (iCode);
  ETRACE_BITS (uiCode, uiLength );
  ETRACE_COUNT(uiLength);
  ETRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcWriter::writeCode( UInt uiCode, UInt uiLength, const Char* pcTraceString )
{
  AOT_DBG(uiLength<1);
  ETRACE_TH( pcTraceString );

  if( uiLength > 9 )
  {
    Char acType[] = "u(00)";
    acType[2] += uiLength/10;
    acType[3] += uiLength%10;
    ETRACE_TY( acType );
  }
  else
  {
    Char acType[] = " u(0)";
    acType[3] += uiLength;
    ETRACE_TY( acType );
  }

  RNOK( m_pcBitWriteBufferIf->write( uiCode, uiLength ) );

  ETRACE_POS;
  ETRACE_CODE (uiCode);
  ETRACE_BITS (uiCode, uiLength );
  ETRACE_COUNT(uiLength);
  ETRACE_N;
  return Err::m_nOK;
}



ErrVal UvlcWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx();
  RNOK( xWriteRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), --uiRefFrame ) )
  return Err::m_nOK;
}

ErrVal UvlcWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xWriteRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), --uiRefFrame ) )
  return Err::m_nOK;
}

ErrVal UvlcWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xWriteRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), --uiRefFrame ) )
  return Err::m_nOK;
}

ErrVal UvlcWriter::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx  )
{
  UInt uiRefFrame = rcMbDataAccess.getMbMotionData( eLstIdx ).getRefIdx( eParIdx );
  RNOK( xWriteRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), --uiRefFrame ) )
  return Err::m_nOK;
}


ErrVal  UvlcWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  return xWriteMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag() );
}
ErrVal  UvlcWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx   )
{
  return xWriteMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag( eParIdx ) );
}
ErrVal  UvlcWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx   )
{
  return xWriteMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag( eParIdx ) );
}
ErrVal  UvlcWriter::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx   )
{
  return xWriteMotionPredFlag( rcMbDataAccess.getMbMotionData( eLstIdx ).getMotPredFlag( eParIdx ) );
}



ErrVal UvlcWriter::blockModes( MbDataAccess& rcMbDataAccess )
{
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    ETRACE_TH( "BlockMode" );

    UInt uiBlockMode = rcMbDataAccess.getConvertBlkMode( c8x8Idx.b8x8Index() );

    AOT_DBG( uiBlockMode > 12);

    RNOK( xWriteUvlcCode( uiBlockMode ) );

    ETRACE_N;
  }
  return Err::m_nOK;
}

ErrVal UvlcWriter::fieldFlag( MbDataAccess& rcMbDataAccess )
{
  ETRACE_TH( "MbFieldFlag" );

  UInt uiBit = rcMbDataAccess.getMbData().getFieldFlag() ? 1 : 0;

  RNOK( xWriteFlag( uiBit ) );

  ETRACE_N;

  return Err::m_nOK;
}
ErrVal UvlcWriter::skipFlag( MbDataAccess& rcMbDataAccess )
{
  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );

  ROFRS( m_bRunLengthCoding, Err::m_nOK );

  if( rcMbDataAccess.isSkippedMb() )
  {
    m_uiRun++;
  }
  else
  {
    ETRACE_TH( "Run" );
    RNOK( xWriteUvlcCode( m_uiRun ) );
    ETRACE_N;

    m_uiRun = 0;
  }

  rcMbDataAccess.getMbData().setSkipFlag( m_uiRun > 0 );

  return Err::m_nOK;
}


ErrVal UvlcWriter::BLSkipFlag( MbDataAccess& rcMbDataAccess )
{
  UInt  uiCode = ( rcMbDataAccess.getMbData().getBLSkipFlag() ? 1 : 0 );

  ETRACE_TH( "BLSkipFlag" );
  RNOK( xWriteFlag( uiCode ) );
  ETRACE_N;

  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );

  return Err::m_nOK;
}


ErrVal UvlcWriter::mbMode( MbDataAccess& rcMbDataAccess )
{
  UInt uiMbMode = rcMbDataAccess.getConvertMbType( );

  if( m_bRunLengthCoding )
  {
    uiMbMode--;
  }
  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );

	ETRACE_TH( "MbMode" );
  RNOK( xWriteUvlcCode( uiMbMode ) );
  ETRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcWriter::resPredFlag( MbDataAccess& rcMbDataAccess )
{
  UInt uiCode = ( rcMbDataAccess.getMbData().getResidualPredFlag() ? 1 : 0 );

  ETRACE_TH( "ResidualPredFlag" );
  RNOK( xWriteFlag( uiCode ) );
  ETRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv();
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}

ErrVal UvlcWriter::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx )
{
  Mv cMv = rcMbDataAccess.getMbMvdData( eLstIdx ).getMv( eParIdx, eSParIdx );
  RNOK( xWriteMvd( cMv ) );
  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteMvd( Mv cMv )
{
  ETRACE_TH( "Mvd: x" );

  UInt  uiTemp;
  Short sHor   = cMv.getHor();
  Short sVer   = cMv.getVer();

  uiTemp = xConvertToUInt( sHor );
  RNOK( xWriteUvlcCode( uiTemp ) );

  ETRACE_CODE( sHor );
  ETRACE_TY("se(v)");
  ETRACE_N;

  ETRACE_TH( "Mvd: y" );

  uiTemp = xConvertToUInt( sVer );
  RNOK( xWriteUvlcCode( uiTemp ) );

  ETRACE_CODE( sVer );
  ETRACE_TY("se(v)");
  ETRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcWriter::intraPredModeChroma( MbDataAccess& rcMbDataAccess )
{
  ETRACE_TH( "IntraPredModeChroma" );

  AOT_DBG( 4 < rcMbDataAccess.getMbData().getChromaPredMode() );
  RNOK( xWriteUvlcCode( rcMbDataAccess.getMbData().getChromaPredMode() ) );

  ETRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcWriter::intraPredModeLuma( MbDataAccess& rcMbDataAccess, LumaIdx cIdx )
{
  ETRACE_TH( "IntraPredModeLuma" );
  ETRACE_POS;

  Int iIntraPredModeLuma = rcMbDataAccess.encodeIntraPredMode(cIdx);
  ROT( iIntraPredModeLuma > 7);

  UInt uiBits = (iIntraPredModeLuma < 0) ? 1 : 0;

  RNOK( m_pcBitWriteBufferIf->write( uiBits, 1 ) );
  ETRACE_BITS( uiBits,1 );
  ETRACE_DO( m_uiBitCounter = 1 );

  if( ! uiBits )
  {
    RNOK( m_pcBitWriteBufferIf->write( iIntraPredModeLuma, 3 ) );
    ETRACE_BITS( iIntraPredModeLuma, 3 );
    ETRACE_DO( m_uiBitCounter = 4 );
  }

  ETRACE_COUNT(m_uiBitCounter);
  ETRACE_CODE(iIntraPredModeLuma);
  ETRACE_N;

  return Err::m_nOK;
}

UvlcWriter* UvlcWriter::xGetUvlcWriterNextSlice( Bool bStartNewBitstream )
{
  if( !m_pcNextUvlcWriter || bStartNewBitstream )
  {
    if( !m_pcNextUvlcWriter )
    {
      UvlcWriter::create( m_pcNextUvlcWriter );
    }
    m_pcNextUvlcWriter->init( m_pcBitWriteBufferIf->getNextBitWriteBuffer( true ) );
  }
  return m_pcNextUvlcWriter;
}


ErrVal UvlcWriter::cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  UInt uiCbp = rcMbDataAccess.getMbData().calcMbCbp( uiStart, uiStop ) ;
  ETRACE_TH( "Cbp: " );
  ETRACE_X ( uiCbp );

  AOT_DBG( 48 < uiCbp );

  Bool bIntra = ( !rcMbDataAccess.getMbData().getBLSkipFlag() && rcMbDataAccess.getMbData().isIntra() );
  if( rcMbDataAccess.getMbData().isIntra() )
  {
    ROF( rcMbDataAccess.getMbData().isIntraBL() == rcMbDataAccess.getMbData().getBLSkipFlag() );
  }
  if( uiStart != 0 || uiStop != 16 )
  {
    ROT( bIntra );
  }
  UInt uiTemp = ( bIntra ? g_aucCbpIntra[uiCbp]: g_aucCbpInter[uiCbp] );

  if( ! rcMbDataAccess.getSH().getNoInterLayerPredFlag() && ( uiStart != 0 || uiStop != 16 ) )
  {
    UInt uiPrevCbp = 0;
    if( rcMbDataAccess.isAvailableLeft() )
      uiPrevCbp = rcMbDataAccess.getMbDataLeft().calcMbCbp( uiStart, uiStop );
    else if ( rcMbDataAccess.isAvailableAbove() )
      uiPrevCbp = rcMbDataAccess.getMbDataAbove().calcMbCbp( uiStart, uiStop );
    UInt uiPrevTemp = ( bIntra ? g_aucCbpIntra[uiPrevCbp]: g_aucCbpInter[uiPrevCbp] );
    RNOK( xWriteUvlcCode( (uiTemp == uiPrevTemp) ? 0:(uiTemp+1) ) );
  }
  else
  {
    RNOK( xWriteUvlcCode( uiTemp ) );
  }
  ETRACE_N;
  return Err::m_nOK;
}



const UChar g_aucTcoeffCDc[3][2]=
{
  {0,0},
  {2,6},
  {4,1}
};
const UChar g_aucRunCDc[4]=
{
  2,1,0,0
};



const UChar g_aucRunSScan[16]=
{
  4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0
};
const UChar g_aucTcoeffSScan[4][10]=
{
  { 1, 3, 5, 9,11,13,21,23,25,27},
  { 7,17,19, 0, 0, 0, 0, 0, 0, 0},
  {15, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {29, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};




const UChar g_aucRunDScan[8]=
{
  9,3,1,1,1,0,0,0
};
const UChar g_aucTcoeffDScan[9][5] =
{
  { 1, 3, 7,15,17},
  { 5,19, 0, 0, 0},
  { 9,21, 0, 0, 0},
  {11, 0, 0, 0, 0},
  {13, 0, 0, 0, 0},
  {23, 0, 0, 0, 0},
  {25, 0, 0, 0, 0},
  {27, 0, 0, 0, 0},
  {29, 0, 0, 0, 0},
};


ErrVal UvlcWriter::residualBlock( MbDataAccess& rcMbDataAccess,
                                  LumaIdx       cIdx,
                                  ResidualMode  eResidualMode,
                                  UInt          uiStart,
                                  UInt          uiStop )
{
  const UChar*  pucScan;
  TCoeff*       piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );
  const Bool    bFrame  = ( FRAME == rcMbDataAccess.getMbPicType());

  Int   iLevel;
  Int   iRun     = 0;
  UInt  uiPos    = uiStart;
  UInt  uiMaxPos = uiStop;

  switch( eResidualMode )
  {
  case LUMA_I16_DC:
    {
      ROF( uiStart == 0 );
      pucScan = ( bFrame ? g_aucLumaFrameDCScan : g_aucLumaFieldDCScan );
      break;
    }
  case LUMA_I16_AC:
    {
      ROF( uiStop > 1 );
      pucScan = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      uiPos   = gMax( 1, uiStart );
      break;
    }
  case LUMA_SCAN:
    {
      pucScan = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      break;
    }
  default:
    return Err::m_nERR;
  }

  Int   aiLevelRun[32];
  UInt  uiTrailingOnes = 0;
  UInt  uiTotalRun     = 0;
  UInt  uiCoeffCnt     = 0;

  while( uiPos < uiMaxPos )
  {
    if( ( iLevel = piCoeff[ pucScan [ uiPos++ ] ]) )
    {
      if( abs(iLevel) == 1 )
      {
        m_uiCoeffCost += COEFF_COST[iRun];
        uiTrailingOnes++;
      }
      else
      {
        m_uiCoeffCost += MAX_VALUE;                // set high cost, shall not be discarded
        uiTrailingOnes = 0;
      }

      aiLevelRun[uiCoeffCnt]      = iLevel;
      aiLevelRun[uiCoeffCnt+0x10] = iRun;
      uiTotalRun += iRun;
      uiCoeffCnt++;
      iRun = 0;
    }
    else
    {
      iRun++;
    }
  }

  if( uiTrailingOnes > 3 )
  {
    uiTrailingOnes = 3;
  }

  Bool bDefaultScanIdx = ( uiStart == 0 && uiStop == 16 );

  switch( eResidualMode )
  {
  case LUMA_I16_DC:
    {
      ETRACE_TH( "Luma:" );
      ETRACE_V( cIdx );
      ETRACE_N;
      RNOK( xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, true ) );
      RNOK( xWriteRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, 16, uiTotalRun, rcMbDataAccess, true ) );
      break;
    }
  case LUMA_I16_AC:
    {
      ETRACE_TH( "Luma:" );
      ETRACE_V( cIdx );
      ETRACE_N;
      RNOK( xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, false ) );
      RNOK( xWriteRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop-gMax(1,uiStart), uiTotalRun, rcMbDataAccess, bDefaultScanIdx ) );
      break;
    }
  case LUMA_SCAN:
    {
      ETRACE_TH( "Luma:" );
      ETRACE_V( cIdx );
      ETRACE_N;
      RNOK( xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, false ) );
      RNOK( xWriteRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop-uiStart, uiTotalRun, rcMbDataAccess, bDefaultScanIdx ) );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( cIdx, uiCoeffCnt != 0, true);
      break;
    }
  default:
    {
      AF();
    }
  }

  return Err::m_nOK;
}


ErrVal UvlcWriter::residualBlock( MbDataAccess& rcMbDataAccess,
                                  ChromaIdx     cIdx,
                                  ResidualMode  eResidualMode,
                                  UInt          uiStart,
                                  UInt          uiStop )
{

  TCoeff*       piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );
  const Bool    bFrame  = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar*  pucScan;
  Int           iRun = 0, iLevel;
  UInt          uiPos, uiMaxPos;

  switch( eResidualMode )
  {
  case CHROMA_DC:
    {
      ROF( uiStart == 0 );
      pucScan  = g_aucIndexChromaDCScan;
      uiPos    = 0;
      uiMaxPos = 4;
      break;
    }
  case CHROMA_AC:
    {
      ROF( uiStop > 1 );
      pucScan  = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      uiPos    = gMax( 1, uiStart);
      uiMaxPos = uiStop;
      break;
    }
  default:
    return Err::m_nERR;
  }

  Int aiLevelRun[32];

  UInt uiTrailingOnes = 0;
  UInt uiTotalRun     = 0;
  UInt uiCoeffCnt     = 0;

  while( uiPos < uiMaxPos )
  {
    if( ( iLevel = piCoeff[ pucScan [ uiPos++ ] ]) )
    {
      if( abs(iLevel) == 1 )
      {
        m_uiCoeffCost += COEFF_COST[iRun];
        uiTrailingOnes++;
      }
      else
      {
        m_uiCoeffCost += MAX_VALUE;                // set high cost, shall not be discarded
        uiTrailingOnes = 0;
      }

      aiLevelRun[uiCoeffCnt]      = iLevel;
      aiLevelRun[uiCoeffCnt+0x10] = iRun;
      uiTotalRun += iRun;
      uiCoeffCnt++;
      iRun = 0;
    }
    else
    {
      iRun++;
    }
  }

  if( uiTrailingOnes > 3 )
  {
    uiTrailingOnes = 3;
  }

  Bool bDefaultScanIdx = ( uiStart == 0 && uiStop == 16 );

  switch( eResidualMode )
  {
  case CHROMA_AC:
    {
      ETRACE_TH( "CHROMA_AC:" );
      ETRACE_V( cIdx );
      ETRACE_N;
      RNOK( xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop ) );
      RNOK( xWriteRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop - gMax( 1, uiStart), uiTotalRun, rcMbDataAccess, bDefaultScanIdx ) );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( 16 + cIdx, uiCoeffCnt != 0, true);
      break;
    }
  case CHROMA_DC:
    {
      ETRACE_TH( "CHROMA_DC:" );
      ETRACE_V( cIdx );
      ETRACE_N;
      RNOK( xWriteTrailingOnes4( uiCoeffCnt, uiTrailingOnes ) );
      RNOK( xWriteRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, 4, uiTotalRun, rcMbDataAccess, true ) );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( 24 + cIdx.plane(), uiCoeffCnt != 0, true);
      break;
    }
  default:
    {
      AF();
    }
  }

  return Err::m_nOK;
}


ErrVal UvlcWriter::deltaQp( MbDataAccess& rcMbDataAccess )
{
  ETRACE_TH ("DQp");

  RNOK( xWriteSvlcCode( rcMbDataAccess.getDeltaQp() ) );

  ETRACE_TY ("se(v)");
  ETRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcWriter::finishSlice()
{
  if( m_bRunLengthCoding && m_uiRun )
  {
    ETRACE_TH( "Run" );
    RNOK( xWriteUvlcCode( m_uiRun ) );
    ETRACE_N;
  }

  return Err::m_nOK;
}


ErrVal UvlcWriter::xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, UInt uiCoeffCount, UInt uiTrailingOnes, UInt uiStart, UInt uiStop )
{
  UInt uiCoeffCountCtx = rcMbDataAccess.getCtxCoeffCount( cIdx, uiStart, uiStop );

  if( uiStart != 0 || uiStop != 16 )
  {
    // Re-pack VLC matrix
    // NB: This only needs to be done once at the start of the slice.

    UInt uiMapIdx, ui, uj;
    UChar aucCwMapTO16[4][17];
    UInt uiBw     = uiStop-uiStart;
    UInt uiXdim   = gMin(16,uiBw)+1;
    UInt uiYdim   = gMin(3, uiBw)+1;
    UInt uiZ[5]   = {0,1,2,4,7};
    UInt uiMaxVal = uiXdim*uiYdim - uiZ[uiYdim];

    UInt uiCoeffCountCtxMap = gMin(uiCoeffCountCtx, 2);
    memcpy(aucCwMapTO16, g_aucCwMapTO16[uiCoeffCountCtxMap], sizeof(UChar)*4*17);
    for( uiMapIdx=1; uiMapIdx<=uiMaxVal; uiMapIdx++ )
    {
      Bool bFound = false;
      for( ui=0; ui<uiYdim; ui++ )
        for( uj=1; uj<uiXdim; uj++ )
          if( aucCwMapTO16[ui][uj] == uiMapIdx )
          {
            bFound = true;
            break;
          }
      if( !bFound )
      {
        for( ui=0; ui<uiYdim; ui++ )
          for( uj=1; uj<uiXdim; uj++ )
            if( aucCwMapTO16[ui][uj] > uiMapIdx )
              aucCwMapTO16[ui][uj]--;
        uiMapIdx--;
      }
    }
    xWriteCodeNT(g_aucCwCodeVectTO16[uiCoeffCountCtx][aucCwMapTO16[uiTrailingOnes][uiCoeffCount]], g_aucCwLenVectTO16[uiCoeffCountCtx][aucCwMapTO16[uiTrailingOnes][uiCoeffCount]]);
  }
  else
  {
    xWriteTrailingOnes16( uiCoeffCountCtx, uiCoeffCount, uiTrailingOnes );
  }

  rcMbDataAccess.getMbTCoeffs().setCoeffCount( cIdx, uiCoeffCount );

  return Err::m_nOK;
}


ErrVal UvlcWriter::xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, LumaIdx cIdx, UInt uiCoeffCount, UInt uiTrailingOnes, UInt uiStart, UInt uiStop, Bool bDC )
{
  UInt uiCoeffCountCtx = rcMbDataAccess.getCtxCoeffCount( cIdx, uiStart, uiStop );

  if( ( uiStart != 0 || uiStop != 16 ) && !bDC )
  {
    // Re-pack VLC matrix
    // NB: This only needs to be done once at the start of the slice.

    UInt uiMapIdx, ui, uj;
    UChar aucCwMapTO16[4][17];
    UInt uiBw     = uiStop-uiStart;
    UInt uiXdim   = gMin(16,uiBw)+1;
    UInt uiYdim   = gMin(3, uiBw)+1;
    UInt uiZ[5]   = {0,1,2,4,7};
    UInt uiMaxVal = uiXdim*uiYdim - uiZ[uiYdim];

    UInt uiCoeffCountCtxMap = gMin(uiCoeffCountCtx, 2);
    memcpy(aucCwMapTO16, g_aucCwMapTO16[uiCoeffCountCtxMap], sizeof(UChar)*4*17);
    for( uiMapIdx=1; uiMapIdx<=uiMaxVal; uiMapIdx++ )
    {
      Bool bFound = false;
      for( ui=0; ui<uiYdim; ui++ )
        for( uj=1; uj<uiXdim; uj++ )
          if( aucCwMapTO16[ui][uj] == uiMapIdx )
          {
            bFound = true;
            break;
          }
      if( !bFound )
      {
        for( ui=0; ui<uiYdim; ui++ )
          for( uj=1; uj<uiXdim; uj++ )
            if( aucCwMapTO16[ui][uj] > uiMapIdx )
              aucCwMapTO16[ui][uj]--;
        uiMapIdx--;
      }
    }

    xWriteCodeNT(g_aucCwCodeVectTO16[uiCoeffCountCtx][aucCwMapTO16[uiTrailingOnes][uiCoeffCount]], g_aucCwLenVectTO16[uiCoeffCountCtx][aucCwMapTO16[uiTrailingOnes][uiCoeffCount]]);
  }
  else
  {
    xWriteTrailingOnes16( uiCoeffCountCtx, uiCoeffCount, uiTrailingOnes );
  }

  rcMbDataAccess.getMbTCoeffs().setCoeffCount( cIdx, uiCoeffCount );

  return Err::m_nOK;
}

ErrVal UvlcWriter::xWriteRunLevel( Int* aiLevelRun, UInt uiCoeffCnt, UInt uiTrailingOnes, UInt uiMaxCoeffs, UInt uiTotalRun, MbDataAccess &rcMbDataAccess, Bool bDefaultScanIdx )
{
  ROTRS( 0 == uiCoeffCnt, Err::m_nOK );

  if( uiTrailingOnes )
  {
    UInt uiBits = 0;
    Int n = uiTrailingOnes-1;
    for( UInt k = uiCoeffCnt; k > uiCoeffCnt-uiTrailingOnes; k--, n--)
    {
      if( aiLevelRun[k-1] < 0)
      {
        uiBits |= 1<<n;
      }
    }

    RNOK( m_pcBitWriteBufferIf->write( uiBits, uiTrailingOnes ))
    ETRACE_POS;
    ETRACE_TH( "  TrailingOnesSigns: " );
    ETRACE_V( uiBits );
    ETRACE_N;
    ETRACE_COUNT(uiTrailingOnes);
  }


  Int iHighLevel = ( uiCoeffCnt > 3 && uiTrailingOnes == 3) ? 0 : 1;
  Int iVlcTable  = ( uiCoeffCnt > 10 && uiTrailingOnes < 3) ? 1 : 0;

  for( Int k = uiCoeffCnt - 1 - uiTrailingOnes; k >= 0; k--)
  {
    Int iLevel;
    iLevel = aiLevelRun[k];

    UInt uiAbsLevel = (UInt)abs(iLevel);

    if( iHighLevel )
    {
      iLevel -= ( iLevel > 0 ) ? 1 : -1;
	    iHighLevel = 0;
    }

    if( iVlcTable == 0 )
    {
	    xWriteLevelVLC0( iLevel );
    }
    else
    {
	    xWriteLevelVLCN( iLevel, iVlcTable );
    }

    // update VLC table
    if( uiAbsLevel > g_auiIncVlc[ iVlcTable ] )
    {
      iVlcTable++;
    }

    if( k == Int(uiCoeffCnt - 1 - uiTrailingOnes) && uiAbsLevel > 3)
    {
      iVlcTable = 2;
    }

  }

  ROFRS( uiCoeffCnt < uiMaxCoeffs, Err::m_nOK );


  iVlcTable = uiCoeffCnt-1;
  if( ! bDefaultScanIdx )
  {
    if( uiMaxCoeffs <= 4 )
    {
      UInt uiTempVlcTable = gMin(iVlcTable + 4-uiMaxCoeffs, 2);
      xWriteTotalRun4( uiTempVlcTable, uiTotalRun );
    }
    else if( uiMaxCoeffs <= 8 )
    {
      UInt uiTempVlcTable = gMin(iVlcTable + 8-uiMaxCoeffs, 6);
      xWriteTotalRun8( uiTempVlcTable, uiTotalRun );
    }
    else
    {
      UInt uiTempVlcTable = iVlcTable;
      if( uiMaxCoeffs < 15 )
        uiTempVlcTable = gMin(iVlcTable + 16-uiMaxCoeffs, 14);
      xWriteTotalRun16( uiTempVlcTable, uiTotalRun );
    }
  }
  else
  {
    if( uiMaxCoeffs <= 4 )
    {
      xWriteTotalRun4( iVlcTable, uiTotalRun );
    }
    else
    {
      xWriteTotalRun16( iVlcTable, uiTotalRun );
    }
  }

  // decode run before each coefficient
  uiCoeffCnt--;
  if( uiTotalRun > 0 && uiCoeffCnt > 0)
  {
    do
    {
      iVlcTable = (( uiTotalRun > RUNBEFORE_NUM) ? RUNBEFORE_NUM : uiTotalRun) - 1;
      UInt uiRun = aiLevelRun[uiCoeffCnt+0x10];

      xWriteRun( iVlcTable, uiRun );

      uiTotalRun -= uiRun;
      uiCoeffCnt--;
    } while( uiTotalRun != 0 && uiCoeffCnt != 0);
  }

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteTrailingOnes16( UInt uiLastCoeffCount, UInt uiCoeffCount, UInt uiTrailingOnes )
{
  UInt uiVal;
  UInt uiSize;

  ETRACE_POS;
  if( 3 == uiLastCoeffCount )
  {
    UInt uiBits = 3;
    if( uiCoeffCount )
    {
      uiBits = (uiCoeffCount-1)<<2 | uiTrailingOnes;
    }
    RNOK( m_pcBitWriteBufferIf->write( uiBits, 6) );
    ETRACE_DO( m_uiBitCounter = 6 );

    uiVal = uiBits;
    uiSize = 6;
  }
  else
  {
    RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount],
                                  g_aucLenTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount] ) );
    ETRACE_DO( m_uiBitCounter = g_aucLenTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount] );

    uiVal = g_aucCodeTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount];
    uiSize = g_aucLenTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount];
  }

  ETRACE_TH( "  TrailingOnes16: Vlc: " );
  ETRACE_V( uiLastCoeffCount );
  ETRACE_TH( " CoeffCnt: " );
  ETRACE_V( uiCoeffCount );
  ETRACE_TH( " TraiOnes: " );
  ETRACE_V( uiTrailingOnes );
  ETRACE_N;
  ETRACE_COUNT(m_uiBitCounter);

  return Err::m_nOK;
}



ErrVal UvlcWriter::xWriteTrailingOnes4( UInt uiCoeffCount, UInt uiTrailingOnes )
{
  RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTableTO4[uiTrailingOnes][uiCoeffCount],
                                g_aucLenTableTO4[uiTrailingOnes][uiCoeffCount] ) );

  ETRACE_POS;
  ETRACE_TH( "  TrailingOnes4: CoeffCnt: " );
  ETRACE_V( uiCoeffCount );
  ETRACE_TH( " TraiOnes: " );
  ETRACE_V( uiTrailingOnes );
  ETRACE_N;
  ETRACE_COUNT(g_aucLenTableTO4[uiTrailingOnes][uiCoeffCount]);

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteTotalRun4( UInt uiVlcPos, UInt uiTotalRun )
{
  RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTableTZ4[uiVlcPos][uiTotalRun],
                                g_aucLenTableTZ4[uiVlcPos][uiTotalRun] ) );

  ETRACE_POS;
  ETRACE_TH( "  TotalZeros4 vlc: " );
  ETRACE_V( uiVlcPos );
  ETRACE_TH( " TotalRun: " );
  ETRACE_V( uiTotalRun );
  ETRACE_N;
  ETRACE_COUNT(g_aucLenTableTZ4[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteTotalRun8( UInt uiVlcPos, UInt uiTotalRun )
{
  RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTableTZ8[uiVlcPos][uiTotalRun],
    g_aucLenTableTZ8[uiVlcPos][uiTotalRun] ) );

  ETRACE_POS;
  ETRACE_TH( "  TotalZeros8 vlc: " );
  ETRACE_V( uiVlcPos );
  ETRACE_TH( " TotalRun: " );
  ETRACE_V( uiTotalRun );
  ETRACE_N;
  ETRACE_COUNT(g_aucLenTableTZ8[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteTotalRun16( UInt uiVlcPos, UInt uiTotalRun )
{
  RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTableTZ16[uiVlcPos][uiTotalRun],
                                g_aucLenTableTZ16[uiVlcPos][uiTotalRun] ) );

  ETRACE_POS;
  ETRACE_TH( "  TotalRun16 vlc: " );
  ETRACE_V( uiVlcPos );
  ETRACE_TH( " TotalRun: " );
  ETRACE_V( uiTotalRun );
  ETRACE_N;
  ETRACE_COUNT(g_aucLenTableTZ16[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteRun( UInt uiVlcPos, UInt uiRun  )
{
  RNOK( m_pcBitWriteBufferIf->write( g_aucCodeTable3[uiVlcPos][uiRun],
                                g_aucLenTable3[uiVlcPos][uiRun] ) );

  ETRACE_POS;
  ETRACE_TH( "  Run" );
  ETRACE_CODE( uiRun );
  ETRACE_COUNT (g_aucLenTable3[uiVlcPos][uiRun]);
  ETRACE_N;

  return Err::m_nOK;
}




ErrVal UvlcWriter::xWriteLevelVLC0( Int iLevel )
{

  UInt uiLength;
  UInt uiLevel = abs( iLevel );
  UInt uiSign = ((UInt)iLevel)>>31;

  UInt uiBits;

  if( 8 > uiLevel )
  {
    uiBits   = 1;
    uiLength = 2 * uiLevel - 1 + uiSign;
  }
  else if( 16 > uiLevel )
  {
    uiBits   = 2*uiLevel + uiSign;
    uiLength = 15 + 4;
  }
  else
  {
    uiBits   = 0x1000-32 + (uiLevel<<1) + uiSign;
    uiLength = 16 + 12;
  }


  RNOK( m_pcBitWriteBufferIf->write( uiBits, uiLength ) );

  ETRACE_POS;
  ETRACE_TH( "  VLC0 lev " );
  ETRACE_CODE( iLevel );
  ETRACE_N;
  ETRACE_COUNT( uiLength );

  return Err::m_nOK;

}

ErrVal UvlcWriter::xWriteLevelVLCN( Int iLevel, UInt uiVlcLength )
{
  UInt uiLength;
  UInt uiLevel = abs( iLevel );
  UInt uiSign = ((UInt)iLevel)>>31;
  UInt uiBits;

  UInt uiShift = uiVlcLength-1;
  UInt uiEscapeCode = (0xf<<uiShift)+1;

  if( uiLevel < uiEscapeCode )
  {
    uiLevel--;
	  uiLength = (uiLevel>>uiShift) + uiVlcLength + 1;
    uiLevel &= ~((0xffffffff)<<uiShift);
	  uiBits   = (2<<uiShift) | 2*uiLevel | uiSign;
  }
  else
  {
	  uiLength = 28;
	  uiBits   = 0x1000 + 2*(uiLevel-uiEscapeCode) + uiSign;
  }



  RNOK( m_pcBitWriteBufferIf->write( uiBits, uiLength ) );

  ETRACE_POS;
  ETRACE_TH( "  VLCN lev: " );
  ETRACE_CODE( iLevel );
  ETRACE_N;
  ETRACE_COUNT( uiLength );

  return Err::m_nOK;
}



ErrVal UvlcWriter::samplesPCM( MbDataAccess& rcMbDataAccess )
{
  ETRACE_POS;
  ETRACE_TH( "  PCM SAMPLES: " );

  RNOK( m_pcBitWriteBufferIf->writeAlignZero() );

  AOF_DBG( rcMbDataAccess.getMbData().isPCM() );

  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 16 );
  TCoeff* pCoeff = rcMbDataAccess.getMbTCoeffs().getTCoeffBuffer();

  const UInt uiFactor = 8*8;
  const UInt uiSize   = uiFactor*2*3;
  RNOK( m_pcBitWriteBufferIf->pcmSamples( pCoeff, uiSize ) );

  ETRACE_N;
  ETRACE_COUNT( uiFactor*6 );

  return Err::m_nOK;
}



ErrVal UvlcWriter::xWriteRefFrame( Bool bWriteBit, UInt uiRefFrame )
{
  ETRACE_TH( "RefFrame" );

  if( bWriteBit )
  {
    RNOK( xWriteFlag( 1-uiRefFrame ) );
  }
  else
  {
    RNOK( xWriteUvlcCode( uiRefFrame ) );
  }

  ETRACE_V( uiRefFrame+1 );
  ETRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcWriter::xWriteMotionPredFlag( Bool bFlag )
{
  ETRACE_TH( "MotionPredFlag" );

  UInt  uiCode = ( bFlag ? 1 : 0 );
  RNOK( xWriteFlag( uiCode) );

  ETRACE_V( uiCode );
  ETRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcWriter::transformSize8x8Flag( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  ETRACE_TH( "transformSize8x8Flag:" );

  UInt  uiCode = rcMbDataAccess.getMbData().isTransformSize8x8() ? 1 : 0;
  RNOK( xWriteFlag( uiCode) );

  ETRACE_V( uiCode );
  ETRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcWriter::residualBlock8x8( MbDataAccess&  rcMbDataAccess,
                                     B8x8Idx        c8x8Idx,
                                     ResidualMode   eResidualMode,
                                     UInt           uiStart,
                                     UInt           uiStop )
{
  ROF( eResidualMode == LUMA_SCAN );

  const Bool    bFrame  = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar*  pucScan = (bFrame) ? g_aucFrameScan64 : g_aucFieldScan64;
  TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get8x8( c8x8Idx );

  UInt  uiBlk;
  Int   iLevel;
  Int   iOverallRun = 0;
  UInt  uiPos       = uiStart << 2;
  UInt  uiMaxPos    = uiStop  << 2;

  Int   aaiLevelRun     [4][32];
  Int   aiRun           [4]     = { 0, 0, 0, 0 };
  UInt  auiTrailingOnes [4]     = { 0, 0, 0, 0 };
  UInt  auiTotalRun     [4]     = { 0, 0, 0, 0 };
  UInt  auiCoeffCnt     [4]     = { 0, 0, 0, 0 };

  while( uiPos < uiMaxPos )
  {
    uiBlk = ( uiPos % 4 );

    if( ( iLevel = piCoeff[ pucScan[ uiPos++ ] ] ) )
    {
      if( abs(iLevel) == 1 )
      {
        m_uiCoeffCost         += COEFF_COST8x8[ iOverallRun ];
        auiTrailingOnes[uiBlk]++;
      }
      else
      {
        m_uiCoeffCost         += MAX_VALUE;
        auiTrailingOnes[uiBlk] = 0;
      }

      aaiLevelRun[uiBlk][auiCoeffCnt[uiBlk]]      = iLevel;
      aaiLevelRun[uiBlk][auiCoeffCnt[uiBlk]+0x10] = aiRun[uiBlk];
      auiTotalRun[uiBlk]  += aiRun[uiBlk];
      auiCoeffCnt[uiBlk]  ++;
      aiRun      [uiBlk]  = 0;
      iOverallRun         = 0;
    }
    else
    {
      aiRun[uiBlk]++;
      iOverallRun ++;
    }
  }


  //===== loop over 4x4 blocks =====
  Bool bDefaultScanIdx = ( uiStart == 0 && uiStop == 16 );
  for( uiBlk = 0; uiBlk < 4; uiBlk++ )
  {
    if( auiTrailingOnes[uiBlk] > 3 )
    {
      auiTrailingOnes[uiBlk] = 3;
    }
    B4x4Idx cIdx( c8x8Idx.b4x4() + 4*(uiBlk/2) + (uiBlk%2) );
    RNOK( xPredictNonZeroCnt( rcMbDataAccess, cIdx, auiCoeffCnt[uiBlk], auiTrailingOnes[uiBlk], uiStart, uiStop, false ) );
    RNOK( xWriteRunLevel( aaiLevelRun[uiBlk],
                          auiCoeffCnt[uiBlk],
                          auiTrailingOnes[uiBlk],
                          uiStop-uiStart,
                          auiTotalRun[uiBlk],
                          rcMbDataAccess, bDefaultScanIdx ) );
  }

  return Err::m_nOK;
}



ErrVal
UvlcWriter::xWriteGolomb(UInt uiSymbol, UInt uiK)
{
  UInt uiQ = uiSymbol / uiK;
  UInt uiR = uiSymbol - uiQ * uiK;
  UInt uiC = 0;
  UInt uiT = uiK >> 1;

  while ( uiT > 0 )
  {
    uiC++;
    uiT >>= 1;
  }

  // Unary part
  for ( UInt ui = 0; ui < uiQ; ui++ )
  {
    RNOK( xWriteFlag( 1 ) );
  }
  RNOK( xWriteFlag( 0 ) );

  // Binary part
  if ( uiR < uiC )
  {
    RNOK( xWriteCode( uiR, uiC ) );
  } else if ( uiC > 0 ) {
    RNOK( xWriteFlag( 1 ) );
    RNOK( xWriteCode( uiR - uiC, uiC ) );
  }
  ETRACE_N;

  return Err::m_nOK;
}

UInt g_auiSigRunTabCode[] = {0x01, 0x01, 0x01, 0x01, 0x00};
UInt g_auiSigRunTabCodeLen[] = {1, 2, 3, 4, 4};


ErrVal
UvlcWriter::xEncodeMonSeq ( UInt* auiSeq, UInt uiStartVal, UInt uiLen )
{
  UInt uiRun   = 0;
  UInt uiLevel = uiStartVal;
  for (UInt uiPos=0; uiPos<uiLen && uiLevel > 0; uiPos++)
  {
    if (auiSeq[uiPos] == uiLevel)
    {
      uiRun++;
    } else {
      ETRACE_TH("eob_run");
      RNOK( xWriteGolomb( uiRun, 1 ) );
      uiRun = 1;
      uiLevel--;
      while ( uiLevel > auiSeq[uiPos] )
      {
        ETRACE_TH("eob_run");
        RNOK( xWriteGolomb( 0, 1 ) );
        uiLevel--;
      }
    }
  }
  if (uiLevel > 0)
  {
    ETRACE_TH("eob_run");
    RNOK( xWriteGolomb( uiRun, 1 ) );
  }
  return Err::m_nOK;
}

ErrVal
UvlcWriter::xWriteSigRunCode ( UInt uiSymbol, UInt uiTableIdx )
{

  assert( uiTableIdx >= 0 && uiTableIdx <= 4 );
  if(uiTableIdx == 0)
  {
    // unary code
    RNOK ( xWriteUnaryCode (uiSymbol) );
  }
  else if (uiTableIdx == 1)
  {
    RNOK ( xWriteCodeCB1 (uiSymbol) );
  }
  else if (uiTableIdx == 2)
  {
    RNOK ( xWriteCodeCB2 (uiSymbol) );
  }
  else if (uiTableIdx == 3)
  {
    if ( uiSymbol == 0 )
    {
      RNOK( xWriteFlag( 1 ) );
    }
    else
    {
      RNOK( xWriteFlag( 0 ) );
      RNOK( xWriteCodeCB2 (uiSymbol-1) );
    }
  }
  else // uiTableIdx == 4
  {
    if(uiSymbol == 0)
    {
      RNOK (xWriteFlag ( 1 ));
    }
    else
    {
      RNOK (xWriteCodeCB1(uiSymbol+1));
    }
  }

  return Err::m_nOK;
}

ErrVal
UvlcWriter::xWriteUnaryCode ( UInt uiSymbol )
{
  UInt uiStart = 0;
  do
  {
    if(uiSymbol == uiStart)
    {
      RNOK( xWriteFlag (1) );
      break;
    }
    else
    {
      RNOK( xWriteFlag (0) );
      uiStart++;
    }
  }
  while (true);
  return Err::m_nOK;
}

ErrVal
UvlcWriter::xWriteCodeCB1 ( UInt uiSymbol )
{
  // this function writes codeword for the input symbol according to the {2, 2, 3, 3, 4, 4...} codebook
  for(UInt ui = 0; ui < uiSymbol/2; ui ++)
  {
    RNOK (xWriteFlag (0));
  }

  RNOK (xWriteCode((3-(uiSymbol%2)), 2)) ;

  return Err::m_nOK;
}

ErrVal
UvlcWriter::xWriteCodeCB2 ( UInt uiSymbol )
{
  // this function writes codeword for the input symbol according to the {2, 2, 2, 4, 4, 4...} codebook
  for(UInt ui = 0; ui < uiSymbol/3; ui ++)
  {
    RNOK(xWriteCode (0, 2));
  }

  RNOK (xWriteCode((3-(uiSymbol%3)), 2));

  return Err::m_nOK;
}


//JVT-X046 {
void
UvlcWriter::loadUvlcWrite(MbSymbolWriteIf *pcMbSymbolWriteIf)
{
	UvlcWriter* pcUvlcWriter = (UvlcWriter*) (pcMbSymbolWriteIf);
	m_uiBitCounter = pcUvlcWriter->getBitCounter();
	m_uiPosCounter = pcUvlcWriter->getPosCounter();

	m_uiCoeffCost = pcUvlcWriter->getCoeffCost();
	m_bTraceEnable = pcUvlcWriter->getTraceEnable();

	m_bRunLengthCoding = pcUvlcWriter->getRunLengthCoding();
	m_uiRun = pcUvlcWriter->getRun();

  AOF( m_pcBitWriteBufferIf );
	m_pcBitWriteBufferIf->loadBitWriteBuffer(pcUvlcWriter->getBitWriteBufferIf());
}
//JVT-X046 }
H264AVC_NAMESPACE_END
