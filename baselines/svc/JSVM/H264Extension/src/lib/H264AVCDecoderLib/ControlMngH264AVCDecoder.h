
#if !defined(AFX_CONTROLMNG_H__CE22E161_6ACE_4E6F_9E1B_08F0A99F9742__INCLUDED_)
#define AFX_CONTROLMNG_H__CE22E161_6ACE_4E6F_9E1B_08F0A99F9742__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/ControlMngIf.h"
#include "H264AVCCommonLib/ParameterSetMng.h"
#include "H264AVCCommonLib/PocCalculator.h"
#include "SliceReader.h"
#include "NalUnitParser.h"
#include "SliceDecoder.h"
#include "BitReadBuffer.h"
#include "UvlcReader.h"
#include "MbParser.h"
#include "H264AVCCommonLib/LoopFilter.h"
#include "MbDecoder.h"
#include "H264AVCCommonLib/Transform.h"
#include "H264AVCCommonLib/IntraPrediction.h"
#include "H264AVCCommonLib/MotionCompensation.h"
#include "H264AVCCommonLib/YuvBufferCtrl.h"
#include "H264AVCCommonLib/QuarterPelFilter.h"
#include "CabacReader.h"
#include "H264AVCCommonLib/SampleWeighting.h"
#include "GOPDecoder.h"
#include "H264AVCDecoder.h"

H264AVC_NAMESPACE_BEGIN

class ControlMngH264AVCDecoder : public ControlMngIf
{
protected:
	ControlMngH264AVCDecoder();
	virtual ~ControlMngH264AVCDecoder();

public:
  static ErrVal create  ( ControlMngH264AVCDecoder*&  rpcControlMngH264AVCDecoder );
  ErrVal        destroy ();
  ErrVal        init    ( UvlcReader*                 pcUvlcReader,
                          MbParser*                   pcMbParser,
                          MotionCompensation*         pcMotionCompensation,
                          YuvBufferCtrl*              pcYuvFullPelBufferCtrl[MAX_LAYERS],
                          CabacReader*                pcCabacReader,
                          SampleWeighting*            pcSampleWeighting,
                          LayerDecoder*               apcLayerDecoder       [MAX_LAYERS] );
  ErrVal        uninit  ();

  ErrVal initMbForParsing     ( MbDataAccess*& rpcMbDataAccess, UInt uiMbIndex );
  ErrVal initMbForDecoding    ( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff );
  ErrVal initMbForFiltering   ( MbDataAccess*& rpcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff );

  ErrVal initSlice0           ( SliceHeader*                rcSH);
  ErrVal initSPS              ( SequenceParameterSet&       rcSequenceParameterSet, UInt  uiLayer );
  ErrVal initParameterSets    ( const SequenceParameterSet& rcSPS,
                                const PictureParameterSet&  rcPPS )    { return Err::m_nERR; }

  ErrVal initSliceForCoding   ( const SliceHeader& rcSH ) { return Err::m_nERR; }
  ErrVal initSliceForReading  ( const SliceHeader& rcSH );
  ErrVal initSliceForDecoding ( const SliceHeader& rcSH );
  ErrVal initSliceForFiltering( const SliceHeader& rcSH );
  ErrVal initSliceForWeighting( const SliceHeader& rcSH ) { return m_pcSampleWeighting->initSlice( rcSH ); }
  ErrVal finishSlice          ( const SliceHeader& rcSH, Bool& rbPicDone, Bool& rbFrameDone );

  ErrVal initMbForCoding      ( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff, Bool bFieldFlag ) { return Err::m_nERR; }
  ErrVal initMbForDecoding    ( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff );
  ErrVal initMbForFiltering   ( MbDataAccess& rcMbDataAccess, UInt uiMbY, UInt uiMbX, Bool bMbAff );

protected:
  UInt                      m_uiCurrLayer;
  UInt                      m_auiMbXinFrame           [MAX_LAYERS];
  UInt                      m_auiMbYinFrame           [MAX_LAYERS];
  MbDataCtrl*               m_pcMbDataCtrl;

  UvlcReader*               m_pcUvlcReader;
  MbParser*                 m_pcMbParser;
  MotionCompensation*       m_pcMotionCompensation;
  YuvBufferCtrl*            m_apcYuvFullPelBufferCtrl [MAX_LAYERS];
  CabacReader*              m_pcCabacReader;
  SampleWeighting*          m_pcSampleWeighting;
  LayerDecoder*             m_apcLayerDecoder         [MAX_LAYERS];
  Bool                      m_uiInitialized           [MAX_LAYERS];
};

H264AVC_NAMESPACE_END

#endif // !defined(AFX_CONTROLMNG_H__CE22E161_6ACE_4E6F_9E1B_08F0A99F9742__INCLUDED_)
