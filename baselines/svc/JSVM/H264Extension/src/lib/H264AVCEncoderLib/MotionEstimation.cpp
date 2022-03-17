
#include "H264AVCEncoderLib.h"

#include "H264AVCCommonLib/SampleWeighting.h"
#include "RateDistortionIf.h"
#include "CodingParameter.h"
#include "MotionEstimation.h"

#include "H264AVCCommonLib/Transform.h"


H264AVC_NAMESPACE_BEGIN


MotionEstimation::MotionEstimation() :
  m_pcQuarterPelFilter( NULL ),
  m_pcXDistortion( NULL ),
  m_iMaxLogStep( 0 ),
  m_pcMvSpiralSearch( NULL ),
  m_uiSpiralSearchEntries(0)
  ,m_bELWithBLMv(false)
{
}


MotionEstimation::~MotionEstimation()
{
}


ErrVal MotionEstimation::destroy()
{
  delete this;
  return Err::m_nOK;
}


ErrVal MotionEstimation::init( XDistortion*      pcXDistortion,
                               CodingParameter*  pcCodingParameter,
                               RateDistortionIf* pcRateDistortionIf,
                               QuarterPelFilter* pcQuarterPelFilter,
                               Transform*        pcTransform,
                               SampleWeighting*  pcSampleWeighting )
{
  RNOK( MotionCompensation::init( pcQuarterPelFilter, pcTransform, pcSampleWeighting ) );
  ROT( NULL == pcXDistortion );
  ROT( NULL == pcCodingParameter );
  ROT( NULL == pcQuarterPelFilter );
  m_pcXDistortion       = pcXDistortion;
  m_pcQuarterPelFilter  = pcQuarterPelFilter;
  m_cParams             = pcCodingParameter->getMotionVectorSearchParams();

  {
    UInt uiCurrLimit = m_cParams.getSearchRange() >>4;
    m_iMaxLogStep = 1;
    while( uiCurrLimit != 0)
    {
      uiCurrLimit   >>= 1;
      m_iMaxLogStep <<= 1;
    }

  }

  RNOK( MotionEstimationCost::xInit( (m_cParams.getSearchRange() << 2), pcRateDistortionIf ) );

  ROTRS( NULL != m_pcMvSpiralSearch, Err::m_nOK ) ;

  UInt uiSubPelSearchEntries = 7;
  m_uiSpiralSearchEntries    = 2*m_cParams.getSearchRange() + 1;

  UInt uiSize = gMax( uiSubPelSearchEntries, m_uiSpiralSearchEntries );

  m_pcMvSpiralSearch = new Mv [ uiSize*uiSize ];
  m_uiSpiralSearchEntries *= m_uiSpiralSearchEntries;

  ROT( NULL == m_pcMvSpiralSearch );

  m_pcMvSpiralSearch[0].setZero();

  Int n = 1;
  uiSize++;
  uiSize /= 2;

  const Int iRange = m_cParams.getSearchRange();
  // reference spiral search
  for( Int offset = 1; offset <= iRange; offset++ )
  {
    Int l;
    for( l = -offset+1; l < offset; l++ )
    {
      m_pcMvSpiralSearch[n].setHor( l );
      m_pcMvSpiralSearch[n].setVer( -offset );
      n++;
      m_pcMvSpiralSearch[n].setHor( l );
      m_pcMvSpiralSearch[n].setVer( +offset );
      n++;
    }

    for( l = -offset; l <= offset; l++ )
    {
      m_pcMvSpiralSearch[n].setHor( -offset );
      m_pcMvSpiralSearch[n].setVer( l );
      n++;
      m_pcMvSpiralSearch[n].setHor( +offset );
      m_pcMvSpiralSearch[n].setVer( l );
      n++;
    }
  }

  return Err::m_nOK;
}


ErrVal MotionEstimation::uninit()
{
  RNOK( MotionCompensation::uninit() );
  RNOK( MotionEstimationCost::xUninit() );

  if( m_pcMvSpiralSearch )
  {
    delete [] m_pcMvSpiralSearch;
    m_pcMvSpiralSearch = NULL;
  }

  return Err::m_nOK;
}


ErrVal
MotionEstimation::estimateBlockWithStart( const MbDataAccess&          rcMbDataAccess,
                                          const Frame&                 rcRefFrame,
                                          Mv&                          rcMv, // start and result
                                          const Mv&                    rcMvPred,
                                          UInt&                        ruiBits,
                                          UInt&                        ruiCost,
                                          const UInt                   uiBlk,
                                          const UInt                   uiMode,
                                          const UInt                   uiSearchRange,
                                          const PredWeight*            pcPW,
                                          const MEBiSearchParameters*  pcBSP )
{
  const LumaIdx cIdx                 = B4x4Idx( uiBlk );
  YuvMbBuffer*  pcWeightedYuvBuffer  = 0;
  YuvPicBuffer* pcRefPelData[2];
  YuvMbBuffer   cWeightedYuvBuffer;

  pcRefPelData[0] = const_cast<Frame&>(rcRefFrame).getFullPelYuvBuffer();
  pcRefPelData[1] = const_cast<Frame&>(rcRefFrame).getHalfPelYuvBuffer();

  m_pcXDistortion->set4x4Block( cIdx );
  pcRefPelData[0]->set4x4Block( cIdx );
  pcRefPelData[1]->set4x4Block( cIdx );

  UInt   uiMinSAD  = MSYS_UINT_MAX;
  Mv     cMv       = rcMv; // start value
  Double fWeight   = 1.0;
  Double afCW[2]   = { 1.0, 1.0 };

  Bool      bOriginalSearchModeIsYUVSAD  = ( m_cParams.getFullPelDFunc() == DF_YUV_SAD );
  const Int iXSize                       = m_pcXDistortion->getBlockWidth  ( uiMode );
  const Int iYSize                       = m_pcXDistortion->getBlockHeight ( uiMode );

  if( pcBSP ) // bi prediction
  {
    ROF( pcBSP->pcAltRefPelData   );
    ROF( pcBSP->pcAltRefFrame     );
    ROF( pcBSP->apcWeight[LIST_0] );
    ROF( pcBSP->apcWeight[LIST_1] );

    pcWeightedYuvBuffer   = &cWeightedYuvBuffer;
    pcWeightedYuvBuffer   ->set4x4Block( cIdx );
    pcBSP->pcAltRefPelData->set4x4Block( cIdx );

    if( rcMbDataAccess.getSH().getPPS().getWeightedBiPredIdc() == 2 ) // implicit weighting
    {
      //----- get implicit weights -----
      PredWeight   acIPW[2];
      const Frame* pcFrameL0 = ( pcBSP->uiL1Search ? pcBSP->pcAltRefFrame : &rcRefFrame          );
      const Frame* pcFrameL1 = ( pcBSP->uiL1Search ? &rcRefFrame          : pcBSP->pcAltRefFrame );
      Int          iScale    = rcMbDataAccess.getSH().getDistScaleFactorWP( pcFrameL0, pcFrameL1 );
      //----- weighting -----
      if( iScale == 128 ) // same distance -> use normal function for same result
      {
        m_pcSampleWeighting->inverseLumaSamples( pcWeightedYuvBuffer,
                                                 m_pcXDistortion->getYuvMbBuffer(),
                                                 pcBSP->pcAltRefPelData,
                                                 iYSize, iXSize );
        fWeight = 0.5;
      }
      else
      {
        acIPW[1].scaleL1Weight( iScale   );
        acIPW[0].scaleL0Weight( acIPW[1] );
        m_pcSampleWeighting->weightInverseLumaSamples( pcWeightedYuvBuffer,
                                                       m_pcXDistortion->getYuvMbBuffer(),
                                                       pcBSP->pcAltRefPelData,
                                                       &acIPW[pcBSP->uiL1Search],
                                                       &acIPW[1-pcBSP->uiL1Search],
                                                       fWeight, iYSize, iXSize );
      }
    }
    else if( pcBSP->apcWeight[LIST_0]->getLumaWeightFlag() ||
             pcBSP->apcWeight[LIST_1]->getLumaWeightFlag()   )
    {
      //----- explicit weighting -----
      m_pcSampleWeighting->weightInverseLumaSamples( pcWeightedYuvBuffer,
                                                     m_pcXDistortion->getYuvMbBuffer(),
                                                     pcBSP->pcAltRefPelData,
                                                     pcBSP->apcWeight[pcBSP->uiL1Search],
                                                     pcBSP->apcWeight[1-pcBSP->uiL1Search],
                                                     fWeight, iYSize, iXSize );
    }
    else
    {
      //----- standard weighting -----
      m_pcSampleWeighting->inverseLumaSamples( pcWeightedYuvBuffer,
                                               m_pcXDistortion->getYuvMbBuffer(),
                                               pcBSP->pcAltRefPelData,
                                               iYSize, iXSize );
      fWeight = 0.5;
    }
  }
  else // unidirectional prediction
  {
    ROF( pcPW );
    if( pcPW->getLumaWeightFlag() || ( bOriginalSearchModeIsYUVSAD && pcPW->getChromaWeightFlag() ) )
    {
      pcWeightedYuvBuffer = &cWeightedYuvBuffer;
      pcWeightedYuvBuffer ->set4x4Block( cIdx );
      //----- weighting -----
      m_pcSampleWeighting->weightInverseLumaSamples( pcWeightedYuvBuffer,
                                                     m_pcXDistortion->getYuvMbBuffer(),
                                                     pcPW, fWeight, iYSize, iXSize );
      if( bOriginalSearchModeIsYUVSAD )
      {
        m_pcSampleWeighting->weightInverseChromaSamples( pcWeightedYuvBuffer,
                                                         m_pcXDistortion->getYuvMbBuffer(),
                                                         pcPW, afCW, iYSize, iXSize );
      }
    }
    else
    {
      //----- no weighting -----
      pcWeightedYuvBuffer = m_pcXDistortion->getYuvMbBuffer();
      fWeight = afCW[0] = afCW[1] = 1.0;
    }
  }

  //===== FULL-PEL ESTIMATION ======
  if( bOriginalSearchModeIsYUVSAD && ( pcBSP /* bi-prediction */ || fWeight != afCW[0] || fWeight != afCW[1] /* different component weights */ ) )
  {
    m_cParams.setFullPelDFunc( DF_SAD ); // set to normal SAD
  }
  // <<< heiko.schwarz@hhi.fhg.de (fix for uninitialized memory with YUV_SAD and bi-directional search)
  xSetMEPars      ( 2, ( 1 != m_cParams.getFullPelDFunc() ) );
  xSetPredictor   ( rcMvPred );
  m_pcXDistortion ->getDistStruct( uiMode, m_cParams.getFullPelDFunc(), false, m_cXDSS );
  m_cXDSS.pYOrg   = pcWeightedYuvBuffer->getLumBlk();
  m_cXDSS.pUOrg   = pcWeightedYuvBuffer->getCbBlk ();
  m_cXDSS.pVOrg   = pcWeightedYuvBuffer->getCrBlk ();
  
  m_acMvCandList.clear();

  if( uiSearchRange )
  {
    if( m_cParams.getFastBiSearch() )
    {
      xTZSearch( pcRefPelData[0], cMv, uiMinSAD, m_bELWithBLMv, uiSearchRange );
    }
    else
    {
      xPelBlockSearch( pcRefPelData[0], cMv, uiMinSAD, uiSearchRange );
    }
  }
  else
  {
    switch( m_cParams.getSearchMode() )
    {
    case BLOCK_SEARCH:
      {
        xPelBlockSearch ( pcRefPelData[0], cMv, uiMinSAD );
      }
      break;
    case SPIRAL_SEARCH:
      {
        xPelSpiralSearch( pcRefPelData[0], cMv, uiMinSAD );
      }
      break;
    case LOG_SEARCH:
      {
        xPelLogSearch   ( pcRefPelData[0], cMv, uiMinSAD, false, m_iMaxLogStep << ( pcBSP ? 0 : 1 ) );
      }
      break;
    case FAST_SEARCH:
      {
        m_acMvCandList.push_back( rcMvPred );
        rcMbDataAccess.addMvPredictors( m_acMvCandList );
        xPelLogSearch   ( pcRefPelData[0], cMv, uiMinSAD, true, ( pcBSP ? 1 : 2 ) );
      }
      break;
    case TZ_SEARCH:
      {
        m_acMvCandList.push_back( rcMvPred );
        rcMbDataAccess.addMvPredictors( m_acMvCandList );
        xTZSearch( pcRefPelData[0], cMv, uiMinSAD, m_bELWithBLMv );
      }
      break;
    default:
      RERR();
      break;
    }
  }
  cMv <<= 2;

  // heiko.schwarz@hhi.fhg.de (fix for uninitialized memory with YUV_SAD and bi-directional search) >>>>
  if( bOriginalSearchModeIsYUVSAD )
  {
    m_cParams.setFullPelDFunc( DF_YUV_SAD );
  }
  // <<< heiko.schwarz@hhi.fhg.de (fix for uninitialized memory with YUV_SAD and bi-directional search)


  //===== SUB-PEL ESTIMATION =====
  xSetMEPars( 0, ( 1 != ( 1 & m_cParams.getSubPelDFunc() ) ) );
  m_pcXDistortion->getDistStruct( uiMode, m_cParams.getSubPelDFunc(), false, m_cXDSS );
  m_cXDSS.pYOrg = pcWeightedYuvBuffer->getLumBlk();

  xSubPelSearch( pcRefPelData[1], cMv, uiMinSAD, uiBlk, uiMode );

  Short sHor      = cMv.getHor();
  Short sVer      = cMv.getVer();
  UInt  uiMvBits  = xGetBits( sHor, sVer );
  ruiBits        += uiMvBits;
  ruiCost         = (UInt)floor( fWeight * (Double)( uiMinSAD - xGetCost( uiMvBits ) ) ) + xGetCost( ruiBits );
  rcMv            = cMv;
  m_bELWithBLMv   = false;

  return Err::m_nOK;
}


ErrVal MotionEstimation::initMb( UInt uiMbPosY, UInt uiMbPosX, MbDataAccess& rcMbDataAccess )
{
  RNOK( MotionCompensation::initMb( uiMbPosY, uiMbPosX, rcMbDataAccess) );// TMM_INTERLACE

  return Err::m_nOK;
}


Void MotionEstimation::xPelBlockSearch( YuvPicBuffer *pcPelData, Mv& rcMv, UInt& ruiSAD, UInt uiSearchRange )
{
  if( ! uiSearchRange )
  {
    uiSearchRange = m_cParams.getSearchRange();
  }

  Int     iYStride   = pcPelData->getLStride();
  XPel*   pucYRef    = pcPelData->getLumBlk ();
  XPel*   pucYSearch;
  Int     iCStride   = pcPelData->getCStride();
  XPel*   pucURef    = pcPelData->getCbBlk  ();
  XPel*   pucVRef    = pcPelData->getCrBlk  ();
  UInt    uiSad;

  Int y = 0;
  Int x = 0;

  m_cXDSS.iYStride = iYStride;
  m_cXDSS.iCStride = iCStride;

  ruiSAD = MSYS_UINT_MAX;
  rcMv.limitComponents( m_cMin, m_cMax );
  rcMv >>= 2;

  SearchRect cSearchRect;
  cSearchRect.init( uiSearchRange, rcMv, m_cMin, m_cMax );

  pucYSearch  = pucYRef - iYStride * (cSearchRect.iNegVerLimit);

  for(  y = -cSearchRect.iNegVerLimit; y <= cSearchRect.iPosVerLimit; y++ )
  {
    for( x = -cSearchRect.iNegHorLimit; x <= cSearchRect.iPosHorLimit; x++ )
    {
      m_cXDSS.pYSearch  = pucYSearch + x;
      m_cXDSS.pUSearch  = pucURef + (y>>1)*iCStride + (x>>1);
      m_cXDSS.pVSearch  = pucVRef + (y>>1)*iCStride + (x>>1);

      uiSad  = m_cXDSS.Func( &m_cXDSS );
      uiSad += xGetCost( x, y);

      if( ruiSAD > uiSad )
      {
        ruiSAD = uiSad;
        rcMv.setVer( y );
        rcMv.setHor( x );
      }
    }
    pucYSearch += iYStride;
  }

  y = rcMv.getVer();
  x = rcMv.getHor();

  ruiSAD -= xGetCost( x, y);

  DO_DBG( m_cXDSS.pYSearch = pucYRef +  y     * iYStride +  x     );
  DO_DBG( m_cXDSS.pUSearch = pucURef + (y>>1) * iCStride + (x>>1) );
  DO_DBG( m_cXDSS.pVSearch = pucVRef + (y>>1) * iCStride + (x>>1) );
  uiSad  = m_cXDSS.Func( &m_cXDSS );
  AOF_DBG( ruiSAD == ( uiSad  = m_cXDSS.Func( &m_cXDSS ) ) );
}


Void MotionEstimation::xPelSpiralSearch( YuvPicBuffer *pcPelData, Mv& rcMv, UInt& ruiSAD, UInt uiSearchRange )
{
  if( ! uiSearchRange )
  {
    uiSearchRange = m_cParams.getSearchRange();
  }

  m_cXDSS.iYStride  = pcPelData->getLStride();
  m_cXDSS.iCStride  = pcPelData->getCStride();
  XPel* pucYRef     = pcPelData->getLumBlk();
  XPel* pucURef     = pcPelData->getCbBlk ();
  XPel* pucVRef     = pcPelData->getCrBlk ();
  UInt  uiSad;
  UInt  uiBestPos = 0;

  Mv cLimitMv( 4*uiSearchRange, 4*uiSearchRange );
  Mv cMin = m_cMin + cLimitMv;
  Mv cMax = m_cMax - cLimitMv;

  rcMv.limitComponents( cMin, cMax );
  rcMv >>= 2;

  ruiSAD = MSYS_UINT_MAX;
  for( UInt n = 0; n < m_uiSpiralSearchEntries; n++ )
  {
    Int x = rcMv.getHor() + m_pcMvSpiralSearch[n].getHor();
    Int y = rcMv.getVer() + m_pcMvSpiralSearch[n].getVer();

    m_cXDSS.pYSearch = pucYRef + y * m_cXDSS.iYStride + x;
    m_cXDSS.pUSearch = pucURef + (y>>1) * m_cXDSS.iCStride + (x>>1);
    m_cXDSS.pVSearch = pucVRef + (y>>1) * m_cXDSS.iCStride + (x>>1);

    uiSad  = m_cXDSS.Func( &m_cXDSS );
    uiSad += xGetCost( x, y );

    if( ruiSAD > uiSad )
    {
      ruiSAD = uiSad;
      uiBestPos = n;
    }

  }
  rcMv += m_pcMvSpiralSearch[uiBestPos];

  Int y = rcMv.getVer();
  Int x = rcMv.getHor();

  ruiSAD -= xGetCost( x, y);

  DO_DBG( m_cXDSS.pYSearch = pucYRef +  y     * m_cXDSS.iYStride +  x );
  DO_DBG( m_cXDSS.pUSearch = pucURef + (y>>1) * m_cXDSS.iCStride + (x>>1) );
  DO_DBG( m_cXDSS.pVSearch = pucVRef + (y>>1) * m_cXDSS.iCStride + (x>>1) );
  AOF_DBG( ruiSAD == ( uiSad  = m_cXDSS.Func( &m_cXDSS ) ) );
}



Void MotionEstimation::xPelLogSearch( YuvPicBuffer *pcPelData, Mv& rcMv, UInt& ruiSAD, Bool bFme, UInt uiStep, UInt uiSearchRange )
{
  if( ! uiSearchRange )
  {
    uiSearchRange = m_cParams.getSearchRange();
  }

  XPel* pucRef  = pcPelData->getLumBlk();
  XPel* pucSearch;
  Int   iStride = pcPelData->getLStride();
  UInt  uiSad;
  UInt  uiBestSad;

  Int   iStepSizeStart = uiStep;

  Mv cMvPred = rcMv;
  cMvPred.limitComponents( m_cMin, m_cMax );
  cMvPred >>= 2;

  SearchRect cSearchRect;
  cSearchRect.init( uiSearchRange, cMvPred, m_cMin, m_cMax );

  rcMv.limitComponents( m_cMin, m_cMax );
  rcMv >>= 2;


  m_cXDSS.iYStride = iStride;
  m_cXDSS.pYSearch = pucRef + rcMv.getHor() + rcMv.getVer() * iStride;
  uiBestSad = m_cXDSS.Func( &m_cXDSS );
  uiBestSad += xGetCost( rcMv.getHor(), rcMv.getVer() );

  if( bFme )
  {
    for( UInt n = 0; n < m_acMvCandList.size(); n++ )
    {
      Mv cMv = m_acMvCandList[n];
      {
        cMv.limitComponents( m_cMin, m_cMax );
        cMv >>= 2;
        XPel* pStart = pucRef + cMv.getHor() + cMv.getVer() * iStride;
        m_cXDSS.pYSearch = pStart;
        uiSad  = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( cMv.getHor(), cMv.getVer());
        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          rcMv = cMv;
        }
      }
    }
  }


  Int x = rcMv.getHor();
  Int y = rcMv.getVer();
  pucSearch = pucRef + x + y * iStride;

  Int dxOld = 0;
  Int dyOld = 0;
  Bool  bContinue;
  Int   iStepStride;
  Int iStep;


  for( iStep = iStepSizeStart; iStep != 0; iStep >>= 1 )
  {
    bContinue   = true;
    iStepStride = iStep * iStride;
    dxOld = 0;
    dyOld = 0;

    while( ruiSAD && bContinue )
    {
      Int   dx = 0;
      Int   dy = 0;

      if( dxOld <= 0 && -x <= cSearchRect.iNegHorLimit - iStep )
      {
        m_cXDSS.pYSearch = pucSearch - iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x - iStep, y);

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = -iStep;
        }
      }


      if( dxOld >= 0 && x < cSearchRect.iPosHorLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch + iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x + iStep, y);

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = iStep;
        }
      }


      if( dyOld <= 0 && -y <= cSearchRect.iNegVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch - iStepStride;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x, y - iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx =  0;
          dy = -iStep;
        }
      }


      if( dyOld >= 0 && y < cSearchRect.iPosVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch + iStepStride;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x, y + iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = 0;
          dy = iStep;
        }
      }

      bContinue = ( dx != 0 || dy !=0 );

      if( bContinue )
      {
        dxOld = dx;
        dyOld = dy;
        x += dx;
        y += dy;
        pucSearch = pucRef + x + y * iStride;
      }
    }
  }




  iStep = 1;
  iStepStride = iStep * iStride;
  {
    Int   dx = 0;
    Int   dy = 0;

    if( dxOld != 1 && -x <= cSearchRect.iNegHorLimit - iStep )
    {
      if( dyOld != 1 && -y <= cSearchRect.iNegVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch - iStepStride - iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x - iStep, y - iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = -iStep;
          dy = -iStep;
        }
      }


      if( dyOld != -1 && y < cSearchRect.iPosVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch + iStepStride - iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x - iStep, y + iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = -iStep;
          dy =  iStep;
        }
      }
    }


    if( dxOld != -1 && x < cSearchRect.iPosHorLimit - iStep)
    {
      if( dyOld != 1 && -y <= cSearchRect.iNegVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch - iStepStride + iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x + iStep, y - iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx =  iStep;
          dy = -iStep;
        }
      }


      if( dyOld != -1 && y < cSearchRect.iPosVerLimit - iStep)
      {
        m_cXDSS.pYSearch = pucSearch + iStepStride + iStep;
        uiSad = m_cXDSS.Func( &m_cXDSS );
        uiSad += xGetCost( x + iStep, y + iStep );

        if( uiBestSad > uiSad )
        {
          uiBestSad = uiSad;
          dx = iStep;
          dy = iStep;
        }
      }
    }

    bContinue = ( dx != 0 || dy !=0 );

    if( bContinue )
    {
      x += dx;
      y += dy;
      pucSearch = pucRef + x + y * iStride;
    }
  }

  ruiSAD = uiBestSad - xGetCost( x, y );
  rcMv.setHor( x );
  rcMv.setVer( y );


  DO_DBG( m_cXDSS.pYSearch = pucRef + y * iStride + x );
  AOF_DBG( ruiSAD == ( uiSad  = m_cXDSS.Func( &m_cXDSS ) ) );
}



#define TZ_SEARCH_CONFIGURATION                                                                                 \
  const Int  iRaster                  = 3;  /* TZ soll von aussen übergeben werden */                           \
  const Bool bTestOtherPredictedMV    = 1;                                                                      \
  const Bool bTestZeroVector          = 1;                                                                      \
  const Bool bTestZeroVectorStar      = 0;                                                                      \
  const Bool bTestZeroVectorStop      = 0;                                                                      \
  const Bool bFirstSearchDiamond      = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
  const Bool bFirstSearchStop         = 0;                                                                      \
  const UInt uiFirstSearchRounds      = 3;  /* first search stop X rounds after best match (must be >=1) */     \
  const Bool bEnableRasterSearch      = 1;                                                                      \
  const Bool bAlwaysRasterSearch      = 0;  /* ===== 1: BETTER but factor 2 slower ===== */                     \
  const Bool bRasterRefinementEnable  = 0;  /* enable either raster refinement or star refinement */            \
  const Bool bRasterRefinementDiamond = 0;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
  const Bool bStarRefinementEnable    = 1;  /* enable either star refinement or raster refinement */            \
  const Bool bStarRefinementDiamond   = 1;  /* 1 = xTZ8PointDiamondSearch   0 = xTZ8PointSquareSearch */        \
  const Bool bStarRefinementStop      = 0;                                                                      \
  const UInt uiStarRefinementRounds   = 2;  /* star refinement stop X rounds after best match (must be >=1) */  \



__inline
Void MotionEstimation::xTZCheckPoint( IntTZSearchStrukt& rcStrukt, const Int iSearchX, const Int iSearchY, const UChar ucPointNr, const UInt uiDistance )
{
#if 0 //degug
  std::cout << "Pruefpunkt           "
            << "  PX:"  << std::setw(4) << std::setfill(' ') << iSearchX
            << "  PY:"  << std::setw(4) << std::setfill(' ') << iSearchY
            << "  Dis:" << std::setw(6) << std::setfill(' ') << uiDistance << std::endl;
#endif

  m_cXDSS.pYSearch = rcStrukt.pucYRef +  iSearchY     * rcStrukt.iYStride +  iSearchX;
  m_cXDSS.pUSearch = rcStrukt.pucURef + (iSearchY>>1) * rcStrukt.iCStride + (iSearchX>>1);
  m_cXDSS.pVSearch = rcStrukt.pucVRef + (iSearchY>>1) * rcStrukt.iCStride + (iSearchX>>1);

  UInt uiSad       = m_cXDSS.Func( &m_cXDSS );
  uiSad           += MotionEstimationCost::xGetCost( iSearchX, iSearchY );
  if( rcStrukt.uiBestSad > uiSad )
  {
    rcStrukt.uiBestSad      = uiSad;
    rcStrukt.iBestX         = iSearchX;
    rcStrukt.iBestY         = iSearchY;
    rcStrukt.uiBestDistance = uiDistance;
    rcStrukt.uiBestRound    = 0;
    rcStrukt.ucPointNr      = ucPointNr;
  }
}

__inline
Void MotionEstimation::xTZ2PointSearch( IntTZSearchStrukt& rcStrukt, SearchRect rcSearchRect )
{ // 2 point search,                   //   1 2 3
  // check only the 2 untested points  //   4 0 5
  // around the start point            //   6 7 8
  Int iStartX = rcStrukt.iBestX;
  Int iStartY = rcStrukt.iBestY;
  switch( rcStrukt.ucPointNr )
  {
  case 1:
    {
      if ( (iStartX - 1) >= -rcSearchRect.iNegHorLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX - 1, iStartY, 0, 2 );
      }
      if ( (iStartY - 1) >= -rcSearchRect.iNegVerLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX, iStartY - 1, 0, 2 );
      }
    }
    break;
  case 2:
    {
      if ( (iStartY - 1) >= -rcSearchRect.iNegVerLimit )
      {
        if ( (iStartX - 1) >= -rcSearchRect.iNegHorLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX - 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartX + 1) <= rcSearchRect.iPosHorLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX + 1, iStartY - 1, 0, 2 );
        }
      }
    }
    break;
  case 3:
    {
      if ( (iStartY - 1) >= -rcSearchRect.iNegVerLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX, iStartY - 1, 0, 2 );
      }
      if ( (iStartX + 1) <= rcSearchRect.iPosHorLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX + 1, iStartY, 0, 2 );
      }
    }
    break;
  case 4:
    {
      if ( (iStartX - 1) >= -rcSearchRect.iNegHorLimit )
      {
        if ( (iStartY + 1) <= rcSearchRect.iPosVerLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartY - 1) >= -rcSearchRect.iNegVerLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX - 1, iStartY - 1, 0, 2 );
        }
      }
    }
    break;
  case 5:
    {
      if ( (iStartX + 1) <= rcSearchRect.iPosHorLimit )
      {
        if ( (iStartY - 1) >= -rcSearchRect.iNegVerLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX + 1, iStartY - 1, 0, 2 );
        }
        if ( (iStartY + 1) <= rcSearchRect.iPosVerLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
    break;
  case 6:
    {
      if ( (iStartX - 1) >= -rcSearchRect.iNegHorLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX - 1, iStartY , 0, 2 );
      }
      if ( (iStartY + 1) <= rcSearchRect.iPosVerLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX, iStartY + 1, 0, 2 );
      }
    }
    break;
  case 7:
    {
      if ( (iStartY + 1) <= rcSearchRect.iPosVerLimit )
      {
        if ( (iStartX - 1) >= -rcSearchRect.iNegHorLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX - 1, iStartY + 1, 0, 2 );
        }
        if ( (iStartX + 1) <= rcSearchRect.iPosHorLimit )
        {
          xTZCheckPoint( rcStrukt, iStartX + 1, iStartY + 1, 0, 2 );
        }
      }
    }
    break;
  case 8:
    {
      if ( (iStartX + 1) <= rcSearchRect.iPosHorLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX + 1, iStartY, 0, 2 );
      }
      if ( (iStartY + 1) <= rcSearchRect.iPosVerLimit )
      {
        xTZCheckPoint( rcStrukt, iStartX, iStartY + 1, 0, 2 );
      }
    }
    break;
  default:
    {
      AF();
    }
    break;
  } // switch( rcStrukt.ucPointNr )
}

__inline
Void MotionEstimation::xTZ8PointSquareSearch( IntTZSearchStrukt& rcStrukt, SearchRect rcSearchRect,
                                              const Int iStartX, const Int iStartY, const Int iDist )
{ // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  AOT_DBG( iDist == 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStrukt.uiBestRound += 1;

  if ( iTop >= -rcSearchRect.iNegVerLimit ) // check top
  {
    if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check top left
    {
      xTZCheckPoint( rcStrukt, iLeft, iTop, 1, iDist );
    }
    // top middle
    xTZCheckPoint( rcStrukt, iStartX, iTop, 2, iDist );

    if ( iRight <= rcSearchRect.iPosHorLimit ) // check top right
    {
      xTZCheckPoint( rcStrukt, iRight, iTop, 3, iDist );
    }
  } // check top
  if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check middle left
  {
    xTZCheckPoint( rcStrukt, iLeft, iStartY, 4, iDist );
  }
  if ( iRight <= rcSearchRect.iPosHorLimit ) // check middle right
  {
    xTZCheckPoint( rcStrukt, iRight, iStartY, 5, iDist );
  }
  if ( iBottom <= rcSearchRect.iPosVerLimit ) // check bottom
  {
    if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check bottom left
    {
      xTZCheckPoint( rcStrukt, iLeft, iBottom, 6, iDist );
    }
    // check bottom middle
    xTZCheckPoint( rcStrukt, iStartX, iBottom, 7, iDist );

    if ( iRight <= rcSearchRect.iPosHorLimit ) // check bottom right
    {
      xTZCheckPoint( rcStrukt, iRight, iBottom, 8, iDist );
    }
  } // check bottom
}

__inline
Void MotionEstimation::xTZ8PointDiamondSearch( IntTZSearchStrukt& rcStrukt, SearchRect rcSearchRect,
                                        const Int iStartX, const Int iStartY, const Int iDist )
{ // 8 point search,                   //   1 2 3
  // search around the start point     //   4 0 5
  // with the required  distance       //   6 7 8
  AOT_DBG( iDist == 0 );
  const Int iTop        = iStartY - iDist;
  const Int iBottom     = iStartY + iDist;
  const Int iLeft       = iStartX - iDist;
  const Int iRight      = iStartX + iDist;
  rcStrukt.uiBestRound += 1;

  if ( iDist == 1 ) // iDist == 1
  {
    if ( iTop >= -rcSearchRect.iNegVerLimit ) // check top
    {
      xTZCheckPoint( rcStrukt, iStartX, iTop, 2, iDist );
    }
    if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check middle left
    {
      xTZCheckPoint( rcStrukt, iLeft, iStartY, 4, iDist );
    }
    if ( iRight <= rcSearchRect.iPosHorLimit ) // check middle right
    {
      xTZCheckPoint( rcStrukt, iRight, iStartY, 5, iDist );
    }
    if ( iBottom <= rcSearchRect.iPosVerLimit ) // check bottom
    {
      xTZCheckPoint( rcStrukt, iStartX, iBottom, 7, iDist );
    }
  }
  else // iDist == 1
  {
    if ( iDist <= 8 )
    {
      const Int iTop_2      = iStartY - (iDist>>1);
      const Int iBottom_2   = iStartY + (iDist>>1);
      const Int iLeft_2     = iStartX - (iDist>>1);
      const Int iRight_2    = iStartX + (iDist>>1);

      if (  iTop >= -rcSearchRect.iNegVerLimit && iLeft   >= -rcSearchRect.iNegHorLimit &&
          iRight <=  rcSearchRect.iPosHorLimit && iBottom <= rcSearchRect.iPosVerLimit ) // check border
      {
        xTZCheckPoint( rcStrukt, iStartX,  iTop,      2, iDist    );
        xTZCheckPoint( rcStrukt, iLeft_2,  iTop_2,    1, iDist>>1 );
        xTZCheckPoint( rcStrukt, iRight_2, iTop_2,    3, iDist>>1 );
        xTZCheckPoint( rcStrukt, iLeft,    iStartY,   4, iDist    );
        xTZCheckPoint( rcStrukt, iRight,   iStartY,   5, iDist    );
        xTZCheckPoint( rcStrukt, iLeft_2,  iBottom_2, 6, iDist>>1 );
        xTZCheckPoint( rcStrukt, iRight_2, iBottom_2, 8, iDist>>1 );
        xTZCheckPoint( rcStrukt, iStartX,  iBottom,   7, iDist    );
      }
      else // check border
      {
        if ( iTop >= -rcSearchRect.iNegVerLimit ) // check top
        {
          xTZCheckPoint( rcStrukt, iStartX, iTop, 2, iDist );
        }
        if ( iTop_2 >= -rcSearchRect.iNegVerLimit ) // check half top
        {
          if ( iLeft_2 >= -rcSearchRect.iNegHorLimit ) // check half left
          {
            xTZCheckPoint( rcStrukt, iLeft_2, iTop_2, 1, (iDist>>1) );
          }
          if ( iRight_2 <= rcSearchRect.iPosHorLimit ) // check half right
          {
            xTZCheckPoint( rcStrukt, iRight_2, iTop_2, 3, (iDist>>1) );
          }
        } // check half top
        if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check left
        {
          xTZCheckPoint( rcStrukt, iLeft, iStartY, 4, iDist );
        }
        if ( iRight <= rcSearchRect.iPosHorLimit ) // check right
        {
          xTZCheckPoint( rcStrukt, iRight, iStartY, 5, iDist );
        }
        if ( iBottom_2 <= rcSearchRect.iPosVerLimit ) // check half bottom
        {
          if ( iLeft_2 >= -rcSearchRect.iNegHorLimit ) // check half left
          {
            xTZCheckPoint( rcStrukt, iLeft_2, iBottom_2, 6, (iDist>>1) );
          }
          if ( iRight_2 <= rcSearchRect.iPosHorLimit ) // check half right
          {
            xTZCheckPoint( rcStrukt, iRight_2, iBottom_2, 8, (iDist>>1) );
          }
        } // check half bottom
        if ( iBottom <= rcSearchRect.iPosVerLimit ) // check bottom
        {
          xTZCheckPoint( rcStrukt, iStartX, iBottom, 7, iDist );
        }
      } // check border
    }
    else // iDist <= 8
    {
      if ( iTop >= -rcSearchRect.iNegVerLimit && iLeft   >= -rcSearchRect.iNegHorLimit &&
          iRight <=  rcSearchRect.iPosHorLimit && iBottom <= rcSearchRect.iPosVerLimit ) // check border
      {
        xTZCheckPoint( rcStrukt, iStartX, iTop,    0, iDist );
        xTZCheckPoint( rcStrukt, iLeft,   iStartY, 0, iDist );
        xTZCheckPoint( rcStrukt, iRight,  iStartY, 0, iDist );
        xTZCheckPoint( rcStrukt, iStartX, iBottom, 0, iDist );
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);
          xTZCheckPoint( rcStrukt, iPosXL, iPosYT, 0, iDist );
          xTZCheckPoint( rcStrukt, iPosXR, iPosYT, 0, iDist );
          xTZCheckPoint( rcStrukt, iPosXL, iPosYB, 0, iDist );
          xTZCheckPoint( rcStrukt, iPosXR, iPosYB, 0, iDist );
        }
      }
      else // check border
      {
        if ( iTop >= -rcSearchRect.iNegVerLimit ) // check top
        {
          xTZCheckPoint( rcStrukt, iStartX, iTop, 0, iDist );
        }
        if ( iLeft >= -rcSearchRect.iNegHorLimit ) // check left
        {
          xTZCheckPoint( rcStrukt, iLeft, iStartY, 0, iDist );
        }
        if ( iRight <= rcSearchRect.iPosHorLimit ) // check right
        {
          xTZCheckPoint( rcStrukt, iRight, iStartY, 0, iDist );
        }
        if ( iBottom <= rcSearchRect.iPosVerLimit ) // check bottom
        {
          xTZCheckPoint( rcStrukt, iStartX, iBottom, 0, iDist );
        }
        for ( Int index = 1; index < 4; index++ )
        {
          Int iPosYT = iTop    + ((iDist>>2) * index);
          Int iPosYB = iBottom - ((iDist>>2) * index);
          Int iPosXL = iStartX - ((iDist>>2) * index);
          Int iPosXR = iStartX + ((iDist>>2) * index);

          if ( iPosYT >= -rcSearchRect.iNegVerLimit ) // check top
          {
            if ( iPosXL >= -rcSearchRect.iNegHorLimit ) // check left
            {
              xTZCheckPoint( rcStrukt, iPosXL, iPosYT, 0, iDist );
            }
            if ( iPosXR <= rcSearchRect.iPosHorLimit ) // check right
            {
              xTZCheckPoint( rcStrukt, iPosXR, iPosYT, 0, iDist );
            }
          } // check top
          if ( iPosYB <= rcSearchRect.iPosVerLimit ) // check bottom
          {
            if ( iPosXL >= -rcSearchRect.iNegHorLimit ) // check left
            {
              xTZCheckPoint( rcStrukt, iPosXL, iPosYB, 0, iDist );
            }
            if ( iPosXR <= rcSearchRect.iPosHorLimit ) // check right
            {
              xTZCheckPoint( rcStrukt, iPosXR, iPosYB, 0, iDist );
            }
          } // check bottom
        } // for ...
      } // check border
    } // iDist <= 8
  } // iDist == 1

}

Void MotionEstimation::xTZSearch( YuvPicBuffer *pcPelData, Mv& rcMv, UInt& ruiSAD, Bool bEL, Int iSearchRange )
{
  TZ_SEARCH_CONFIGURATION

  // limit search range
  if( ! iSearchRange )
  {
    if( bEL )
      iSearchRange = m_cParams.getELSearchRange();
    else
      iSearchRange = m_cParams.getSearchRange();
  }
  rcMv.limitComponents(MotionCompensation::m_cMin, MotionCompensation::m_cMax );
  SearchRect cSearchRect;
  rcMv >>= 2;
  cSearchRect.init( iSearchRange, rcMv, MotionCompensation::m_cMin, MotionCompensation::m_cMax );

  // init TZSearchStrukt
  IntTZSearchStrukt cStrukt;
  cStrukt.iYStride    = pcPelData->getLStride();
  cStrukt.iCStride    = pcPelData->getCStride();
  m_cXDSS.iYStride    = cStrukt.iYStride;
  m_cXDSS.iCStride    = cStrukt.iCStride;
  cStrukt.pucYRef     = pcPelData->getLumBlk();
  cStrukt.pucURef     = pcPelData->getCbBlk ();
  cStrukt.pucVRef     = pcPelData->getCrBlk ();
  cStrukt.uiBestSad   = MSYS_UINT_MAX;

  // set rcMv as start point and as best point
  xTZCheckPoint( cStrukt, rcMv.getHor(), rcMv.getVer(), 0, 0 );


  if( bTestOtherPredictedMV )
  {
    for( UInt index = 0; index < m_acMvCandList.size(); index++ )
    {
      Mv cMv = m_acMvCandList[index];
      cMv.limitComponents( MotionCompensation::m_cMin, MotionCompensation::m_cMax );
      cMv >>= 2;
      xTZCheckPoint( cStrukt, cMv.getHor(), cMv.getVer(), 0, 0 );
    }
  }

  // test whether zerovektor is a better start point than the current rcMv
  if( bTestZeroVector )
  {
    xTZCheckPoint( cStrukt, 0, 0, 0, 0 );
  }

  // start search                     // ucPointNr
  Int  iDist       = 0;               //   1 2 3
  Int  iStartX     = cStrukt.iBestX;  //   4 0 5
  Int  iStartY     = cStrukt.iBestY;  //   6 7 8

  // fist search
  for( iDist = 1; iDist <= iSearchRange; iDist*=2 )
  {
    if( bFirstSearchDiamond == 1 )
    {
      xTZ8PointDiamondSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
    }
    else
    {
      xTZ8PointSquareSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
    }
    if( bFirstSearchStop && (cStrukt.uiBestRound >= uiFirstSearchRounds) ) // stop criterion
    {
      break;
    }
  }

  // test whether zerovektor is a better start point than current rcMv
  if( bTestZeroVectorStar && ((cStrukt.iBestX != 0) || (cStrukt.iBestY != 0)) )
  {
    xTZCheckPoint( cStrukt, 0, 0, 0, 0 );
    if( (cStrukt.iBestX == 0) && (cStrukt.iBestY == 0) )
    {
      // test his neighborhood
      for( iDist = 1; iDist <= iSearchRange; iDist*=2 )
      {
        xTZ8PointDiamondSearch( cStrukt, cSearchRect, 0, 0, iDist );
        if( bTestZeroVectorStop && (cStrukt.uiBestRound > 0) ) // stop criterion
        {
          break;
        }
      }
    }
  }

  // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
  if( cStrukt.uiBestDistance == 1 )
  {
    cStrukt.uiBestDistance = 0;
    xTZ2PointSearch( cStrukt, cSearchRect );
  }

  // raster search if distance is to big
  if( bEnableRasterSearch && ( (cStrukt.uiBestDistance > iRaster) || bAlwaysRasterSearch ) )
  {
    cStrukt.uiBestDistance = iRaster;
    for( iStartY = -cSearchRect.iNegVerLimit; iStartY <= cSearchRect.iPosVerLimit; iStartY += iRaster )
    {
      for( iStartX = -cSearchRect.iNegHorLimit; iStartX <= cSearchRect.iPosHorLimit; iStartX += iRaster )
      {
        xTZCheckPoint( cStrukt, iStartX, iStartY, 0, iRaster );
      }
    }
  }

  // raster refinement
  if( bRasterRefinementEnable && cStrukt.uiBestDistance > 0 )
  {
    while( cStrukt.uiBestDistance > 0 )
    {
      iStartX = cStrukt.iBestX;
      iStartY = cStrukt.iBestY;
      if( cStrukt.uiBestDistance > 1 )
      {
        iDist   = cStrukt.uiBestDistance >>= 1;
        if( bRasterRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
        }
      }
      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if( cStrukt.uiBestDistance == 1 )
      {
        cStrukt.uiBestDistance = 0;
        if( cStrukt.ucPointNr != 0 )
        {
          xTZ2PointSearch( cStrukt, cSearchRect );
        }
      }
    }
  }

  // star refinement
  if( bStarRefinementEnable && cStrukt.uiBestDistance > 0 )
  {
    while( cStrukt.uiBestDistance > 0 )
    {
      iStartX                 = cStrukt.iBestX;
      iStartY                 = cStrukt.iBestY;
      cStrukt.uiBestDistance  = 0;
      cStrukt.ucPointNr       = 0;
      for( iDist = 1; iDist < iSearchRange + 1; iDist*=2 )
      {
        if( bStarRefinementDiamond == 1 )
        {
          xTZ8PointDiamondSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
        }
        else
        {
          xTZ8PointSquareSearch( cStrukt, cSearchRect, iStartX, iStartY, iDist );
        }
        if( bStarRefinementStop && (cStrukt.uiBestRound >= uiStarRefinementRounds) ) // stop criterion
        {
          break;
        }
      }
      // calculate only 2 missing points instead 8 points if cStrukt.uiBestDistance == 1
      if( cStrukt.uiBestDistance == 1 )
      {
        cStrukt.uiBestDistance = 0;
        if( cStrukt.ucPointNr != 0 )
        {
          xTZ2PointSearch( cStrukt, cSearchRect );
        }
      }
    }
  }

  // write out best match
  ruiSAD = cStrukt.uiBestSad - MotionEstimationCost::xGetCost( cStrukt.iBestX, cStrukt.iBestY);
  rcMv.setHor( cStrukt.iBestX );
  rcMv.setVer( cStrukt.iBestY );

  // test for errors in debug mode
  DO_DBG( m_cXDSS.pYSearch = cStrukt.pucYRef +  cStrukt.iBestY     * cStrukt.iYStride +  cStrukt.iBestX );
  DO_DBG( m_cXDSS.pUSearch = cStrukt.pucURef + (cStrukt.iBestY>>1) * cStrukt.iCStride + (cStrukt.iBestX>>1) );
  DO_DBG( m_cXDSS.pVSearch = cStrukt.pucVRef + (cStrukt.iBestY>>1) * cStrukt.iCStride + (cStrukt.iBestX>>1) );
  AOF_DBG( ruiSAD == ( m_cXDSS.Func( &m_cXDSS ) ) );
}


H264AVC_NAMESPACE_END







