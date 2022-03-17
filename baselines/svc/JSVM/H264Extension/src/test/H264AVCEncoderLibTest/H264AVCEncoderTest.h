
#ifndef __H264AVCENCODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
#define __H264AVCENCODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79

#include <algorithm>
#include <list>

#include "WriteBitstreamToFile.h"
#include "ReadYuvFile.h"
#include "WriteYuvToFile.h"



class EncoderCodingParameter;



typedef struct
{
  UInt    uiNumberOfLayers;
  std::string cBitstreamFilename;
  Int     nResult;
  UInt    nFrames;
} EncoderIoParameter;




class H264AVCEncoderTest
{
private:
  H264AVCEncoderTest();
  virtual ~H264AVCEncoderTest();

public:
  static ErrVal create( H264AVCEncoderTest*& rpcH264AVCEncoderTest );

  ErrVal init     ( Int     argc,
                    Char**  argv );
  ErrVal go       ();
  ErrVal destroy  ();
  ErrVal ScalableDealing ();

protected:
  ErrVal  xGetNewPicBuffer( PicBuffer*&             rpcPicBuffer,
                            UInt                    uiLayer,
                            UInt                    uiSize );
  ErrVal  xRemovePicBuffer( PicBufferList&          rcPicBufferUnusedList,
                            UInt                    uiLayer );

  ErrVal  xWrite          ( ExtBinDataAccessorList& rcList,
                            UInt&                   ruiBytesInFrame );
  ErrVal  xRelease        ( ExtBinDataAccessorList& rcList );

  ErrVal  xWrite          ( PicBufferList&          rcList,
                            UInt                    uiLayer );
  ErrVal  xRelease        ( PicBufferList&          rcList,
                            UInt                    uiLayer );
#if DOLBY_ENCMUX_ENABLE
  //Dolby muxing functions
  void sbsMux(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height, Int offset0=0, Int offset1=1, Int iFilterIdx=7);
  ErrVal padBuf(UChar *output, Int iStrideOut, Int width, Int height, Int width_out, Int height_out, Int fillMode);
  void sbsMuxFR(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height);
  void tabMux(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height, Int offset0=0, Int offset1=1, Int iFilterIdx=7);
  void tabMuxFR(UChar *output, Int iStrideOut, UChar *input0, UChar *input1, Int iStrideIn, Int width, Int height);
#endif

protected:
  EncoderIoParameter            m_cEncoderIoParameter;
  EncoderCodingParameter*       m_pcEncoderCodingParameter;
  h264::CreaterH264AVCEncoder*  m_pcH264AVCEncoder;
  WriteBitstreamToFile*         m_pcWriteBitstreamToFile;
  WriteYuvToFile*               m_apcWriteYuv           [MAX_LAYERS];
  ReadYuvFile*                  m_apcReadYuv            [MAX_LAYERS];

  PicBufferList                 m_acActivePicBufferList [MAX_LAYERS];
  PicBufferList                 m_acUnusedPicBufferList [MAX_LAYERS];
  UInt                          m_auiLumOffset          [MAX_LAYERS];
  UInt                          m_auiCbOffset           [MAX_LAYERS];
  UInt                          m_auiCrOffset           [MAX_LAYERS];
  UInt                          m_auiHeight             [MAX_LAYERS];
  UInt                          m_auiWidth              [MAX_LAYERS];
  UInt                          m_auiStride             [MAX_LAYERS];
  UInt                          m_aauiCropping          [MAX_LAYERS][4];

  UChar                         m_aucStartCodeBuffer[5];
  BinData                       m_cBinDataStartCode;
  std::string                   m_cWriteToBitFileName;
  std::string                   m_cWriteToBitFileTempName;
};




#endif //__H264AVCENCODERTEST_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
