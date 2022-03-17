
#ifndef _QUALITY_LEVEL_ASSIGNER_H
#define _QUALITY_LEVEL_ASSIGNER_H


#include "Typedefs.h"

#include "ReadBitstreamFile.h"  //bug-fix suffix
#include "H264AVCDecoderLib.h"
#include "CreaterH264AVCDecoder.h"
#include "H264AVCEncoderLib.h"
#include "QualityLevelEstimation.h"
#include "../src/lib/H264AVCEncoderLib/BitCounter.h"
#include "../src/lib/H264AVCEncoderLib/BitWriteBuffer.h"
#include "../src/lib/H264AVCEncoderLib/UvlcWriter.h"
#include "../src/lib/H264AVCEncoderLib/NalUnitEncoder.h"

class ReadBitstreamFile;
class WriteBitstreamToFile;
class ReadYuvFile;
class QualityLevelParameter;



class QualityLevelAssigner
{
public:
  QualityLevelAssigner();
  ~QualityLevelAssigner();

  static ErrVal   create  ( QualityLevelAssigner*&  rpcQualityLevelAssigner );
  ErrVal          destroy ();
  ErrVal          init    ( QualityLevelParameter*  pcQualityLevelParameter );
  ErrVal          go      ();

protected:
  //====== initialization ======
  ErrVal          xInitStreamParameters       ();

  //====== picture buffer and NAL unit handling ======
  ErrVal          xGetNewPicBuffer            ( PicBuffer*&           rpcPicBuffer,
                                                UInt                  uiSize );
  ErrVal          xRemovePicBuffer            ( PicBufferList&        rcPicBufferUnusedList );
  ErrVal          xClearPicBufferLists        ();
  ErrVal          xGetNextValidPacket         ( BinData*&             rpcBinData,
                                                ReadBitstreamFile*    pcReadBitStream,
                                                UInt                  uiTopLayer,
                                                UInt                  uiLayer,
                                                UInt                  uiFGSLayer,
                                                UInt                  uiLevel,
                                                Bool                  bIndependent,
                                                Bool&                 rbEOS,
                                                UInt*                 auiFrameNum );

  //====== get rate and distortion ======
  ErrVal          xInitRateAndDistortion      (Bool bMultiLayer);
  ErrVal          xInitRateValues             ();
  ErrVal          xInitDistortion             ( UInt*                 auiDistortion,
                                                UInt                  uiTopLayer,
                                                UInt                  uiLayer,
                                                UInt                  uiFGSLayer,
                                                UInt                  uiLevel      = MSYS_UINT_MAX,
                                                Bool                  bIndependent = false );
  ErrVal          xGetDistortion              ( UInt&                 ruiDistortion,
                                                const UChar*          pucReconstruction,
                                                const UChar*          pucReference,
                                                UInt                  uiHeight,
                                                UInt                  uiWidth,
                                                UInt                  uiStride );
  Double          xLog                        ( UInt                  uiDistortion )  { if( uiDistortion ) return log10( (Double)uiDistortion ); else return 0; }

  //====== read from and write to data file =====
  ErrVal          xWriteDataFile              ( const std::string&    cFileName );
  ErrVal          xReadDataFile               ( const std::string&    cFileName );

  //====== determine and write quality id's =====
  ErrVal          xDetermineQualityIDs        ();
  ErrVal          xWriteQualityLayerStreamPID ();
  ErrVal          xWriteQualityLayerStreamSEI ();
  ErrVal          xUpdateScalableSEI          ();
 ErrVal          xInsertPriorityLevelSEI      ( WriteBitstreamToFile* pcWriteBitStream,
                                                UInt                  uiLayer,
                                                UInt                  uiFrameNum );
	//SEI changes update }
  //JVT-S043
  ErrVal          xDetermineMultiLayerQualityIDs        ();
  //SEI changes update {
	//ErrVal          xInsertMultiLayerQualityLayerSEI ( WriteBitstreamToFile* pcWriteBitStream,
 //                                                    UInt                  uiLayer,
 //                                                    UInt                  uiFrameNum );
	ErrVal          xInsertMultiLayerPriorityLevelSEI ( WriteBitstreamToFile* pcWriteBitStream,
                                                     UInt                  uiLayer,
                                                     UInt                  uiFrameNum );
	//SEI changes update }
private:
  QualityLevelParameter*        m_pcParameter;
  h264::H264AVCPacketAnalyzer*  m_pcH264AVCPacketAnalyzer;
  h264::CreaterH264AVCDecoder*  m_pcH264AVCDecoder;
    //bug-fix suffix{{
  ReadBitstreamIf*            m_pcReadBitstream;
  //bug-fix suffix}}
  // for SEI writing
  h264::BitCounter*             m_pcBitCounter;
  h264::BitWriteBuffer*         m_pcBitWriteBuffer;
  h264::UvlcWriter*             m_pcUvlcWriter;
  h264::UvlcWriter*             m_pcUvlcTester;
  h264::NalUnitEncoder*         m_pcNalUnitEncoder;

  Bool                          m_bOutputReconstructions;
  UInt                          m_uiNumLayers;
  UInt                          m_auiNumFGSLayers     [MAX_LAYERS];
  UInt                          m_auiNumFrames        [MAX_LAYERS];
  UInt                          m_auiGOPSize          [MAX_LAYERS];
  UInt                          m_auiNumTempLevel     [MAX_LAYERS];
  UInt                          m_auiFrameWidth       [MAX_LAYERS];
  UInt                          m_auiFrameHeight      [MAX_LAYERS];
  UInt                          m_auiSPSRequired      [32];
  UInt                          m_auiSubsetSPSRequired[32];
  UInt                          m_auiPPSRequired      [256];
  Double*                       m_aaadDeltaDist       [MAX_LAYERS][MAX_QUALITY_LEVELS];
  UInt*                         m_aaauiPacketSize     [MAX_LAYERS][MAX_QUALITY_LEVELS];
  UInt*                         m_aaauiQualityID      [MAX_LAYERS][MAX_QUALITY_LEVELS];
  UInt*                         m_aauiPicNumToFrmID   [MAX_LAYERS];

  PicBufferList                 m_cActivePicBufferList;
  PicBufferList                 m_cUnusedPicBufferList;

  UChar                         m_aucStartCodeBuffer[5];
  BinData                       m_cBinDataStartCode;
  UInt                          m_uiPriorityLevelSEISize;
  UInt*                         m_aaauiNewPacketSize   [MAX_LAYERS][MAX_QUALITY_LEVELS];
  UInt                          m_aauiPrFrameRate      [MAX_LAYERS][MAX_SIZE_PID];
  UInt*                         m_aauiBaseIndex        [MAX_LAYERS];
};



#endif // _QUALITY_LEVEL_ASSIGNER_H
