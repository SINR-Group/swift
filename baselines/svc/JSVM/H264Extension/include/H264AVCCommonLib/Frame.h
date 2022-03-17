
#if !defined(AFX_INTFRAME_H__98AFB9AC_5EE3_45A9_B09B_859511AC9090__INCLUDED_)
#define AFX_INTFRAME_H__98AFB9AC_5EE3_45A9_B09B_859511AC9090__INCLUDED_



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/YuvPicBuffer.h"
#include "ResizeParameters.h"

H264AVC_NAMESPACE_BEGIN


class DownConvert;
class QuarterPelFilter;
class MbDataCtrl;
class DPBUnit;
class RecPicBufUnit;
class ReconstructionBypass;

class H264AVCCOMMONLIB_API Frame
{
public:
	Frame                ( YuvBufferCtrl&    rcYuvFullPelBufferCtrl,
                         YuvBufferCtrl&    rcYuvHalfPelBufferCtrl,
												 PicType           ePicType,
                         Frame*            pcFrame );
	virtual ~Frame       ();

  ErrVal  init            ();
  ErrVal  initHalfPel     ();
  ErrVal  initHalfPel     ( XPel*& rpucYuvBuffer );

  static ErrVal create    ( Frame*& rpcIntFrame, YuvBufferCtrl& rcYuvFullPelBufferCtrl, YuvBufferCtrl& rcYuvHalfPelBufferCtrl, PicType ePicType, Frame* pcFrame );
	ErrVal  destroy         ();
  ErrVal  uninit          ();
  ErrVal  uninitHalfPel   ();

  ErrVal  load            ( PicBuffer*        pcPicBuffer );
  ErrVal  store           ( PicBuffer*        pcPicBuffer, PicType ePicType = FRAME );

  ErrVal  addFrameFieldBuffer   ();
	ErrVal  removeFrameFieldBuffer();
  ErrVal  addFieldBuffer        ( PicType ePicType );
  ErrVal  removeFieldBuffer     ( PicType ePicType );
  ErrVal  extendFrame           ( QuarterPelFilter* pcQuarterPelFilter, PicType ePicType=FRAME, Bool bFrameMbsOnlyFlag=true );

  Void      setDPBUnit      ( DPBUnit*  pcDPBUnit ) { m_pcDPBUnit = pcDPBUnit; }
  DPBUnit*  getDPBUnit      ()                      { return m_pcDPBUnit; }
  Bool      isLongTerm  ()          const { return m_bLongTerm; }
  Void      setLongTerm ( Bool b )
  {
    m_bLongTerm = b;
    if( NULL != m_pcFrameTopField ) m_pcFrameTopField->setLongTerm( m_bLongTerm );
    if( NULL != m_pcFrameBotField ) m_pcFrameBotField->setLongTerm( m_bLongTerm );
  }

  const PictureParameters&  getPicParameters()                    const;
  const PictureParameters&  getPicParameters( PicType ePicType )  const;
  ErrVal  setPicParameters  ( const ResizeParameters&   rcRP,     const SliceHeader* pcSH     = 0 );
  ErrVal  setPicParameters  ( const PictureParameters&  rcPP,     PicType            ePicType = FRAME );
  ErrVal  copyPicParameters ( const Frame&              rcFrame,  PicType            ePicType = FRAME );

  const Frame*  getFrame() const { return m_pcFrame; }

  Void            setRecPicBufUnit( RecPicBufUnit* pcUnit ) { m_pcDPBUnit = (DPBUnit*)(Void*)pcUnit; }
  RecPicBufUnit*  getRecPicBufUnit()                        { return (RecPicBufUnit*)(Void*)m_pcDPBUnit; }
  ErrVal clip()
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( getFullPelYuvBuffer()->clip() );
    return Err::m_nOK;
  }

  ErrVal prediction       ( Frame* pcMCPFrame, Frame* pcSrcFrame, PicType ePicType )
  {
    ASSERT( m_ePicType==FRAME );

    if( ePicType==FRAME )
  {
      RNOK( getFullPelYuvBuffer()->prediction( pcSrcFrame->getFullPelYuvBuffer(),
                                               pcMCPFrame->getFullPelYuvBuffer() ) );
    }
    else
  {
			RNOK(             addFieldBuffer( ePicType ) );
			RNOK( pcSrcFrame->addFieldBuffer( ePicType ) );
			RNOK( pcMCPFrame->addFieldBuffer( ePicType ) );
      RNOK( getPic( ePicType )->getFullPelYuvBuffer()->prediction( pcSrcFrame->getPic( ePicType )->getFullPelYuvBuffer(),
                                                                   pcMCPFrame->getPic( ePicType )->getFullPelYuvBuffer() ) );

    }
    return Err::m_nOK;
  }

  ErrVal update           ( Frame* pcMCPFrame, Frame* pcSrcFrame, UInt uiShift )
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( getFullPelYuvBuffer()->update           ( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame->getFullPelYuvBuffer(), uiShift ) );
    return Err::m_nOK;
  }

  ErrVal inverseUpdate    ( Frame* pcMCPFrame, Frame* pcSrcFrame, UInt uiShift )
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( getFullPelYuvBuffer()->inverseUpdate    ( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame->getFullPelYuvBuffer(), uiShift ) );
    return Err::m_nOK;
  }


  ErrVal update           ( Frame* pcMCPFrame0, Frame* pcMCPFrame1, Frame* pcSrcFrame )
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( getFullPelYuvBuffer()->update           ( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame0->getFullPelYuvBuffer(), pcMCPFrame1->getFullPelYuvBuffer() ) );
    return Err::m_nOK;
  }

  ErrVal inverseUpdate    ( Frame* pcMCPFrame0, Frame* pcMCPFrame1, Frame* pcSrcFrame )
  {
    ASSERT( m_ePicType==FRAME );
		if (pcMCPFrame0 && pcMCPFrame1){
			RNOK( getFullPelYuvBuffer()->inverseUpdate    ( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame0->getFullPelYuvBuffer(), pcMCPFrame1->getFullPelYuvBuffer() ) );
		}else if (pcMCPFrame0){
			RNOK( getFullPelYuvBuffer()->inverseUpdate    ( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame0->getFullPelYuvBuffer(), (YuvPicBuffer*)NULL ) );
		}else{
			RNOK( getFullPelYuvBuffer()->inverseUpdate    ( pcSrcFrame->getFullPelYuvBuffer(), (YuvPicBuffer*)NULL, pcMCPFrame1->getFullPelYuvBuffer() ) );
		}
    return Err::m_nOK;
  }

  ErrVal inversePrediction( Frame* pcMCPFrame,  Frame* pcSrcFrame, PicType ePicType )
  {
    ASSERT( m_ePicType==FRAME );

    if( ePicType==FRAME )
  {
      RNOK( getFullPelYuvBuffer()->inversePrediction( pcSrcFrame->getFullPelYuvBuffer(),
                                                      pcMCPFrame->getFullPelYuvBuffer() ) );
    }
    else
  {
			RNOK(             addFieldBuffer( ePicType ) );
			RNOK( pcSrcFrame->addFieldBuffer( ePicType ) );
			RNOK( pcMCPFrame->addFieldBuffer( ePicType ) );
      RNOK( getPic( ePicType )->getFullPelYuvBuffer()->inversePrediction( pcSrcFrame->getPic( ePicType )->getFullPelYuvBuffer(),
                                                                          pcMCPFrame->getPic( ePicType )->getFullPelYuvBuffer() ) );

    }
    return Err::m_nOK;
  }


  ErrVal  copyAll     ( Frame* pcSrcFrame )
  {
    ASSERT( m_ePicType==FRAME );
    m_iPoc          = pcSrcFrame->m_iPoc;
    m_iTopFieldPoc  = pcSrcFrame->m_iTopFieldPoc;
    m_iBotFieldPoc  = pcSrcFrame->m_iBotFieldPoc;
    RNOK( getFullPelYuvBuffer()->copy( pcSrcFrame->getFullPelYuvBuffer() ) );

    return Err::m_nOK;
  }

  ErrVal copy             ( Frame* pcSrcFrame, PicType ePicType )
  {
    ASSERT( m_ePicType==FRAME );
    if( ePicType==FRAME )
    {
      RNOK( getFullPelYuvBuffer()->copy( pcSrcFrame->getFullPelYuvBuffer()) );
    }
    else
    {
      RNOK(             addFieldBuffer( ePicType ) );
      RNOK( pcSrcFrame->addFieldBuffer( ePicType ) );
      RNOK( getPic( ePicType )->getFullPelYuvBuffer()->copy( pcSrcFrame->getPic( ePicType )->getFullPelYuvBuffer() ) );
    }
    return Err::m_nOK;
  }

    //JVT-X046 {
  ErrVal predictionSlices       (Frame* pcSrcFrame,Frame* pcMCPFrame, UInt uiMbY, UInt uiMbX)
  {
	  RNOK( getFullPelYuvBuffer()->predictionSlices( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame->getFullPelYuvBuffer(), uiMbY, uiMbX ) );
	  return Err::m_nOK;
  }
  ErrVal inversepredictionSlices       (Frame* pcSrcFrame,Frame* pcMCPFrame, UInt uiMbY, UInt uiMbX)
  {
	  RNOK( getFullPelYuvBuffer()->inversepredictionSlices( pcSrcFrame->getFullPelYuvBuffer(), pcMCPFrame->getFullPelYuvBuffer(), uiMbY, uiMbX ) );
	  return Err::m_nOK;
  }
  ErrVal copyMb       (Frame* pcSrcFrame, UInt uiMbY ,UInt uiMbX)
  {
	  RNOK( getFullPelYuvBuffer()->copyMb( pcSrcFrame->getFullPelYuvBuffer(), uiMbY, uiMbX ) );
	  return Err::m_nOK;
  }
	void   setMBZero( UInt uiMBY, UInt uiMBX ) { getFullPelYuvBuffer()->setMBZero(uiMBY,uiMBX); }
	ErrVal copySlice (Frame* pcSrcFrame, PicType ePicType, UInt uiFirstMB, UInt uiLastMB)
  {
	  ASSERT( m_ePicType==FRAME );
	  if( ePicType==FRAME )
	  {
		  RNOK( getFullPelYuvBuffer()->copySlice( pcSrcFrame->getFullPelYuvBuffer(),uiFirstMB,uiLastMB) );
	  }
	  return Err::m_nOK;
  }
	//JVT-X046 }

  ErrVal  subtract    ( Frame* pcSrcFrame0, Frame* pcSrcFrame1 )
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( getFullPelYuvBuffer()->subtract( pcSrcFrame0->getFullPelYuvBuffer(), pcSrcFrame1->getFullPelYuvBuffer() ) );
    return Err::m_nOK;
  }

  ErrVal add              ( Frame* pcSrcFrame,  PicType ePicType )
  {
    ASSERT( m_ePicType==FRAME );

    if( ePicType==FRAME )
  {
    RNOK( getFullPelYuvBuffer()->add ( pcSrcFrame->getFullPelYuvBuffer()) );
    }
    else
    {
			RNOK(             addFieldBuffer( ePicType ) );
			RNOK( pcSrcFrame->addFieldBuffer( ePicType ) );
      RNOK( getPic( ePicType )->getFullPelYuvBuffer()->add( pcSrcFrame->getPic( ePicType )->getFullPelYuvBuffer() ) );
    }
    return Err::m_nOK;
  }

  ErrVal  setZero     ()
  {
    ASSERT( m_ePicType==FRAME );
    getFullPelYuvBuffer()->setZero();
    return Err::m_nOK;
  }

  ErrVal  setNonZeroFlags( UShort* pusNonZeroFlags, UInt uiStride )
  {
    ASSERT( m_ePicType==FRAME );
    return getFullPelYuvBuffer()->setNonZeroFlags( pusNonZeroFlags, uiStride );
  }

  ErrVal getSSD( Double& dSSDY, Double& dSSDU, Double& dSSDV, PicBuffer* pcOrgPicBuffer )
  {
    ASSERT( m_ePicType==FRAME );
    RNOK( m_cFullPelYuvBuffer.getSSD( dSSDY, dSSDU, dSSDV, pcOrgPicBuffer ) );
    return Err::m_nOK;
  }

  ErrVal dump( FILE* pFile, Int uiBandType, MbDataCtrl* pcMbDataCtrl )
  {
    if( uiBandType != 0 )
    {
      RNOK( getFullPelYuvBuffer()->dumpHPS( pFile, pcMbDataCtrl ) );
    }
    else
    {
      RNOK( getFullPelYuvBuffer()->dumpLPS( pFile ) );
    }
		fflush( pFile );
    return Err::m_nOK;
  }

  ErrVal  intraUpsampling   ( Frame*                pcBaseFrame,
                              Frame*                pcTempBaseFrame,
                              Frame*                pcTempFrame,
                              DownConvert&          rcDownConvert,
                              ResizeParameters*     pcParameters,
                              MbDataCtrl*           pcMbDataCtrlBase,
                              MbDataCtrl*           pcMbDataCtrlPredFrm,
                              MbDataCtrl*           pcMbDataCtrlPredFld,
                              ReconstructionBypass* pcReconstructionBypass,
                              Bool                  bConstrainedIntraUpsamplingFlag,
                              Bool*                 pabBaseModeAllowedFlagArrayFrm = 0,
                              Bool*                 pabBaseModeAllowedFlagArrayFld = 0 );
  ErrVal residualUpsampling ( Frame*                pcBaseFrame,
                              DownConvert&          rcDownConvert,
                              ResizeParameters*     pcParameters,
                              MbDataCtrl*           pcMbDataCtrlBase );

  const YuvPicBuffer* getFullPelYuvBuffer() const  { return &m_cFullPelYuvBuffer; }
  YuvPicBuffer*       getFullPelYuvBuffer()        { return &m_cFullPelYuvBuffer; }
  YuvPicBuffer*       getHalfPelYuvBuffer()        { return &m_cHalfPelYuvBuffer; }

	Bool  isPocAvailable()           const { return m_bPocIsSet; }
	Int   getPoc        ()           const { return m_iPoc; }
  Int   getTopFieldPoc()           const { return m_iTopFieldPoc; }
  Int   getBotFieldPoc()           const { return m_iBotFieldPoc; }

  Void  clearPoc      ()
  {
    if( m_pcFrameTopField )
    {
      m_pcFrameTopField->clearPoc();
    }
    if( m_pcFrameBotField )
    {
      m_pcFrameBotField->clearPoc();
    }
    m_iTopFieldPoc  = MSYS_INT_MIN;
    m_iBotFieldPoc  = MSYS_INT_MIN;
    m_iPoc          = MSYS_INT_MIN;
    m_bPocIsSet     = false;
  }

	Void  setPoc        ( Int iPoc )                    { m_iPoc = iPoc; m_bPocIsSet = true; }
	Void  setPoc        ( const SliceHeader& rcSH )
	{
		ASSERT( m_ePicType==FRAME );
		const PicType ePicType = rcSH.getPicType();

    if( ePicType & TOP_FIELD )
    {
      m_iTopFieldPoc = rcSH.getTopFieldPoc();
      if( m_pcFrameTopField && m_pcFrameBotField )
      {
        m_pcFrameTopField->setPoc( m_iTopFieldPoc );
        setPoc( m_pcFrameBotField->isPocAvailable() ? gMax( m_pcFrameBotField->getPoc(), m_iTopFieldPoc ) : m_iTopFieldPoc );
      }
    }
    if( ePicType & BOT_FIELD )
    {
      m_iBotFieldPoc = rcSH.getBotFieldPoc();
      if( m_pcFrameTopField && m_pcFrameBotField )
      {
        m_pcFrameBotField->setPoc( m_iBotFieldPoc );
        setPoc( m_pcFrameTopField->isPocAvailable() ? gMin( m_pcFrameTopField->getPoc(), m_iBotFieldPoc ) : m_iBotFieldPoc );
      }
    }
    if( ! m_pcFrameTopField || ! m_pcFrameBotField )
    {
      setPoc( gMax( m_iTopFieldPoc, m_iBotFieldPoc ) );
    }
  }

  Bool  isHalfPel()   { return m_bHalfPel; }

  const Frame*  getPic( PicType ePicType ) const;
  Frame*        getPic( PicType ePicType );

	PicType getPicType ()            const { return m_ePicType; }
	Void setPicType    ( PicType ePicType ){ m_ePicType = ePicType; }

  Bool  isExtended () { return m_bExtended; }
  Void  clearExtended() { m_bExtended = false; }
  Void  setExtended  ()                  { m_bExtended = true; }

  // JVT-R057 LA-RDO{
  Void   initChannelDistortion();
  Void   uninitChannelDistortion()  {
	  if(m_piChannelDistortion)
		  delete[] m_piChannelDistortion;
  }
  const UInt* getChannelDistortion()  const { return m_piChannelDistortion; }
  UInt*       getChannelDistortion()        { return m_piChannelDistortion; }
  Void   copyChannelDistortion(Frame*p1);
  Void   zeroChannelDistortion();
  Void   setChannelDistortion(Frame*p1) { if(p1) m_piChannelDistortion=p1->m_piChannelDistortion; else m_piChannelDistortion=NULL;}
  // JVT-R057 LA-RDO}

  Void  setUnvalid()  { m_bUnvalid = true;  }
  Void  setValid  ()  { m_bUnvalid = false; }
  Bool  isUnvalid ()  { return m_bUnvalid;  }

protected:
  ErrVal  xUpdatePicParameters();

protected:
  YuvPicBuffer      m_cFullPelYuvBuffer;
  YuvPicBuffer      m_cHalfPelYuvBuffer;
  PictureParameters m_cPicParameters;     // for frame or top field
  PictureParameters m_cPicParametersBot;  // for bottom field

  Bool            m_bHalfPel;
  Bool            m_bExtended;
	Bool            m_bPocIsSet;
  Int             m_iTopFieldPoc;
  Int             m_iBotFieldPoc;
  Int             m_iPoc;
	PicType         m_ePicType;
  Frame*          m_pcFrameTopField;
  Frame*          m_pcFrameBotField;
  Frame*          m_pcFrame;

  DPBUnit*        m_pcDPBUnit;
  // JVT-R057 LA-RDO{
  UInt*            m_piChannelDistortion;
  // JVT-R057 LA-RDO}

  Bool      m_bLongTerm;
  Bool      m_bUnvalid;
};

H264AVCCOMMONLIB_API extern __inline 
ErrVal gSetFrameFieldLists ( RefFrameList& rcTopFieldList, RefFrameList& rcBotFieldList, RefFrameList& rcRefFrameList )
{
  rcTopFieldList.reset();
  rcBotFieldList.reset();
  const Int iMaxEntries = gMin( rcRefFrameList.getSize(), rcRefFrameList.getActive() );
  for( Int iFrmIdx = 0; iFrmIdx < iMaxEntries; iFrmIdx++ )
  {
    Frame* pcTopField = rcRefFrameList.getEntry( iFrmIdx )->getPic( TOP_FIELD );
    Frame* pcBotField = rcRefFrameList.getEntry( iFrmIdx )->getPic( BOT_FIELD );
    rcTopFieldList.add( pcTopField );
    rcTopFieldList.add( pcBotField );
    rcBotFieldList.add( pcBotField );
    rcBotFieldList.add( pcTopField );
  }
  return Err::m_nOK;
}


H264AVCCOMMONLIB_API extern __inline 
ErrVal gSetFrameFieldLists ( RefListStruct& rcTopFieldStruct, RefListStruct& rcBotFieldStruct, RefListStruct& rcRefFrameStruct )
{
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListME[0], rcBotFieldStruct.acRefFrameListME[0], rcRefFrameStruct.acRefFrameListME[0] ) );
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListME[1], rcBotFieldStruct.acRefFrameListME[1], rcRefFrameStruct.acRefFrameListME[1] ) );
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListMC[0], rcBotFieldStruct.acRefFrameListMC[0], rcRefFrameStruct.acRefFrameListMC[0] ) );
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListMC[1], rcBotFieldStruct.acRefFrameListMC[1], rcRefFrameStruct.acRefFrameListMC[1] ) );
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListRC[0], rcBotFieldStruct.acRefFrameListRC[0], rcRefFrameStruct.acRefFrameListRC[0] ) );
  RNOK( gSetFrameFieldLists( rcTopFieldStruct.acRefFrameListRC[1], rcBotFieldStruct.acRefFrameListRC[1], rcRefFrameStruct.acRefFrameListRC[1] ) );
  rcTopFieldStruct.bMCandRClistsDiffer  = rcRefFrameStruct.bMCandRClistsDiffer;
  rcBotFieldStruct.bMCandRClistsDiffer  = rcRefFrameStruct.bMCandRClistsDiffer;
  return Err::m_nOK;
}


H264AVCCOMMONLIB_API extern __inline ErrVal gSetFrameFieldArrays( Frame* apcFrame[4], Frame* pcFrame )
{
  if( pcFrame == NULL )
  {
    apcFrame[0] = NULL;
    apcFrame[1] = NULL;
    apcFrame[2] = NULL;
    apcFrame[3] = NULL;
  }
  else
  {
		RNOK( pcFrame->addFrameFieldBuffer() );
    apcFrame[0] = pcFrame->getPic( TOP_FIELD );
    apcFrame[1] = pcFrame->getPic( BOT_FIELD );
    apcFrame[2] = pcFrame->getPic( FRAME     );
    apcFrame[3] = pcFrame->getPic( FRAME     );
  }
  return Err::m_nOK;
}

H264AVC_NAMESPACE_END


#endif // !defined(AFX_INTFRAME_H__98AFB9AC_5EE3_45A9_B09B_859511AC9090__INCLUDED_)
