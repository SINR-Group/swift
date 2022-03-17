
#if !defined  _REC_PIC_BUFFER_INCLUDED_
#define       _REC_PIC_BUFFER_INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/MbDataCtrl.h"
#include "H264AVCCommonLib/Frame.h"


H264AVC_NAMESPACE_BEGIN


class RecPicBufUnit
{
protected:
  RecPicBufUnit             ();
  virtual ~RecPicBufUnit    ();

public:
  static ErrVal create          ( RecPicBufUnit*&             rpcRecPicBufUnit,
                                  YuvBufferCtrl&              rcYuvBufferCtrlFullPel,
                                  YuvBufferCtrl&              rcYuvBufferCtrlHalfPel,
                                  const SequenceParameterSet& rcSPS );
  ErrVal        destroy         ();

  ErrVal        init            ( SliceHeader*                pcSliceHeader,
                                  PicBuffer*                  pcPicBuffer );
  ErrVal        initNonEx       ( Int                         iPoc,
                                  UInt                        uiFrameNum );
  ErrVal        uninit          ();

  ErrVal        markNonRef      ();
  ErrVal        markOutputted   ();

  Int           getPoc          ()  const { return m_iPoc; }
  UInt          getFrameNum     ()  const { return m_uiFrameNum; }
  Bool          isExisting      ()  const { return m_bExisting; }
  Bool          isNeededForRef  ()  const { return m_bNeededForReference; }
  Bool          isOutputted     ()  const { return m_bOutputted; }
  Frame*     getRecFrame     ()        { return m_pcReconstructedFrame; }
  MbDataCtrl*   getMbDataCtrl   ()        { return m_pcMbDataCtrl; }
  PicBuffer*    getPicBuffer    ()        { return m_pcPicBuffer; }

  Int           getPicNum       ( UInt uiCurrFrameNum,
                                  UInt uiMaxFrameNum )  const
  {
    if( m_uiFrameNum > uiCurrFrameNum )
    {
      return (Int)m_uiFrameNum - (Int)uiMaxFrameNum;
    }
    return (Int)m_uiFrameNum;
  }


private:
  Int           m_iPoc;
  UInt          m_uiFrameNum;
  Bool          m_bExisting;
  Bool          m_bNeededForReference;
  Bool          m_bOutputted;
  Frame*     m_pcReconstructedFrame;
  MbDataCtrl*   m_pcMbDataCtrl;
  PicBuffer*    m_pcPicBuffer;
};


typedef MyList<RecPicBufUnit*>  RecPicBufUnitList;



class RecPicBuffer
{
protected:
  RecPicBuffer          ();
  virtual ~RecPicBuffer ();

public:
  static ErrVal   create                ( RecPicBuffer*&              rpcRecPicBuffer );
  ErrVal          destroy               ();
  ErrVal          init                  ( YuvBufferCtrl*              pcYuvBufferCtrlFullPel,
                                          YuvBufferCtrl*              pcYuvBufferCtrlHalfPel );
  ErrVal          initSPS               ( const SequenceParameterSet& rcSPS );
  ErrVal          uninit                ();
  ErrVal          clear                 ( PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );

  ErrVal          initCurrRecPicBufUnit ( RecPicBufUnit*&             rpcCurrRecPicBufUnit,
                                          PicBuffer*                  pcPicBuffer,
                                          SliceHeader*                pcSliceHeader,
                                          PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );
  ErrVal          store                 ( RecPicBufUnit*              pcRecPicBufUnit,
                                          SliceHeader*                pcSliceHeader,
                                          PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );
  ErrVal          getRefLists           ( RefFrameList&               rcList0,
                                          RefFrameList&               rcList1,
                                          SliceHeader&                rcSliceHeader );

  RecPicBufUnit*  getRecPicBufUnit      ( Int                         iPoc );

private:
  ErrVal          xCreateData           ( UInt                        uiMaxFramesInDPB,
                                          const SequenceParameterSet& rcSPS );
  ErrVal          xDeleteData           ();


  //===== memory management =====
  ErrVal          xCheckMissingPics     ( SliceHeader*                pcSliceHeader,
                                          PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );
  ErrVal          xStorePicture         ( RecPicBufUnit*              pcRecPicBufUnit,
                                          PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList,
                                          SliceHeader*                pcSliceHeader,
                                          Bool                        bTreatAsIdr );
  ErrVal          xOutput               ( PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );
  ErrVal          xClearOutputAll       ( PicBufferList&              rcOutputList,
                                          PicBufferList&              rcUnusedList );
  ErrVal          xUpdateMemory         ( SliceHeader*                pcSliceHeader );
  ErrVal          xClearBuffer          ();
  ErrVal          xSlidingWindow        ();
  ErrVal          xMMCO                 ( SliceHeader*                pcSliceHeader );
  ErrVal          xMarkShortTermUnused  ( RecPicBufUnit*              pcCurrentRecPicBufUnit,
                                          UInt                        uiDiffOfPicNums );

  //===== reference picture lists =====
  ErrVal          xInitRefListPSlice    ( RefFrameList&               rcList );
  ErrVal          xInitRefListsBSlice   ( RefFrameList&               rcList0,
                                          RefFrameList&               rcList1 );
  ErrVal          xRefListRemapping     ( RefFrameList&               rcList,
                                          ListIdx                     eListIdx,
                                          SliceHeader*                pcSliceHeader );
  ErrVal          xAdaptListSize        ( RefFrameList&               rcList,
                                          ListIdx                     eListIdx,
                                          SliceHeader&                rcSliceHeader );

  //--- debug ---
  ErrVal          xDumpRecPicBuffer     ();
  ErrVal          xDumpRefList          ( RefFrameList&               rcList,
                                          ListIdx                     eListIdx );

private:
  Bool                m_bInitDone;
  YuvBufferCtrl*      m_pcYuvBufferCtrlFullPel;
  YuvBufferCtrl*      m_pcYuvBufferCtrlHalfPel;

  UInt                m_uiNumRefFrames;
  UInt                m_uiMaxFrameNum;
  UInt                m_uiLastRefFrameNum;
  RecPicBufUnitList   m_cUsedRecPicBufUnitList;
  RecPicBufUnitList   m_cFreeRecPicBufUnitList;
  RecPicBufUnit*      m_pcCurrRecPicBufUnit;
};


H264AVC_NAMESPACE_END


#endif // _REC_PIC_BUFFER_INCLUDED_

