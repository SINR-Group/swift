
#ifndef _MCTF_PRE_PROCESSOR_TEST_H
#define _MCTF_PRE_PROCESSOR_TEST_H


#include "Typedefs.h"
#include "H264AVCEncoderLib.h"
#include "H264AVCVideoIoLib.h"
#include "MCTFPreProcessor.h"
#include "PreProcessorParameter.h"
#include "WriteYuvToFile.h"
#include "ReadYuvFile.h"
#include "CodingParameter.h"


class MCTFPreProcessorTest
{
public:
  MCTFPreProcessorTest    ();
  ~MCTFPreProcessorTest   ();

  static ErrVal   create  ( MCTFPreProcessorTest*&  rpcMCTFPreProcessorTest );
  ErrVal          destroy ();
  ErrVal          init    ( PreProcessorParameter*  pcPreProcessorParameter );
  ErrVal          go      ();

protected:
  ErrVal  xInitCodingParameter  ();
  ErrVal  xGetNewPicBuffer      ( PicBuffer*&     rpcPicBuffer,
                                  UInt            uiSize );
  ErrVal  xRemovePicBuffer      ( PicBufferList&  rcList );
  ErrVal  xWrite                ( PicBufferList&  rcList );

private:
  PreProcessorParameter*        m_pcParameter;
  h264::CodingParameter         m_cCodingParameter;
  h264::MCTFPreProcessor*       m_pcMCTFPreProcessor;
  WriteYuvToFile*               m_pcWriteYuv;
  ReadYuvFile*                  m_pcReadYuv;
  PicBufferList                 m_cActivePicBufferList;
  PicBufferList                 m_cUnusedPicBufferList;
  UInt                          m_uiPicSize;
  UInt                          m_uiLumOffset;
  UInt                          m_uiCbOffset;
  UInt                          m_uiCrOffset;
  UInt                          m_uiHeight;
  UInt                          m_uiWidth;
  UInt                          m_uiStride;
};


#endif // _MCTF_PRE_PROCESSOR_TEST_H

