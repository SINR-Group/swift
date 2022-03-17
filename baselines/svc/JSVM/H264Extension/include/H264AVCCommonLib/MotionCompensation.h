
#if !defined(AFX_MOTIONCOMPENSATION_H__820D6942_007B_42EA_838B_AC025E866DBE__INCLUDED_)
#define AFX_MOTIONCOMPENSATION_H__820D6942_007B_42EA_838B_AC025E866DBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/MotionVectorCalculation.h"
#include "H264AVCCommonLib/YuvMbBuffer.h"
#include "H264AVCCommonLib/YuvPicBuffer.h"
#include "H264AVCCommonLib/Frame.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"


H264AVC_NAMESPACE_BEGIN


class SampleWeighting;
class QuarterPelFilter;

class Transform;

#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif

class H264AVCCOMMONLIB_API MotionCompensation :
public MotionVectorCalculation
{
protected:
  class MC8x8
  {
  public:
    MC8x8( Par8x8 ePar8x8 ) :  m_cIdx( B8x8Idx(ePar8x8) )  { clear(); }
    Void clear()
    {
      m_apcRefBuffer[0] = m_apcRefBuffer[1] = NULL;
      m_apcPW[0]        = m_apcPW[1]        = NULL;
    }

    B4x4Idx           m_cIdx;
    PredWeight        m_acPW[2];
    const PredWeight* m_apcPW[2];
    YuvPicBuffer*     m_apcRefBuffer[2];
    Mv3D              m_aacMv[2][6];
    Mv3D              m_aacMvd[2][6];  // differential motion vector
		Short             m_sChromaOffset[2];
  };

protected:
	MotionCompensation();
	virtual ~MotionCompensation();

public:
  static ErrVal create( MotionCompensation*& rpcMotionCompensation );
  ErrVal destroy();

  ErrVal init( QuarterPelFilter* pcQuarterPelFilter,
               Transform*        pcTransform,
               SampleWeighting* pcSampleWeighting);

  ErrVal initSlice( const SliceHeader& rcSH );
  ErrVal uninit();

  ErrVal compensateMbBLSkipIntra( MbDataAccess&   rcMbDataAccessBase,
                                  YuvMbBuffer*    pcRecBuffer,
                                  Frame*          pcBaseLayerRec );
  ErrVal updateMbBLSkipResidual ( MbDataAccess&   rcMbDataAccess,
                                  YuvMbBuffer&    rcMbResBuffer );

  ErrVal copyMbBuffer(  YuvMbBuffer*    pcMbBufSrc,
                        YuvMbBuffer*    pcMbBufDes,
                        Int sX, Int sY, Int eX, Int eY);
  ErrVal clearMbBuffer( YuvMbBuffer& rcMbBuffer, Int sX, Int sY, Int eX, Int eY );

  Void setResizeParameters   (ResizeParameters*				resizeParameters)
  {
    m_pcResizeParameters = resizeParameters;
  };

  ErrVal compensateMb     ( MbDataAccess&   rcMbDataAccess,
                            RefFrameList&   rcRefFrameList0,
                            RefFrameList&   rcRefFrameList1,
                            YuvMbBuffer* pcRecBuffer,
                            Bool            bCalcMv );
  ErrVal compensateSubMb  ( B8x8Idx         c8x8Idx,
                            MbDataAccess&   rcMbDataAccess,
                            RefFrameList&   rcRefFrameList0,
                            RefFrameList&   rcRefFrameList1,
                            YuvMbBuffer* pcRecBuffer,
                            Bool            bCalcMv,
                            Bool            bFaultTolerant );

  ErrVal updateMb(MbDataAccess&   rcMbDataAccess,
                  Frame*       pcMCFrame,
                  Frame*       pcPrdFrame,
                  ListIdx         eListPrd,
                  Int             iRefIdx);

  ErrVal updateSubMb( B8x8Idx         c8x8Idx,
                      MbDataAccess&   rcMbDataAccess,
                      Frame*       pcMCFrame,
                      Frame*       pcPrdFrame,
                      ListIdx         eListPrd );

  Void xUpdateMb8x8Mode(    B8x8Idx         c8x8Idx,
                            MbDataAccess&   rcMbDataAccess,
                            Frame*       pcMCFrame,
                            Frame*       pcPrdFrame,
                            ListIdx         eListPrd );

  ErrVal updateDirectBlock( MbDataAccess&   rcMbDataAccess,
                            Frame*       pcMCFrame,
                            Frame*       pcPrdFrame,
                            ListIdx         eListPrd,
                            Int             iRefIdx,
                            B8x8Idx         c8x8Idx );

  Void xUpdateBlk( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D );
  Void xUpdateBlk( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx );

  Void xUpdateLuma( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, UShort *usWeight );
  Void xUpdateLuma( Frame* pcPrdFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx, UShort *usWeight );

  Void updateBlkAdapt( YuvPicBuffer* pcSrcBuffer, YuvPicBuffer* pcDesBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX,
                                      UShort *usWeight);

  Void xUpdAdapt( XPel* pucDest, XPel* pucSrc, Int iDestStride, Int iSrcStride, Int iDx, Int iDy,
                                    UInt uiSizeY, UInt uiSizeX, UShort weight, UShort wMax );

  __inline Void xUpdateChroma( YuvPicBuffer* pcSrcBuffer, YuvPicBuffer* pcDesBuffer,  LumaIdx cIdx, Mv cMv,
    Int iSizeY, Int iSizeX, UShort *usWeight);
  Void xUpdateChroma( Frame* pcSrcFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx, UShort *usWeight );
  Void xUpdateChroma( Frame* pcSrcFrame, Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, UShort *usWeight );
  __inline Void xUpdateChromaPel( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX, UShort weight );

  ErrVal compensateDirectBlock( MbDataAccess& rcMbDataAccess, YuvMbBuffer *pcRecBuffer, B8x8Idx c8x8Idx, RefFrameList& rcRefFrameListL0, RefFrameList& rcRefFrameListL1 );
  ErrVal initMb( UInt uiMbY, UInt uiMbX, MbDataAccess& rcMbDataAccess );


protected:
  Void xPredMb8x8Mode( B8x8Idx c8x8Idx, MbDataAccess& rcMbDataAccess, const Frame* pcRefFrame0, const Frame* pcRefFrame1, YuvMbBuffer* pcRecBuffer );

  Void xPredLuma  ( YuvMbBuffer* pcRecBuffer,      Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D );
  Void xPredLuma  ( YuvMbBuffer* apcTarBuffer[2],  Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx );
  Void xPredChroma( YuvMbBuffer* pcRecBuffer,      Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D );
  Void xPredChroma( YuvMbBuffer* apcTarBuffer[2],  Int iSizeX, Int iSizeY, MC8x8& rcMc8x8D, SParIdx4x4 eSParIdx );

private:
	__inline Short xCorrectChromaMv( const MbDataAccess& rcMbDataAccess, PicType eRefPicType );

  __inline Void xGetMbPredData  ( MbDataAccess& rcMbDataAccess, const Frame* pcRefFrame, MC8x8& rcMC8x8D );
  __inline Void xGetBlkPredData ( MbDataAccess& rcMbDataAccess, const Frame* pcRefFrame, MC8x8& rcMC8x8D, BlkMode eBlkMode );

  __inline Void xPredChromaPel  ( XPel* pucDest, Int iDestStride, XPel* pucSrc, Int iSrcStride, Mv cMv, Int iSizeY, Int iSizeX );
  __inline Void xPredChroma     ( YuvMbBuffer* pcDesBuffer, YuvPicBuffer* pcSrcBuffer, LumaIdx cIdx, Mv cMv, Int iSizeY, Int iSizeX);

  __inline Void xGetMbPredData  ( MbDataAccess& rcMbDataAccess, const Frame* pcRefFrame0, const Frame* pcRefFrame1, MC8x8& rcMC8x8D );
  __inline Void xGetBlkPredData ( MbDataAccess& rcMbDataAccess, const Frame* pcRefFrame0, const Frame* pcRefFrame1, MC8x8& rcMC8x8D, BlkMode eBlkMode );

protected:
  QuarterPelFilter* m_pcQuarterPelFilter;
  Transform*        m_pcTransform;
  SampleWeighting* m_pcSampleWeighting;
  Mv   m_cMin;
  Mv   m_cMax;
  UInt m_uiMbInFrameY;
  UInt m_uiMbInFrameX;
  int m_curMbX;
  int m_curMbY;

  ResizeParameters*				m_pcResizeParameters;

  UInt  m_uiFrameNum;
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

#define DMV_THRES   5


H264AVC_NAMESPACE_END


#endif // !defined(AFX_MOTIONCOMPENSATION_H__820D6942_007B_42EA_838B_AC025E866DBE__INCLUDED_)
