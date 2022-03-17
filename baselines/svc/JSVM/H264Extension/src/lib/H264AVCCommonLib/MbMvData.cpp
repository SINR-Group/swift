
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MbMvData.h"
#include "H264AVCCommonLib/Frame.h"

#include<stdio.h>

H264AVC_NAMESPACE_BEGIN

const UInt MbMotionData::m_auiBlk2Part [16 ]  = { 0, 0, 1, 1,  0, 0, 1, 1,  2, 2, 3, 3,  2, 2, 3, 3 };  // XDIRECT


ErrVal
MbMvData::save( FILE* pFile )
{
  ROF( pFile );

  UInt uiSave = (UInt) ::fwrite( &m_acMv[0], sizeof(Mv), 16, pFile );
  ROF( uiSave == 16 );

  return Err::m_nOK;
}


ErrVal
MbMvData::load( FILE* pFile )
{
  ROF( pFile );

  UInt uiRead = (UInt) ::fread( &m_acMv[0], sizeof(Mv), 16, pFile );
  ROF( uiRead == 16 );

  return Err::m_nOK;
}


ErrVal
MbMotionData::save( FILE* pFile )
{
  ROF( pFile );

  RNOK( MbMvData::save( pFile ) );

  UInt uiSave  = (UInt) ::fwrite( &m_ascRefIdx[0],   sizeof(SChar),      4, pFile );
  uiSave      += (UInt) ::fwrite( &m_usMotPredFlags, sizeof(UShort), 1, pFile );

  ROF( uiSave == ( 4 + 1 ) );

  return Err::m_nOK;
}


ErrVal
MbMotionData::load( FILE* pFile )
{
  ROF( pFile );

  RNOK( MbMvData::load( pFile ) );

  UInt uiRead  = (UInt) ::fread( &m_ascRefIdx[0],   sizeof(SChar),  4, pFile );
  uiRead      += (UInt) ::fread( &m_usMotPredFlags, sizeof(UShort), 1, pFile );

  ROF( uiRead == ( 4 + 1 ) );

  return Err::m_nOK;
}


Void MbMvData::copyFrom( const MbMvData& rcMbMvData, const ParIdx8x8 eParIdx )
{
  m_acMv[ eParIdx     ] = rcMbMvData.m_acMv[ eParIdx     ];
  m_acMv[ eParIdx + 1 ] = rcMbMvData.m_acMv[ eParIdx + 1 ];
  m_acMv[ eParIdx + 4 ] = rcMbMvData.m_acMv[ eParIdx + 4 ];
  m_acMv[ eParIdx + 5 ] = rcMbMvData.m_acMv[ eParIdx + 5 ];
}

Void  MbMotionData::copyFrom( const MbMotionData& rcMbMotionData, const ParIdx8x8 eParIdx )
{
  UInt uiOffset = m_auiBlk2Part[ eParIdx ];
  m_ascRefIdx[ uiOffset ] = rcMbMotionData.m_ascRefIdx[ uiOffset ];
  m_acRefPicIdc[ uiOffset ] = rcMbMotionData.m_acRefPicIdc[ uiOffset ];

  setMotPredFlag( rcMbMotionData.getMotPredFlag( eParIdx ), eParIdx );

  MbMvData::copyFrom( rcMbMotionData, eParIdx );
}


Void MbMvData::copyFrom( const MbMvData& rcMbMvData )
{
  memcpy( m_acMv, rcMbMvData.m_acMv, sizeof(m_acMv) );
  m_bFieldFlag  = rcMbMvData.m_bFieldFlag;
}

Void
RefPicIdc::set( const Frame* pcFrame )
{
  if( pcFrame )
  {
    m_iPoc      = pcFrame->getPoc     ();
    m_ePicType  = pcFrame->getPicType ();
    m_pcFrame   = pcFrame->getFrame   ();
    AOF( m_pcFrame );
    return;
  }
  m_iPoc      = 0;
  m_ePicType  = NOT_SPECIFIED;
  m_pcFrame   = 0;
}

Bool
RefPicIdc::isValid() const
{
  ROTRS( m_ePicType == NOT_SPECIFIED,                         false );
  const Frame* pcFrame = m_pcFrame->getPic( m_ePicType );
  AOF  ( pcFrame );
  ROFRS( pcFrame->getPoc() == m_iPoc,                         false );
  return true;
}

const Frame*
RefPicIdc::getPic() const
{
  return m_pcFrame->getPic( m_ePicType );
}

ErrVal
MbMotionData::setRefPicIdcs( RefFrameList* pcRefFrameList )
{
  for( UInt ui = 0; ui < 4; ui++ )
  {
    RefPicIdc cRefPicIdc;
    SChar     scRefIdx = m_ascRefIdx[ui];
    if( scRefIdx > 0 )
    {
      ROF( pcRefFrameList );
      ROF( scRefIdx <= (Int)pcRefFrameList->getActive() );
      const Frame* pcFrame = (*pcRefFrameList)[ scRefIdx ];
      ROF( pcFrame );
      cRefPicIdc.set( pcFrame );
    }
    m_acRefPicIdc[ui] = cRefPicIdc;
  }
  return Err::m_nOK;
}


BlkMode MbMotionData::getBlkMode( const ParIdx8x8 eParIdx, BlkMode eBlkMode )
{
  Bool bR1 = m_acMv[ eParIdx + 0] == m_acMv[ eParIdx + 1];
  Bool bR2 = m_acMv[ eParIdx + 4] == m_acMv[ eParIdx + 5];
  Bool bC1 = m_acMv[ eParIdx + 0] == m_acMv[ eParIdx + 4];
  Bool bC2 = m_acMv[ eParIdx + 1] == m_acMv[ eParIdx + 5];

  if( ! bC1 || ! bC2 )
  {
    eBlkMode = ( eBlkMode == BLK_8x8 ) ? BLK_8x4 : BLK_4x4;
  }
  if( ! bR1 || ! bR2 )
  {
    eBlkMode = ( eBlkMode == BLK_8x8 ) ? BLK_4x8  : BLK_4x4;
  }

  return eBlkMode;
}


Void  MbMotionData::copyFrom( const MbMotionData& rcMbMotionData )
{
  memcpy( m_ascRefIdx,  rcMbMotionData.m_ascRefIdx, 4 * sizeof(SChar) );
  memcpy( m_acRefPicIdc, rcMbMotionData.m_acRefPicIdc, 4 * sizeof(RefPicIdc) );
  m_usMotPredFlags = rcMbMotionData.m_usMotPredFlags;

  m_bFieldFlag = rcMbMotionData.m_bFieldFlag;
  MbMvData::copyFrom( rcMbMotionData );
}


H264AVC_NAMESPACE_END
