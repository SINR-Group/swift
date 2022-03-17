
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CommonTypes.h"
#include "H264AVCCommonLib/MbDataStruct.h"

#include<stdio.h>


H264AVC_NAMESPACE_BEGIN



ErrVal
MbDataStruct::save( FILE* pFile )
{
  ROF( pFile );

  UInt uiSave  = (UInt) ::fwrite( this, sizeof(MbDataStruct), 1, pFile );

  ROF( uiSave == 1 );

  return Err::m_nOK;
}


ErrVal
MbDataStruct::load( FILE* pFile )
{
  ROF( pFile );

  UInt uiRead  = (UInt) ::fread( this, sizeof(MbDataStruct), 1, pFile );

  ROF( uiRead == 1 );

  return Err::m_nOK;
}




const UChar MbDataStruct::m_aucACTab[7] =
{
  0,1,2,0,1,2,2
};

MbDataStruct::MbDataStruct()
: m_uiSliceId           ( 0 )
, m_uiMbAddr            ( MSYS_UINT_MAX )
, m_uiMapUnit           ( MSYS_UINT_MAX )
, m_eSliceType          ( NOT_SPECIFIED_SLICE )
, m_bBLSkipFlag         ( false )
, m_eMbMode             ( MODE_SKIP )
, m_uiMbCbp             ( 0 )
, m_uiBCBP              ( 0 )
, m_usFwdBwd            ( 0 )
, m_ucChromaPredMode    ( 0 )
, m_ucQp                ( 0 )
, m_ucQp4LF             ( 0 )
, m_bResidualPredFlag   ( false )
, m_bTransformSize8x8   ( false )
, m_bSkipFlag           ( true )
, m_bInCropWindowFlag   ( false ) //TMM_ESS
, m_bFieldFlag          ( 0 )
, m_uiMbCbpResidual     ( 0 )
, m_uiMbCbpDQId0        ( 0 )    
, m_uiMbCbpLevelsLF     ( 0 )
, m_uiSliceIdcLF        ( 0 )
, m_bRPSafe							( true )
{
  DO_DBG( clearIntraPredictionModes( true ) );//TMM_INTERLACE
  m_aBlkMode[0] = m_aBlkMode[1] = m_aBlkMode[2] = m_aBlkMode[3] = BLK_8x8;  //TMM_ESS
}


Void MbDataStruct::reset()
{
  m_uiBCBP              = 0;
  m_usFwdBwd            = 0;
  m_uiSliceId           = 0;
  m_uiMbAddr            = MSYS_UINT_MAX;
  m_uiMapUnit           = MSYS_UINT_MAX;
  m_eSliceType          = NOT_SPECIFIED_SLICE;
  m_bBLSkipFlag         = false;
  m_eMbMode             = MODE_SKIP;
  m_uiMbCbp             = 0;
  m_ucChromaPredMode    = 0;
  m_ucQp                = 0;
  m_ucQp4LF             = 0;
  m_bResidualPredFlag   = false;
  m_bTransformSize8x8   = false;
  m_bInCropWindowFlag   = false; //TMM_ESS
  DO_DBG( clearIntraPredictionModes( true ) );//TMM_INTERLACE
  m_aBlkMode[0] = m_aBlkMode[1] = m_aBlkMode[2] = m_aBlkMode[3] = BLK_8x8;  //TMM_ESS
	m_bFieldFlag          = 0;
}


Void MbDataStruct::clear()
{
  m_usFwdBwd            = 0;
  m_bBLSkipFlag         = false;
  m_eMbMode             = MODE_SKIP;
  m_uiMbCbp             = 0;
  m_ucChromaPredMode    = 0;
  m_uiBCBP              = 0;
  m_bResidualPredFlag   = false;
  m_bTransformSize8x8   = false;
  m_bInCropWindowFlag   = false; //TMM_ESS
  clearIntraPredictionModes( true );
  m_aBlkMode[0] = m_aBlkMode[1] = m_aBlkMode[2] = m_aBlkMode[3] = BLK_8x8;  //TMM_ESS
  //m_bSkipFlag						= false; TMM_JV_DEBUG
}


Void MbDataStruct::clearIntraPredictionModes( Bool bAll )
{
  ::memset( m_ascIPredMode, DC_PRED, sizeof(UChar)* 16 );
  ROFVS( bAll );
  m_ucChromaPredMode = 0;
}



Void MbDataStruct::setMbCbp( UInt uiCbp )
{
  UInt uiExtMbCbp = 0;
  UInt uiMbCbpTmp = uiCbp;

  uiExtMbCbp += (uiCbp & 0x4) ? 0x33 : 0x00;
  uiExtMbCbp += (uiCbp & 0x8) ? 0xcc : 0x00;
  uiExtMbCbp <<= 8;
  uiExtMbCbp += (uiCbp & 0x1) ? 0x33 : 0x00;
  uiExtMbCbp += (uiCbp & 0x2) ? 0xcc : 0x00;
  uiExtMbCbp |= uiMbCbpTmp << 24;

  m_uiMbCbp = uiExtMbCbp;
}



Void MbDataStruct::setAndConvertMbExtCbp( UInt uiExtCbp )
{
  UInt uiMbCbp;
  {
    UInt uiCbp = uiExtCbp;
    uiMbCbp  = (0 != (uiCbp & 0x33)) ? 1 : 0;
    uiMbCbp += (0 != (uiCbp & 0xcc)) ? 2 : 0;
    uiCbp >>= 8;
    uiMbCbp += (0 != (uiCbp & 0x33)) ? 4 : 0;
    uiMbCbp += (0 != (uiCbp & 0xcc)) ? 8 : 0;
  }
  uiMbCbp += (uiExtCbp >> 16) << 4;

  m_uiMbCbp = (uiMbCbp<<24) | uiExtCbp;
}


Void MbDataStruct::copy( const MbDataStruct& rcMbDataStruct )
{
  copyFrom( rcMbDataStruct );
  m_bSkipFlag                 = rcMbDataStruct.m_bSkipFlag;
  m_bInCropWindowFlag         = rcMbDataStruct.m_bInCropWindowFlag;
}

Void MbDataStruct::copyFrom( const MbDataStruct& rcMbDataStruct )
{
  m_usFwdBwd            = rcMbDataStruct.m_usFwdBwd;
  m_uiSliceId           = rcMbDataStruct.m_uiSliceId;
  m_uiMbAddr            = rcMbDataStruct.m_uiMbAddr;
  m_uiMapUnit           = rcMbDataStruct.m_uiMapUnit;
  m_eSliceType          = rcMbDataStruct.m_eSliceType;
  m_bBLSkipFlag         = rcMbDataStruct.m_bBLSkipFlag;
  m_eMbMode             = rcMbDataStruct.m_eMbMode;
  m_ucQp                = rcMbDataStruct.m_ucQp;
  m_ucQp4LF             = rcMbDataStruct.m_ucQp4LF;
  m_uiMbCbp             = rcMbDataStruct.m_uiMbCbp;
  m_ucChromaPredMode    = rcMbDataStruct.m_ucChromaPredMode;
  m_uiBCBP              = rcMbDataStruct.m_uiBCBP;
  m_bResidualPredFlag   = rcMbDataStruct.m_bResidualPredFlag;
  m_bTransformSize8x8   = rcMbDataStruct.m_bTransformSize8x8;
  m_bFieldFlag          = rcMbDataStruct.m_bFieldFlag;

  memcpy( m_aBlkMode,     rcMbDataStruct.m_aBlkMode,      sizeof(m_aBlkMode) );
  memcpy( m_ascIPredMode, rcMbDataStruct.m_ascIPredMode,  sizeof(m_ascIPredMode) );
}


Bool
MbDataStruct::is8x8TrafoFlagPresent( Bool bDirect8x8Inference ) const
{
  ROTRS( m_eMbMode == INTRA_BL,   true  );
  ROTRS( m_eMbMode > INTRA_4X4,   false );

  ROTRS( MODE_SKIP == m_eMbMode,  bDirect8x8Inference );

  if( ( MODE_8x8 == m_eMbMode ) || ( MODE_8x8ref0 == m_eMbMode ) )
  {
    for( UInt n = 0; n < 4; n++ )
    {
      if( BLK_8x8 != m_aBlkMode[n] && BLK_SKIP != m_aBlkMode[n] )
      {
        return false;
      }
      if( BLK_SKIP == m_aBlkMode[n] && !bDirect8x8Inference )
      {
        return false;
      }
    }
  }
  return true;
}


H264AVC_NAMESPACE_END
