
#if !defined(AFX_SLICEDECODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_)
#define AFX_SLICEDECODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/ControlMngIf.h"

H264AVC_NAMESPACE_BEGIN

class MbDecoder;
class Frame;

class SliceDecoder
{
protected:
	SliceDecoder();
	virtual ~SliceDecoder();

public:
  static ErrVal create  ( SliceDecoder*&      rpcSliceDecoder );
  ErrVal        destroy ();

  ErrVal        init    ( MbDecoder*          pcMbDecoder,
                          ControlMngIf*       pcControlMng );
  ErrVal        uninit  ();

  ErrVal  decode        ( SliceHeader&        rcSH,
                          MbDataCtrl*         pcMbDataCtrl,
                          MbDataCtrl*         pcMbDataCtrlBase,
                          Frame*              pcFrame,
                          Frame*              pcResidualLF,
                          Frame*              pcResidualILPred,
                          Frame*              pcBaseLayer,
                          Frame*              pcBaseLayerResidual,
                          RefFrameList*       pcRefFrameList0,
                          RefFrameList*       pcRefFrameList1,
                          MbDataCtrl*         pcMbDataCtrl0L1,
                          Bool                bReconstructAll );
  ErrVal  decodeMbAff   ( SliceHeader&        rcSH,
                          MbDataCtrl*         pcMbDataCtrl,
                          MbDataCtrl*         pcMbDataCtrlBase,
                          MbDataCtrl*         pcMbDataCtrlBaseField,
                          Frame*              pcFrame,
                          Frame*              pcResidualLF,
                          Frame*              pcResidualILPred,
                          Frame*              pcBaseLayer,
                          Frame*              pcBaseLayerResidual,
                          RefFrameList*       pcRefFrameList0,
                          RefFrameList*       pcRefFrameList1,
                          MbDataCtrl*         pcMbDataCtrl0L1,
                          Bool                bReconstructAll );

  Void setIntraBLFlagArrays( Bool* apabBaseModeFlagAllowedArrays[2] )
  {
    m_apabBaseModeFlagAllowedArrays[0] = apabBaseModeFlagAllowedArrays[0];
    m_apabBaseModeFlagAllowedArrays[1] = apabBaseModeFlagAllowedArrays[1];
  }

protected:
  Bool            m_bInitDone;
  ControlMngIf*   m_pcControlMng;
  MbDecoder*      m_pcMbDecoder;
  Bool*           m_apabBaseModeFlagAllowedArrays[2];
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_SLICEDECODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_)
