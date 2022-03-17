
#ifndef __EXTRACTOR_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
#define __EXTRACTOR_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79


#define MAX_PACKET_SIZE 1000000
// JVT-S080 LMI
// If defined to 1, the initial scalability_info SEI should be updated after an extraction;
// Otherwise, it's unchanged and followed by a layers_not_present scalable SEI.
#define UPDATE_SCALABLE_SEI 1
#include "H264AVCCommonLib/Sei.h"

#include "ReadBitstreamFile.h"
#include "WriteBitstreamToFile.h"
#include "ExtractorParameter.h"
#define MAX_ROIS      5
#define MAX_QLAYERS   (MAX_TLAYERS*MAX_QUALITY_LEVELS)
#define MAX_TLAYERS   8


enum NalUnitType
{
  NAL_UNIT_UNSPECIFIED_0            =  0,
  NAL_UNIT_CODED_SLICE              =  1,
  NAL_UNIT_CODED_SLICE_DATAPART_A   =  2,
  NAL_UNIT_CODED_SLICE_DATAPART_B   =  3,
  NAL_UNIT_CODED_SLICE_DATAPART_C   =  4,
  NAL_UNIT_CODED_SLICE_IDR          =  5,
  NAL_UNIT_SEI                      =  6,
  NAL_UNIT_SPS                      =  7,
  NAL_UNIT_PPS                      =  8,
  NAL_UNIT_ACCESS_UNIT_DELIMITER    =  9,
  NAL_UNIT_END_OF_SEQUENCE          = 10,
  NAL_UNIT_END_OF_STREAM            = 11,
  NAL_UNIT_FILLER_DATA              = 12,
  NAL_UNIT_SPS_EXTENSION            = 13,
  NAL_UNIT_PREFIX										= 14,
  NAL_UNIT_SUBSET_SPS               = 15,
  NAL_UNIT_RESERVED_16              = 16,
  NAL_UNIT_RESERVED_17              = 17,
  NAL_UNIT_RESERVED_18              = 18,
  NAL_UNIT_AUX_CODED_SLICE          = 19,
  NAL_UNIT_CODED_SLICE_SCALABLE     = 20,
  NAL_UNIT_RESERVED_21              = 21,
  NAL_UNIT_RESERVED_22              = 22,
  NAL_UNIT_RESERVED_23              = 23
};


class ScalableStreamDescription
{
public:
  ScalableStreamDescription   ();
  ~ScalableStreamDescription  ();

  ErrVal  init      ( h264::SEI::ScalableSei* pcScalableSei );
  ErrVal  uninit    ();
  ErrVal  addPacket ( UInt                    uiNumBytes,
                      UInt                    uiLayer,
                      UInt                    uiLevel,
                      UInt                    uiFGSLayer,
                      Bool                    bNewPicture );
  //S051{
  ErrVal  addPacketNoUse (  UInt                    uiNumBytes,
                            UInt                    uiLayer,
                            UInt                    uiLevel,
                            UInt                    uiFGSLayer,
                            Bool                    bNewPicture );
  ErrVal addPic( h264::PacketDescription& rcPacketDescription );

  UInt64  getNALUBytesNoUse (  UInt uiLayer,
                               UInt uiLevel,
                               UInt uiFGS   )    const { return m_aaaui64NumNALUBytesNoUse[uiLayer][uiLevel][uiFGS]; }

  //S051}

  ErrVal  analyse   ();
  Void    output    ( );

  UInt    getNumberOfLayers ()                  const { return m_uiNumLayers; }
  UInt    getNumOfScalableLayers()              const { return m_uiScalableNumLayersMinus1 + 1; }
  UInt    getNumberOfScalableLayers ( UInt uiLayer, UInt uiTL, UInt uiQL ) const { return m_aaauiScalableLayerId[uiLayer][uiTL][uiQL]; }
  UInt    getBitrateOfScalableLayers( UInt uiScalableLayer ) const { return m_auiBitrate[uiScalableLayer]; }
  UInt    getDependencyId           ( UInt uiScalableLayer ) const { return m_auiDependencyId[uiScalableLayer]; }
  UInt    getTempLevel              ( UInt uiScalableLayer ) const { return m_auiTempLevel[uiScalableLayer]; }
  UInt    getFGSLevel               ( UInt uiScalableLayer ) const { return m_auiQualityLevel[uiScalableLayer]; }
  UInt    getFrmWidth               ( UInt uiScalableLayer ) const { return m_auiFrmWidth[uiScalableLayer]; }
  UInt    getFrmHeight              ( UInt uiScalableLayer ) const { return m_auiFrmHeight[uiScalableLayer]; }
  Double  getFrameRate              ( UInt uiScalableLayer ) const { return m_adFramerate[uiScalableLayer]; }
  Bool    getBaseLayerModeAVC       ()                       const { return m_bAVCBaseLayer;                }
  UInt    getScalableLayer  ( UInt uiLayer, UInt uiTL, UInt uiQL )
                                              const { return m_aaauiScalableLayerId[uiLayer][uiTL][uiQL]; }
  Void    setBaseLayerMode  ( Bool bAVCCompatible  )       { m_bAVCBaseLayer = bAVCCompatible;      }
  UInt    getFrameWidth     ( UInt uiLayer )    const { return m_auiFrameWidth  [uiLayer]; }
  UInt    getFrameHeight    ( UInt uiLayer )    const { return m_auiFrameHeight [uiLayer]; }

  UInt    getMaxLevel       ( UInt uiLayer )    const { return m_auiDecStages   [uiLayer]; }
  UInt    getNumPictures    ( UInt uiLayer,
                              UInt uiLevel )    const { return m_aauiNumPictures[uiLayer][uiLevel]; }
  UInt64  getNALUBytes      ( UInt uiLayer,
                              UInt uiLevel,
                              UInt uiFGS   )    const { return m_aaaui64NumNALUBytes[uiLayer][uiLevel][uiFGS]; }
  //add France Telecom
  Double getFrameRate(UInt uiExtLayer, UInt uiLevel) { return m_aaadFramerate[uiExtLayer][uiLevel][0];}
  //~add France Telecom
  Bool    m_bSPSRequired[MAX_LAYERS][32];
  Bool    m_bSubsetSPSRequired[MAX_LAYERS][32];
  Bool    m_bPPSRequired[MAX_LAYERS][256];



private:
  Bool    m_bInit;
  Bool    m_bAnalyzed;

  UInt    m_uiNumLayers;
  Bool    m_bAVCBaseLayer;
	UInt    m_uiMaxDecStages;
  UInt    m_auiFrameWidth       [MAX_LAYERS];
  UInt    m_auiFrameHeight      [MAX_LAYERS];
  UInt    m_auiDecStages        [MAX_LAYERS];

  //S051{
  UInt64  m_aaaui64NumNALUBytesNoUse [MAX_LAYERS][MAX_DSTAGES+1][MAX_QUALITY_LEVELS];
  //S051}

  // for performing proportional extraction of FGS layer with fragments, a dirty solution
   //NS extractor memory fix begin
  //#define MAX_NUM_PICTURES          1200
  //UInt    m_aaaauiPictureNALUBytes  [MAX_NUM_PICTURES][MAX_LAYERS][MAX_QUALITY_LEVELS];
  //NS extractor memory fix end

  UInt64  m_aaaui64NumNALUBytes [MAX_LAYERS][MAX_DSTAGES+1][MAX_QUALITY_LEVELS];
  UInt64  m_aaui64BaseLayerBytes[MAX_LAYERS][MAX_DSTAGES+1];
  UInt64  m_aaui64FGSLayerBytes [MAX_LAYERS][MAX_DSTAGES+1];
  UInt    m_aauiNumPictures     [MAX_LAYERS][MAX_DSTAGES+1];
	UInt    m_uiScalableNumLayersMinus1;
  UInt    m_auiBitrate              [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_auiTempLevel            [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_auiDependencyId         [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_auiQualityLevel         [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  Double  m_adFramerate             [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_auiFrmWidth             [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_auiFrmHeight            [MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];
  UInt    m_aaauiScalableLayerId    [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiBitrate            [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiTempLevel          [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiDependencyId       [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiQualityLevel       [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  Double  m_aaadFramerate           [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiFrmWidth           [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt    m_aaauiFrmHeight          [MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
};



class Extractor
{
protected:
  Extractor();
  virtual ~Extractor();

public:
  static ErrVal create              ( Extractor*&         rpcExtractor );
  ErrVal        init                ( ExtractorParameter* pcExtractorParameter );
  ErrVal        go                  ();
  ErrVal        destroy             ();

  // ROI ICU/ETRI
  Int     ROI_ID[8][8];
  void    setROI_ID(UInt did, UInt sg_id,Int value){ROI_ID[did][sg_id]=value;};
  Int     getROI_ID(UInt did, UInt sg_id){if(sg_id ==-1) return -1;  return ROI_ID[did][sg_id];};
  void    init_ROI_ID()
  {
    for(int i=0;i<8;i++)
    for(int j=0;j<8;j++)
      ROI_ID[i][j]= -1;
  }
  Int      getNumSlice      ()    const { return   m_iNumSlice; };


 //JVT-S080 LMI {
  ErrVal        xWriteScalableSEIToBuffer( h264::SEI::SEIMessage* pcScalableSei, BinData* pcBinData );
 //JVT-S080 LMI }
  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  //if there is R/D information (with or without Dead substreams)
  ErrVal        go_QL                  ();
  //}}Quality level estimation and modified truncation- JVTO044 and m12007

  //S051{
  ErrVal    go_SIP();
  //S051}

protected:
  ErrVal        xAnalyse            ();
  ErrVal        xPrimaryAnalyse     ();
  ErrVal        xSetParameters      ();
  ErrVal        xExtractPoints      ();
  ErrVal        xExtractLayerLevel  ();

  ErrVal  xExtractMaxRate ( Double  dMaxRate,
                            Bool    bDontTruncQLayer,
                            Bool    bPercentageMode );
  ErrVal  xAnalyse        ( UInt    uiTargetLayer,
                            Double& rdFrameRate,
                            Bool&   rbQualityLayerPresent,
                            UInt    auiQLRate[],
                            Bool    bNoSpecialFirstFrame );
  ErrVal  xExtract        ( UInt    uiTargetLayer,
                            Bool    bQualityLayerPresent,
                            Int     iQualityLayer,
                            UInt&   ruiInLayerRate,
                            Bool    bNoSpecialFirstFrame );

  // ROI ICU/ETRI
  Int CurNalKeepingNeed(h264::PacketDescription cPacketDescription
                 , const ExtractorParameter::Point& rcExtPoint);

  void          xSetROIParameters   ();
  ErrVal        GetPictureDataKeep  ( h264::PacketDescription* pcPacketDescription,
		                                  Double dRemainingBytes,
																			Double dCurrPacketBytes,
																			Bool& bKeep );
	ErrVal        xResetSLFGSBitrate  ( UInt uiDependencyId, UInt uiTempLevel, UInt uiFGSLayer, Double dDecBitrate ); //cleaning
  ErrVal        xReCalculateBr      ();
  Void          xMakeDepLayerList   ( UInt uiScalableLayerId, std::list<UInt>& DepLayerList );


  Void          setBaseLayerAVCCompatible( Bool bAVCCompatible ) { m_bAVCCompatible = bAVCCompatible; }
  Bool          getBaseLayerAVCCompatible() const { return m_bAVCCompatible; }
  // JVT-S080 LMI {
  ErrVal        xChangeScalableSEIMessage( BinData *pcBinData,
		                                        BinData *pcBinDataSEI,
																						h264::SEI::SEIMessage* pcScalableSEIMessage,
                                            UInt uiKeepScalableLayer,
																						UInt& uiWantedScalableLayer,
																						UInt& uiMaxLayer,
																						UInt& uiMaxTempLevel,
																						Double& dMaxFGSLayer,
																						UInt uiMaxBitrate    );
  // JVT-S080 LMI }
  // HS: packet trace
  ErrVal        xReadLineExtractTrace ( const Char*         ,
                                        UInt*               puiStart,
                                        UInt*               puiLength );
  ErrVal        xExtractTrace         ();

  //initialize temporal level of a frame
  Void setLevel(      UInt          uiLayer,
                      UInt          uiLevel,
                      UInt          uiNumImage );
  //intialize max rate for a frame from SEI dead substream information
  Void setMaxRateDS(  UInt          uiMaxRate,
                      UInt          uiLayer,
                      UInt          uiNumImage );
  //count size of packets for each frame
  Void addPacket(     UInt          uiLayer,
                      UInt          uiLevel,
                      UInt          uiFGSLayer,
                      UInt          uiNumBytes );

  //determine layer, level and target rate for output stream
  ErrVal xGetExtParameters();
  ErrVal GetAndCheckBaseLayerPackets( Double& dRemainingBytes );
	UInt   GetWantedScalableLayer();
  //search optimal quality for target rate
  ErrVal QualityLevelSearch(Bool bOrderedTopLayerTrunc);
  //extract NALs given optimal quality
  // ErrVal ExtractPointsFromRate();

  //JVT-S043 : Added the parameters uiMinTruncLayer & uiMaxTruncLayer.
  //get total rate for a given quality
  Double GetTotalRateForQualityLevel(Double QualityLevel, UInt uiExtLevel, UInt uiExtLayer,
                                     UInt uiMinTruncLayer, UInt uiMaxTruncLayer);
  //intialize R/D arrays from SEI information
  Void setQualityLevel();
  //get image rate for a given quality
  Double GetImageRateForQualityLevel(UInt uiLayer, UInt uiNumImage, Double QualityLevel,
                                     UInt uiMinTruncLayer, UInt uiMaxTruncLayer);
  //Calculate max rate for each frame of a layer (in case of dead substreams use of SEI information
  // from dead substreams)
  Void CalculateMaxRate(UInt uiLayer);
  //}}Quality level estimation and modified truncation- JVTO044 and m12007

  UInt getPIDIndex(UInt uiPID);
  UInt addPIDToTable(UInt uiPID);
  Double GetTruncatedRate(Double dQL, UInt uiExtLevel,  UInt uiExtLayer,
                          UInt uiMinTruncLayer, UInt uiMaxTruncLayer);
  UInt GetNearestPIDForQualityLevel(UInt uiLayer, UInt uiNumImage, Double QualityLevel);
  Double GetImageRateForQualityLevelActual(UInt uiLayer, UInt uiNumImage, Double QualityLevel, Double dRatio,
                                           UInt uiMinTruncLayer, UInt uiMaxTruncLayer);
  Double CalculateSizeOfIncludedLayers(UInt uiExtLevel, UInt uiExtLayer);
  Double CalculateSizeOfBQLayers(UInt uiExtLevel, UInt uiExtLayer);
  Double CalculateSizeOfMaxQuality(UInt uiExtLevel, UInt uiExtLayer);

  Bool IsFrameToCut(UInt uiFrame);
  Void AllocateAndInitializeDatas();

  //S051{
  ErrVal        xSetParameters_SIP      ();
  //ErrVal        xExtractPoints_SIP      ();
  //S051}
	ErrVal        xCalcSIPBitrate         (); //cleaning, add function

  UInt getScalableLayerIdFromActualLayerId( UInt uiActualLayerId ) const { return m_auiScalableLayerIdFromActualLayerId[uiActualLayerId]; }

protected:
  ReadBitstreamIf*              m_pcReadBitstream;
  WriteBitstreamIf*             m_pcWriteBitstream;
  ExtractorParameter*           m_pcExtractorParameter;
  h264::H264AVCPacketAnalyzer*  m_pcH264AVCPacketAnalyzer;

  UChar                         m_aucStartCodeBuffer[5];
  BinData                       m_cBinDataStartCode;

  ScalableStreamDescription     m_cScalableStreamDescription;
  Double                        m_aadTargetSNRLayer[MAX_LAYERS][MAX_DSTAGES+1];

  // HS: packet trace
  FILE*                         m_pcTraceFile;
  FILE*                         m_pcExtractionTraceFile;
  LargeFile                     m_cLargeFile;
  UInt                          m_uiMaxSize;

  Void    xOutput          ( );
  UInt    getScalableLayer ( UInt uiLayer, UInt uiTL, UInt uiQL ) const 
  {
    return m_aaauiScalableLayerId[uiLayer][uiTL][uiQL]; 
  }
  
  Bool                          m_bAVCCompatible;
  UInt                          m_uiScalableNumLayersMinus1;
  UInt                          m_aaauiScalableLayerId[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  UInt                          m_auiDependencyId[MAX_SCALABLE_LAYERS];
  UInt                          m_auiTempLevel[MAX_SCALABLE_LAYERS];
  UInt                          m_auiQualityLevel[MAX_SCALABLE_LAYERS];
  UInt                          m_auiFrmWidth[MAX_SCALABLE_LAYERS];
  UInt                          m_auiFrmHeight[MAX_SCALABLE_LAYERS];
  Double                        m_adFramerate[MAX_SCALABLE_LAYERS];
  UInt                          m_auiDirDepLayerDelta[MAX_SCALABLE_LAYERS][2];
  std::list<UInt>               m_acDepLayerList[MAX_SCALABLE_LAYERS];
  Double                        m_adFrameRate[MAX_TEMP_LEVELS];
  Double                        m_aadMinBitrate[MAX_LAYERS][MAX_TEMP_LEVELS];
  Double                        m_aaadSingleBitrate[MAX_LAYERS][MAX_TEMP_LEVELS][MAX_QUALITY_LEVELS];
  Double                        m_adTotalBitrate[MAX_SCALABLE_LAYERS];
  
  // a map from actual layer_id value in the Scalable SEI message to m_aaauiScalableLayerId
  UInt                          m_auiScalableLayerIdFromActualLayerId[MAX_LAYERS*MAX_TEMP_LEVELS*MAX_QUALITY_LEVELS];

  //JVT-W046 {
	//SEI changes update {
 // Bool													m_bAvc_Layer_Conversion_Flag[MAX_SCALABLE_LAYERS];
	//UInt													m_uiAvc_Conversion_Type_Idc[MAX_SCALABLE_LAYERS];
	//Bool													m_bAvc_Info_Flag[MAX_SCALABLE_LAYERS][2];
	//UInt													m_uiAvc_Profile_Level_Idc[MAX_SCALABLE_LAYERS][2];
	//UInt												  m_dAvc_Avg_Bitrate[MAX_SCALABLE_LAYERS][2];
	//UInt												  m_dAvc_Max_Bitrate[MAX_SCALABLE_LAYERS][2];
	Bool													m_bLayer_Conversion_Flag[MAX_SCALABLE_LAYERS];
	UInt													m_uiConversion_Type_Idc[MAX_SCALABLE_LAYERS];
	Bool													m_bRewriting_Info_Flag[MAX_SCALABLE_LAYERS][2];
	UInt													m_uiRewriting_Profile_Level_Idc[MAX_SCALABLE_LAYERS][2];
	UInt												  m_dRewriting_Avg_Bitrate[MAX_SCALABLE_LAYERS][2];
	UInt												  m_dRewriting_Max_Bitrate[MAX_SCALABLE_LAYERS][2];
//JVT-W046 }
//SEI changes update }
  //{{Quality level estimation and modified truncation- JVTO044 and m12007
  //France Telecom R&D-(nathalie.cammas@francetelecom.com)
  Double*           m_aaadMaxRate[MAX_LAYERS]; //size of each frame for each layer without deadsubstream
  Double*           m_aaadTargetBytesFGS[MAX_LAYERS][MAX_QUALITY_LEVELS]; //bytes to be extracted for each FGS layer for each frame                                               // at each layer
  Int*              m_aaiLevelForFrame[MAX_LAYERS];//temporal level of each frame
  Double*           m_aaadBytesForFrameFGS[MAX_LAYERS][MAX_QUALITY_LEVELS]; //size of each FGS layer for each frame at each layer
  Bool              m_bInInputStreamQL;// indicate if RD informations are in the input bitstream
  Double*           m_aadTargetByteForFrame[MAX_LAYERS];
  UInt*             m_aaauiBytesForQualityLevel[MAX_LAYERS][MAX_NUM_RD_LEVELS];
	Double*           m_aaadPriorityLevel[MAX_LAYERS][MAX_NUM_RD_LEVELS];//SEI changes update
  Int*              m_aaiNumLevels[MAX_LAYERS];
  UInt              m_auiNbImages[MAX_LAYERS];
  //}}Quality level estimation and modified truncation- JVTO044 and m12007
  UInt              m_uiExtractNonRequiredPics;
  UInt m_uiQualityId;
  UInt m_auiPID[64];
  UInt m_uiNbPID;
  Bool m_bQualityLevelInSEI; //indicates if QualityLayers are in SEI messages

	UInt m_uiTruncateLayer;
	UInt m_uiTruncateLevel;
	UInt m_uiTruncateFGSLayer;

  //S051{
  Bool              m_bUseSIP;
  Double            m_aadTargetSNRLayerNoUse[MAX_LAYERS][MAX_DSTAGES+1];
  UInt              m_uiPreAndSuffixUnitEnable;
  //S051}

  //-- ICU/ETRI ROI
  int               m_iNumSlice;
  int               m_aiSilceIDOfSubPicLayer[MAX_SCALABLE_LAYERS];
  int               m_aaiRelatedROIofSubPicLayer[MAX_SCALABLE_LAYERS][MAX_ROIS];
  int               m_aiDepIDOfSubPicLayer[MAX_SCALABLE_LAYERS];
  UInt              m_auiXinFirstMB[MAX_LAYERS][MAX_ROI_NUM];
  UInt              m_auiYinFirstMB[MAX_LAYERS][MAX_ROI_NUM];
  UInt              m_auiXinLastMB[MAX_LAYERS][MAX_ROI_NUM];
  UInt              m_auiYinLastMB[MAX_LAYERS][MAX_ROI_NUM];
  UInt              m_auiAddrFirstMBofROIs[MAX_LAYERS][MAX_ROI_NUM];
  UInt              m_auiAddrLastMBofROIs[MAX_LAYERS][MAX_ROI_NUM];
};

class ExtractStop{};

#endif //__EXTRACTOR_H_D65BE9B4_A8DA_11D3_AFE7_005004464B79
