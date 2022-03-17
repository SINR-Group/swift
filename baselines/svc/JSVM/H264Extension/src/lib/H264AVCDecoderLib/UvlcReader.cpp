
#include "H264AVCDecoderLib.h"

#include "H264AVCCommonLib/TraceFile.h"
#include "H264AVCCommonLib/Tables.h"
#include "UvlcReader.h"
#include "BitReadBuffer.h"
#include "DecError.h"



H264AVC_NAMESPACE_BEGIN

#define RUNBEFORE_NUM 7
#define MAX_VALUE     0xdead
#define TOTRUN_NUM    15

const UInt g_auiIncVlc[7] =
{
  0, 3, 6, 12, 24, 48, 32768
};

const UChar COEFF_COST[16] =
{
  3, 2,2,1, 1,1,0,0,0,0,0,0,0,0,0,0
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

const UChar g_aucCbpIntra[48] =
{
   47, 31, 15,  0,
   23, 27, 29, 30,
    7, 11, 13, 14,
   39, 43, 45, 46,
   16,  3,  5, 10,
   12, 19, 21, 26,
   28, 35, 37, 42,
   44,  1,  2,  4,
    8, 17, 18, 20,
   24,  6,  9, 22,
   25, 32, 33, 34,
   36, 40, 38, 41
};


const UChar g_aucCbpInter[48] =
{
    0, 16,  1,  2,
    4,  8, 32,  3,
    5, 10, 12, 15,
   47,  7, 11, 13,
   14,  6,  9, 31,
   35, 37, 42, 44,
   33, 34, 36, 40,
   39, 43, 45, 46,
   17, 18, 20, 24,
   19, 21, 26, 28,
   23, 27, 29, 30,
   22, 25, 38, 41
};

const UChar g_aucCbpIntraMonochrome[16] =
{
  15,  0,  7, 11,
  13, 14,  3,  5,
  10, 12,  1,  2,
   4,  8,  6,  9
};

const UChar g_aucCbpInterMonochrome[16] =
{
   0,  1,  2,  4,
   8,  3,  5, 10,
  12, 15,  7, 11,
  13, 14,  6,  9
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
const UChar g_aucCwMapTO16[3][62] =
{
  {0,1,2,3,0,1,3,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,0,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,1,0,1,2,3,0,1,2,3,0,1,2,0},
  {0,1,2,3,3,1,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,0,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,0,0,1,2,3,0,1,2,3,0,2,3,1,0,1,2,3,0,1,2},
  {0,1,2,3,3,3,3,3,1,2,3,1,2,1,2,1,0,1,2,3,0,1,2,0,0,1,2,3,0,2,0,0,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,0,1,0,1,2,3,0,1,2,3,0,1,2,3,0}
};
const UChar g_aucCwMapTC16[3][62] =
{
  {0,1,2,3,1,2,4,3,5,2,3,4,6,3,4,5,7,4,5,6,8,5,6,7,9,6,7,8,10,7,8,9,8,9,9,10,11,10,10,11,12,11,11,12,13,12,12,13,14,13,13,14,14,15,14,15,15,16,15,16,16,16},
  {0,1,2,3,4,2,5,1,3,3,6,2,4,4,7,3,5,5,8,4,6,6,5,6,7,7,9,7,8,8,10,8,9,9,11,9,10,10,12,10,11,11,11,12,12,12,13,13,13,13,14,14,14,15,14,15,15,15,16,16,16,16},
  {0,1,2,3,4,5,6,7,2,3,8,3,4,4,5,5,1,6,6,9,2,7,7,3,4,8,8,10,5,9,6,7,8,9,10,11,9,10,11,12,10,11,12,13,11,12,13,12,13,13,14,14,14,14,15,15,15,15,16,16,16,16}
};

const UChar g_aucLenTableTO16[3][4][17] =
{
  {
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

UvlcReader::UvlcReader()
: m_pcBitReadBuffer ( 0 )
, m_uiBitCounter    ( 0 )
, m_uiPosCounter    ( 0 )
, m_bRunLengthCoding( false )
, m_uiRun           ( 0 )
{
}

UvlcReader::~UvlcReader()
{
}

__inline ErrVal UvlcReader::xGetCode( UInt& ruiCode, UInt uiLength )
{
  if( uiLength > 9 )
  {
    Char acType[] = "u(00)";
    acType[2] += uiLength/10;
    acType[3] += uiLength%10;
    DTRACE_TY( acType );
  }
  else
  {
    Char acType[] = " u(0)";
    acType[3] += uiLength;
    DTRACE_TY( acType );
  }

  ErrVal retVal = m_pcBitReadBuffer->get( ruiCode, uiLength );

  DTRACE_POS;
  DTRACE_CODE (ruiCode);
  DTRACE_BITS (ruiCode, uiLength );
  DTRACE_COUNT(uiLength);

  return retVal;
}


__inline ErrVal UvlcReader::xGetFlag( UInt& ruiCode )
{
  DTRACE_TY( " u(1)" );

  ErrVal retVal = m_pcBitReadBuffer->get( ruiCode );

  DTRACE_POS;
  DTRACE_CODE (ruiCode);
  DTRACE_BITS (ruiCode, 1 );
  DTRACE_COUNT(1);

  return retVal;
}


Bool UvlcReader::moreRBSPData()
{
  return m_pcBitReadBuffer->isValid();
}


ErrVal UvlcReader::create( UvlcReader*& rpcUvlcReader )
{
  rpcUvlcReader = new UvlcReader;

  ROT( NULL == rpcUvlcReader );

  return Err::m_nOK;
}

ErrVal UvlcReader::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal UvlcReader::init(  BitReadBuffer* pcBitReadBuffer )
{
  ROT( NULL == pcBitReadBuffer );

  m_pcBitReadBuffer = pcBitReadBuffer;

  m_bRunLengthCoding  = false;
  m_uiRun             = 0;

  return Err::m_nOK;
}

ErrVal UvlcReader::uninit()
{
  m_pcBitReadBuffer = NULL;
  return Err::m_nOK;
}



ErrVal UvlcReader::xGetUvlcCode( UInt& ruiVal)
{
  UInt uiVal = 0;
  UInt uiCode = 0;
  UInt uiLength;

  DTRACE_DO( m_uiBitCounter = 1 );
  DTRACE_TY( "ue(v)" );

  DECRNOK( m_pcBitReadBuffer->get( uiCode, 1 ) );
  DTRACE_BITS(uiCode, 1);

  if( 0 == uiCode )
  {
    uiLength = 0;

    while( ! ( uiCode & 1 ))
    {
      DECRNOK( m_pcBitReadBuffer->get( uiCode, 1 ) );
      DTRACE_BITS(uiCode, 1);
      uiLength++;
    }

    DTRACE_DO( m_uiBitCounter += 2*uiLength );

    DECRNOK( m_pcBitReadBuffer->get( uiVal, uiLength ) );
    DTRACE_BITS(uiVal, uiLength);

    uiVal += (1 << uiLength)-1;
  }

  ruiVal = uiVal;

  DTRACE_POS;
  DTRACE_CODE(uiVal);
  DTRACE_COUNT(m_uiBitCounter);

  return Err::m_nOK;
}


ErrVal UvlcReader::xGetSvlcCode( Int& riVal)
{
  UInt uiBits = 0;

  DTRACE_DO( m_uiBitCounter = 1 );
  DTRACE_TY( "se(v)" );

  DECRNOK( m_pcBitReadBuffer->get( uiBits, 1 ) );
  DTRACE_BITS(uiBits, 1);
  if( 0 == uiBits )
  {
    UInt uiLength = 0;

    while( ! ( uiBits & 1 ))
    {
      DECRNOK( m_pcBitReadBuffer->get( uiBits, 1 ) );
      DTRACE_BITS(uiBits, 1);
      uiLength++;
    }

    DTRACE_DO( m_uiBitCounter += 2*uiLength );

    DECRNOK( m_pcBitReadBuffer->get( uiBits, uiLength ) );
    DTRACE_BITS(uiBits, uiLength);

    uiBits += (1 << uiLength);
    riVal = ( uiBits & 1) ? -(Int)(uiBits>>1) : (Int)(uiBits>>1);
  }
  else
  {
    riVal = 0;
  }

  DTRACE_POS;
  DTRACE_CODE(riVal);
  DTRACE_COUNT(m_uiBitCounter);

  return Err::m_nOK;
}




ErrVal UvlcReader::getFlag( Bool& rbFlag, const Char* pcTraceString )
{
  DTRACE_TH( pcTraceString );

  UInt uiCode;
  DECRNOK( xGetFlag( uiCode ) );

  rbFlag = (1 == uiCode);
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::getCode( UInt& ruiCode, UInt uiLength, const Char* pcTraceString )
{
  DTRACE_TH( pcTraceString );
  RNOK( xGetCode( ruiCode, uiLength) );
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::getUvlc( UInt& ruiCode, const Char* pcTraceString )
{
  DTRACE_TH( pcTraceString );

  DECRNOK( xGetUvlcCode( ruiCode ) );

  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::getSvlc( Int& riCode, const Char* pcTraceString )
{
  DTRACE_TH( pcTraceString );

  DECRNOK( xGetSvlcCode( riCode ) );

  DTRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcReader::readByteAlign()
{
  UInt uiCode;
  UInt uiLength = m_pcBitReadBuffer->getBitsUntilByteAligned();

  ROTRS( 0 == uiLength, Err::m_nOK );

  DECRNOK( m_pcBitReadBuffer->get( uiCode, uiLength ) );

  DECROF( (UInt(1<<uiLength)>>1) == uiCode );

  while( uiLength-- )
  {
    DTRACE_POS;
    DTRACE_TH( "SEI: alignment_bit" );
    DTRACE_TY( " u(1)" );
    DTRACE_CODE( ( uiCode >> uiLength ) & 1 );
    DTRACE_BITS( ( uiCode >> uiLength ) & 1, 1 );
    DTRACE_N;
    DTRACE_COUNT( 1 );
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::readZeroByteAlign()
{
  UInt uiCode;
  UInt uiLength = m_pcBitReadBuffer->getBitsUntilByteAligned();

  ROTRS( 0 == uiLength, Err::m_nOK );

  DECRNOK( m_pcBitReadBuffer->get( uiCode, uiLength ) );

  while( uiLength-- )
  {
    DTRACE_POS;
    DTRACE_TH( "Scalable NestingSEI: zero_alignment_bit" );
    DTRACE_TY( " u(1)" );
    DTRACE_CODE( ( uiCode >> uiLength ) & 1 );
    DTRACE_BITS( ( uiCode >> uiLength ) & 1, 1 );
    DTRACE_N;
    DTRACE_COUNT( 1 );
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::getSCode( Int& riCode, UInt uiLength, const Char* pcTraceString )
{
  DTRACE_TH( pcTraceString );
  DTRACE_TY( " u(v)" );

  UInt uiCode;
  DECRNOK( m_pcBitReadBuffer->get( uiCode, uiLength ) );

  UInt uiShift = 32 - uiLength;
  riCode = ((Int)(uiCode << uiShift)) >> uiShift;

  DTRACE_POS;
  DTRACE_CODE (riCode);
  DTRACE_BITS (uiCode, uiLength );
  DTRACE_COUNT(uiLength);
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::startSlice( const SliceHeader& rcSliceHeader )
{
  m_bRunLengthCoding  = ! rcSliceHeader.isIntraSlice();
  m_uiRun             = 0;
  return Err::m_nOK;
}

ErrVal UvlcReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  UInt uiRefFrame = 0;
	RNOK( xGetRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), uiRefFrame, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( ++uiRefFrame );
  return Err::m_nOK;
}

ErrVal UvlcReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  UInt uiRefFrame = 0;
	RNOK( xGetRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), uiRefFrame, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( ++uiRefFrame, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  UInt uiRefFrame = 0;
	RNOK( xGetRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), uiRefFrame, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( ++uiRefFrame, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::refFrame( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx  )
{
  UInt uiRefFrame = 0;
	RNOK( xGetRefFrame( 2 == rcMbDataAccess.getNumActiveRef( eLstIdx ), uiRefFrame, eLstIdx ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setRefIdx( ++uiRefFrame, eParIdx );
  return Err::m_nOK;
}

ErrVal  UvlcReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Bool bFlag;
  RNOK( xGetMotionPredFlag( bFlag ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag );
  return Err::m_nOK;
}
ErrVal  UvlcReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx   )
{
  Bool bFlag;
  RNOK( xGetMotionPredFlag( bFlag ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}
ErrVal  UvlcReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx   )
{
  Bool bFlag;
  RNOK( xGetMotionPredFlag( bFlag ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}
ErrVal  UvlcReader::motionPredFlag( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8 eParIdx   )
{
  Bool bFlag;
  RNOK( xGetMotionPredFlag( bFlag ) );
  rcMbDataAccess.getMbMotionData( eLstIdx ).setMotPredFlag( bFlag, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::blockModes( MbDataAccess& rcMbDataAccess )
{
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    DTRACE_TH( "BlockMode" );

    UInt uiBlockMode = 0;
    RNOK( xGetUvlcCode( uiBlockMode ) );

    rcMbDataAccess.setConvertBlkMode( c8x8Idx.b8x8Index(), uiBlockMode );

    DTRACE_N;
  }
  return Err::m_nOK;
}

Bool UvlcReader::isMbSkipped( MbDataAccess& rcMbDataAccess, UInt& uiNextSkippedVLC )
{
  uiNextSkippedVLC = 0;
  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );

  ROFRS( m_bRunLengthCoding, false );

  if ( m_uiRun > 0 )
  {
    m_uiRun--;
  } else {
    DTRACE_TH( "Run" );
    ANOK( xGetUvlcCode( m_uiRun ) );
    for( UInt ui = 0, uiMbA = rcMbDataAccess.getMbAddress(); ui < m_uiRun; ui++ )
    {
      if( ui + 1 < m_uiRun || m_pcBitReadBuffer->isValid() )
      {
        uiMbA = rcMbDataAccess.getSH().getFMO()->getNextMBNr( uiMbA );
        DTRACE_NEWMB( uiMbA );
      }
    }
    DTRACE_N;
  }
  rcMbDataAccess.getMbData().setSkipFlag( m_uiRun != 0 );
  uiNextSkippedVLC = m_uiRun;

  return ( rcMbDataAccess.getMbData().getSkipFlag() );
}

Bool UvlcReader::isBLSkipped( MbDataAccess& rcMbDataAccess )
{
  UInt uiCode;
  ANOK( xGetFlag( uiCode ) );

  DTRACE_TH( "BLSkipFlag" );
  DTRACE_TY( " u(1)" );
  DTRACE_CODE( uiCode );
  DTRACE_N;

  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );

  rcMbDataAccess.getMbData().setBLSkipFlag( ( uiCode != 0 ) );
  return rcMbDataAccess.getMbData().getBLSkipFlag();
}

ErrVal UvlcReader::mbMode( MbDataAccess& rcMbDataAccess )
{
  UInt uiMbMode = 0;
  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 0 );
  rcMbDataAccess.getMbData().setBCBPAll( 0 );

  DTRACE_TH( "MbMode" );
  RNOK( xGetUvlcCode( uiMbMode ) );
  DTRACE_N;

  rcMbDataAccess.setConvertMbType( uiMbMode );

  return Err::m_nOK;
}

ErrVal UvlcReader::resPredFlag( MbDataAccess& rcMbDataAccess )
{
  UInt uiCode;
  DTRACE_TH( "ResidualPredFlag" );
  RNOK( xGetFlag( uiCode ) );
  DTRACE_N;
  rcMbDataAccess.getMbData().setResidualPredFlag( uiCode?true:false );

  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx16x8 eParIdx  )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x16 eParIdx  )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx  )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx8x4 eSParIdx )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x8 eSParIdx )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::mvd( MbDataAccess& rcMbDataAccess, ListIdx eLstIdx, ParIdx8x8  eParIdx, SParIdx4x4 eSParIdx )
{
  Mv cMv;
  RNOK( xGetMvd( cMv ) );
  rcMbDataAccess.getMbMvdData( eLstIdx ).setAllMv( cMv, eParIdx, eSParIdx );
  return Err::m_nOK;
}

ErrVal UvlcReader::fieldFlag( MbDataAccess& rcMbDataAccess )
{
  DTRACE_TH( "MbFieldFlag" );

  UInt uiBit;

  DECRNOK( xGetFlag( uiBit ) );

  rcMbDataAccess.setFieldMode(uiBit != 0);

  DTRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcReader::intraPredModeChroma( MbDataAccess& rcMbDataAccess )
{
  DTRACE_TH( "IntraPredModeChroma" );

  UInt uiCode = 0;
  RNOK( xGetUvlcCode( uiCode ) );
  rcMbDataAccess.getMbData().setChromaPredMode( uiCode );

  DTRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcReader::intraPredModeLuma( MbDataAccess& rcMbDataAccess, LumaIdx cIdx )
{
  DTRACE_TH( "IntraPredModeLuma" );
  DTRACE_POS;

  UInt uiBits;
  RNOK( m_pcBitReadBuffer->get( uiBits, 1 ) );
  DTRACE_BITS( uiBits,1 );
  DTRACE_DO( m_uiBitCounter = 1 );

  if( ! uiBits )
  {
    UInt uiCode;
    RNOK( m_pcBitReadBuffer->get( uiCode, 3 ) );
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = uiCode;
    DTRACE_BITS( uiCode, 3 );
    DTRACE_DO( m_uiBitCounter = 4 );
  }
  else
  {
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = -1;
  }

  DTRACE_COUNT(m_uiBitCounter);
  DTRACE_CODE(rcMbDataAccess.getMbData().intraPredMode( cIdx ));
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::intraPredModeLuma8x8( MbDataAccess& rcMbDataAccess, B8x8Idx cIdx ) // HS: bug fix by Nokia
{
  DTRACE_TH( "IntraPredModeLuma" );
  DTRACE_POS;

  UInt uiBits;
  RNOK( m_pcBitReadBuffer->get( uiBits, 1 ) );
  DTRACE_BITS( uiBits,1 );
  DTRACE_DO( m_uiBitCounter = 1 );

  if( ! uiBits )
  {
    UInt uiCode;
    RNOK( m_pcBitReadBuffer->get( uiCode, 3 ) );
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = uiCode;
    DTRACE_BITS( uiCode, 3 );
    DTRACE_DO( m_uiBitCounter = 4 );
  }
  else
  {
    rcMbDataAccess.getMbData().intraPredMode( cIdx ) = -1;
  }

  DTRACE_COUNT(m_uiBitCounter);
  DTRACE_CODE(rcMbDataAccess.getMbData().intraPredMode( cIdx ));
  DTRACE_N;

  return Err::m_nOK;
}


ErrVal UvlcReader::cbp( MbDataAccess& rcMbDataAccess, UInt uiStart, UInt uiStop )
{
  UInt uiTemp = 0;
  DTRACE_TH( "Cbp: " );
  RNOK( xGetUvlcCode( uiTemp ) );
  Bool bIntra = ( !rcMbDataAccess.getMbData().getBLSkipFlag() && rcMbDataAccess.getMbData().isIntra() );
  UInt uiCbp;
  if( ! rcMbDataAccess.getSH().getNoInterLayerPredFlag() && ! rcMbDataAccess.getSH().hasDefaultScanIdx() )
  {
    if( uiTemp == 0 )
    {
      uiCbp = 0;
      if( rcMbDataAccess.isAvailableLeft() )
      {
        uiCbp = rcMbDataAccess.getMbDataLeft().getMbCbp();
      }
      else if ( rcMbDataAccess.isAvailableAbove() )
      {
        uiCbp = rcMbDataAccess.getMbDataAbove().getMbCbp();
      }
    }
    else
    {
      uiTemp--;
      uiCbp  = ( bIntra ? g_aucCbpIntra[uiTemp] : g_aucCbpInter[uiTemp] );
    }
  }
  else
  {
    if( rcMbDataAccess.getSH().getSPS().getChromaFormatIdc() )
    {
      ROF( uiTemp < 48 );
      uiCbp  = ( bIntra ? g_aucCbpIntra[uiTemp]: g_aucCbpInter[uiTemp] );
    }
    else
    {
      ROF( uiTemp < 16 );
      uiCbp  = ( bIntra ? g_aucCbpIntraMonochrome[uiTemp]: g_aucCbpInterMonochrome[uiTemp] );
    }
  }

  DTRACE_X ( uiCbp );
  DTRACE_N;

  rcMbDataAccess.getMbData().setMbCbp( uiCbp );

  return Err::m_nOK;
}

ErrVal UvlcReader::deltaQp( MbDataAccess& rcMbDataAccess )
{
  DTRACE_TH ("DQp");

  Int uiCode = 0;
  RNOK( xGetSvlcCode( uiCode ) );
  rcMbDataAccess.addDeltaQp( uiCode );

  DTRACE_TY ("se(v)");
  DTRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcReader::samplesPCM( MbDataAccess& rcMbDataAccess )
{
  DTRACE_POS;
  DTRACE_TH( "  PCM SAMPLES: " );

  RNOK( m_pcBitReadBuffer->flush( m_pcBitReadBuffer->getBitsUntilByteAligned() ) );

  AOF_DBG( rcMbDataAccess.getMbData().isPCM() );

  rcMbDataAccess.getMbTCoeffs().setAllCoeffCount( 16 );
  TCoeff* pCoeff = rcMbDataAccess.getMbTCoeffs().getTCoeffBuffer();

  const UInt uiFactor = 8*8;
  const UInt uiSize   = uiFactor*2*3;
  RNOK( m_pcBitReadBuffer->pcmSamples( pCoeff, uiSize ) );

  DTRACE_N;
  DTRACE_COUNT( uiFactor*6 );

  return Err::m_nOK;
}

ErrVal UvlcReader::residualBlock( MbDataAccess& rcMbDataAccess,
                                  LumaIdx       cIdx,
                                  ResidualMode  eResidualMode,
                                  UInt&         ruiMbExtCbp,
                                  UInt          uiStart,
                                  UInt          uiStop )
{
  const UChar*  pucScan;
	const Bool    bFrame  = ( FRAME == rcMbDataAccess.getMbPicType());
  TCoeff*       piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );

  UInt  uiPos;
  Int   aiLevelRun[32];
  UInt  uiTrailingOnes = 0;
  UInt  uiTotalRun     = 0;
  UInt  uiCoeffCnt     = 0;

  for ( uiPos = 0; uiPos < 32; uiPos++ )
  {
    aiLevelRun[uiPos] = 0;
  }
  uiPos = uiStart;

  switch( eResidualMode )
  {
  case LUMA_I16_DC:
    {
      ROF( uiStart == 0 );
			pucScan = ( bFrame ? g_aucLumaFrameDCScan : g_aucLumaFieldDCScan );
      DTRACE_TH( "Luma:" );
      DTRACE_V( cIdx );
      DTRACE_N;
      xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, true );
      xGetRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, 16, uiTotalRun, rcMbDataAccess, true );
      break;
    }
  case LUMA_I16_AC:
    {
      ROF( uiStop > 1 );
			pucScan = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      uiPos   = gMax( 1, uiStart );
      DTRACE_TH( "Luma:" );
      DTRACE_V( cIdx );
      DTRACE_N;
      xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, false );
      xGetRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop - gMax( 1, uiStart ), uiTotalRun, rcMbDataAccess, false );
      break;
    }
  case LUMA_SCAN:
    {
			pucScan = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      DTRACE_TH( "Luma:" );
      DTRACE_V( cIdx );
      DTRACE_N;
      xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop, false );
      xGetRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop - uiStart, uiTotalRun, rcMbDataAccess, false );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( cIdx, uiCoeffCnt != 0 );
      break;
    }
  default:
    return Err::m_nERR;
  }

  uiPos += uiTotalRun + uiCoeffCnt - 1;
  for ( Int i = (Int)uiCoeffCnt; i > 0; i-- )
  {
    piCoeff[ pucScan [uiPos--] ] = aiLevelRun[i-1];
    for ( Int j = 0; j < aiLevelRun[i-1+0x10]; j++ )
    {
      piCoeff[ pucScan [uiPos--] ] = 0;
    }
  }

  Bool bCoded = (uiCoeffCnt > 0);
  if( ! bCoded )
  {
    ruiMbExtCbp &= ~(1 << cIdx.b4x4() );
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::residualBlock( MbDataAccess& rcMbDataAccess,
                                  ChromaIdx     cIdx,
                                  ResidualMode  eResidualMode,
                                  UInt          uiStart,
                                  UInt          uiStop )
{
	const Bool    bFrame  = ( FRAME == rcMbDataAccess.getMbPicType() );
  TCoeff*       piCoeff = rcMbDataAccess.getMbTCoeffs().get( cIdx );
  const UChar*  pucScan;
  UInt          uiPos;
  Int aiLevelRun[32];

  UInt uiTrailingOnes = 0;
  UInt uiTotalRun     = 0;
  UInt uiCoeffCnt     = 0;

  for( uiPos = 0; uiPos < 32; uiPos++ )
  {
    aiLevelRun[uiPos] = 0;
  }
  uiPos = 0;

  switch( eResidualMode )
  {
  case CHROMA_DC:
    {
      ROF( uiStart == 0 );
      pucScan = g_aucIndexChromaDCScan;
      DTRACE_TH( "CHROMA_DC:" );
      DTRACE_V( cIdx );
      DTRACE_N;
      xGetTrailingOnes4( uiCoeffCnt, uiTrailingOnes );
      xGetRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, 4, uiTotalRun, rcMbDataAccess, true );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( 24 + cIdx.plane(), uiCoeffCnt != 0);
      break;
    }
  case CHROMA_AC:
    {
      ROF( uiStop > 1 );
			pucScan  = ( bFrame ? g_aucFrameScan : g_aucFieldScan );
      uiPos    = gMax( 1, uiStart );
      DTRACE_TH( "CHROMA_AC:" );
      DTRACE_V( cIdx );
      DTRACE_N;
      xPredictNonZeroCnt( rcMbDataAccess, cIdx, uiCoeffCnt, uiTrailingOnes, uiStart, uiStop );
      xGetRunLevel( aiLevelRun, uiCoeffCnt, uiTrailingOnes, uiStop - gMax( 1, uiStart ), uiTotalRun, rcMbDataAccess, false );
      // this is useful only in AR_FGS
      rcMbDataAccess.getMbData().setBCBP( 16 + cIdx, uiCoeffCnt != 0 );
      break;
    }
  default:
    return Err::m_nERR;
  }

  uiPos += uiTotalRun + uiCoeffCnt - 1;
  for ( Int i = (Int)uiCoeffCnt; i > 0; i-- )
  {
    piCoeff[ pucScan [uiPos--] ] = aiLevelRun[i-1];
    for ( Int j = 0; j < aiLevelRun[i-1+0x10]; j++ )
    {
      piCoeff[ pucScan [uiPos--] ] = 0;
    }
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::transformSize8x8Flag( MbDataAccess& rcMbDataAccess )
{
  DTRACE_TH( "transformSize8x8Flag:" );

  UInt  uiCode;
  RNOK( xGetFlag( uiCode) );
  rcMbDataAccess.getMbData().setTransformSize8x8( uiCode?true:false );

  DTRACE_V( uiCode );
  DTRACE_N;
  return Err::m_nOK;
}


ErrVal UvlcReader::xGetRefFrame( Bool bWriteBit, UInt& uiRefFrame, ListIdx eLstIdx )
{
  DTRACE_TH( "RefFrame" );
  UInt uiCode;

  if( bWriteBit )
  {
    RNOK( xGetFlag( uiCode ) );
    uiRefFrame = 1-uiCode;
  }
  else
  {
    RNOK( xGetUvlcCode( uiRefFrame ) );
  }

  DTRACE_V( uiRefFrame+1 );
  DTRACE_N;
  return Err::m_nOK;
}

ErrVal UvlcReader::xGetMotionPredFlag( Bool& rbFlag )
{
  DTRACE_TH( "MotionPredFlag" );

  UInt uiCode;
  RNOK( xGetFlag( uiCode ) );

  DTRACE_V( uiCode );
  DTRACE_N;
  rbFlag = (uiCode == 1);
  return Err::m_nOK;
}


ErrVal UvlcReader::xGetMvd( Mv& cMv )
{
  DTRACE_TH( "Mvd: x" );

  UInt  uiTemp = 0;

  RNOK( xGetUvlcCode( uiTemp ) );

  Short sHor = ( uiTemp & 1) ? (Int)((uiTemp+1)>>1) : -(Int)(uiTemp>>1);
  DTRACE_CODE( sHor );
  DTRACE_TY("se(v)");
  DTRACE_N;

  DTRACE_TH( "Mvd: y" );

  RNOK( xGetUvlcCode( uiTemp ) );

  Short sVer = ( uiTemp & 1) ? (Int)((uiTemp+1)>>1) : -(Int)(uiTemp>>1);

  DTRACE_CODE( sVer );
  DTRACE_TY("se(v)");
  DTRACE_N;

  cMv.setHor( sHor );
  cMv.setVer( sVer );

  return Err::m_nOK;
}

ErrVal UvlcReader::xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, LumaIdx cIdx, UInt& uiCoeffCount, UInt& uiTrailingOnes, UInt uiStart, UInt uiStop, Bool bDC )
{
  UInt uiCoeffCountCtx = rcMbDataAccess.getCtxCoeffCount( cIdx, uiStart, uiStop );

  if( ! rcMbDataAccess.getSH().hasDefaultScanIdx() && !bDC )
  {
    UInt uiDummy, uiTargetCoeffTokenIdx;
    RNOK( xCodeFromBitstream2D( &g_aucCwCodeVectTO16[uiCoeffCountCtx][0], &g_aucCwLenVectTO16[uiCoeffCountCtx][0], 62, 1, uiTargetCoeffTokenIdx, uiDummy ) );

    uiCoeffCountCtx = gMin(uiCoeffCountCtx, 2);
    UInt uiBw       = uiStop-uiStart;
    UInt uiXdim     = gMin(16,uiBw)+1;
    UInt uiYdim     = gMin(3, uiBw)+1;
    UInt uiCoeffTokenIdx, uiIdx;
    for( uiCoeffTokenIdx = uiIdx = 0; uiIdx <= uiTargetCoeffTokenIdx; uiCoeffTokenIdx++ )
    {
      if( g_aucCwMapTO16[uiCoeffCountCtx][uiCoeffTokenIdx] < uiYdim && g_aucCwMapTC16[uiCoeffCountCtx][uiCoeffTokenIdx] < uiXdim )
      {
        uiIdx++;
      }
    }
    uiTrailingOnes = g_aucCwMapTO16[uiCoeffCountCtx][uiCoeffTokenIdx-1];
    uiCoeffCount   = g_aucCwMapTC16[uiCoeffCountCtx][uiCoeffTokenIdx-1];
  }
  else
  {
    xGetTrailingOnes16( uiCoeffCountCtx, uiCoeffCount, uiTrailingOnes );
  }

  rcMbDataAccess.getMbTCoeffs().setCoeffCount( cIdx, uiCoeffCount );

  return Err::m_nOK;
}


ErrVal UvlcReader::xPredictNonZeroCnt( MbDataAccess& rcMbDataAccess, ChromaIdx cIdx, UInt& uiCoeffCount, UInt& uiTrailingOnes, UInt uiStart, UInt uiStop )
{
  UInt uiCoeffCountCtx = rcMbDataAccess.getCtxCoeffCount( cIdx, uiStart, uiStop );

  if( ! rcMbDataAccess.getSH().hasDefaultScanIdx() )
  {
    UInt uiDummy, uiTargetCoeffTokenIdx;
    RNOK( xCodeFromBitstream2D( &g_aucCwCodeVectTO16[uiCoeffCountCtx][0], &g_aucCwLenVectTO16[uiCoeffCountCtx][0], 62, 1, uiTargetCoeffTokenIdx, uiDummy ) );

    uiCoeffCountCtx = gMin(uiCoeffCountCtx, 2);
    UInt uiBw       = uiStop-uiStart;
    UInt uiXdim     = gMin(16,uiBw)+1;
    UInt uiYdim     = gMin(3, uiBw)+1;
    UInt uiCoeffTokenIdx, uiIdx;
    for( uiCoeffTokenIdx = uiIdx = 0; uiIdx <= uiTargetCoeffTokenIdx; uiCoeffTokenIdx++ )
    {
      if( g_aucCwMapTO16[uiCoeffCountCtx][uiCoeffTokenIdx] < uiYdim && g_aucCwMapTC16[uiCoeffCountCtx][uiCoeffTokenIdx] < uiXdim )
      {
        uiIdx++;
      }
    }
    uiTrailingOnes = g_aucCwMapTO16[uiCoeffCountCtx][uiCoeffTokenIdx-1];
    uiCoeffCount   = g_aucCwMapTC16[uiCoeffCountCtx][uiCoeffTokenIdx-1];
  }
  else
  {
    xGetTrailingOnes16( uiCoeffCountCtx, uiCoeffCount, uiTrailingOnes );
  }

  rcMbDataAccess.getMbTCoeffs().setCoeffCount( cIdx, uiCoeffCount );

  return Err::m_nOK;
}

ErrVal UvlcReader::xGetTrailingOnes16( UInt uiLastCoeffCount, UInt& uiCoeffCount, UInt& uiTrailingOnes )
{
  DTRACE_POS;
  if( 3 == uiLastCoeffCount )
  {
    UInt uiBits;
    RNOK( m_pcBitReadBuffer->get( uiBits, 6 ) );
    DTRACE_DO( m_uiBitCounter = 6 );

    uiTrailingOnes = ( uiBits & 0x3 );
    uiCoeffCount   = ( uiBits >>  2 );
    if ( !uiCoeffCount && uiTrailingOnes == 3 )
    {
      uiTrailingOnes = 0;
    }
    else
    {
      uiCoeffCount++;
    }
  }
  else
  {
    ROF( uiLastCoeffCount < 3 );
    RNOK( xCodeFromBitstream2D( &g_aucCodeTableTO16[uiLastCoeffCount][0][0], &g_aucLenTableTO16[uiLastCoeffCount][0][0], 17, 4, uiCoeffCount, uiTrailingOnes ) );
    DTRACE_DO( m_uiBitCounter = g_aucLenTableTO16[uiLastCoeffCount][uiTrailingOnes][uiCoeffCount] );
  }

  DTRACE_TH( "  TrailingOnes16: Vlc: " );
  DTRACE_V( uiLastCoeffCount );
  DTRACE_TH( " CoeffCnt: " );
  DTRACE_V( uiCoeffCount );
  DTRACE_TH( " TraiOnes: " );
  DTRACE_V( uiTrailingOnes );
  DTRACE_N;
  DTRACE_COUNT(m_uiBitCounter);

  return Err::m_nOK;
}

ErrVal UvlcReader::xCodeFromBitstream2D( const UChar* aucCod, const UChar* aucLen, UInt uiWidth, UInt uiHeight, UInt& uiVal1, UInt& uiVal2 )
{
  const UChar *paucLenTab;
  const UChar *paucCodTab;
  UChar  uiLenRead = 0;
  UChar  uiCode    = 0;
  UChar  uiMaxLen  = 0;

  // Find maximum number of bits to read before generating error
  paucLenTab = aucLen;
  paucCodTab = aucCod;
  for (UInt j = 0; j < uiHeight; j++, paucLenTab += uiWidth, paucCodTab += uiWidth)
  {
    for ( UInt i = 0; i < uiWidth; i++ )
    {
      if ( paucLenTab[i] > uiMaxLen )
      {
        uiMaxLen = paucLenTab[i];
      }
    }
  }

  while ( uiLenRead < uiMaxLen )
  {
    // Read next bit
    UInt uiBit;
    RNOKS( m_pcBitReadBuffer->get( uiBit, 1 ) );
    uiCode = ( uiCode << 1 ) + uiBit;
    uiLenRead++;

    // Check for matches
    paucLenTab = aucLen;
    paucCodTab = aucCod;
    for (UInt j = 0; j < uiHeight; j++, paucLenTab += uiWidth, paucCodTab += uiWidth)
    {
      for (UInt i = 0; i < uiWidth; i++)
      {
        if ( (paucLenTab[i] == uiLenRead) && (paucCodTab[i] == uiCode) )
        {
          uiVal1 = i;
          uiVal2 = j;
          return Err::m_nOK;
        }
      }
    }

  }
  return Err::m_nERR;
}


ErrVal UvlcReader::xGetRunLevel( Int* aiLevelRun, UInt uiCoeffCnt, UInt uiTrailingOnes, UInt uiMaxCoeffs, UInt& uiTotalRun, MbDataAccess &rcMbDataAccess, Bool bDC )
{
  ROTRS( 0 == uiCoeffCnt, Err::m_nOK );

  if( uiTrailingOnes )
  {
    UInt uiBits;
    RNOK( m_pcBitReadBuffer->get( uiBits, uiTrailingOnes ));

    Int n = uiTrailingOnes-1;
    for( UInt k = uiCoeffCnt; k > uiCoeffCnt-uiTrailingOnes; k--, n--)
    {
      aiLevelRun[k-1] = (uiBits & (1<<n)) ? -1 : 1;
    }

    DTRACE_POS;
    DTRACE_TH( "  TrailingOnesSigns: " );
    DTRACE_V( uiBits );
    DTRACE_N;
    DTRACE_COUNT(uiTrailingOnes);
  }

  UInt uiHighLevel = ( uiCoeffCnt > 3 && uiTrailingOnes == 3) ? 0 : 1;
  UInt uiVlcTable  = ( uiCoeffCnt > 10 && uiTrailingOnes < 3) ? 1 : 0;

  for( Int k = uiCoeffCnt - 1 - uiTrailingOnes; k >= 0; k--)
  {
    Int iLevel;

    if( uiVlcTable == 0 )
    {
	    xGetLevelVLC0( iLevel );
    }
    else
    {
	    xGetLevelVLCN( iLevel, uiVlcTable );
    }

    if( uiHighLevel )
    {
      iLevel += ( iLevel > 0 ) ? 1 : -1;
	    uiHighLevel = 0;
    }
    aiLevelRun[k] = iLevel;

    UInt uiAbsLevel = (UInt)abs(iLevel);

    // update VLC table
    if( uiAbsLevel > g_auiIncVlc[ uiVlcTable ] )
    {
      uiVlcTable++;
    }

    if( k == Int(uiCoeffCnt - 1 - uiTrailingOnes) && uiAbsLevel > 3)
    {
      uiVlcTable = 2;
    }

  }

  ROFRS( uiCoeffCnt < uiMaxCoeffs, Err::m_nOK );


  uiVlcTable = uiCoeffCnt-1;
  if( ! rcMbDataAccess.getSH().hasDefaultScanIdx() && !bDC )
  {
    if( uiMaxCoeffs <= 4 )
    {
      UInt uiTempVlcTable = gMin(uiVlcTable + 4-uiMaxCoeffs, 2);
      xGetTotalRun4( uiTempVlcTable, uiTotalRun );
    }
    else if( uiMaxCoeffs <= 8 )
    {
      UInt uiTempVlcTable = gMin(uiVlcTable + 8-uiMaxCoeffs, 6);
      xGetTotalRun8( uiTempVlcTable, uiTotalRun );
    }
    else
    {
      UInt uiTempVlcTable = uiVlcTable;
      if( uiMaxCoeffs < 15 )
        uiTempVlcTable = gMin(uiVlcTable + 16-uiMaxCoeffs, 14);
      xGetTotalRun16( uiTempVlcTable, uiTotalRun );
    }
  }
  else
  {
    if( uiMaxCoeffs <= 4 )
    {
      xGetTotalRun4( uiVlcTable, uiTotalRun );
    }
    else
    {
      xGetTotalRun16( uiVlcTable, uiTotalRun );
    }
  }

  // decode run before each coefficient
  for ( UInt i = 0; i < uiCoeffCnt; i++ )
  {
    aiLevelRun[i + 0x10] = 0;
  }
  uiCoeffCnt--;
  UInt uiRunCount = uiTotalRun;
  if( uiRunCount > 0 && uiCoeffCnt > 0)
  {
    do
    {
      uiVlcTable = (( uiRunCount > RUNBEFORE_NUM) ? RUNBEFORE_NUM : uiRunCount) - 1;
      UInt uiRun = 0;

      xGetRun( uiVlcTable, uiRun );
      aiLevelRun[uiCoeffCnt+0x10] = uiRun;

      uiRunCount -= uiRun;
      uiCoeffCnt--;
    } while( uiRunCount != 0 && uiCoeffCnt != 0);
    aiLevelRun[uiCoeffCnt+0x10] = uiRunCount;
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::xGetTrailingOnes4( UInt& uiCoeffCount, UInt& uiTrailingOnes )
{
  RNOK( xCodeFromBitstream2D( &g_aucCodeTableTO4[0][0], &g_aucLenTableTO4[0][0], 5, 4, uiCoeffCount, uiTrailingOnes ) );

  DTRACE_POS;
  DTRACE_TH( "  TrailingOnes4: CoeffCnt: " );
  DTRACE_V( uiCoeffCount );
  DTRACE_TH( " TraiOnes: " );
  DTRACE_V( uiTrailingOnes );
  DTRACE_N;
  DTRACE_COUNT(g_aucLenTableTO4[uiTrailingOnes][uiCoeffCount]);

  return Err::m_nOK;
}


ErrVal UvlcReader::xGetTotalRun4( UInt& uiVlcPos, UInt& uiTotalRun )
{
  UInt uiTemp;
  RNOK( xCodeFromBitstream2D( &g_aucCodeTableTZ4[uiVlcPos][0], &g_aucLenTableTZ4[uiVlcPos][0], 4, 1, uiTotalRun, uiTemp ) );

  DTRACE_POS;
  DTRACE_TH( "  TotalZeros4 vlc: " );
  DTRACE_V( uiVlcPos );
  DTRACE_TH( " TotalRun: " );
  DTRACE_V( uiTotalRun );
  DTRACE_N;
  DTRACE_COUNT(g_aucLenTableTZ4[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcReader::xGetTotalRun8( UInt& uiVlcPos, UInt& uiTotalRun )
{
  UInt uiTemp;
  RNOK( xCodeFromBitstream2D( &g_aucCodeTableTZ8[uiVlcPos][0], &g_aucLenTableTZ8[uiVlcPos][0], 8, 1, uiTotalRun, uiTemp ) );

  DTRACE_POS;
  DTRACE_TH( "  TotalZeros8 vlc: " );
  DTRACE_V( uiVlcPos );
  DTRACE_TH( " TotalRun: " );
  DTRACE_V( uiTotalRun );
  DTRACE_N;
  DTRACE_COUNT(g_aucLenTableTZ8[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcReader::xGetTotalRun16( UInt uiVlcPos, UInt& uiTotalRun )
{
  UInt uiTemp;
  RNOK( xCodeFromBitstream2D( &g_aucCodeTableTZ16[uiVlcPos][0], &g_aucLenTableTZ16[uiVlcPos][0], 16, 1, uiTotalRun, uiTemp ) );

  DTRACE_POS;
  DTRACE_TH( "  TotalRun16 vlc: " );
  DTRACE_V( uiVlcPos );
  DTRACE_TH( " TotalRun: " );
  DTRACE_V( uiTotalRun );
  DTRACE_N;
  DTRACE_COUNT(g_aucLenTableTZ16[uiVlcPos][uiTotalRun]);

  return Err::m_nOK;
}


ErrVal UvlcReader::xGetRun( UInt uiVlcPos, UInt& uiRun  )
{
  UInt uiTemp;

  RNOK( xCodeFromBitstream2D( &g_aucCodeTable3[uiVlcPos][0], &g_aucLenTable3[uiVlcPos][0], 15, 1, uiRun, uiTemp ) );

  DTRACE_POS;
  DTRACE_TH( "  Run" );
  DTRACE_CODE( uiRun );
  DTRACE_COUNT (g_aucLenTable3[uiVlcPos][uiRun]);
  DTRACE_N;

  return Err::m_nOK;
}

ErrVal UvlcReader::xGetLevelVLC0( Int& iLevel )
{
  UInt uiLength = 0;
  UInt uiCode   = 0;
  UInt uiTemp   = 0;
  UInt uiSign   = 0;
  UInt uiLevel  = 0;

  do
  {
    RNOK( m_pcBitReadBuffer->get( uiTemp, 1 ) );
    uiLength++;
    uiCode = ( uiCode << 1 ) + uiTemp;
  } while ( uiCode == 0 );

  if ( uiLength < 15 )
  {
    uiSign  = (uiLength - 1) & 1;
    uiLevel = (uiLength - 1) / 2 + 1;
  }
  else if (uiLength == 15)
  {
    // escape code
    RNOK( m_pcBitReadBuffer->get( uiTemp, 4 ) );
    uiCode = (uiCode << 4) | uiTemp;
    uiLength += 4;
    uiSign  = (uiCode & 1);
    uiLevel = ((uiCode >> 1) & 0x7) + 8;
  }
  else if (uiLength >= 16)
  {
    // escape code
    UInt uiAddBit = uiLength - 16;
    RNOK( m_pcBitReadBuffer->get( uiCode, uiLength-4 ) );
    uiLength -= 4;
    uiSign    = (uiCode & 1);
    uiLevel   = (uiCode >> 1) + (2048<<uiAddBit)+16-2048;
    uiCode   |= (1 << (uiLength)); // for display purpose only
    uiLength += uiAddBit + 16;
 }

  iLevel = uiSign ? -(Int)uiLevel : (Int)uiLevel;

  DTRACE_POS;
  DTRACE_TH( "  VLC0 lev " );
  DTRACE_CODE( iLevel );
  DTRACE_N;
  DTRACE_COUNT( uiLength );

  return Err::m_nOK;

}

ErrVal UvlcReader::xGetLevelVLCN( Int& iLevel, UInt uiVlcLength )
{
  UInt uiTemp;
  UInt uiLength;
  UInt uiCode;
  UInt uiLevAbs;
  UInt uiSb;
  UInt uiSign;
  UInt uiAddBit;
  UInt uiOffset;

  UInt uiNumPrefix = 0;
  UInt uiShift     = uiVlcLength - 1;
  UInt uiEscape    = (15<<uiShift)+1;

  // read pre zeros
  do
  {
    RNOK( m_pcBitReadBuffer->get( uiTemp, 1 ) );
    uiNumPrefix++;
  } while ( uiTemp == 0 );

  uiLength = uiNumPrefix;
  uiCode   = 1;
  uiNumPrefix--;

  if (uiNumPrefix < 15)
  {
    uiLevAbs = (uiNumPrefix<<uiShift) + 1;

    if ( uiVlcLength-1 )
    {
      RNOK( m_pcBitReadBuffer->get( uiSb, uiVlcLength-1 ) );
      uiCode = (uiCode << (uiVlcLength-1) )| uiSb;
      uiLevAbs += uiSb;
      uiLength += (uiVlcLength-1);

    }

    // read 1 bit -> sign
    RNOK( m_pcBitReadBuffer->get( uiSign, 1 ) );
    uiCode = (uiCode << 1)| uiSign;
    uiLength++;
  }
  else // escape
  {
    uiAddBit = uiNumPrefix - 15;

    RNOK( m_pcBitReadBuffer->get( uiSb, (11+uiAddBit) ) );
    uiCode = (uiCode << (11+uiAddBit) )| uiSb;

    uiLength += (11+uiAddBit);
    uiOffset  = (2048<<uiAddBit)+uiEscape-2048;
    uiLevAbs  = uiSb + uiOffset;

    // read 1 bit -> sign
    RNOK( m_pcBitReadBuffer->get( uiSign, 1 ) );
    uiCode = (uiCode << 1)| uiSign;
    uiLength++;
  }

  iLevel = (uiSign) ? -(Int)uiLevAbs : (Int)uiLevAbs;

  DTRACE_POS;
  DTRACE_TH( "  VLCN lev: " );
  DTRACE_CODE( iLevel );
  DTRACE_N;
  DTRACE_COUNT( uiLength );

  return Err::m_nOK;
}

Bool UvlcReader::isEndOfSlice()
{
  UInt uiEOS = ( m_uiRun > 1 ) ? 0 : !( moreRBSPData() );
  return (uiEOS == 1);
}

ErrVal UvlcReader::finishSlice( )
{
  if( m_bRunLengthCoding && m_uiRun )
  {
    DTRACE_TH( "Run" );
    RNOK( xGetUvlcCode( m_uiRun ) );
    DTRACE_N;
  }

  return Err::m_nOK;
}

ErrVal UvlcReader::residualBlock8x8( MbDataAccess&  rcMbDataAccess,
                                     B8x8Idx        c8x8Idx,
                                     UInt           uiStart,
                                     UInt           uiStop )
{
  const Bool bFrame = ( FRAME == rcMbDataAccess.getMbPicType());
  const UChar* pucScan = (bFrame) ? g_aucFrameScan64 : g_aucFieldScan64;
  TCoeff* piCoeff = rcMbDataAccess.getMbTCoeffs().get8x8( c8x8Idx );

  UInt  uiBlk;
  UInt  uiPos;

  Int   aaiLevelRun     [4][32];
  UInt  auiTrailingOnes [4]     = { 0, 0, 0, 0 };
  UInt  auiTotalRun     [4]     = { 0, 0, 0, 0 };
  UInt  auiCoeffCnt     [4]     = { 0, 0, 0, 0 };

  for ( uiBlk  = 0; uiBlk < 4; uiBlk++ )
   for ( uiPos = 0; uiPos < 32; uiPos++ )
     aaiLevelRun[uiBlk][uiPos] = 0;

  {
    UInt uiBitPos = c8x8Idx;
    rcMbDataAccess.getMbData().setBCBP( uiBitPos,   1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+1, 1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+4, 1);
    rcMbDataAccess.getMbData().setBCBP( uiBitPos+5, 1);
  }
  //===== loop over 4x4 blocks =====
  for( uiBlk = 0; uiBlk < 4; uiBlk++ )
  {
    B4x4Idx cIdx( c8x8Idx.b4x4() + 4*(uiBlk/2) + (uiBlk%2) );

    xPredictNonZeroCnt( rcMbDataAccess, cIdx, auiCoeffCnt[uiBlk], auiTrailingOnes[uiBlk], uiStart, uiStop, false );
    xGetRunLevel( aaiLevelRun[uiBlk], auiCoeffCnt[uiBlk], auiTrailingOnes[uiBlk], uiStop - uiStart, auiTotalRun[uiBlk], rcMbDataAccess, false );
    uiPos = ((auiTotalRun[uiBlk] + auiCoeffCnt[uiBlk] - 1) << 2) + uiBlk;
    uiPos += 4*uiStart;

    for ( Int i = (Int)auiCoeffCnt[uiBlk]; i > 0; i-- )
    {
      piCoeff[ pucScan [uiPos] ] = aaiLevelRun[uiBlk][i-1];
      uiPos -= 4;
      for ( Int j = 0; j < aaiLevelRun[uiBlk][i-1+0x10]; j++ )
      {
        piCoeff[ pucScan [uiPos] ] = 0;
        uiPos -= 4;
      }
    }
  }
  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
