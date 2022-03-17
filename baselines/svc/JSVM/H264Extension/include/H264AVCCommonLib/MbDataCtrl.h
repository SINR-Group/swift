
#if !defined(AFX_MBDATACTRL_H__50D2B462_28AB_46CA_86AC_35502BD296BC__INCLUDED_)
#define AFX_MBDATACTRL_H__50D2B462_28AB_46CA_86AC_35502BD296BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


class MbDataCtrl;

class H264AVCCOMMONLIB_API PosCalcParam
{
public:
  PosCalcParam( ResizeParameters& rcResizeParameters );

public:
  Bool  m_bRSChangeFlag;
  Int   m_iBaseMbAff;
  Int   m_iCurrMbAff;
  Int   m_iBaseField;
  Int   m_iCurrField;
  Int   m_iBaseBotField;
  Int   m_iCurrBotField;
  Int   m_iRefW;
  Int   m_iRefH;
  Int   m_iScaledW;
  Int   m_iScaledH;
  Int   m_iOffsetX;
  Int   m_iOffsetY;
  Int   m_iShiftX;
  Int   m_iShiftY;
  Int   m_iScaleX;
  Int   m_iScaleY;
  Int   m_iAddX;
  Int   m_iAddY;
};


class H264AVCCOMMONLIB_API MvScaleParam
{
public:
  MvScaleParam( ResizeParameters& rcResizeParameters, RefFrameList* pcRefFrameList0, RefFrameList* pcRefFrameList1 );

public:
  Bool  m_bRSChangeFlag;
  Bool  m_bCropChangeFlag;
  Int   m_iCurrMbAff;
  Int   m_iBaseField;
  Int   m_iCurrField;
  Int   m_iRefW;
  Int   m_iRefH;
  Int   m_iScaledW;
  Int   m_iScaledH;
  Int   m_iOffsetX;
  Int   m_iOffsetY;
  Int   m_iScaleX;
  Int   m_iScaleY;
  Int   m_aiNumActive [2];
  Int   m_aaidOX      [2][33];
  Int   m_aaidOY      [2][33];
  Int   m_aaidSW      [2][33];
  Int   m_aaidSH      [2][33];
};

class H264AVCCOMMONLIB_API MotionUpsampling
{
public:
  MotionUpsampling  ( MbDataCtrl&       rcMbDataCtrlCurr,
                      SliceHeader&      rcSliceHeaderCurr,
                      ResizeParameters& rcResizeParameters,
                      MbDataCtrl&       rcMbDataCtrlBase,
                      RefFrameList*     pcRefFrameList0,
                      RefFrameList*     pcRefFrameList1,
                      Bool              bFieldMacroblocks,
                      Bool              bCheckResidualPred  = false,
                      Int               iMvThreshold        = 0 );
  ~MotionUpsampling ();

  ErrVal  resample  ( Int iMbXCurr, Int iMbYCurr );

private:
  ErrVal  xInitMb                     ( Int       iMbXCurr,
                                        Int       iMbYCurr );
  Bool    xIsInCropWindow             ();
  ErrVal  xGetRefLayerMb              ( Int       iXInsideCurrMb,     // subclause G.6.1
                                        Int       iYInsideCurrMb,
                                        Int&      riBaseMbIdx,
                                        Int&      riXInsideBaseMb,
                                        Int&      riYInsideBaseMb );
  ErrVal  xGetRefLayerPartIdc         ( Int       iXInsideCurrMb,     // subclause G.6.2
                                        Int       iYInsideCurrMb,
                                        Int&      riPartIdc );
  ErrVal  xSetPartIdcArray            ();                             // subclause G.8.6.1.1

  ErrVal  xGetInitialBaseRefIdxAndMv  ( Int       i4x4BlkX,           // reference index and motion vector scaling of subclause G.8.6.1.2
                                        Int       i4x4BlkY,
                                        ListIdx   eListIdx,
                                        Int       iPartIdc,
                                        Int&      riRefIdx,
                                        Mv&       rcMv );
  Int     xGetMinRefIdx               ( Int       iRefIdxA,           // function MinPositive() specified in subclause G.8.6.1.2
                                        Int       iRefIdxB );
  ErrVal  xGetRefIdxAndInitialMvPred  ( ListIdx   eListIdx );         // first part of subclause G.8.6.1.2

  Int     xMvDiff                     ( const Mv& rcMvA,              // function mvDiff specified in subclause G.8.6.1.2
                                        const Mv& rcMbB );
  ErrVal  xDeriveBlockModeAndUpdateMv ( Int       i8x8BlkIdx );       // second part of subclause G.8.6.1.2

  Bool    x8x8BlocksHaveSameMotion    ( ListIdx   eListIdx,
                                        Int       i8x8IdxA,
                                        Int       i8x8IdxB );         // compare motion parameters of 8x8 blocks
  ErrVal  xDeriveMbMode               ();                             // derive predicted macroblock mode
  ErrVal  xDeriveFwdBwd               ();                             // set forward/backward indication

  ErrVal  xSetInterIntraIdc           ();                             // set indications for intra-inter combination process
  ErrVal  xSetResPredSafeFlag         ();                             // set flag indicating whether residual prediction is "safe"

  ErrVal  xSetPredMbData              ();                             // store prediction data in MbData structure


private:
  //===== general parameters =====
  Bool              m_bCheckResidualPred;
  Bool              m_bDirect8x8Inference;
  Int               m_iMvThreshold;
  Bool              m_bCurrFieldMb;
  SliceType         m_eSliceType;
  Int               m_iRefLayerDQId;
  Int               m_iMaxListIdx;
  Bool              m_bSCoeffPred;
  Bool              m_bTCoeffPred;
  MbDataCtrl&       m_rcMbDataCtrlCurr;
  ResizeParameters& m_rcResizeParameters;
  PosCalcParam      m_cPosCalc;
  MvScaleParam      m_cMvScale;
  MbDataCtrl&       m_rcMbDataCtrlBase;
  Int               m_iMbX0CropFrm;
  Int               m_iMbY0CropFrm;
  Int               m_iMbX1CropFrm;
  Int               m_iMbY1CropFrm;
  //===== macroblock parameters =====
  Int               m_iMbXCurr;             // inside current picture (i.e. frame or field)
  Int               m_iMbYCurr;             // inside current picture (i.e. frame or field)
  //===== auxiliary arrays and variables =====
  Bool              m_bInCropWindow;        // set  in xInitMb
  Bool              m_bIntraBL;             // set  in xSetPartIdcArray
  Int               m_aaiPartIdc    [4][4]; // set  in xSetPartIdcArray
  Int               m_aaiRefIdxTemp [4][4]; // used in xGetRefIdxAndInitialMvPred
  Int               m_aaaiRefIdx [2][2][2]; // set  in xGetRefIdxAndInitialMvPred
  Mv                m_aaacMv     [2][4][4]; // set  in xGetRefIdxAndInitialMvPred, modified in xDeriveBlockModeAndUpdateMv
  BlkMode           m_aeBlkMode  [4];       // set  in xDeriveBlockModeAndUpdateMv
  MbMode            m_eMbMode;              // set  in xDeriveMbMode
  UInt              m_uiFwdBwd;             // set  in xDeriveFwdBwd
  Bool              m_aabBaseIntra  [2][2]; // set  in xSetInterIntraIdc
  Bool              m_bResPredSafe;         // set  in xInitMb, modified in xSetResPredSafeFlag
};



class H264AVCCOMMONLIB_API MbDataCtrl
{
public:
	MbDataCtrl();
  ~MbDataCtrl();

public:
  ErrVal getBoundaryMask      ( Int iMbY, Int iMbX, Bool& rbIntra, UInt& ruiMask, UInt uiCurrentSliceID ) const;
  ErrVal getBoundaryMask_MbAff( Int iMbY, Int iMbX, Bool& rbIntra, UInt& ruiMask, UInt uiCurrentSliceID ) const;

  ErrVal initMb( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, const Int iForceQp = -1 );
	ErrVal initMb( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, const Bool bFieldFlag, const Int iForceQp );
  ErrVal init( const SequenceParameterSet& rcSPS );

  ErrVal uninit();
  ErrVal reset();
  ErrVal resetData() { return xResetData(); }
  ErrVal initSliceLF( SliceHeader& rcSH, const MbStatus* apcMbStatus );
  ErrVal initSlice( SliceHeader& rcSH, ProcessingState eProcessingState, Bool bDecoder, MbDataCtrl* pcMbDataCtrl );
  ErrVal initUsedField(SliceHeader& rcSH, RefFrameList&           rcRefFrameList1); //TMM
  Bool isPicDone( const SliceHeader& rcSH );
  Bool isFrameDone( const SliceHeader& rcSH );
  UInt  getSize() { return m_uiSize; }

	SliceHeader* getSliceHeader()                             { return m_pcSliceHeader; }
	Void         setSliceHeader( SliceHeader* pcSliceHeader ) { m_pcSliceHeader = pcSliceHeader; }

  const DBFilterParameter&  getDBFPars( const MbDataAccess& rcMbDataAccess )  const
  {
    const DBFilterParameter* pcDBFPars = m_cDBFPBuffer.get( rcMbDataAccess.getMbData().getSliceId() );
    AOF( pcDBFPars );
    return *pcDBFPars;
  }

  const MbData& getMbData( UInt uiIndex )   const { AOT_DBG( uiIndex >= m_uiSize );  return m_pcMbData[ uiIndex ]; }
  const Bool isPicCodedField( )              const { return m_bPicCodedField; }

  MbData& getMbDataByIndex( UInt uiIndex )        { AOT_DBG( uiIndex >= m_uiSize );  return m_pcMbData[ uiIndex ]; }

  ErrVal clear() { return xResetData(); }

  MbData& getMbData( UInt uiMbX, UInt uiMbY )   { AOT_DBG( uiMbY*m_uiMbStride+uiMbX+m_uiMbOffset >= m_uiSize );  return m_pcMbData[uiMbY*m_uiMbStride+uiMbX+m_uiMbOffset]; }
//	TMM_EC {{
	MbData& getMbData( UInt uiMbX, UInt uiMbY ) const  { AOT_DBG( uiMbY*m_uiMbStride+uiMbX+m_uiMbOffset >= m_uiSize );  return m_pcMbData[uiMbY*m_uiMbStride+uiMbX+m_uiMbOffset]; }
//  TMM_EC }}

  ErrVal  copyMotion    ( MbDataCtrl&       rcMbDataCtrl,
                          PicType           ePicType = FRAME );
  ErrVal  upsampleMotion( SliceHeader*      pcSliceHeader,
                          ResizeParameters* pcResizeParameters,
                          MbDataCtrl*       pcBaseMbDataCtrl,
                          RefFrameList*     pcRefFrameList0,
                          RefFrameList*     pcRefFrameList1,
                          Bool              bFieldResampling,
                          Bool              bResidualPredCheck = false,
                          Int               iMvThreshold       = 0 );

  //--ICU/ETRI FMO Implementation
  const Int getSliceGroupIDofMb(Int mb);

  // JVT-S054 (ADD)
  UInt getSliceId() const { return m_uiSliceId;}

  //JVT-X046 {
	MbData* xGetMbData(UInt uiMbAddress) { return &m_pcMbData[uiMbAddress]; }

protected:
  const MbData& xGetOutMbData()            const { return m_pcMbData[m_uiSize]; }
  const MbData& xGetRefMbData( UInt uiSliceId, Int uiCurrSliceID, Int iMbY, Int iMbX, Bool bLoopFilter );
  const MbData& xGetColMbData( UInt uiIndex );

  ErrVal xCreateData( UInt uiSize );
  ErrVal xDeleteData();
  ErrVal xResetData();

protected:
  DynBuf<DBFilterParameter*>  m_cDBFPBuffer;
  MbTransformCoeffs*  m_pcMbTCoeffs;
  MbMotionData*       m_apcMbMotionData[2];
  MbMvData*           m_apcMbMvdData[2];
  MbData*             m_pcMbData;
  MbDataAccess*       m_pcMbDataAccess;

  UChar               m_ucLastMbQp;
  UChar               m_ucLastMbQp4LF;
  UInt                m_uiMbStride;
  UInt                m_uiMbOffset;
  Int                 m_iMbPerLine;
  Int                 m_iMbPerColumn;
  UInt                m_uiSize;
  UInt                m_uiMbProcessed;
  UInt                m_uiSliceId;
	Int                 m_iColocatedOffset;
  ProcessingState     m_eProcessingState;
  const MbDataCtrl*   m_pcMbDataCtrl0L1;
  Bool                m_bUseTopField;
  Bool                m_bPicCodedField;
  Bool                m_bInitDone;
  SliceHeader*        m_pcSliceHeader;
};



class ControlData
{
public:
  ControlData   ();
  ~ControlData  ();

  Void          clear               ();
  ErrVal        init                ( SliceHeader*  pcSliceHeader,
                                      MbDataCtrl*   pcMbDataCtrl,
                                      Double        dLambda );
  ErrVal        init                ( SliceHeader*  pcSliceHeader );

  Double        getLambda           ()  { return  m_dLambda;            }
	SliceHeader*  getSliceHeader      ( PicType ePicType = FRAME ) { return ( ePicType==BOT_FIELD ) ? m_pcSliceHeaderBot : m_pcSliceHeader;	}

  MbDataCtrl*   getMbDataCtrl       ()  { return  m_pcMbDataCtrl;       }
  Bool          isInitialized       ()  { return  m_pcMbDataCtrl != 0;  }

  ErrVal        setMbDataCtrl       ( MbDataCtrl* pcMbDataCtrl )
  {
    m_pcMbDataCtrl = pcMbDataCtrl;
    return Err::m_nOK;
  }

  ErrVal        setSliceHeader      ( SliceHeader* pcSliceHeader,
		                                  PicType      ePicType = FRAME )
  {
		if( ePicType==BOT_FIELD )
		{
			m_pcSliceHeaderBot = pcSliceHeader;
		}
		else
  {
    m_pcSliceHeader = pcSliceHeader;
		}

    return Err::m_nOK;
  }

  Frame*     getBaseLayerRec     ()  { return  m_pcBaseLayerRec;     }
  Frame*     getBaseLayerSbb     ()  { return  m_pcBaseLayerSbb;     }
  MbDataCtrl*   getBaseLayerCtrl    ()  { return  m_pcBaseLayerCtrl;    }
  MbDataCtrl*   getMbDataCtrl0L1    ()  { return m_pcMbDataCtrl0L1; }
  Void          setMbDataCtrl0L1    ( MbDataCtrl* p )  { m_pcMbDataCtrl0L1 = p; }
  MbDataCtrl*   getBaseLayerCtrlField ()  { return  m_pcBaseLayerCtrlField;    }
  UInt          getUseBLMotion      ()  { return  m_uiUseBLMotion;      }
  Void          setBaseLayerRec     ( Frame*   pcBaseLayerRec  )   { m_pcBaseLayerRec    = pcBaseLayerRec;   }
  Void          setBaseLayerSbb     ( Frame*   pcBaseLayerSbb  )   { m_pcBaseLayerSbb    = pcBaseLayerSbb;   }
  Void          setBaseLayerCtrl    ( MbDataCtrl* pcBaseLayerCtrl )   { m_pcBaseLayerCtrl   = pcBaseLayerCtrl;  }
  Void          setBaseLayerCtrlField ( MbDataCtrl* pcBaseLayerCtrl )   { m_pcBaseLayerCtrlField = pcBaseLayerCtrl;  }
  Void          setUseBLMotion      ( UInt        uiUseBLMotion   )   { m_uiUseBLMotion     = uiUseBLMotion;    }

  Void          setLambda           ( Double d ) { m_dLambda = d; }

  Void          setScalingFactor    ( Double  d ) { m_dScalingFactor      = d; }
  Double        getScalingFactor    ()  const     { return m_dScalingFactor;      }

  Void          setBaseLayer        ( UInt  uiBaseLayerId )
  {
    m_uiBaseLayerId = uiBaseLayerId;
  }

  UInt          getBaseLayerId    () { return m_uiBaseLayerId; }

  RefFrameList&   getPrdFrameList   ( UInt ui )     { return m_cRefListStruct.acRefFrameListRC[ui]; }
  RefListStruct&  getRefListStruct  ()              { return m_cRefListStruct; }

  Void          setSpatialScalability ( Bool b )  { m_bSpatialScalability = b; }
  Bool          getSpatialScalability ()  const   { return m_bSpatialScalability; }

private:
  MbDataCtrl*   m_pcMbDataCtrl;
  MbDataCtrl*   m_pcMbDataCtrl0L1;
  SliceHeader*  m_pcSliceHeader;
  SliceHeader*  m_pcSliceHeaderBot;

  Double        m_dLambda;

  Frame*     m_pcBaseLayerRec;
  Frame*     m_pcBaseLayerSbb;
  MbDataCtrl*   m_pcBaseLayerCtrl;
  MbDataCtrl*   m_pcBaseLayerCtrlField;
  UInt          m_uiUseBLMotion;

  Double        m_dScalingFactor;

  UInt          m_uiBaseLayerId;
  Bool          m_bSpatialScalability;     // TMM_ESS

  RefListStruct m_cRefListStruct;
	//JVT-X046 {
public:
	UInt m_uiCurrentFirstMB;
	bool m_bSliceGroupAllCoded;
  //JVT-X046 }
};






#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif



H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBDATACTRL_H__50D2B462_28AB_46CA_86AC_35502BD296BC__INCLUDED_)
