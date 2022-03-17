
#if !defined(AFX_SLICEENCODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_)
#define AFX_SLICEENCODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "H264AVCCommonLib/ControlMngIf.h"
#include "H264AVCCommonLib/TraceFile.h"
#include "MbEncoder.h"


H264AVC_NAMESPACE_BEGIN


class CodingParameter;
class MbCoder;
class PocCalculator;


class SliceEncoder
{
protected:
	SliceEncoder();
	virtual ~SliceEncoder();

public:
  static ErrVal create        ( SliceEncoder*&    rpcSliceEncoder );
  ErrVal destroy              ();
  ErrVal init                 ( MbEncoder*        pcMbEncoder,
                                MbCoder*          pcMbCoder,
                                ControlMngIf*     pcControlMng,
                                CodingParameter*  pcCodingParameter,
                                PocCalculator*    pcPocCalculator,
                                Transform*        pcTransform);
  ErrVal uninit               ();

  ErrVal  encodeSliceSVC      ( ControlData&      rcControlData,
                                Frame&            rcOrgFrame,
                                Frame&            rcFrame,
                                Frame*            pcResidualFrameLF,
                                Frame*            pcResidualFrameILPred,
                                Frame*            pcPredFrame,
                                PicType           ePicType,
                                UInt              uiNumMaxIter,
                                UInt              uiIterSearchRange,
                                Bool              bBiPred8x8Disable,
                                Bool              bMCBlks8x8Disable,
                                UInt              uiMaxDeltaQp,
                                UInt&             ruiBits );
  ErrVal  encodeMbAffSliceSVC ( ControlData&      rcControlData,
                                Frame&            rcOrgFrame,
                                Frame&            rcFrame,
                                Frame*            pcResidualFrameLF,
                                Frame*            pcResidualFrameILPred,
                                Frame*            pcPredFrame,
                                UInt              uiNumMaxIter,
                                UInt              uiIterSearchRange,
                                Bool              bBiPred8x8Disable,
                                Bool              bMCBlks8x8Disable,
                                UInt              uiMaxDeltaQp,
                                UInt&             ruiBits );
  ErrVal  encodeSlice         ( SliceHeader&      rcSliceHeader,
                                Frame*            pcFrame,
                                MbDataCtrl*       pcMbDataCtrl,
                                RefListStruct&    rcRefListStruct,
                                Bool              bMCBlks8x8Disable,
                                UInt              uiMbInRow,
                                Double            dlambda );
  MbEncoder*  getMbEncoder    ()                  { return m_pcMbEncoder; }

//TMM_WP
  ErrVal  xSetPredWeights     ( SliceHeader&      rcSliceHeader,
                                Frame*            pOrgFrame,
                                RefListStruct&    rcRefListStruct );
  ErrVal  xInitDefaultWeights ( Double*           pdWeights,
                                UInt              uiLumaWeightDenom,
                                UInt              uiChromaWeightDenom );
//TMM_WP
  //S051{
  Void		setUseBDir			    ( Bool b )          { m_pcMbEncoder->setUseBDir( b ); }
  //S051}

  ErrVal updatePictureResTransform  ( ControlData&    rcControlData,
                                      UInt            uiMbInRow );
  ErrVal updateBaseLayerResidual    ( ControlData&    rcControlData,
                                      UInt            uiMbInRow );
  // JVT-V035
  ErrVal updatePictureAVCRewrite    ( ControlData&    rcControlData,
                                      UInt            uiMbInRow );

  ErrVal xAddTCoeffs2               ( MbDataAccess&   rcMbDataAccess,
                                      MbDataAccess&   rcMbDataAccessBase );

  Void   setIntraBLFlagArrays       ( Bool*           apabBaseModeFlagAllowedArrays[2] )
  {
    m_apabBaseModeFlagAllowedArrays[0] = apabBaseModeFlagAllowedArrays[0];
    m_apabBaseModeFlagAllowedArrays[1] = apabBaseModeFlagAllowedArrays[1];
  }

public://protected://JVT-X046
  MbEncoder*        m_pcMbEncoder;
  MbCoder*          m_pcMbCoder;
  ControlMngIf*     m_pcControlMng;
  CodingParameter*  m_pcCodingParameter;
  PocCalculator*    m_pcPocCalculator;
  Transform*        m_pcTransform;
  Bool              m_bInitDone;
  UInt              m_uiFrameCount;
  SliceType         m_eSliceType;
  Bool              m_bTraceEnable;
  Bool*             m_apabBaseModeFlagAllowedArrays[2];
	//JVT-X046 {
  UInt              m_uiSliceMode;
  UInt              m_uiSliceArgument;
  //JVT-X046 }
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_SLICEENCODER_H__A0183156_A54B_425D_8CF1_D350651F638C__INCLUDED_)
