
#if !defined  _MCTF_PRE_PROCESSOR_H_
#define       _MCTF_PRE_PROCESSOR_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class PreProcessorParameter;

H264AVC_NAMESPACE_BEGIN

class Transform;
class YuvBufferCtrl;
class QuarterPelFilter;
class SampleWeighting;
class MbEncoder;
class IntraPredictionSearch;
class MotionEstimation;
class MotionEstimationQuarterPel;
class RateDistortion;
class MCTF;
class XDistortion;
class CodingParameter;


class MCTFPreProcessor
{
protected:
  MCTFPreProcessor          ();
  virtual ~MCTFPreProcessor ();

public:
  static ErrVal create      ( MCTFPreProcessor*&      rpcMCTFPreProcessor );
  ErrVal        destroy     ();

  ErrVal init               ( PreProcessorParameter*  pcParameter,
                              CodingParameter*        pcCodingParameter );
  ErrVal uninit             ();
  ErrVal process            ( PicBuffer*              pcOriginalPicBuffer,
                              PicBuffer*              pcReconstructPicBuffer,
                              PicBufferList&          rcPicBufferOutputList,
                              PicBufferList&          rcPicBufferUnusedList );
  ErrVal finish             ( PicBufferList&          rcPicBufferOutputList,
                              PicBufferList&          rcPicBufferUnusedList );

protected:
  ErrVal xCreateMCTFPreProcessor();

protected:
  MCTF*                   m_pcMCTF;
  MbEncoder*              m_pcMbEncoder;
  Transform*              m_pcTransform;
  IntraPredictionSearch*  m_pcIntraPrediction;
  YuvBufferCtrl*          m_pcYuvFullPelBufferCtrl;
  YuvBufferCtrl*          m_pcYuvHalfPelBufferCtrl;
  QuarterPelFilter*       m_pcQuarterPelFilter;
  SampleWeighting*        m_pcSampleWeighting;
  XDistortion*            m_pcXDistortion;
  MotionEstimation*       m_pcMotionEstimation;
  RateDistortion*         m_pcRateDistortion;
};


H264AVC_NAMESPACE_END


#endif // _MCTF_PRE_PROCESSOR_H_

