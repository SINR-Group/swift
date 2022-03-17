
#include "H264AVCEncoderLib.h"
#include "MbEncoder.h"
#include "SliceEncoder.h"
#include "MbCoder.h"
#include "CodingParameter.h"
#include "RecPicBuffer.h"
#include "H264AVCCommonLib/PocCalculator.h"
#include "H264AVCCommonLib/Transform.h"

#include "H264AVCCommonLib/CFMO.h"

// JVT-W043 {
#include "RateCtlBase.h"
#include "RateCtlQuadratic.h"
// JVT-W043 }

H264AVC_NAMESPACE_BEGIN


SliceEncoder::SliceEncoder():
  m_pcMbEncoder       ( NULL ),
  m_pcMbCoder         ( NULL ),
  m_pcControlMng      ( NULL ),
  m_pcCodingParameter ( NULL ),
  m_pcPocCalculator   ( NULL ),
  m_bInitDone         ( false ),
  m_uiFrameCount      ( 0 ),
  m_eSliceType        ( I_SLICE ),
  m_bTraceEnable      ( true ),
  m_pcTransform       ( NULL )
{
}


SliceEncoder::~SliceEncoder()
{
}

ErrVal SliceEncoder::create( SliceEncoder*& rpcSliceEncoder )
{
  rpcSliceEncoder = new SliceEncoder;

  ROT( NULL == rpcSliceEncoder );

  return Err::m_nOK;
}


ErrVal SliceEncoder::destroy()
{
  delete this;
  return Err::m_nOK;
}



ErrVal SliceEncoder::init( MbEncoder* pcMbEncoder,
                           MbCoder* pcMbCoder,
                           ControlMngIf* pcControlMng,
                           CodingParameter* pcCodingParameter,
                           PocCalculator* pcPocCalculator,
                           Transform* pcTransform)
{
  ROT( m_bInitDone );
  ROT( NULL == pcMbEncoder );
  ROT( NULL == pcMbCoder );
  ROT( NULL == pcControlMng );
  ROT( NULL == pcPocCalculator );
  ROT( NULL == pcTransform );

  m_pcTransform = pcTransform;
  m_pcMbEncoder = pcMbEncoder;
  m_pcMbCoder = pcMbCoder;
  m_pcControlMng = pcControlMng;
  m_pcCodingParameter = pcCodingParameter;
  m_pcPocCalculator = pcPocCalculator;


  m_uiFrameCount = 0;
  m_eSliceType =  I_SLICE;
  m_bTraceEnable = true;

  m_bInitDone = true;
  return Err::m_nOK;
}


ErrVal SliceEncoder::uninit()
{
  ROF( m_bInitDone );
  m_pcMbEncoder =  NULL;
  m_pcMbCoder =  NULL;
  m_pcControlMng =  NULL;
  m_bInitDone = false;

  m_uiFrameCount = 0;
  m_eSliceType =  I_SLICE;
  m_bTraceEnable = false;
  return Err::m_nOK;
}


ErrVal
SliceEncoder::encodeSliceSVC( ControlData&  rcControlData,        // control data
                              Frame&        rcOrgFrame,           // original frame
                              Frame&        rcFrame,              // reconstructed frame
                              Frame*        pcResidualFrameLF,    // reconstructed residual for loop filter
                              Frame*        pcResidualFrameILPred,// reconstructed residual for inter-layer prediction
                              Frame*        pcPredFrame,          // prediction signal
                              PicType       ePicType,             // picture type
                              UInt          uiNumMaxIter,         // maximum number of iteration for bi-predictive search
                              UInt          uiIterSearchRange,    // search range for iterative search
                              Bool          bBiPred8x8Disable,    // if bi-prediction for blocks smaller than 8x8 is allowed
                              Bool          bMCBlks8x8Disable,    // if blocks smaller than 8x8 are disabled
                              UInt          uiMaxDeltaQp,         // maximum delta QP
                              UInt&         ruiBits               // size of coded data
                              )
{
  ROF( m_bInitDone );

	SliceHeader&    rcSliceHeader         = *rcControlData.getSliceHeader           ( ePicType );
  MbDataCtrl*     pcMbDataCtrl          =  rcControlData.getMbDataCtrl            ();
  MbDataCtrl*     pcMbDataCtrl0L1       =  rcControlData.getMbDataCtrl0L1         ();
  Frame*          pcBaseIntraRecFrame   =  rcControlData.getBaseLayerRec          ();
  Frame*          pcBaseResidualFrame   =  rcControlData.getBaseLayerSbb          ();
  MbDataCtrl*     pcBaseMbDataCtrl      =  rcControlData.getBaseLayerCtrl         ();
  Double          dLambda               =  rcControlData.getLambda                ();
  RefListStruct&  rcRefListStruct       =  rcControlData.getRefListStruct         ();
  UInt            uiMbAddress           =  rcSliceHeader.getFirstMbInSlice        ();
  UInt            uiLastMbAddress       =  rcSliceHeader.getLastMbInSlice         ();
  UInt            uiMaxMvPerMb          =  rcSliceHeader.getSPS().getMaxMVsPer2Mb () >> 1; // hard limit (doesn't take into account last macroblock)
  UInt            uiNumMbsCoded         =  0;
  UInt            uiDeltaQp             =  0;

  //===== get field buffers if required =====
  if( ePicType!=FRAME )
  {
    RNOK( rcOrgFrame.addFieldBuffer( ePicType ) );
    RNOK( rcFrame   .addFieldBuffer( ePicType ) );
    if( pcResidualFrameLF     ) RNOK( pcResidualFrameLF     ->addFieldBuffer( ePicType ) );
    if( pcResidualFrameILPred ) RNOK( pcResidualFrameILPred ->addFieldBuffer( ePicType ) );
    if( pcPredFrame           ) RNOK( pcPredFrame           ->addFieldBuffer( ePicType ) );
    if( pcBaseIntraRecFrame   ) RNOK( pcBaseIntraRecFrame   ->addFieldBuffer( ePicType ) );
    if( pcBaseResidualFrame   ) RNOK( pcBaseResidualFrame   ->addFieldBuffer( ePicType ) );
  }
  Frame&  rcOrgPic            = *rcOrgFrame.getPic( ePicType );
  Frame&  rcPic               = *rcFrame   .getPic( ePicType );
  Frame*  pcResidualPicLF     = ( pcResidualFrameLF     ? pcResidualFrameLF     ->getPic( ePicType ) : 0 );
  Frame*  pcResidualPicILPred = ( pcResidualFrameILPred ? pcResidualFrameILPred ->getPic( ePicType ) : 0 );
  Frame*  pcPredPic           = ( pcPredFrame           ? pcPredFrame           ->getPic( ePicType ) : 0 );
  Frame*  pcBaseIntraRecPic   = ( pcBaseIntraRecFrame   ? pcBaseIntraRecFrame   ->getPic( ePicType ) : 0 );
  Frame*  pcBaseResidualPic   = ( pcBaseResidualFrame   ? pcBaseResidualFrame   ->getPic( ePicType ) : 0 );


  //====== initialization ======
  RNOK( pcMbDataCtrl      ->initSlice         ( rcSliceHeader, ENCODE_PROCESS, false, pcMbDataCtrl0L1 ) );
  if( pcBaseMbDataCtrl )
  {
    RNOK( pcBaseMbDataCtrl->initSlice         ( rcSliceHeader, PRE_PROCESS,    false, NULL ) );
  }
  RNOK( m_pcControlMng    ->initSliceForCoding( rcSliceHeader ) );
  UInt  uiBitsStart = m_pcMbCoder->getBitCount();


  //=====   M A I N   L O O P   O V E R   M A C R O B L O C K S   =====
  for( m_pcMbCoder->bSliceCodedDone = false; uiMbAddress <= uiLastMbAddress; ) //--ICU/ETRI FMO Implementation
  {
    //===== rate control initialization =====
    if( bRateControlEnable && !pcJSVMParams->m_uiLayerId )
    {
      pcGenericRC->m_pcJSVMParams->number             = pcGenericRC->m_pcJSVMParams->current_frame_number;
      pcJSVMParams->CurrGopLevel                      = pcGenericRC->getCurrGopLevel( pcGenericRC->m_pcJSVMParams->number );
      pcGenericRC->m_pcJSVMParams->nal_reference_idc  = ( pcJSVMParams->CurrGopLevel == 0 ? 0 : 1 );
      pcGenericRC->m_pcJSVMParams->current_mb_nr      = uiMbAddress;
      if( pcGenericRC->m_pcJSVMParams->basicunit == pcGenericRC->m_pcJSVMParams->FrameSizeInMbs )
      { // qp remains unchanged
      }
      else if( pcGenericRC->m_pcJSVMParams->type == P_SLICE )
      { // basic unit layer rate control (P slice)
        if( pcGenericRC->m_pcJSVMParams->NumberofCodedMacroBlocks > 0 &&
           (pcGenericRC->m_pcJSVMParams->NumberofCodedMacroBlocks % pcGenericRC->m_pcJSVMParams->BasicUnit) == 0 )
        { // frame coding
          if( pcGenericRC->m_pcJSVMParams->frame_mbs_only_flag )
          {
            pcQuadraticRC->updateRCModel();
            pcGenericRC->m_pcJSVMParams->qp = pcQuadraticRC->updateQPRC2( pcGenericRC->m_iTopFieldFlag );
          }
        }
      }
      else if ( pcGenericRC->m_pcJSVMParams->RCUpdateMode == RC_MODE_1 )
      { // basic unit layer rate control (I slice)
        if( pcGenericRC->m_pcJSVMParams->NumberofCodedMacroBlocks > 0 &&
           (pcGenericRC->m_pcJSVMParams->NumberofCodedMacroBlocks % pcGenericRC->m_pcJSVMParams->BasicUnit) == 0 ) 
        { // frame coding
          if( pcGenericRC->m_pcJSVMParams->frame_mbs_only_flag ) 
          {
            pcQuadraticRC->updateRCModel();
            if ( pcJSVMParams->m_uiIntraPeriod != 1 )
              pcGenericRC->m_pcJSVMParams->qp = pcQuadraticRC->updateQPRC2(pcGenericRC->m_iTopFieldFlag);
            else
              pcGenericRC->m_pcJSVMParams->qp = pcQuadraticRC->updateQPRC1(pcGenericRC->m_iTopFieldFlag);
          }
        }
      }
    }


    //===== macroblock initialization =====
    MbDataAccess* pcMbDataAccess     = NULL;
    MbDataAccess* pcMbDataAccessBase = NULL;
    Double        dCost              = 0;
    UInt          uiMbY, uiMbX;
    rcSliceHeader.getMbPositionFromAddress        (                     uiMbY, uiMbX, uiMbAddress );
    RNOK( pcMbDataCtrl      ->initMb              ( pcMbDataAccess,     uiMbY, uiMbX ) );
    if  ( pcBaseMbDataCtrl )
    {
      RNOK( pcBaseMbDataCtrl->initMb              ( pcMbDataAccessBase, uiMbY, uiMbX ) );
  	}
    RNOK( m_pcControlMng    ->initMbForCoding     ( *pcMbDataAccess,    uiMbY, uiMbX, false, false  ) );
    pcMbDataAccess          ->setMbDataAccessBase ( pcMbDataAccessBase );
    if  ( ! rcSliceHeader.getNoInterLayerPredFlag () )
    {
      m_pcMbEncoder->setBaseModeAllowedFlag       ( m_apabBaseModeFlagAllowedArrays[ ePicType == FRAME ? 0 : 1 ][ uiMbAddress ] );
    }
    if  ( bRateControlEnable && !pcJSVMParams->m_uiLayerId )
    {
      pcMbDataAccess->getMbData().setQp           ( pcGenericRC->m_pcJSVMParams->qp );
      uiDeltaQp = 0;
    }
    else
    {
      pcMbDataAccess->getMbData().setQp           ( rcSliceHeader.getSliceQp() );
      uiDeltaQp = ( rcSliceHeader.getTemporalId() == 0 ? 0 : uiMaxDeltaQp ); // old JSVM behaviour
    }


    //===== determine macroblock data =====
    RNOK( m_pcMbEncoder->encodeMacroblockSVC( *pcMbDataAccess, pcMbDataAccessBase, 
                                              rcOrgPic, rcPic, pcResidualPicLF, pcResidualPicILPred, pcPredPic, pcBaseIntraRecPic, pcBaseResidualPic, rcRefListStruct,
                                              uiMaxMvPerMb, uiNumMaxIter, uiIterSearchRange, bBiPred8x8Disable, bMCBlks8x8Disable, 
                                              true, uiDeltaQp, dLambda, dCost ) );


    //===== check for end of slice and code data into NAL unit ====
    uiNumMbsCoded++;
		if( m_uiSliceMode == 1 ) //fixed slice size in number of MBs
		{
			if( uiNumMbsCoded >= m_uiSliceArgument ) // this slice is done
			{
				RNOK( m_pcMbCoder->encode( *pcMbDataAccess, pcMbDataAccessBase, true, true ) );
				rcControlData.m_uiCurrentFirstMB = rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress );
				rcSliceHeader.setNumMbsInSlice( uiNumMbsCoded );
				if( rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress ) == -1 )
        {
          rcControlData.m_bSliceGroupAllCoded = true;
        }
				break;
			}
			else
			{
				RNOK( m_pcMbCoder->encode( *pcMbDataAccess, pcMbDataAccessBase, ( uiMbAddress == uiLastMbAddress ), true ) );
			}
		}
		else if( m_uiSliceMode == 2 ) //fixed slice size in number of bytes
		{
			RNOK( m_pcMbCoder->encode( *pcMbDataAccess, pcMbDataAccessBase, ( uiMbAddress == uiLastMbAddress ), true ) );
			if( ( rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress ) == -1 ) && ( !m_pcMbCoder->bSliceCodedDone ) )
      {
        rcControlData.m_bSliceGroupAllCoded = true;
      }
			if( m_pcMbCoder->bSliceCodedDone )
			{
				rcControlData.m_uiCurrentFirstMB = uiMbAddress;
				rcSliceHeader.setNumMbsInSlice( uiNumMbsCoded - 1 );
				//set SliceId equal to zero because this MBData belong to the next slice
        UInt uiDummyX = 0, uiDummyY = 0, uiMbIdx = 0;
        rcSliceHeader.getMbPosAndIndexFromAddress( uiDummyX, uiDummyY, uiMbIdx, rcControlData.m_uiCurrentFirstMB );
        pcMbDataCtrl->xGetMbData( uiMbIdx )->setSliceId( 0 );
				break;
			}
		}
		else // no slices (one slice per slice group)
		{
			RNOK( m_pcMbCoder->encode( *pcMbDataAccess, pcMbDataAccessBase, ( uiMbAddress == uiLastMbAddress ), true ) );
		}

    
    //===== update rate control =====
    if( bRateControlEnable && !pcJSVMParams->m_uiLayerId )
    {
      pcGenericRC->m_iNumberofHeaderBits           += pcGenericRC->m_iRCHeaderBits;
      pcGenericRC->m_iNumberofBasicUnitHeaderBits  += pcGenericRC->m_iRCHeaderBits;
      pcGenericRC->m_iNumberofTextureBits          += pcGenericRC->m_iRCTextureBits;
      pcGenericRC->m_iNumberofBasicUnitTextureBits += pcGenericRC->m_iRCTextureBits;
      pcGenericRC->m_pcJSVMParams->NumberofCodedMacroBlocks++;
    }
		
    
    //===== update macroblock address and check for end of slice group
    uiMbAddress = rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress );
		if( uiMbAddress == -1 )
		{
			rcControlData.m_bSliceGroupAllCoded = true;
			break;
		}
  }


  //===== remove field buffers if required =====
  if( ePicType!=FRAME )
  {
    RNOK( rcOrgFrame.removeFieldBuffer( ePicType ) );
    RNOK( rcFrame   .removeFieldBuffer( ePicType ) );
    if( pcResidualFrameLF     ) RNOK( pcResidualFrameLF     ->removeFieldBuffer( ePicType ) );
    if( pcResidualFrameILPred ) RNOK( pcResidualFrameILPred ->removeFieldBuffer( ePicType ) );
    if( pcPredFrame           ) RNOK( pcPredFrame           ->removeFieldBuffer( ePicType ) );
    if( pcBaseIntraRecFrame   ) RNOK( pcBaseIntraRecFrame   ->removeFieldBuffer( ePicType ) );
    if( pcBaseResidualFrame   ) RNOK( pcBaseResidualFrame   ->removeFieldBuffer( ePicType ) );
  }


  //===== update bits =====
  ruiBits += m_pcMbCoder->getBitCount() - uiBitsStart;
  return Err::m_nOK;
}


ErrVal
SliceEncoder::encodeMbAffSliceSVC( ControlData&  rcControlData,        // control data
                                   Frame&        rcOrgFrame,           // original frame
                                   Frame&        rcFrame,              // reconstructed frame
                                   Frame*        pcResidualFrameLF,    // reconstructed residual for loop filter
                                   Frame*        pcResidualFrameILPred,// reconstructed residual for inter-layer prediction
                                   Frame*        pcPredFrame,          // prediction signal
                                   UInt          uiNumMaxIter,         // maximum number of iteration for bi-predictive search
                                   UInt          uiIterSearchRange,    // search range for iterative search
                                   Bool          bBiPred8x8Disable,    // if bi-prediction for blocks smaller than 8x8 is allowed
                                   Bool          bMCBlks8x8Disable,    // if blocks smaller than 8x8 are disabled
                                   UInt          uiMaxDeltaQp,         // maximum delta QP
                                   UInt&         ruiBits               // size of coded data
                                   )
{
  ROF( m_bInitDone );

	SliceHeader&    rcSliceHeader         = *rcControlData.getSliceHeader           ();
  MbDataCtrl*     pcMbDataCtrl          =  rcControlData.getMbDataCtrl            ();
  MbDataCtrl*     pcMbDataCtrl0L1       =  rcControlData.getMbDataCtrl0L1         ();
  Frame*          pcBaseIntraRecFrame   =  rcControlData.getBaseLayerRec          ();
  Frame*          pcBaseResidualFrame   =  rcControlData.getBaseLayerSbb          ();
  MbDataCtrl*     pcBaseMbDataCtrl      =  rcControlData.getBaseLayerCtrl         ();
  MbDataCtrl*     pcBaseMbDataCtrlField =  rcControlData.getBaseLayerCtrlField    ();
  Double          dLambda               =  rcControlData.getLambda                ();
  RefListStruct&  rcRefListStruct       =  rcControlData.getRefListStruct         ();
  UInt            uiLastQP              =  rcSliceHeader.getSliceQp               ();
  UInt            uiMbAddress           =  rcSliceHeader.getFirstMbInSlice        ();
  UInt            uiLastMbAddress       =  rcSliceHeader.getLastMbInSlice         ();
  UInt            uiMaxMvPerMb          =  rcSliceHeader.getSPS().getMaxMVsPer2Mb () >> 1; // hard limit (doesn't take into account last macroblock)
  Bool            bSNR                  =  rcSliceHeader.getSCoeffResidualPredFlag() || rcSliceHeader.getTCoeffLevelPredictionFlag();
  UInt            uiNumMbsCoded         =  0;

  //===== set frame/field buffers =====
  Frame*  apcOrgPic           [4] = { 0, 0, 0, 0 };
  Frame*  apcPic              [4] = { 0, 0, 0, 0 };
  Frame*  apcResidualPicLF    [4] = { 0, 0, 0, 0 };
  Frame*  apcResidualPicILPred[4] = { 0, 0, 0, 0 };
  Frame*  apcPredPic          [4] = { 0, 0, 0, 0 };
  Frame*  apcBaseIntraRecPic  [4] = { 0, 0, 0, 0 };
  Frame*  apcBaseResidualPic  [4] = { 0, 0, 0, 0 };
  RNOK( gSetFrameFieldArrays( apcOrgPic,            &rcOrgFrame           ) );
  RNOK( gSetFrameFieldArrays( apcPic,               &rcFrame              ) );
  RNOK( gSetFrameFieldArrays( apcResidualPicLF,     pcResidualFrameLF     ) );
  RNOK( gSetFrameFieldArrays( apcResidualPicILPred, pcResidualFrameILPred ) );
  RNOK( gSetFrameFieldArrays( apcPredPic,           pcPredFrame           ) );
  RNOK( gSetFrameFieldArrays( apcBaseIntraRecPic,   pcBaseIntraRecFrame   ) );
  RNOK( gSetFrameFieldArrays( apcBaseResidualPic,   pcBaseResidualFrame   ) );
  MbDataBuffer  acFldMbData      [2];
  YuvMbBuffer   acFldMbRec       [2];
  YuvMbBuffer   acFldMbResLF     [2];
  YuvMbBuffer   acFldMbResILPred [2];
  YuvMbBuffer   acFldMbPredSignal[2];

  //====== set frame/field reference lists =====
  RefListStruct acRefListFieldStruct[2];
  RNOK( gSetFrameFieldLists( acRefListFieldStruct[0], acRefListFieldStruct[1], rcRefListStruct ) );

  //====== initialization ======
  RNOK( pcMbDataCtrl            ->initSlice         ( rcSliceHeader, ENCODE_PROCESS, false, pcMbDataCtrl0L1 ) );
  if( pcBaseMbDataCtrl )
  {
    RNOK( pcBaseMbDataCtrl      ->initSlice         ( rcSliceHeader, PRE_PROCESS,    false, NULL ) );
  }
  if( pcBaseMbDataCtrlField )
  {
    RNOK( pcBaseMbDataCtrlField ->initSlice         ( rcSliceHeader, PRE_PROCESS,    false, NULL ) );
  }
  RNOK( m_pcControlMng          ->initSliceForCoding( rcSliceHeader ) );
  UInt  uiBitsStart = m_pcMbCoder->getBitCount();


  //=====   M A I N   L O O P   O V E R   M A C R O B L O C K   P A I R S   =====
  for( m_pcMbCoder->bSliceCodedDone = false; uiMbAddress <= uiLastMbAddress; )
  {
    UInt    auiLastQpTest     [2] = { uiLastQP, uiLastQP };
    Bool    abSkipModeAllowed [4] = { true, true, true, true };
    Double  adMbPairCost      [2] = { 0.0, 0.0 };

    //===== determine macroblock data =====
    for( UInt uiMbTest = 0; uiMbTest < 4; uiMbTest++ )
    {
      //--- set basic parameters ---
      UInt            uiMbAddressMbAff    = ( uiMbTest % 2 ) + uiMbAddress;
      UInt            uiFieldMode         = ( uiMbTest < 2 ? 1 : 0 );
      RefListStruct&  rcRefListPicStruct  = ( uiFieldMode ? acRefListFieldStruct[ uiMbTest ] : rcRefListStruct );

      //--- macroblock initialization ---
      MbDataAccess* pcMbDataAccess      = NULL;
      MbDataAccess* pcMbDataAccessBase  = NULL;
      Double        dCost               = 0;
      UInt          uiMbY, uiMbX;
      rcSliceHeader.getMbPositionFromAddress        (                     uiMbY, uiMbX, uiMbAddressMbAff );
      RNOK( pcMbDataCtrl            ->initMb        ( pcMbDataAccess,     uiMbY, uiMbX ) );
      if  ( pcBaseMbDataCtrl && ( !uiFieldMode || bSNR ) )
      {
        RNOK( pcBaseMbDataCtrl      ->initMb        ( pcMbDataAccessBase, uiMbY, uiMbX ) );
  	  }
      if  ( pcBaseMbDataCtrlField && uiFieldMode && !bSNR )
      {
        RNOK( pcBaseMbDataCtrlField ->initMb        ( pcMbDataAccessBase, uiMbY, uiMbX ) );
      }
      RNOK( m_pcControlMng    ->initMbForCoding     ( *pcMbDataAccess,    uiMbY, uiMbX, true, uiFieldMode == 1  ) );
      pcMbDataAccess          ->setMbDataAccessBase ( pcMbDataAccessBase );
      pcMbDataAccess          ->setLastQp           ( auiLastQpTest[ uiFieldMode ] );
      if( uiMbTest == 0 )
      {
        abSkipModeAllowed[ 1 ]  = pcMbDataAccess->getDefaultFieldFlag();
        abSkipModeAllowed[ 3 ]  = ! abSkipModeAllowed[ 1 ];
      }
      if( ! rcSliceHeader.getNoInterLayerPredFlag () )
      {
        m_pcMbEncoder->setBaseModeAllowedFlag       ( m_apabBaseModeFlagAllowedArrays[ uiFieldMode ][ uiMbAddressMbAff ] );
      }

      //--- determine macroblock data ---
      pcMbDataAccess->getMbData().setQp( rcSliceHeader.getSliceQp() );
      UInt uiDeltaQp = ( rcSliceHeader.getTemporalId() == 0 ? 0 : uiMaxDeltaQp ); // old JSVM behaviour
      RNOK( m_pcMbEncoder->encodeMacroblockSVC( *pcMbDataAccess, pcMbDataAccessBase, 
                                                *apcOrgPic[ uiMbTest ], *apcPic[ uiMbTest ], apcResidualPicLF[ uiMbTest ], apcResidualPicILPred[ uiMbTest ], apcPredPic[ uiMbTest ], 
                                                apcBaseIntraRecPic[ uiMbTest ], apcBaseResidualPic[ uiMbTest ], rcRefListPicStruct,
                                                uiMaxMvPerMb, uiNumMaxIter, uiIterSearchRange, bBiPred8x8Disable, bMCBlks8x8Disable, 
                                                abSkipModeAllowed[ uiMbTest ], uiDeltaQp, dLambda, dCost ) );
      auiLastQpTest   [ uiFieldMode ]  = pcMbDataAccess->getMbData().getQp();
      if( adMbPairCost[ uiFieldMode ] != DOUBLE_MAX )
      {
        adMbPairCost  [ uiFieldMode ] += dCost;
      }

      //--- store field macroblock data ---
      if( uiFieldMode )
      {
        acFldMbData[uiMbTest].copy( pcMbDataAccess->getMbData() );
        if( apcPic              [uiMbTest] )  acFldMbRec       [uiMbTest].loadBuffer( apcPic              [uiMbTest]->getFullPelYuvBuffer() );
        if( apcResidualPicLF    [uiMbTest] )  acFldMbResLF     [uiMbTest].loadBuffer( apcResidualPicLF    [uiMbTest]->getFullPelYuvBuffer() );
        if( apcResidualPicILPred[uiMbTest] )  acFldMbResILPred [uiMbTest].loadBuffer( apcResidualPicILPred[uiMbTest]->getFullPelYuvBuffer() );
        if( apcPredPic          [uiMbTest] )  acFldMbPredSignal[uiMbTest].loadBuffer( apcPredPic          [uiMbTest]->getFullPelYuvBuffer() );
      }
    }

    
    //===== decide frame/field mode =====
#ifdef RANDOM_MBAFF
    Bool bFieldCodingMode = gBoolRandom();
#else
    Bool bFieldCodingMode = ( adMbPairCost[1] < adMbPairCost[0] );
#endif


    //===== code macroblock pair into NAL unit =====
    for( UInt uiMbIdx = 0; uiMbIdx < 2; uiMbIdx++ )
    {
      //--- macroblock initialization ---
      UInt          uiMbAddressMbAff    = uiMbAddress + uiMbIdx;
      MbDataAccess* pcMbDataAccess      = NULL;
      MbDataAccess* pcMbDataAccessBase  = NULL;
      UInt          uiMbY, uiMbX;
      rcSliceHeader.getMbPositionFromAddress        (                     uiMbY, uiMbX, uiMbAddressMbAff );
      RNOK( pcMbDataCtrl            ->initMb        ( pcMbDataAccess,     uiMbY, uiMbX ) );
      if  ( pcBaseMbDataCtrl && ( !bFieldCodingMode || bSNR ) )
      {
        RNOK( pcBaseMbDataCtrl      ->initMb        ( pcMbDataAccessBase, uiMbY, uiMbX ) );
      }
      if  ( pcBaseMbDataCtrlField && bFieldCodingMode && !bSNR )
      {
        RNOK( pcBaseMbDataCtrlField ->initMb        ( pcMbDataAccessBase, uiMbY, uiMbX ) );
      }
      RNOK( m_pcControlMng     ->initMbForCoding    ( *pcMbDataAccess,    uiMbY, uiMbX, true, bFieldCodingMode  ) );
      pcMbDataAccess           ->setMbDataAccessBase( pcMbDataAccessBase );
      pcMbDataAccess           ->setLastQp          ( uiLastQP );
      
      //--- restore field data ---
      if( bFieldCodingMode )
      {
        pcMbDataAccess->getMbData().copy( acFldMbData[uiMbIdx] );
        if( apcPic              [uiMbIdx] ) apcPic              [uiMbIdx]->getFullPelYuvBuffer()->loadBuffer( &acFldMbRec       [uiMbIdx] );
        if( apcResidualPicLF    [uiMbIdx] ) apcResidualPicLF    [uiMbIdx]->getFullPelYuvBuffer()->loadBuffer( &acFldMbResLF     [uiMbIdx] );
        if( apcResidualPicILPred[uiMbIdx] ) apcResidualPicILPred[uiMbIdx]->getFullPelYuvBuffer()->loadBuffer( &acFldMbResILPred [uiMbIdx] );
        if( apcPredPic          [uiMbIdx] ) apcPredPic          [uiMbIdx]->getFullPelYuvBuffer()->loadBuffer( &acFldMbPredSignal[uiMbIdx] );
      }

      //--- write data ---
      uiNumMbsCoded++;
      Bool bEndOfSlice = ( uiMbIdx == 1 ) && ( ( uiMbAddressMbAff == uiLastMbAddress ) || ( m_uiSliceMode == 1 && uiNumMbsCoded + 1 >= m_uiSliceArgument ) );
      RNOK( m_pcMbCoder->encode( *pcMbDataAccess, pcMbDataAccessBase, bEndOfSlice, ( uiMbIdx == 1 ) ) );

      //--- update last QP ---
      uiLastQP = pcMbDataAccess->getMbData().getQp();

      //--- check for end of slice ---
      if( m_uiSliceMode == 2 && m_pcMbCoder->bSliceCodedDone && uiMbIdx == 0 )
      {
        uiNumMbsCoded++;
        break;
      }
    }


    //===== check for end of slice ====
		if( m_uiSliceMode == 1 && uiNumMbsCoded + 1 >= m_uiSliceArgument ) //fixed slice size in number of MBs
  	{
	  	rcControlData.m_uiCurrentFirstMB = rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress + 1 );
			rcSliceHeader.setNumMbsInSlice( uiNumMbsCoded );
			if( rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress + 1 ) == -1 )
      {
        rcControlData.m_bSliceGroupAllCoded = true;
      }
		  break;
		}
		else if( m_uiSliceMode == 2 ) //fixed slice size in number of bytes
		{
			if( ( rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress + 1 ) == -1 ) && ( !m_pcMbCoder->bSliceCodedDone ) )
      {
        rcControlData.m_bSliceGroupAllCoded = true;
      }
			if( m_pcMbCoder->bSliceCodedDone )
			{
				rcControlData.m_uiCurrentFirstMB = uiMbAddress;
				rcSliceHeader.setNumMbsInSlice( uiNumMbsCoded - 2 );
				//set SliceId equal to zero because this MBData belong to the next slice
        UInt uiDummyX = 0, uiDummyY = 0, uiMbTopIdx = 0, uiMbBotIdx = 0;
        rcSliceHeader.getMbPosAndIndexFromAddress( uiDummyX, uiDummyY, uiMbTopIdx, rcControlData.m_uiCurrentFirstMB     );
        rcSliceHeader.getMbPosAndIndexFromAddress( uiDummyX, uiDummyY, uiMbBotIdx, rcControlData.m_uiCurrentFirstMB + 1 );
        pcMbDataCtrl->xGetMbData( uiMbTopIdx )->setSliceId( 0 );
        pcMbDataCtrl->xGetMbData( uiMbBotIdx )->setSliceId( 0 );
				break;
			}
		}

    
    //===== update macroblock address and check for end of slice group
    uiMbAddress = rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress + 1 );
		if( uiMbAddress == -1 )
		{
			rcControlData.m_bSliceGroupAllCoded = true;
			break;
		}
  }

  //===== remove field buffers =====
  RNOK( rcOrgFrame.removeFrameFieldBuffer() );
  RNOK( rcFrame   .removeFrameFieldBuffer() );
  if( pcResidualFrameLF     ) RNOK( pcResidualFrameLF     ->removeFrameFieldBuffer() );
  if( pcResidualFrameILPred ) RNOK( pcResidualFrameILPred ->removeFrameFieldBuffer() );
  if( pcPredFrame           ) RNOK( pcPredFrame           ->removeFrameFieldBuffer() );
  if( pcBaseIntraRecFrame   ) RNOK( pcBaseIntraRecFrame   ->removeFrameFieldBuffer() );
  if( pcBaseResidualFrame   ) RNOK( pcBaseResidualFrame   ->removeFrameFieldBuffer() );

  //===== update bits =====
  ruiBits += m_pcMbCoder->getBitCount() - uiBitsStart;
  return Err::m_nOK;
}





ErrVal
SliceEncoder::encodeSlice( SliceHeader&  rcSliceHeader,
                           Frame*        pcFrame,
                           MbDataCtrl*   pcMbDataCtrl,
                           RefListStruct& rcRefListStruct,
                           Bool          bMCBlks8x8Disable,
                           UInt          uiMbInRow,
                           Double        dlambda )
{
  ROF( pcFrame );
  ROF( pcMbDataCtrl );
  UInt uiMaxMvPerMb = rcSliceHeader.getSPS().getMaxMVsPer2Mb () >> 1; // hard limit (don't take into account last macroblock)

  //===== get co-located picture =====
  MbDataCtrl*   pcMbDataCtrlL1 = NULL;
  RefFrameList& rcList1        = rcRefListStruct.acRefFrameListME[1];
  if( rcList1.getActive() && rcList1.getEntry( 0 )->getRecPicBufUnit() )
  {
    pcMbDataCtrlL1 = rcList1.getEntry( 0 )->getRecPicBufUnit()->getMbDataCtrl();
  }
  ROT( rcSliceHeader.isBSlice() && ! pcMbDataCtrlL1 );

  //===== initialization =====
  RNOK( pcMbDataCtrl  ->initSlice         ( rcSliceHeader, ENCODE_PROCESS, false, pcMbDataCtrlL1 ) );
  RNOK( m_pcControlMng->initSliceForCoding( rcSliceHeader ) );



  //===== loop over macroblocks =====
  for( UInt uiMbAddress = rcSliceHeader.getFirstMbInSlice(); uiMbAddress <= rcSliceHeader.getLastMbInSlice(); uiMbAddress = rcSliceHeader.getFMO()->getNextMBNr( uiMbAddress ) )
  {
    UInt          uiMbY           = uiMbAddress / uiMbInRow;
    UInt          uiMbX           = uiMbAddress % uiMbInRow;
    MbDataAccess* pcMbDataAccess  = 0;

    RNOK( pcMbDataCtrl  ->initMb          (  pcMbDataAccess, uiMbY, uiMbX ) );
    RNOK( m_pcControlMng->initMbForCoding ( *pcMbDataAccess, uiMbY, uiMbX, false, false  ) );
    pcMbDataAccess->setMbDataAccessBase   ( NULL );

    RNOK( m_pcMbEncoder ->encodeMacroblock( *pcMbDataAccess,
                                             pcFrame,
                                             rcRefListStruct,
                                             uiMaxMvPerMb,
                                             bMCBlks8x8Disable,
                                             m_pcCodingParameter->getBiPred8x8Disable() > 0,
                                             m_pcCodingParameter->getMotionVectorSearchParams().getNumMaxIter(),
                                             m_pcCodingParameter->getMotionVectorSearchParams().getIterSearchRange(),
                                             dlambda ) );
    RNOK( m_pcMbCoder   ->encode          ( *pcMbDataAccess,
                                              NULL,
                                             ( uiMbAddress == rcSliceHeader.getLastMbInSlice() )
                                             ,true ) );
  }

  return Err::m_nOK;
}

//TMM_WP
ErrVal SliceEncoder::xInitDefaultWeights(Double *pdWeights, UInt uiLumaWeightDenom,
                                         UInt uiChromaWeightDenom)
{
    const Int iLumaWeight = 1 << uiLumaWeightDenom;
    const Int iChromaWeight = 1 << uiChromaWeightDenom;

    pdWeights[0] = iLumaWeight;
    pdWeights[1] = pdWeights[2] = iChromaWeight;

    return Err::m_nOK;
}


ErrVal SliceEncoder::xSetPredWeights( SliceHeader& rcSH,
                                      Frame* pOrgFrame,
                                      RefListStruct& rcRefListStruct )
{
  ROTRS( rcSH.isIntraSlice(), Err::m_nOK );

  RefFrameList& rcRefFrameList0 = rcRefListStruct.acRefFrameListRC[0];
  RefFrameList& rcRefFrameList1 = rcRefListStruct.acRefFrameListRC[1];
  const SampleWeightingParams& rcSWP = m_pcCodingParameter->getSampleWeightingParams(rcSH.getDependencyId());

  { // determine denoms
    const UInt uiLumaDenom = rcSWP.getLumaDenom();
    rcSH.setLumaLog2WeightDenom  ( ( uiLumaDenom == MSYS_UINT_MAX ) ? gIntRandom(0,7) : uiLumaDenom );

    const UInt uiChromaDenom = rcSWP.getChromaDenom();
    rcSH.setChromaLog2WeightDenom( ( uiChromaDenom == MSYS_UINT_MAX ) ? gIntRandom(0,7) : uiChromaDenom );
  }

  const Int iChromaScale = 1<<rcSH.getChromaLog2WeightDenom();
  const Int iLumaScale   = 1<<rcSH.getLumaLog2WeightDenom();

  m_pcControlMng->initSliceForWeighting(rcSH);

  if( rcSH.isBSlice() )
  {
      ROTRS( 1 != rcSH.getPPS().getWeightedBiPredIdc(), Err::m_nOK );
  }
  else
  {
    ROTRS( ! rcSH.getPPS().getWeightedPredFlag(), Err::m_nOK );
  }

  if( rcSH.isBSlice() )
  {
      RNOK( rcSH.getPredWeightTable(LIST_1).initDefaults( rcSH.getLumaLog2WeightDenom(), rcSH.getChromaLog2WeightDenom() ) );
  }
  RNOK( rcSH.getPredWeightTable(LIST_0).initDefaults( rcSH.getLumaLog2WeightDenom(), rcSH.getChromaLog2WeightDenom() ) );

  Double afFwWeight[MAX_REF_FRAMES][3];
  Double afBwWeight[MAX_REF_FRAMES][3];

  Double afFwOffsets[MAX_REF_FRAMES][3];
  Double afBwOffsets[MAX_REF_FRAMES][3];

  /* init arrays with default weights */
  for (UInt x = 0; x < MAX_REF_FRAMES; x++)
  {
      xInitDefaultWeights(afFwWeight[x], rcSH.getLumaLog2WeightDenom(), rcSH.getChromaLog2WeightDenom());
      xInitDefaultWeights(afBwWeight[x], rcSH.getLumaLog2WeightDenom(), rcSH.getChromaLog2WeightDenom());

      afFwOffsets[x][0] = afFwOffsets[x][1] = afFwOffsets[x][2] = 0;
      afBwOffsets[x][0] = afBwOffsets[x][1] = afBwOffsets[x][2] = 0;
  }

  if( rcSH.isBSlice() )
  {
      RNOK( m_pcMbEncoder->getPredWeights( rcSH, LIST_1, afBwWeight,
                                           pOrgFrame, rcRefFrameList1 ) );
      RNOK( rcSH.getPredWeightTable( LIST_1).setWeights( afBwWeight, iLumaScale, iChromaScale ) );
  }

  RNOK( m_pcMbEncoder->getPredWeights( rcSH, LIST_0, afFwWeight, pOrgFrame, rcRefFrameList0 ) );
  RNOK( rcSH.getPredWeightTable( LIST_0).setWeights( afFwWeight, iLumaScale, iChromaScale ) );

  return Err::m_nOK;
}
//TMM_WP


ErrVal
SliceEncoder::updatePictureResTransform( ControlData&     rcControlData,
                                         UInt          uiMbInRow )
{
  ROF( m_bInitDone );

  SliceHeader&  rcSliceHeader         = *rcControlData.getSliceHeader           ();
  MbDataCtrl*   pcMbDataCtrl          =  rcControlData.getMbDataCtrl            ();
  MbDataCtrl*   pcBaseLayerCtrl       =  rcControlData.getBaseLayerCtrl         ();
  UInt          uiMbAddress           =  0;
  UInt          uiLastMbAddress       =  rcSliceHeader.getMbInPic() - 1;

  //====== initialization ======
  RNOK( pcMbDataCtrl->initSlice( rcSliceHeader, DECODE_PROCESS, false, NULL ) );

  // Update the macroblock state
  // Must be done after the bit-stream has been constructed
  for( ; uiMbAddress <= uiLastMbAddress; )
  {
    UInt          uiMbY               = 0;
    UInt          uiMbX               = 0;
    MbDataAccess* pcMbDataAccess      = 0;
    MbDataAccess* pcMbDataAccessBase  = 0;

    rcSliceHeader.getMbPositionFromAddress( uiMbY, uiMbX, uiMbAddress );
    RNOK( pcMbDataCtrl      ->initMb          ( pcMbDataAccess,     uiMbY, uiMbX ) );
    if( pcBaseLayerCtrl )
    {
      RNOK( pcBaseLayerCtrl ->initMb          ( pcMbDataAccessBase, uiMbY, uiMbX ) );
      pcMbDataAccess->setMbDataAccessBase( pcMbDataAccessBase );
    }

    // modify QP values (as specified in G.8.1.5.1.2)
    if( pcMbDataAccess->getMbData().getMbCbp() == 0 && ( pcMbDataAccess->getMbData().getMbMode() == INTRA_BL || pcMbDataAccess->getMbData().getResidualPredFlag() ) )
    {
      pcMbDataAccess->getMbData().setQp   ( pcMbDataAccessBase->getMbData().getQp() );
      pcMbDataAccess->getMbData().setQp4LF( pcMbDataAccessBase->getMbData().getQp() );
    }

    // if cbp==0, tranform size is not transmitted, in this case inherit the transform size from base layer
    if( ( pcMbDataAccess->getMbData().getResidualPredFlag() && ! pcMbDataAccess->getMbData().isIntra() && ! pcMbDataAccessBase->getMbData().isIntra() ) || ( pcMbDataAccess->getMbData().getMbMode() == INTRA_BL ) )
    {
      if( ( pcMbDataAccess->getMbData().getMbCbp() & 0x0F ) == 0 )
      {
        pcMbDataAccess->getMbData().setTransformSize8x8( pcMbDataAccessBase->getMbData().isTransformSize8x8() );
      }
    }

    // set I_PCM mode (for deblocking) when CBP is 0, mbMode is I_BL, and base layer mbMode is I_PCM
    if( pcMbDataAccess->getMbData().getMbCbp() == 0 && pcMbDataAccess->getMbData().isIntraBL() && pcMbDataAccessBase->getMbData().isPCM() )
    {
      pcMbDataAccess->getMbData().setMbMode( MODE_PCM );
    }

    uiMbAddress++;
  }

  return Err::m_nOK;
}

ErrVal
SliceEncoder::updateBaseLayerResidual( ControlData&     rcControlData,
                                         UInt          uiMbInRow )
{
  ROF( m_bInitDone );

  SliceHeader&  rcSliceHeader         = *rcControlData.getSliceHeader           ();
  MbDataCtrl*   pcMbDataCtrl          =  rcControlData.getMbDataCtrl            ();
  MbDataCtrl*   pcBaseLayerCtrl       =  rcControlData.getBaseLayerCtrl         ();
  Frame*     pcBaseLayerSbb        =  rcControlData.getBaseLayerSbb         ();
  UInt          uiMbAddress           =  0;
  UInt          uiLastMbAddress       =  rcSliceHeader.getMbInPic() - 1;

  //====== initialization ======
  RNOK( pcMbDataCtrl->initSlice( rcSliceHeader, DECODE_PROCESS, false, NULL ) );

  for( ; uiMbAddress <= uiLastMbAddress; )
  {
    UInt          uiMbY               = uiMbAddress / uiMbInRow;
    UInt          uiMbX               = uiMbAddress % uiMbInRow;
    MbDataAccess* pcMbDataAccess      = 0;
    MbDataAccess* pcMbDataAccessBase  = 0;

    RNOK( pcMbDataCtrl      ->initMb          ( pcMbDataAccess,     uiMbY, uiMbX ) );
    if( pcBaseLayerCtrl )
    {
      RNOK( pcBaseLayerCtrl ->initMb          ( pcMbDataAccessBase, uiMbY, uiMbX ) );
      //pcMbDataAccess->setMbDataAccessBase( pcMbDataAccessBase );
    }

    // Update the state of the baselayer residual data -- it may be reused in subsequent layers - ASEGALL@SHARPLABS.COM
    if( !pcMbDataAccess->getMbData().getResidualPredFlag() )
    {
      if( pcBaseLayerSbb && ( pcMbDataAccess->getMbData().isIntra() || ! pcMbDataAccess->getMbData().getResidualPredFlag() ) )
      {
        YuvPicBuffer* pcBaseResidual = pcBaseLayerSbb->getFullPelYuvBuffer();

        pcBaseResidual->getBufferCtrl().initMb( uiMbY, uiMbX, false);
        pcBaseResidual->clearCurrMb();
      }
    }

    uiMbAddress++;
  }

  return Err::m_nOK;
}

// JVT-V035
ErrVal
SliceEncoder::updatePictureAVCRewrite( ControlData& rcControlData, UInt uiMbInRow )
{
  ROF( m_bInitDone );

  SliceHeader&  rcSliceHeader   = *rcControlData.getSliceHeader();
  FMO&          rcFMO           = *rcSliceHeader.getFMO();
  MbDataCtrl*   pcMbDataCtrl    = rcControlData.getMbDataCtrl();
  MbDataCtrl*   pcBaseLayerCtrl = rcControlData.getBaseLayerCtrl();

  //====== initialization ======
  RNOK( pcMbDataCtrl->initSlice( rcSliceHeader, DECODE_PROCESS, false, NULL ) );

  if( rcSliceHeader.getTCoeffLevelPredictionFlag() == true )
  {
	  // Update the macroblock state
	  // Must be done after the bit-stream has been constructed
    for( Int iSliceGroupId = rcFMO.getFirstSliceGroupId(); ! rcFMO.SliceGroupCompletelyCoded( iSliceGroupId ); iSliceGroupId = rcFMO.getNextSliceGroupId( iSliceGroupId ) )
    {
      UInt  uiFirstMbInSliceGroup = rcFMO.getFirstMBOfSliceGroup( iSliceGroupId );
      UInt  uiLastMbInSliceGroup  = rcFMO.getLastMBInSliceGroup ( iSliceGroupId );

      for( UInt uiMbAddress = uiFirstMbInSliceGroup; uiMbAddress <= uiLastMbInSliceGroup; uiMbAddress = rcFMO.getNextMBNr( uiMbAddress ) )
	    {
		    UInt          uiMbY               = 0;
		    UInt          uiMbX               = 0;
		    MbDataAccess* pcMbDataAccess      = 0;
		    MbDataAccess* pcMbDataAccessBase  = 0;

        rcSliceHeader.getMbPositionFromAddress( uiMbY, uiMbX, uiMbAddress );
		    RNOK( pcMbDataCtrl->initMb( pcMbDataAccess, uiMbY, uiMbX ) );

		    if( pcBaseLayerCtrl )
		    {
			    RNOK( pcBaseLayerCtrl ->initMb( pcMbDataAccessBase, uiMbY, uiMbX ) );
			    pcMbDataAccess->setMbDataAccessBase( pcMbDataAccessBase );
		    }

        // modify QP values (as specified in G.8.1.5.1.2)
        if( pcMbDataAccess->getMbData().getMbCbp() == 0 && ( pcMbDataAccess->getMbData().getMbMode() == INTRA_BL || pcMbDataAccess->getMbData().getResidualPredFlag() ) )
        {
          pcMbDataAccess->getMbData().setQp( pcMbDataAccessBase->getMbData().getQp() );
        }

		    if( pcMbDataAccess->isTCoeffPred() )
		    {
			    if( pcMbDataAccess->getMbData().getMbMode() == INTRA_BL )
			    {
				    // We're going to use the BL skip flag to correctly decode the intra prediction mode
				    AOT( pcMbDataAccess->getMbData().getBLSkipFlag() == false );

				    // Inherit the mode of the base block
				    pcMbDataAccess->getMbData().setMbMode( pcMbDataAccessBase->getMbData().getMbMode() );

				    // Inherit intra prediction modes
				    for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
					    pcMbDataAccess->getMbData().intraPredMode(cIdx) = pcMbDataAccessBase->getMbData().intraPredMode(cIdx);

				    pcMbDataAccess->getMbData().setChromaPredMode( pcMbDataAccessBase->getMbData().getChromaPredMode() );
			    }

  		    // The 8x8 transform flag is present in the bit-stream unless transform coefficients
			    // are not transmitted at the enhancement layer.  In this case, inherit the base layer
			    // transform type.  This makes intra predition work correctly, etc.
          if( ( pcMbDataAccess->getMbData().getMbCbp() & 0x0F ) == 0 )
          {
            pcMbDataAccess->getMbData().setTransformSize8x8( pcMbDataAccessBase->getMbData().isTransformSize8x8() );
          }

			    xAddTCoeffs2( *pcMbDataAccess, *pcMbDataAccessBase );
		    }

        if( pcMbDataAccess->getMbAddress() != uiFirstMbInSliceGroup &&
            pcMbDataAccess->getSH().getTCoeffLevelPredictionFlag() &&
           !pcMbDataAccess->getSH().getNoInterLayerPredFlag() &&
           !pcMbDataAccess->getMbData().isIntra16x16() &&
            pcMbDataAccess->getMbData().getMbExtCbp() == 0 )
        {
          pcMbDataAccess->getMbData().setQp4LF( pcMbDataAccess->getLastQp4LF() );
        }
        else
        {
          pcMbDataAccess->getMbData().setQp4LF( pcMbDataAccess->getMbData().getQp() );
        }
	    }
    }
  }

  return Err::m_nOK;
}

ErrVal
SliceEncoder::xAddTCoeffs2( MbDataAccess& rcMbDataAccess, MbDataAccess& rcMbDataAccessBase )
{
  if( rcMbDataAccess.getMbData().isPCM() )
  {
    TCoeff* pSrc = rcMbDataAccessBase.getMbTCoeffs().getTCoeffBuffer();
    TCoeff* pDes = rcMbDataAccess    .getMbTCoeffs().getTCoeffBuffer();
    for( UInt ui = 0; ui < 384; ui++, pSrc++, pDes++ )
    {
      ROT( pDes->getLevel() );
      pDes->setLevel( pSrc->getLevel() );
    }
    ROT( rcMbDataAccess.getMbData().getMbExtCbp() );
    return Err::m_nOK;
  }

	UInt uiBCBP = 0;
	UInt uiCoded = 0;
	Bool bCoded = false;
	Bool bChromaAC = false;
	Bool bChromaDC = false;

	// Add the luma coefficients and track the new BCBP
	if( rcMbDataAccess.getMbData().isTransformSize8x8() )
	{

		for( B8x8Idx c8x8Idx; c8x8Idx.isLegal(); c8x8Idx++ )
		{
			bCoded = false;

			m_pcTransform->addPrediction8x8Blk( rcMbDataAccess.getMbTCoeffs().get8x8( c8x8Idx ),
				rcMbDataAccessBase.getMbTCoeffs().get8x8( c8x8Idx ),
				rcMbDataAccess.getMbData().getQp(),
				rcMbDataAccessBase.getMbData().getQp(), bCoded );

			if( rcMbDataAccess.getMbData().isIntra16x16() )
				AOT(1);

			if( bCoded )
				uiBCBP |= (0x33 << c8x8Idx.b4x4());
		}
	}
	else
	{

		for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
		{
			uiCoded = 0;

			m_pcTransform->addPrediction4x4Blk( rcMbDataAccess.getMbTCoeffs().get( cIdx ),
				rcMbDataAccessBase.getMbTCoeffs().get( cIdx ),
				rcMbDataAccess.getMbData().getQp(),
				rcMbDataAccessBase.getMbData().getQp(), uiCoded );

			if( rcMbDataAccess.getMbData().isIntra16x16() )
			{
				if( *(rcMbDataAccess.getMbTCoeffs().get( cIdx )) )
					uiCoded--;
			}

			if( uiCoded )
				uiBCBP |= 1<<cIdx;
		}

		if( rcMbDataAccess.getMbData().isIntra16x16() )
		{
			uiBCBP = uiBCBP?((1<<16)-1):0;
		}
	}

	// Add the chroma coefficients and update the BCBP
  m_pcTransform->addPredictionChromaBlocks( rcMbDataAccess.getMbTCoeffs().get( CIdx(0) ),
                                            rcMbDataAccessBase.getMbTCoeffs().get( CIdx(0) ),
                                            rcMbDataAccess.getSH().getCbQp( rcMbDataAccess.getMbData().getQp() ),
                                            rcMbDataAccess.getSH().getBaseSliceHeader()->getCbQp( rcMbDataAccessBase.getMbData().getQp() ),
                                            bChromaDC, bChromaAC );
  m_pcTransform->addPredictionChromaBlocks( rcMbDataAccess.getMbTCoeffs().get( CIdx(4) ),
                                            rcMbDataAccessBase.getMbTCoeffs().get( CIdx(4) ),
                                            rcMbDataAccess.getSH().getCrQp( rcMbDataAccess.getMbData().getQp() ),
                                            rcMbDataAccess.getSH().getBaseSliceHeader()->getCrQp( rcMbDataAccessBase.getMbData().getQp() ),
                                            bChromaDC, bChromaAC );

	uiBCBP |= (bChromaAC?2:(bChromaDC?1:0))<<16;

	// Update the CBP
	rcMbDataAccess.getMbData().setAndConvertMbExtCbp( uiBCBP );

	// Update the Intra16x16 mode
	if( rcMbDataAccess.getMbData().isIntra16x16() )
	{
		UInt uiMbType = INTRA_4X4 + 1;
		UInt uiPredMode = rcMbDataAccess.getMbData().intraPredMode();
		UInt uiChromaCbp = uiBCBP>>16;
		Bool bACcoded = (uiBCBP && ((1<<16)-1));

		uiMbType += uiPredMode;
        uiMbType += ( bACcoded ) ? 12 : 0;
        uiMbType += uiChromaCbp << 2;

		rcMbDataAccess.getMbData().setMbMode( MbMode(uiMbType) );

		// Sanity checks
		if( rcMbDataAccess.getMbData().intraPredMode() != uiPredMode )
			AOT(1);
		if( rcMbDataAccess.getMbData().getCbpChroma16x16() != uiChromaCbp )
			AOT(1);
		if( rcMbDataAccess.getMbData().isAcCoded() != bACcoded )
			AOT(1);
	}

	return Err::m_nOK;



  UInt uiScaleFactor[6] = {8, 9, 10, 11, 13, 14};

  Quantizer cSrcQuantizer, cDstQuantizer;
  cSrcQuantizer.setQp( rcMbDataAccessBase, false );
  cDstQuantizer.setQp( rcMbDataAccess, false );


  // Process luma blocks
  const QpParameter&  cSrcLQp      = cSrcQuantizer.getLumaQp  ();
  const QpParameter&  cDstLQp      = cDstQuantizer.getLumaQp  ();

  QpParameter cScaleQp;
  cScaleQp.setQp( (cSrcLQp.per()-cDstLQp.per())*6+(cSrcLQp.rem()-cDstLQp.rem()), true );

  for( B4x4Idx cIdx; cIdx.isLegal(); cIdx++ )
  {
	  TCoeff *pcDst = rcMbDataAccess.getMbData().getMbTCoeffs().get( cIdx );
	  TCoeff *pcSrc = rcMbDataAccessBase.getMbData().getMbTCoeffs().get( cIdx );

	  for( UInt n=0; n<16; n++ )
	  {
		  TCoeff cCoeff;

		  cCoeff = pcSrc[n]<<cScaleQp.per();
		  cCoeff *= uiScaleFactor[cScaleQp.rem()];
		  cCoeff += ( cCoeff>0 ) ? 4 : -4;
		  cCoeff /= 8;

		  pcDst[n] += cCoeff;

	  }
  }

  // Process chroma blocks
  for( CPlaneIdx cPlaneIdx; cPlaneIdx.isLegal(); cPlaneIdx++ )
  {
    const QpParameter&  cSrcCQp = cSrcQuantizer.getChromaQp( cPlaneIdx );
    const QpParameter&  cDstCQp = cDstQuantizer.getChromaQp( cPlaneIdx );

    // Set scale factor
    cScaleQp.setQp( (cSrcCQp.per()*6+cSrcCQp.rem()) - (cDstCQp.per()*6+cDstCQp.rem()), true );

    for( CIdx cCIdx( cPlaneIdx ); cCIdx.isLegal( cPlaneIdx ); cCIdx++ )
    {
      TCoeff *pcDst = rcMbDataAccess    .getMbData().getMbTCoeffs().get( cCIdx );
      TCoeff *pcSrc = rcMbDataAccessBase.getMbData().getMbTCoeffs().get( cCIdx );

      for( UInt n=0; n<16; n++ )
      {
        TCoeff cCoeff;

        cCoeff = pcSrc[n]<<cScaleQp.per();
        cCoeff *= uiScaleFactor[cScaleQp.rem()];
        cCoeff += ( cCoeff>0 ) ? 4 : -4;
        cCoeff /= 8;

        pcDst[n] += cCoeff;
      }
    }
  }

  return Err::m_nOK;
}

H264AVC_NAMESPACE_END
