
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/MotionVectorCalculation.h"



H264AVC_NAMESPACE_BEGIN


MotionVectorCalculation::MotionVectorCalculation() :
  m_uiMaxBw           ( 0 ),
  m_bSpatialDirectMode( false )
{
}


MotionVectorCalculation::~MotionVectorCalculation()
{
}

ErrVal MotionVectorCalculation::create( MotionVectorCalculation*& rpcMotionVectorCalculation )
{
  rpcMotionVectorCalculation = new MotionVectorCalculation();
  ROF( rpcMotionVectorCalculation );
  return Err::m_nOK;
}

ErrVal MotionVectorCalculation::destroy()
{
  delete this;
  return Err::m_nOK;
}

ErrVal MotionVectorCalculation::initSlice( const SliceHeader& rcSH )
{
  m_bSpatialDirectMode  = rcSH.getDirectSpatialMvPredFlag();
  m_uiMaxBw             = rcSH.isBSlice() ? 2 : 1;

  return Err::m_nOK;
}


ErrVal MotionVectorCalculation::uninit()
{
  return Err::m_nOK;
}


Void MotionVectorCalculation::xCalc16x16( MbDataAccess& rcMbDataAccess,
                                          MbDataAccess* pcMbDataAccessBase )
{
  Mv    cMv;
  SChar scRefPic;

  for( UInt uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
  {
    ListIdx       eListIdx        = ListIdx( uiBw );
    MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
    MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

    if( 0 < (scRefPic = rcMbMotionData.getRefIdx() ) )
    {
      if( rcMbMotionData.getMotPredFlag() )
      {
        AOF( pcMbDataAccessBase );
        cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv();
      }
      else
      {
        rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx );
      }
      cMv += rcMbMvdData.getMv();

      rcMbMotionData.setAllMv( cMv );
    }
  }
}


Void MotionVectorCalculation::xCalc16x8( MbDataAccess&  rcMbDataAccess,
                                         MbDataAccess*  pcMbDataAccessBase )
{
  Mv    cMv;
  SChar scRefPic;

  for( UInt uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
  {
    ListIdx       eListIdx        = ListIdx( uiBw );
    MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
    MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

    if( 0 < (scRefPic = rcMbMotionData.getRefIdx( PART_16x8_0 ) ) )
    {
      if( rcMbMotionData.getMotPredFlag( PART_16x8_0 ) )
      {
        AOF( pcMbDataAccessBase );
        cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( PART_16x8_0 );
      }
      else
      {
        rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, PART_16x8_0 );
      }
      cMv += rcMbMvdData.getMv( PART_16x8_0 );

      rcMbMotionData.setAllMv( cMv, PART_16x8_0 );
    }
    if( 0 < (scRefPic = rcMbMotionData.getRefIdx( PART_16x8_1 ) ) )
    {
      if( rcMbMotionData.getMotPredFlag( PART_16x8_1 ) )
      {
        AOF( pcMbDataAccessBase );
        cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( PART_16x8_1 );
      }
      else
      {
        rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, PART_16x8_1 );
      }
      cMv += rcMbMvdData.getMv( PART_16x8_1 );

      rcMbMotionData.setAllMv( cMv, PART_16x8_1 );
    }
  }
}


Void MotionVectorCalculation::xCalc8x16( MbDataAccess&  rcMbDataAccess,
                                         MbDataAccess*  pcMbDataAccessBase )
{
  Mv    cMv;
  SChar scRefPic;

  for( UInt uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
  {
    ListIdx       eListIdx        = ListIdx( uiBw );
    MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
    MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

    if( 0 < (scRefPic = rcMbMotionData.getRefIdx( PART_8x16_0 ) ) )
    {
      if( rcMbMotionData.getMotPredFlag( PART_8x16_0 ) )
      {
        AOF( pcMbDataAccessBase );
        cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( PART_8x16_0 );
      }
      else
      {
        rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, PART_8x16_0 );
      }
      cMv += rcMbMvdData.getMv( PART_8x16_0 );

      rcMbMotionData.setAllMv( cMv, PART_8x16_0 );
    }

    if( 0 < (scRefPic = rcMbMotionData.getRefIdx( PART_8x16_1 ) ) )
    {
      if( rcMbMotionData.getMotPredFlag( PART_8x16_1 ) )
      {
        AOF( pcMbDataAccessBase );
        cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( PART_8x16_1 );
      }
      else
      {
        rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, PART_8x16_1 );
      }
      cMv += rcMbMvdData.getMv( PART_8x16_1 );

      rcMbMotionData.setAllMv( cMv, PART_8x16_1 );
    }
  }
}



Void MotionVectorCalculation::xCalc8x8( B8x8Idx       c8x8Idx,
                                        MbDataAccess& rcMbDataAccess,
                                        MbDataAccess* pcMbDataAccessBase,
                                        Bool          bFaultTolerant )
{
  Mv    cMv;
  SChar scRefPic;
  UInt  uiBw;

  ParIdx8x8 eParIdx = c8x8Idx.b8x8();

  switch( rcMbDataAccess.getMbData().getBlkMode( c8x8Idx.b8x8Index() ) )
  {
    case BLK_SKIP:
    {
      RefFrameList* pcL0  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_0 );
      RefFrameList* pcL1  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_1 );
      AOF( pcL0 && pcL1 );
      if( rcMbDataAccess.getSH().isH264AVCCompatible() )
      {
        Bool bOneMv;
        AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, false, pcL0, pcL1 ) );
      }
      else
      {
        ANOK( rcMbDataAccess.setSVCDirectModeMvAndRef( *pcL0, *pcL1, (Int)c8x8Idx.b8x8Index() ) );
      }
      break;
    }
    case BLK_8x8:
    {
      for( uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
      {
        ListIdx       eListIdx        = ListIdx( uiBw );
        MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
        MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

        if( 0 < (scRefPic = rcMbMotionData.getRefIdx( eParIdx ) ) )
        {
          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx );
          }
          cMv += rcMbMvdData.getMv( eParIdx );
          rcMbMotionData.setAllMv( cMv, eParIdx );
        }
      }
      break;
    }
    case BLK_8x4:
    {
      for( uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
      {
        ListIdx       eListIdx        = ListIdx( uiBw );
        MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
        MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

        if( 0 < (scRefPic = rcMbMotionData.getRefIdx( eParIdx ) ) )
        {
          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_8x4_0 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_8x4_0 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_8x4_0 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_8x4_0 );

          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_8x4_1 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_8x4_1 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_8x4_1 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_8x4_1 );
        }
      }
      break;
    }
    case BLK_4x8:
    {
      for( uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
      {
        ListIdx       eListIdx        = ListIdx( uiBw );
        MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
        MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

        if( 0 < (scRefPic = rcMbMotionData.getRefIdx( eParIdx ) ) )
        {
          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x8_0 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x8_0 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x8_0 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x8_0 );

          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x8_1 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x8_1 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x8_1 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x8_1 );
        }
      }
      break;
    }
    case BLK_4x4:
    {
      for( uiBw = 0; uiBw < m_uiMaxBw; uiBw++ )
      {
        ListIdx       eListIdx        = ListIdx( uiBw );
        MbMotionData& rcMbMotionData  = rcMbDataAccess.getMbMotionData( eListIdx );
        MbMvData&     rcMbMvdData     = rcMbDataAccess.getMbMvdData   ( eListIdx );

        if( 0 < (scRefPic = rcMbMotionData.getRefIdx( eParIdx ) ) )
        {
          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x4_0 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x4_0 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x4_0 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x4_0 );

          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x4_1 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x4_1 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x4_1 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x4_1 );

          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x4_2 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x4_2 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x4_2 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x4_2 );

          if( rcMbMotionData.getMotPredFlag( eParIdx ) )
          {
            AOF( pcMbDataAccessBase );
            cMv = pcMbDataAccessBase->getMbMotionData( eListIdx ).getMv( eParIdx, SPART_4x4_3 );
          }
          else
          {
            rcMbDataAccess.getMvPredictor( cMv, scRefPic, eListIdx, eParIdx, SPART_4x4_3 );
          }
          cMv +=  rcMbMvdData.getMv( eParIdx, SPART_4x4_3 );
          rcMbMotionData.setAllMv( cMv, eParIdx, SPART_4x4_3 );
        }
      }
      break;
    }
    default:
    {
      AF();
      break;
    }
  }
}




Void MotionVectorCalculation::xCalc8x8( MbDataAccess& rcMbDataAccess,
                                        MbDataAccess* pcMbDataAccessBase,
                                        Bool          bFaultTolerant )
{
  for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
  {
    xCalc8x8( c8x8Idx, rcMbDataAccess, pcMbDataAccessBase, bFaultTolerant );
  }
}

ErrVal
MotionVectorCalculation::calcMvMb( MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase )
{
  switch( rcMbDataAccess.getMbData().getMbMode() )
  {
  case MODE_16x16:
    xCalc16x16( rcMbDataAccess, pcMbDataAccessBase );
    break;
  case MODE_16x8:
    xCalc16x8( rcMbDataAccess, pcMbDataAccessBase );
    break;
  case MODE_8x16:
    xCalc8x16( rcMbDataAccess, pcMbDataAccessBase );
    break;
  case MODE_SKIP:
    if( rcMbDataAccess.getSH().isBSlice() )
    {
      RefFrameList* pcL0  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_0 );
      RefFrameList* pcL1  = rcMbDataAccess.getSH().getRefFrameList( rcMbDataAccess.getMbPicType(), LIST_1 );
      ROF( pcL0 && pcL1 );
      if( rcMbDataAccess.getSH().isH264AVCCompatible() )
      {
        B8x8Idx c8x8Idx;
        Bool    bOneMv;
        AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx++;
        AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx++;
        AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, false, pcL0, pcL1 ) ); c8x8Idx++;
        AOF( rcMbDataAccess.getMvPredictorDirect( c8x8Idx.b8x8(), bOneMv, false, pcL0, pcL1 ) );
      }
      else
      {
        RNOK( rcMbDataAccess.setSVCDirectModeMvAndRef( *pcL0, *pcL1 ) );
      }
    }
    else
    {
      Mv cMvSkip;
      rcMbDataAccess.getMvPredictorSkipMode( cMvSkip );
      rcMbDataAccess.getMbMotionData( LIST_0 ).setAllMv( cMvSkip );
    }
    break;
  case MODE_8x8:
  case MODE_8x8ref0:
    xCalc8x8( rcMbDataAccess, pcMbDataAccessBase, false );
    break;
  default:
    break;
  }
  return Err::m_nOK;
}

ErrVal
MotionVectorCalculation::calcMvSubMb( B8x8Idx c8x8Idx, MbDataAccess& rcMbDataAccess, MbDataAccess* pcMbDataAccessBase )
{
  xCalc8x8( c8x8Idx, rcMbDataAccess, pcMbDataAccessBase, false );
  return Err::m_nOK;
}


H264AVC_NAMESPACE_END

