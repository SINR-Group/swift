
#ifndef __H264AVCDECODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCDECODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79


#include <algorithm>
#include <list>

#include "DecoderParameter.h"
#include "ReadBitstreamFile.h"
#ifdef SHARP_AVC_REWRITE_OUTPUT
#include "WriteBitstreamToFile.h"
#else
#include "WriteYuvToFile.h"
#endif



class H264AVCDecoderTest
{
private:
  class BufferParameters
  {
  public:
    BufferParameters();
    virtual ~BufferParameters();

    ErrVal      init( const h264::AccessUnit& rcAccessUnit );

    UInt        getLumaOffset ()  const { return m_uiLumaOffset; }
    UInt        getCbOffset   ()  const { return m_uiCbOffset; }
    UInt        getCrOffset   ()  const { return m_uiCrOffset; }
    UInt        getLumaHeight ()  const { return m_uiLumaHeight; }
    UInt        getLumaWidth  ()  const { return m_uiLumaWidth; }
    UInt        getLumaStride ()  const { return m_uiLumaStride; }
    UInt        getBufferSize ()  const { return m_uiBufferSize; }
    const UInt* getCropping   ()  const { return m_auiCropping; }

  private:
    UInt  m_uiLumaOffset;
    UInt  m_uiCbOffset;
    UInt  m_uiCrOffset;
    UInt  m_uiLumaHeight;
    UInt  m_uiLumaWidth;
    UInt  m_uiLumaStride;
    UInt  m_uiBufferSize;
    UInt  m_auiCropping[4];
  };

protected:
  H264AVCDecoderTest();
  virtual ~H264AVCDecoderTest();

public:
  static ErrVal create  ( H264AVCDecoderTest*&  rpcH264AVCDecoderTest );
  ErrVal        destroy ();
  ErrVal        init    ( DecoderParameter*     pcDecoderParameter,
                          ReadBitstreamIf*      pcReadBitStream,
#ifdef SHARP_AVC_REWRITE_OUTPUT
                          WriteBitstreamIf*     pcWriteBitstream );
#else
                          WriteYuvIf*           pcWriteYuv );
#endif
  ErrVal        uninit  ();
  ErrVal        go      ();

private:
  ErrVal  xProcessAccessUnit( h264::AccessUnit& rcAccessUnit,
                              Bool&             rbFirstAccessUnit,
                              UInt&             ruiNumProcessed );
  ErrVal  xGetNewPicBuffer  ( PicBuffer*&       rpcPicBuffer,
                              UInt              uiSize );
  ErrVal  xOutputNALUnits   ( BinDataList&      rcBinDataList,
                              UInt&             ruiNumNALUnits );
  ErrVal  xOutputPicBuffer  ( PicBufferList&    rcPicBufferOutputList,
                              UInt&             ruiNumFrames );
  ErrVal  xRemovePicBuffer  ( PicBufferList&    rcPicBufferUnusedList );

private:
  Bool                          m_bInitialized;
  h264::CreaterH264AVCDecoder*  m_pcH264AVCDecoder;
  DecoderParameter*             m_pcParameter;
  ReadBitstreamIf*              m_pcReadBitstream;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  WriteBitstreamIf*             m_pcWriteBitstream;
#else
  WriteYuvIf*                   m_pcWriteYuv;
#endif

  PicBufferList                 m_cActivePicBufferList;
  PicBufferList                 m_cUnusedPicBufferList;
  BufferParameters              m_cBufferParameters;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  UChar                         m_aucStartCodeBuffer[5];
  BinData                       m_cBinDataStartCode;
#else
  Int                           m_iMaxPocDiff;
  Int                           m_iLastPoc;
  UChar*                        m_pcLastFrame;
#endif
};

#endif //__H264AVCDECODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
