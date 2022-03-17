
#if !defined(AFX_SEI_H__06FFFAD0_FB36_4BF0_9392_395C7389C1F4__INCLUDED_)
#define AFX_SEI_H__06FFFAD0_FB36_4BF0_9392_395C7389C1F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#include "H264AVCCommonLib/CommonBuffers.h"
#include "H264AVCCommonLib/HeaderSymbolReadIf.h"
#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"
// JVT-V068 HRD {
#include "H264AVCCommonLib/Vui.h"
#include "H264AVCCommonLib/Hrd.h"
// JVT-V068 HRD }
#include <list>

#define MAX_NUM_LAYER 6



H264AVC_NAMESPACE_BEGIN



class ParameterSetMng;


#if defined( WIN32 )
# pragma warning( disable: 4251 )
#endif


class H264AVCCOMMONLIB_API SEI
{

public:
  enum MessageType
  {
    BUFFERING_PERIOD                      = 0,
    PIC_TIMING                            = 1,
    SCENE_INFO_SEI                        = 9,
    SUB_SEQ_INFO                          = 10,
    MOTION_SEI                            = 18,
    SCALABLE_SEI                          = 24,
    SUB_PIC_SEI                           = 25,
    NON_REQUIRED_SEI                      = 26,
    PRIORITYLEVEL_SEI                     = 27,
    SCALABLE_SEI_LAYERS_NOT_PRESENT       = 28,
    SCALABLE_SEI_DEPENDENCY_CHANGE        = 29,
    SCALABLE_NESTING_SEI                  = 30,
    AVC_COMPATIBLE_HRD_SEI                = 31,
    INTEGRITY_CHECK_SEI										= 32,
    REDUNDANT_PIC_SEI                     = 33,
    TL0_DEP_REP_IDX_SEI                   = 34,
    TL_SWITCHING_POINT_SEI                = 35,
    RESERVED_SEI                          = 36
  };


  class H264AVCCOMMONLIB_API SEIMessage
  {
  public:
    virtual ~SEIMessage()                                                       {}
    MessageType     getMessageType()                                      const { return m_eMessageType; }
    virtual ErrVal  write         ( HeaderSymbolWriteIf* pcWriteIf ) = 0;
    virtual ErrVal  read          ( HeaderSymbolReadIf*  pcReadIf, ParameterSetMng* pcParameterSetMng ) = 0;

  protected:
    SEIMessage( MessageType eMessageType) : m_eMessageType( eMessageType ) {}

  private:
    MessageType m_eMessageType;
  };



  class H264AVCCOMMONLIB_API SubSeqInfo : public SEIMessage
  {
  protected:
    SubSeqInfo()
      : SEIMessage(SUB_SEQ_INFO)
      , m_uiSubSeqLayerNum      (0)
      , m_uiSubSeqId            (0)
      , m_bFirstRefPicFlag      (false)
      , m_bLeadingNonRefPicFlag (false)
      , m_bLastPicFlag          (false)
      , m_bSubSeqFrameNumFlag   (false)
      , m_uiSubSeqFrameNum      (0)
    {}

  public:
    static ErrVal create( SubSeqInfo*&          rpcSEIMessage );
    ErrVal        write ( HeaderSymbolWriteIf*  pcWriteIf );
    ErrVal        read  ( HeaderSymbolReadIf*   pcReadIf, ParameterSetMng* pcParameterSetMng );
    ErrVal        init  ( UInt                  uiSubSeqLayerNum,
                          UInt                  uiSubSeqId,
                          Bool                  bFirstRefPicFlag,
                          Bool                  bLeadingNonRefPicFlag,
                          Bool                  bLastPicFlag        = false,
                          Bool                  bSubSeqFrameNumFlag = false,
                          UInt                  uiSubSeqFrameNum    = 0 );

    UInt getSubSeqId      ()  const { return m_uiSubSeqId; }
    UInt getSubSeqLayerNum()  const { return m_uiSubSeqLayerNum; }

  private:
    UInt  m_uiSubSeqLayerNum;
    UInt  m_uiSubSeqId;
    Bool  m_bFirstRefPicFlag;
    Bool  m_bLeadingNonRefPicFlag;
    Bool  m_bLastPicFlag;
    Bool  m_bSubSeqFrameNumFlag;
    UInt  m_uiSubSeqFrameNum;
  };

  class H264AVCCOMMONLIB_API ScalableSei: public SEIMessage
  {
  protected:
    ScalableSei ();
    virtual ~ScalableSei();

  public:
    static ErrVal create ( ScalableSei*&      rpcSeiMessage);
//TMM_FIX
    ErrVal    destroy ();
//TMM_FIX
    ErrVal write         ( HeaderSymbolWriteIf  *pcWriteIf);
    ErrVal read           ( HeaderSymbolReadIf    *pcReadIf, ParameterSetMng* pcParameterSetMng);

		Void setTlevelNestingFlag( Bool bFlag )                                   { m_temporal_id_nesting_flag = bFlag;                         }//SEI changes update
		Void setPriorityIdSettingFlag( Bool bFlag )                               { m_priority_id_setting_flag = bFlag;                         }//JVT-W053
		Void setPriorityIdSettingUri (UInt index,UChar ucchar )										{ priority_id_setting_uri[index] = ucchar;                    }//JVT-W053
		Void setNumLayersMinus1( UInt ui )                                        { m_num_layers_minus1 = ui;                                   }
    Void setLayerId ( UInt uilayer, UInt uiId )                               { m_layer_id                             [uilayer] = uiId;    }
  //JVT-S036 lsj start
    Void setPriorityId ( UInt uilayer, UInt uiLevel )                         { m_priority_id                          [uilayer] = uiLevel; }//SEI changes update
		Void setDiscardableFlag  (UInt uilayer, Bool bFlag)                       { m_discardable_flag                     [uilayer] = bFlag;   }
    Void setTemporalId ( UInt uilayer, UInt uiLevel )                      { m_temporal_level                       [uilayer] = uiLevel; }
    Void setDependencyId ( UInt uilayer, UInt uiId )                          { m_dependency_id                        [uilayer] = uiId;    }
    Void setQualityLevel ( UInt uilayer, UInt uiLevel )                        { m_quality_level                       [uilayer] = uiLevel; }
    Void setSubPicLayerFlag ( UInt uilayer, Bool bFlag)                        { m_sub_pic_layer_flag[uilayer] = bFlag; }
    Void setSubRegionLayerFlag ( UInt uilayer, Bool bFlag)                    { m_sub_region_layer_flag                  [uilayer] = bFlag; }
		Void setIroiSliceDivisionInfoPresentFlag ( UInt uilayer, Bool bFlag )        { m_iroi_division_info_present_flag    [uilayer] = bFlag; }//JVT-W051
    Void setProfileLevelInfoPresentFlag ( UInt uilayer, Bool bFlag)            { m_profile_level_info_present_flag        [uilayer] = bFlag; }
  //JVT-S036 lsj end

    Void setBitrateInfoPresentFlag ( UInt uilayer, Bool bFlag )                { m_bitrate_info_present_flag              [uilayer] = bFlag; }
    Void setFrmRateInfoPresentFlag ( UInt uilayer, Bool bFlag )                { m_frm_rate_info_present_flag            [uilayer] = bFlag; }
    Void setFrmSizeInfoPresentFlag ( UInt uilayer, Bool bFlag )                { m_frm_size_info_present_flag            [uilayer] = bFlag; }
    Void setLayerDependencyInfoPresentFlag ( UInt uilayer, Bool bFlag )        { m_layer_dependency_info_present_flag    [uilayer] = bFlag; }
    Void setParameterSetsInfoPresentFlag ( UInt uilayer, Bool bFlag )       { m_parameter_sets_info_present_flag  [uilayer] = bFlag; }//SEI changes update
    Void setExactInterlayerPredFlag ( UInt uilayer, Bool bFlag )            { m_exact_interlayer_pred_flag  [uilayer] = bFlag; }        //JVT-S036 lsj
		//SEI changes update {
		Void setExactSampleValueMatchFlag ( UInt uilayer, Bool bFlag )            { m_exact_sample_value_match_flag  [uilayer] = bFlag; }
//JVT-W046 {
		Void setLayerConversionFlag ( UInt uilayer, Bool bFlag )                        { m_layer_conversion_flag [uilayer]         = bFlag;     }
		Void setRewritingInfoFlag ( UInt uilayer, UInt bType, Bool bFlag )              { m_rewriting_info_flag             [uilayer][bType]  = bFlag;     }
		Void setConversionTypeIdc ( UInt uilayer, UInt uiIdc )                          { m_conversion_type_idc   [uilayer]                   = uiIdc;     }
		Void setRewritingProfileLevelIdc ( UInt uilayer, UInt bType, UInt uiIdc )      { m_rewriting_profile_level_idc     [uilayer][bType]  = uiIdc;     }
		Void setRewritingAvgBitrateBPS ( UInt uilayer, UInt bType, Double dBitrate )        { m_rewriting_avg_bitrate           [uilayer][bType]  = xConvertFromBPS( dBitrate ); }
		Void setRewritingMaxBitrateBPS ( UInt uilayer, UInt bType, Double dBitrate )        { m_rewriting_max_bitrate           [uilayer][bType]  = xConvertFromBPS( dBitrate ); }
//JVT-W046 }
		//SEI changes update }
		Void setLayerOutputFlag      ( UInt uilayer,Bool bFlag )                       {m_layer_output_flag													[uilayer] = bFlag; }//JVT-W047 wxwan
    Void setLayerProfileLevelIdc ( UInt uilayer, UInt uiIdc )                      { m_layer_profile_level_idc[uilayer] = uiIdc; }
  //JVT-S036 lsj start
    Void setAvgBitrateBPS ( UInt uilayer, Double dBitrate )                        { m_avg_bitrate                    [uilayer] = xConvertFromBPS( dBitrate ); }
    Void setMaxBitrateLayerBPS ( UInt uilayer, Double dBitrate )                    { m_max_bitrate_layer                [uilayer] = xConvertFromBPS( dBitrate ); }
		Void setMaxBitrateDecodedPictureBPS ( UInt uilayer, Double dBitrate )                { m_max_bitrate_layer_representation            [uilayer] = xConvertFromBPS( dBitrate ); }//JVT-W051
    Void setMaxBitrateCalcWindow ( UInt uilayer, UInt uiBitrate )                  { m_max_bitrate_calc_window              [uilayer] = uiBitrate; }
  //JVT-S036 lsj end


    Void setConstantFrmRateIdc ( UInt uilayer, UInt uiFrmrate )                { m_constant_frm_rate_idc                  [uilayer] = uiFrmrate; }
    Void setAvgFrmRate ( UInt uilayer, UInt uiFrmrate )                        { m_avg_frm_rate                          [uilayer] = uiFrmrate; }
    Void setFrmWidthInMbsMinus1 ( UInt uilayer, UInt uiWidth )                { m_frm_width_in_mbs_minus1                [uilayer] = uiWidth; }
    Void setFrmHeightInMbsMinus1 ( UInt uilayer, UInt uiHeight )              { m_frm_height_in_mbs_minus1              [uilayer] = uiHeight; }
    Void setBaseRegionLayerId ( UInt uilayer, UInt uiId )                      { m_base_region_layer_id                  [uilayer] = uiId; }
    Void setDynamicRectFlag ( UInt uilayer, Bool bFlag )                      { m_dynamic_rect_flag                      [uilayer] = bFlag; }
    Void setHorizontalOffset ( UInt uilayer, UInt uiOffset )                  { m_horizontal_offset                      [uilayer] = uiOffset; }
    Void setVerticalOffset ( UInt uilayer, UInt uiOffset )                    { m_vertical_offset                        [uilayer] = uiOffset; }
    Void setRegionWidth ( UInt uilayer, UInt uiWidth )                        { m_region_width                          [uilayer] = uiWidth; }
    Void setRegionHeight ( UInt uilayer, UInt uiHeight )                      { m_region_height                          [uilayer] = uiHeight; }
  //JVT-S036 lsj start
    Void setRoiId ( UInt uilayer, UInt RoiId )                        { m_roi_id[uilayer]  = RoiId; }
		Void setIroiGridFlag ( UInt uilayer, Bool bFlag )                { m_iroi_grid_flag[uilayer] = bFlag; }//SEI changes update
		Void setGridSliceWidthInMbsMinus1 ( UInt uilayer, UInt bWidth )              { m_grid_width_in_mbs_minus1[uilayer] = bWidth; }//JVT-W051
		Void setGridSliceHeightInMbsMinus1 ( UInt uilayer, UInt bHeight )            { m_grid_height_in_mbs_minus1[uilayer] = bHeight; }//JVT-W051

    Void setROINum(UInt iDependencyId, UInt iNumROI)      { m_aiNumRoi[iDependencyId] = iNumROI; }
    Void setROIID(UInt iDependencyId, UInt* iROIId)
    {
      for (UInt i =0; i < m_aiNumRoi[iDependencyId]; ++i)
      {
        m_aaiRoiID[iDependencyId][i] = iROIId[i];
      }
    }
    Void setSGID(UInt iDependencyId, UInt* iSGId)
    {
      for (UInt i =0; i < m_aiNumRoi[iDependencyId]; ++i)
      {
        m_aaiSGID[iDependencyId][i] = iSGId[i];
      }
    }
    Void setSLID(UInt iDependencyId, UInt* iSGId)
    {
      for (UInt i =0; i < m_aiNumRoi[iDependencyId]; ++i)
      {
        m_aaiSLID[iDependencyId][i] = iSGId[i];
      }
    }

    // JVT-S054 (REPLACE) ->
    Void setNumSliceMinus1 ( UInt uilayer, UInt bNum )
    {
      if ( m_num_rois_minus1[uilayer] != bNum )
      {
        if ( m_first_mb_in_roi[uilayer] != NULL )
        {
          free(m_first_mb_in_roi[uilayer]);
          m_first_mb_in_roi[uilayer] = NULL;
        }
        if ( m_roi_width_in_mbs_minus1[uilayer] != NULL )
        {
          free(m_roi_width_in_mbs_minus1[uilayer]);
          m_roi_width_in_mbs_minus1[uilayer] = NULL;
        }
        if ( m_roi_height_in_mbs_minus1[uilayer] != NULL )
        {
          free(m_roi_height_in_mbs_minus1[uilayer]);
          m_roi_height_in_mbs_minus1[uilayer] = NULL;
        }
      }

      m_num_rois_minus1[uilayer] = bNum;

      if ( m_first_mb_in_roi[uilayer] == NULL )
        m_first_mb_in_roi[uilayer] = (UInt*)malloc((bNum+1)*sizeof(UInt));

      if ( m_roi_width_in_mbs_minus1[uilayer] == NULL )
        m_roi_width_in_mbs_minus1[uilayer] = (UInt*)malloc((bNum+1)*sizeof(UInt));

      if ( m_roi_height_in_mbs_minus1[uilayer] == NULL )
        m_roi_height_in_mbs_minus1[uilayer] = (UInt*)malloc((bNum+1)*sizeof(UInt));
    }
    // JVT-S054 (REPLACE) <-

    Void setFirstMbInSlice ( UInt uilayer, UInt uiTar, UInt bNum )              { m_first_mb_in_roi[uilayer][uiTar] = bNum; }
    Void setSliceWidthInMbsMinus1 ( UInt uilayer, UInt uiTar, UInt bWidth )          { m_roi_width_in_mbs_minus1[uilayer][uiTar] = bWidth; }
    Void setSliceHeightInMbsMinus1 ( UInt uilayer, UInt uiTar, UInt bHeight )        { m_roi_height_in_mbs_minus1[uilayer][uiTar] = bHeight; }
    //JVT-S036 lsj end
    Void setNumDirectlyDependentLayers ( UInt uilayer, UInt uiNum )            { m_num_directly_dependent_layers          [uilayer] = uiNum; }
    Void setDirectlyDependentLayerIdDeltaMinus1( UInt uilayer, UInt uiTar, UInt uiDelta ) { m_directly_dependent_layer_id_delta_minus1[uilayer][uiTar] = uiDelta;} ///JVT-S036 lsj
    Void setLayerDependencyInfoSrcLayerIdDelta( UInt uilayer, UInt uiDelta )      { m_layer_dependency_info_src_layer_id_delta      [uilayer] = uiDelta;} //JVT-S036 lsj
    //SEI changes update {
		Void setNumInitSeqParameterSetMinus1         ( UInt uilayer, UInt uiNum )             { m_num_seq_parameter_set_minus1             [uilayer] = uiNum;          }
    Void setInitSeqParameterSetIdDelta           ( UInt uilayer, UInt uiSPS, UInt uiTar)  { m_seq_parameter_set_id_delta               [uilayer][uiSPS] = uiTar;   }
    Void setNumInitSubsetSeqParameterSetMinus1   ( UInt uilayer, UInt uiNum )             { m_num_subset_seq_parameter_set_minus1      [uilayer] = uiNum;          }
    Void setInitSubsetSeqParameterSetIdDelta     ( UInt uilayer, UInt uiSSPS, UInt uiTar) { m_subset_seq_parameter_set_id_delta        [uilayer][uiSSPS] = uiTar;  }
		Void setNumInitPicParameterSetMinus1         ( UInt uilayer, UInt uiNum )             { m_num_pic_parameter_set_minus1             [uilayer] = uiNum;          }
    Void setInitPicParameterSetIdDelta           ( UInt uilayer, UInt uiPPS, UInt uiTar)  { m_pic_parameter_set_id_delta               [uilayer][uiPPS] = uiTar;   }
    Void setInitParameterSetsInfoSrcLayerIdDelta ( UInt uilayer, UInt uiDelta)            { m_parameter_sets_info_src_layer_id_delta   [uilayer] = uiDelta;        } //JVT-S036 lsj
    //SEI changes update }
		// BUG_FIX liuhui{
    Void setStdAVCOffset( UInt uiOffset )                                     { m_std_AVC_Offset = uiOffset;}
    UInt getStdAVCOffset()const { return m_std_AVC_Offset; }
// BUG_FIX liuhui}

    // JVT-U085 LMI
		Bool getTlevelNestingFlag() const { return m_temporal_id_nesting_flag; }//SEI changes update
		//JVT-W053 wxwan
		Bool getPriorityIdSettingFlag() const { return m_priority_id_setting_flag ; }
		Char* getPriorityIdSetUri() { return priority_id_setting_uri; }
		UChar getPriorityIdSettingUri (UInt index) const { return priority_id_setting_uri[index]; }
		//JVT-W053 wxwan

    UInt getNumLayersMinus1() const {return m_num_layers_minus1;}
    UInt getLayerId ( UInt uilayer ) const { return m_layer_id[uilayer]; }
   //JVT-S036 lsj start
    UInt getPriorityId ( UInt uilayer ) const { return  m_priority_id [uilayer]; }//SEI changes update
    Bool getDiscardableFlag  (UInt uilayer) const { return  m_discardable_flag [uilayer]; }
    UInt getTemporalId ( UInt uilayer ) const { return m_temporal_level[uilayer]; }
    UInt getDependencyId ( UInt uilayer ) const { return m_dependency_id[uilayer]; }
    UInt getQualityId ( UInt uilayer ) const { return m_quality_level[uilayer]; }

    Bool getSubPicLayerFlag ( UInt uilayer ) { return m_sub_pic_layer_flag[uilayer]; }
    Bool getSubRegionLayerFlag ( UInt uilayer ) const { return m_sub_region_layer_flag[uilayer]; }
		Bool getIroiSliceDivisionInfoPresentFlag ( UInt uilayer ) const { return m_iroi_division_info_present_flag[uilayer]; }//JVT-W051
    Bool getProfileLevelInfoPresentFlag ( UInt uilayer ) const { return m_profile_level_info_present_flag[uilayer]; }
   //JVT-S036 lsj end
    Bool getBitrateInfoPresentFlag ( UInt uilayer ) const { return m_bitrate_info_present_flag[uilayer]; }
    Bool getFrmRateInfoPresentFlag ( UInt uilayer ) const { return m_frm_rate_info_present_flag[uilayer]; }
    Bool getFrmSizeInfoPresentFlag ( UInt uilayer ) const { return m_frm_size_info_present_flag[uilayer]; }
    Bool getLayerDependencyInfoPresentFlag ( UInt uilayer ) const { return m_layer_dependency_info_present_flag[uilayer]; }
		Bool getParameterSetsInfoPresentFlag ( UInt uilayer ) const { return m_parameter_sets_info_present_flag[uilayer]; }//SEI changes update

    Bool getExactInterlayerPredFlag ( UInt uilayer )  const { return m_exact_interlayer_pred_flag  [uilayer]; }        //JVT-S036 lsj
		//SEI changes update {
    Bool getExactSampleValueMatchFlag ( UInt uilayer )  const { return m_exact_sample_value_match_flag  [uilayer]; }
		//JVT-W046 {
		Bool   getLayerConversionFlag ( UInt uilayer )                   const { return m_layer_conversion_flag          [uilayer];        }
		Bool   getRewritingInfoFlag ( UInt uilayer, UInt bType )         const { return m_rewriting_info_flag            [uilayer][bType]; }
		UInt   getConversionTypeIdc ( UInt uilayer )                     const { return m_conversion_type_idc            [uilayer];        }
		UInt   getRewritingProfileLevelIdc ( UInt uilayer, UInt bType )  const { return m_rewriting_profile_level_idc    [uilayer][bType]; }
		UInt   getRewritingAvgBitrateBPS ( UInt uilayer, UInt bType )       const { return xConvertToBPS( m_rewriting_avg_bitrate[uilayer][bType] ); }
		UInt   getRewritingMaxBitrateBPS ( UInt uilayer, UInt bType )       const { return xConvertToBPS( m_rewriting_max_bitrate[uilayer][bType] ); }
    UInt   getRewritingAvgBitrateCode( UInt uilayer, UInt bType )       const { return m_rewriting_avg_bitrate[uilayer][bType]; }
    UInt   getRewritingMaxBitrateCode( UInt uilayer, UInt bType )       const { return m_rewriting_max_bitrate[uilayer][bType]; }
		//JVT-W046 }
    //SEI changes update }
		//JVT-W047 wxwan
		Bool getLayerOutputFlag( UInt uilayer ) const { return m_layer_output_flag[uilayer]; }
		//JVT-W047 wxwan
    UInt getLayerProfileLevelIdc( UInt uilayer ) const { return m_layer_profile_level_idc[uilayer]; }
  //JVT-S036 lsj start

    UInt getAvgBitrateCode ( UInt uilayer ) const { return m_avg_bitrate[uilayer]; }
    UInt getMaxBitrateLayerCode ( UInt uilayer ) const { return m_max_bitrate_layer[uilayer]; }
    UInt getMaxBitrateDecodedPictureCode ( UInt uilayer ) const { return m_max_bitrate_layer_representation[uilayer]; }
    UInt getAvgBitrateBPS ( UInt uilayer ) const { return xConvertToBPS( m_avg_bitrate[uilayer] ); }
    UInt getMaxBitrateLayerBPS ( UInt uilayer ) const { return xConvertToBPS( m_max_bitrate_layer[uilayer] ); }
		UInt getMaxBitrateDecodedPictureBPS ( UInt uilayer ) const { return xConvertToBPS( m_max_bitrate_layer_representation[uilayer] ); }
    UInt getMaxBitrateCalcWindow ( UInt uilayer ) const { return m_max_bitrate_calc_window[uilayer]; }
  //JVT-S036 lsj end


    UInt getConstantFrmRateIdc ( UInt uilayer ) const { return m_constant_frm_rate_idc[uilayer]; }
    UInt getAvgFrmRate ( UInt uilayer ) const { return m_avg_frm_rate[uilayer]; }
    UInt getFrmWidthInMbsMinus1 ( UInt uilayer ) const { return m_frm_width_in_mbs_minus1[uilayer]; }
    UInt getFrmHeightInMbsMinus1 ( UInt uilayer ) const { return m_frm_height_in_mbs_minus1[uilayer]; }
    UInt getBaseRegionLayerId ( UInt uilayer ) const { return m_base_region_layer_id[uilayer]; }
    Bool getDynamicRectFlag ( UInt uilayer ) const { return m_dynamic_rect_flag[uilayer]; }
    UInt getHorizontalOffset ( UInt uilayer ) const { return m_horizontal_offset[uilayer]; }
    UInt getVerticalOffset ( UInt uilayer ) const { return m_vertical_offset[uilayer]; }
    UInt getRegionWidth ( UInt uilayer ) const { return m_region_width[uilayer]; }
    UInt getRegionHeight ( UInt uilayer ) const { return m_region_height[uilayer]; }
  //JVT-S036 lsj start
    UInt getRoiId ( UInt uilayer ) const { return m_roi_id[uilayer]; }
		//SEI changes update {
		Bool getIroiGridFlag ( UInt uilayer ) const { return m_iroi_grid_flag[uilayer]; }
		UInt getGridSliceWidthInMbsMinus1 ( UInt uilayer ) const { return m_grid_width_in_mbs_minus1[uilayer]; }//JVT-W051
		UInt getGridSliceHeightInMbsMinus1 ( UInt uilayer ) const { return m_grid_height_in_mbs_minus1[uilayer]; }//JVT-W051
    UInt getNumSliceMinus1 ( UInt uilayer ) const { return m_num_rois_minus1[uilayer]; }
    UInt getFirstMbInSlice ( UInt uilayer, UInt uiTar )  const { return m_first_mb_in_roi[uilayer][uiTar]; }
    UInt getSliceWidthInMbsMinus1 ( UInt uilayer, UInt uiTar ) const { return m_roi_width_in_mbs_minus1[uilayer][uiTar]; }
    UInt getSliceHeightInMbsMinus1 ( UInt uilayer, UInt uiTar ) const { return m_roi_height_in_mbs_minus1[uilayer][uiTar]; }
		//SEI changes update }
  //JVT-S036 lsj end

    UInt getNumDirectlyDependentLayers ( UInt uilayer ) const { return m_num_directly_dependent_layers[uilayer]; }
// BUG_FIX liuhui{
    UInt getNumDirectlyDependentLayerIdDeltaMinus1( UInt uilayer, UInt uiIndex ) const { return m_directly_dependent_layer_id_delta_minus1[uilayer][uiIndex]; } //JVT-S036 lsj
// BUG_FIX liuhui}
    UInt getLayerDependencyInfoSrcLayerIdDelta( UInt uilayer ) const { return m_layer_dependency_info_src_layer_id_delta[uilayer];} //JVT-S036 lsj
    //SEI changes update {
		UInt getNumInitSPSMinus1 ( UInt uilayer ) const { return m_num_seq_parameter_set_minus1[uilayer];        }
		UInt getNumInitSSPSMinus1( UInt uilayer ) const { return m_num_subset_seq_parameter_set_minus1[uilayer]; }
		UInt getNumInitPPSMinus1 ( UInt uilayer ) const { return m_num_pic_parameter_set_minus1[uilayer];        }
// BUG_FIX liuhui{
    UInt getInitSPSIdDelta  ( UInt uilayer, UInt uiIndex ) const { return m_seq_parameter_set_id_delta[uilayer][uiIndex];        }
		UInt getInitSSPSIdDelta ( UInt uilayer, UInt uiIndex ) const { return m_subset_seq_parameter_set_id_delta[uilayer][uiIndex]; }
    UInt getInitPPSIdDelta  ( UInt uilayer, UInt uiIndex ) const { return m_pic_parameter_set_id_delta[uilayer][uiIndex];        }
// BUG_FIX liuhui}
    UInt getInitParameterSetsInfoSrcLayerIdDelta ( UInt uilayer ) const { return m_parameter_sets_info_src_layer_id_delta[uilayer]; } //JVT-S036 lsj
		//JVT-W051 {
		Bool getPriorityLayerInfoPresentFlag ( void ) const { return m_priority_layer_info_present_flag; }
		Bool getBitstreamRestrictionInfoPresentFlag ( UInt uilayer ) const { return m_bitstream_restriction_info_present_flag[uilayer]; }
		//SEI changes update }
		Bool getMotionVectorsOverPicBoundariesFlag ( UInt uilayer ) const { return m_motion_vectors_over_pic_boundaries_flag [uilayer]; }
		UInt getMaxBytesPerPicDenom ( UInt uilayer ) const { return m_max_bytes_per_pic_denom [uilayer]; }
		UInt getMaxBitsPerMbDenom ( UInt uilayer ) const { return m_max_bits_per_mb_denom [uilayer]; }
		UInt getLog2MaxMvLengthHorizontal ( UInt uilayer ) const { return m_log2_max_mv_length_horizontal [uilayer]; }
		UInt getLog2MaxMvLengthVertical ( UInt uilayer ) const { return m_log2_max_mv_length_vertical [uilayer]; }
		UInt getMaxDecFrameBuffering ( UInt uilayer ) const { return m_max_dec_frame_buffering [uilayer]; }
		UInt getNumReorderFrames ( UInt uilayer ) const { return m_num_reorder_frames [uilayer]; }
		//SEI changes update {
    UInt getPrNumdIdMinus1 ( void ) const { return m_pr_num_dId_minus1; }
		UInt getPrNumMinus1 ( UInt uilayer ) const { return m_pr_num_minus1 [uilayer]; }
		UInt getPrDependencyId ( UInt uilayer ) const { return m_pr_dependency_id [uilayer]; }
		UInt getPrId ( UInt uilayer, UInt uiIndex ) const { return m_pr_id [uilayer][uiIndex]; }
		UInt getPrProfileLevelIdc ( UInt uilayer, UInt uiIndex ) const { return m_pr_profile_level_idc [uilayer][uiIndex]; }
    UInt getPrAvgBitrateCode( UInt uilayer, UInt uiIndex ) const { return m_pr_avg_bitrate [uilayer][uiIndex]; }
    UInt getPrMaxBitrateCode( UInt uilayer, UInt uiIndex ) const { return m_pr_max_bitrate [uilayer][uiIndex]; }
		UInt getPrAvgBitrateBPS ( UInt uilayer, UInt uiIndex ) const { return xConvertToBPS( m_pr_avg_bitrate [uilayer][uiIndex] ); }
		UInt getPrMaxBitrateBPS ( UInt uilayer, UInt uiIndex ) const { return xConvertToBPS( m_pr_max_bitrate [uilayer][uiIndex] ); }
		void setPriorityLayerInfoPresentFlag ( Bool bFlag ) { m_priority_layer_info_present_flag = bFlag; }
		void setBitstreamRestrictionInfoPresentFlag ( UInt uilayer, Bool bFlag ){ m_bitstream_restriction_info_present_flag [uilayer] = bFlag; }
    //SEI changes update }
		void setMotionVectorsOverPicBoundariesFlag ( UInt uilayer, Bool bFlag ) { m_motion_vectors_over_pic_boundaries_flag [uilayer] = bFlag; }
		void setMaxBytesPerPicDenom ( UInt uilayer, UInt uiMaxBytesPerPicDenom ) { m_max_bytes_per_pic_denom [uilayer] = uiMaxBytesPerPicDenom; }
		void setMaxBitsPerMbDenom ( UInt uilayer, UInt uiMaxBitsPerMbDenom ) { m_max_bits_per_mb_denom [uilayer] = uiMaxBitsPerMbDenom; }
		void setLog2MaxMvLengthHorizontal ( UInt uilayer, UInt uiLog2MaxMvLengthHorizontal ) { m_log2_max_mv_length_horizontal [uilayer] = uiLog2MaxMvLengthHorizontal; }
		void setLog2MaxMvLengthVertical ( UInt uilayer, UInt uiLog2MaxMvLengthVertical ) { m_log2_max_mv_length_vertical [uilayer] = uiLog2MaxMvLengthVertical; }
		void setMaxDecFrameBuffering ( UInt uilayer, UInt uiMaxDecFrameBuffering ) { m_max_dec_frame_buffering [uilayer] = uiMaxDecFrameBuffering; }
		void setNumReorderFrames ( UInt uilayer, UInt uiNumReorderFrames ) { m_num_reorder_frames [uilayer] = uiNumReorderFrames; }
		//SEI changes update {
    void setPrNumdIdMinus1 (UInt uiPrNumdIdMinus1) { m_pr_num_dId_minus1 = uiPrNumdIdMinus1; }
		void setPrNumMinus1 ( UInt uilayer, UInt uiPrNumMinus1 ) { m_pr_num_minus1 [uilayer] = uiPrNumMinus1; }
		void setPrDependencyId ( UInt uilayer, UInt uiPrDependencyId ) { m_pr_dependency_id [uilayer] = uiPrDependencyId; }
		void setPrId ( UInt uilayer, UInt uiIndex, UInt uiPrId ) { m_pr_id [uilayer][uiIndex] = uiPrId; }
		void setPrProfileLevelIdx ( UInt uilayer, UInt uiIndex, UInt uiPrProfileLevelIdc ) { m_pr_profile_level_idc [uilayer][uiIndex] = uiPrProfileLevelIdc; }
		void setPrAvgBitrateBPS ( UInt uilayer, UInt uiIndex, Double dPrAvgBitrate ) { m_pr_avg_bitrate [uilayer][uiIndex] = xConvertFromBPS(dPrAvgBitrate); }
		void setPrMaxBitrateBPS ( UInt uilayer, UInt uiIndex, Double dPrMaxBitrate ) { m_pr_max_bitrate [uilayer][uiIndex] = xConvertFromBPS(dPrMaxBitrate); }
    //JVT-W051 }
    //SEI changes update }

protected:
    static UInt  xConvertToBPS( UInt x )
    {
      const UInt auiScale[4] = { 100, 1000, 10000, 100000 };
      return ( x & 0x3fff ) * auiScale[ x >> 14 ];
    }

    static UInt  xConvertFromBPS( Double d )
    {
      const Double adScale[4] = { 100.0, 1000.0, 10000.0, 100000.0 };
      for( UInt uiExp = 0; uiExp < 4; uiExp++ )
      {
        UInt x = (UInt)floor( d / adScale[ uiExp ] + 0.5 );
        if( x < ( 1 << 14 ) )
        {
          return ( ( uiExp << 14 ) | x );
        }
      }
      AOT(1);
      return MSYS_UINT_MAX;
    }

private:
// BUG_FIX liuhui{
    UInt m_std_AVC_Offset;
// BUG_FIX liuhui}
    // JVT-U085 LMI
    Bool m_temporal_id_nesting_flag;//SEI changes update
		Bool m_priority_id_setting_flag;//JVT-W053
		char priority_id_setting_uri[20];//JVT-W053

    UInt m_num_layers_minus1;
    UInt m_layer_id[MAX_SCALABLE_LAYERS];
  //JVT-S036 lsj start
    UInt m_priority_id[MAX_SCALABLE_LAYERS];//SEI changes
    Bool m_discardable_flag[MAX_SCALABLE_LAYERS];
    UInt m_temporal_level[MAX_SCALABLE_LAYERS];
    UInt m_dependency_id[MAX_SCALABLE_LAYERS];
    UInt m_quality_level[MAX_SCALABLE_LAYERS];

    Bool m_sub_pic_layer_flag[MAX_SCALABLE_LAYERS];
    Bool m_sub_region_layer_flag[MAX_SCALABLE_LAYERS];
		//JVT-W051 {
		Bool m_iroi_division_info_present_flag[MAX_SCALABLE_LAYERS];
		//JVT-W051 }
    Bool m_profile_level_info_present_flag[MAX_SCALABLE_LAYERS];
  //JVT-S036 lsj end
    Bool m_bitrate_info_present_flag[MAX_SCALABLE_LAYERS];
    Bool m_frm_rate_info_present_flag[MAX_SCALABLE_LAYERS];
    Bool m_frm_size_info_present_flag[MAX_SCALABLE_LAYERS];
    Bool m_layer_dependency_info_present_flag[MAX_SCALABLE_LAYERS];
    //Bool m_init_parameter_sets_info_present_flag[MAX_SCALABLE_LAYERS];//SEI changes update
    Bool m_parameter_sets_info_present_flag[MAX_SCALABLE_LAYERS];//SEI changes update
    Bool m_exact_interlayer_pred_flag[MAX_SCALABLE_LAYERS];  //JVT-S036 lsj
		//SEI changes update {
		Bool m_exact_sample_value_match_flag[MAX_SCALABLE_LAYERS];
	//JVT-W046 {
		Bool    m_layer_conversion_flag[MAX_SCALABLE_LAYERS];
		Bool    m_rewriting_info_flag[MAX_SCALABLE_LAYERS][2];
		UInt    m_conversion_type_idc[MAX_SCALABLE_LAYERS];
		UInt   m_rewriting_profile_level_idc[MAX_SCALABLE_LAYERS][2];
		UInt    m_rewriting_avg_bitrate[MAX_SCALABLE_LAYERS][2];
		UInt    m_rewriting_max_bitrate[MAX_SCALABLE_LAYERS][2];
 //JVT-W046 }
	//SEI changes update }
		//JVT-W051 {
		UInt m_layer_profile_level_idc[MAX_SCALABLE_LAYERS];
		//JVT-W051 }
  //JVT-S036 lsj start



    UInt m_avg_bitrate[MAX_SCALABLE_LAYERS];
    UInt m_max_bitrate_layer[MAX_SCALABLE_LAYERS];//
		//JVT-W051 {
		UInt m_max_bitrate_layer_representation[MAX_SCALABLE_LAYERS];//
		//JVT-W051 }
    UInt m_max_bitrate_calc_window[MAX_SCALABLE_LAYERS];//

    UInt m_constant_frm_rate_idc[MAX_SCALABLE_LAYERS];
    UInt m_avg_frm_rate[MAX_SCALABLE_LAYERS];

    UInt m_frm_width_in_mbs_minus1[MAX_SCALABLE_LAYERS];
    UInt m_frm_height_in_mbs_minus1[MAX_SCALABLE_LAYERS];

    UInt m_base_region_layer_id[MAX_SCALABLE_LAYERS];
    Bool m_dynamic_rect_flag[MAX_SCALABLE_LAYERS];
    UInt m_horizontal_offset[MAX_SCALABLE_LAYERS];
    UInt m_vertical_offset[MAX_SCALABLE_LAYERS];
    UInt m_region_width[MAX_SCALABLE_LAYERS];
    UInt m_region_height[MAX_SCALABLE_LAYERS];

    UInt m_roi_id[MAX_SCALABLE_LAYERS]; //
    //SEI changes update {
		//JVT-W051 {
		Bool m_iroi_grid_flag[MAX_SCALABLE_LAYERS];
		UInt m_grid_width_in_mbs_minus1[MAX_SCALABLE_LAYERS]; //
		UInt m_grid_height_in_mbs_minus1[MAX_SCALABLE_LAYERS]; //
		UInt m_num_rois_minus1[MAX_SCALABLE_LAYERS];//
		//JVT-W051 }
		//JVT-W051 {
		UInt* m_first_mb_in_roi[MAX_SCALABLE_LAYERS];//
		UInt* m_roi_width_in_mbs_minus1[MAX_SCALABLE_LAYERS];//
		UInt* m_roi_height_in_mbs_minus1[MAX_SCALABLE_LAYERS];//
		//JVT-W051 }
    UInt m_num_directly_dependent_layers[MAX_SCALABLE_LAYERS];
    UInt m_directly_dependent_layer_id_delta_minus1[MAX_SCALABLE_LAYERS][MAX_SCALABLE_LAYERS];//

    UInt m_layer_dependency_info_src_layer_id_delta[MAX_SCALABLE_LAYERS];//
		UInt m_num_seq_parameter_set_minus1[MAX_SCALABLE_LAYERS];
    UInt m_seq_parameter_set_id_delta[MAX_SCALABLE_LAYERS][32];
		UInt m_num_subset_seq_parameter_set_minus1[MAX_SCALABLE_LAYERS];
    UInt m_subset_seq_parameter_set_id_delta[MAX_SCALABLE_LAYERS][32];
    UInt m_num_pic_parameter_set_minus1[MAX_SCALABLE_LAYERS];
    UInt m_pic_parameter_set_id_delta[MAX_SCALABLE_LAYERS][256];
// BUG_FIX liuhui}
    UInt m_parameter_sets_info_src_layer_id_delta[MAX_SCALABLE_LAYERS];//
  //JVT-S036 lsj end
		//SEI changes update }
		Bool m_layer_output_flag[MAX_SCALABLE_LAYERS];//JVT-W047 wxwan

    UInt m_aiNumRoi[MAX_SCALABLE_LAYERS];
    UInt m_aaiRoiID[MAX_SCALABLE_LAYERS][MAX_SCALABLE_LAYERS];
    UInt m_aaiSGID[MAX_SCALABLE_LAYERS][MAX_SCALABLE_LAYERS];
    UInt m_aaiSLID[MAX_SCALABLE_LAYERS][MAX_SCALABLE_LAYERS];
		//JVT-W051 & JVT064 {
		//SEI changes update {
    Bool m_priority_layer_info_present_flag;
		Bool m_bitstream_restriction_info_present_flag[MAX_SCALABLE_LAYERS];
		//SEI changes update }
		Bool m_motion_vectors_over_pic_boundaries_flag[MAX_SCALABLE_LAYERS];
		UInt m_max_bytes_per_pic_denom[MAX_SCALABLE_LAYERS];
		UInt m_max_bits_per_mb_denom[MAX_SCALABLE_LAYERS];
		UInt m_log2_max_mv_length_horizontal[MAX_SCALABLE_LAYERS];
		UInt m_log2_max_mv_length_vertical[MAX_SCALABLE_LAYERS];
		UInt m_num_reorder_frames[MAX_SCALABLE_LAYERS];
		UInt m_max_dec_frame_buffering[MAX_SCALABLE_LAYERS];
		//SEI changes update {
    UInt m_pr_num_dId_minus1;
		UInt m_pr_dependency_id[MAX_LAYERS];
		UInt m_pr_num_minus1[MAX_LAYERS];
    UInt m_pr_id[MAX_LAYERS][MAX_SIZE_PID];
		UInt m_pr_profile_level_idc[MAX_LAYERS][MAX_SIZE_PID];
		UInt m_pr_avg_bitrate[MAX_LAYERS][MAX_SIZE_PID];
		UInt m_pr_max_bitrate[MAX_LAYERS][MAX_SIZE_PID];
		//JVT-W051 & JVT064 }
    //SEI changes update }
  };

  class H264AVCCOMMONLIB_API SubPicSei : public SEIMessage
  {
  protected:
    SubPicSei ();
    ~SubPicSei();

  public:
    static ErrVal create  ( SubPicSei*&        rpcSeiMessage );
    ErrVal        write    ( HeaderSymbolWriteIf*  pcWriteIf );
    ErrVal        read    ( HeaderSymbolReadIf*    pcReadIf, ParameterSetMng* pcParameterSetMng  );

    UInt getDependencyId  ()          const  { return m_uiDependencyId;        }
    Void setDependencyId ( UInt uiLayerId) { m_uiDependencyId = uiLayerId;  }

  private:
    UInt m_uiDependencyId;
  };

  class H264AVCCOMMONLIB_API MotionSEI : public SEIMessage
  {

  protected:
    MotionSEI();
    ~MotionSEI();

  public:

    UInt m_num_slice_groups_in_set_minus1;
    UInt m_slice_group_id[8];
    Bool m_exact_sample_value_match_flag;
    Bool m_pan_scan_rect_flag;

    static ErrVal create  ( MotionSEI*&         rpcSeiMessage );
    ErrVal        write   ( HeaderSymbolWriteIf*  pcWriteIf );
    ErrVal        read    ( HeaderSymbolReadIf*   pcReadIf, ParameterSetMng* pcParameterSetMng );
    ErrVal        setSliceGroupId(UInt id);
  UInt          getSliceGroupId(){return m_slice_group_id[0];}
  };

  
  class H264AVCCOMMONLIB_API PriorityLevelSEI : public SEIMessage
  {
  protected:
    PriorityLevelSEI ();
    ~PriorityLevelSEI();

  public:
    static ErrVal create  ( PriorityLevelSEI*&         rpcSeiMessage );
    ErrVal        write   ( HeaderSymbolWriteIf*  pcWriteIf );
    ErrVal        read    ( HeaderSymbolReadIf*   pcReadIf, ParameterSetMng* pcParameterSetMng );

  UInt     getNumPriorityIds() { return m_uiNumPriorityIds;}
  Void     setNumPriorityIds(UInt ui) { m_uiNumPriorityIds = ui;}
  UInt     getAltPriorityId(UInt ui) { return m_auiAltPriorityId[ui];}
  Void     setAltPriorityId(UInt uiIndex, UInt ui) { m_auiAltPriorityId[uiIndex] = ui;}
  UInt     getPrDependencyId() { return m_uiPrDependencyId;}
  Void     setPrDependencyId( UInt ui) { m_uiPrDependencyId = ui;}

  private:
    UInt m_auiAltPriorityId[MAX_NUM_RD_LEVELS];
    UInt m_uiNumPriorityIds;
    UInt m_uiPrDependencyId;
  };
  //SEI changes update }

  class H264AVCCOMMONLIB_API NonRequiredSei : public SEIMessage
  {
  protected:
    NonRequiredSei ();
    ~NonRequiredSei();

  public:
    static ErrVal create  (NonRequiredSei*&      rpcSeiMessage);
    ErrVal    destroy ();
    ErrVal    write  (HeaderSymbolWriteIf*    pcWriteIf);
    ErrVal    read  (HeaderSymbolReadIf*    pcReadIf, ParameterSetMng* pcParameterSetMng);

    UInt      getNumInfoEntriesMinus1()          const{ return m_uiNumInfoEntriesMinus1;}
    UInt      getEntryDependencyId(UInt uiLayer)      const{ return m_uiEntryDependencyId[uiLayer];}
    UInt      getNumNonRequiredPicsMinus1(UInt uiLayer)  const{ return m_uiNumNonRequiredPicsMinus1[uiLayer];}
    UInt      getNonRequiredPicDependencyId(UInt uiLayer, UInt uiNonRequiredLayer)  const{ return m_uiNonRequiredPicDependencyId[uiLayer][uiNonRequiredLayer];}
    UInt      getNonRequiredPicQulityLevel(UInt uiLayer, UInt uiNonRequiredLayer)    const{ return m_uiNonRequiredPicQulityLevel[uiLayer][uiNonRequiredLayer];}
    UInt      getNonRequiredPicFragmentOrder(UInt uiLayer, UInt uiNonRequiredLayer)  const{ return m_uiNonRequiredPicFragmentOrder[uiLayer][uiNonRequiredLayer];}


    Void      setNumInfoEntriesMinus1(UInt ui)          { m_uiNumInfoEntriesMinus1 = ui;}
    Void      setEntryDependencyId(UInt uiLayer, UInt ui)      { m_uiEntryDependencyId[uiLayer] = ui;}
    Void      setNumNonRequiredPicsMinus1(UInt uiLayer, UInt ui)  { m_uiNumNonRequiredPicsMinus1[uiLayer] = ui;}
    Void      setNonNonRequiredPicDependencyId(UInt uiLayer, UInt uiNonRequiredLayer, UInt ui)    {m_uiNonRequiredPicDependencyId[uiLayer][uiNonRequiredLayer] = ui;}
    Void      setNonNonRequiredPicQulityLevel(UInt uiLayer, UInt uiNonRequiredLayer, UInt ui)      {m_uiNonRequiredPicQulityLevel[uiLayer][uiNonRequiredLayer] = ui;}
    Void      setNonNonRequiredPicFragmentOrder(UInt uiLayer, UInt uiNonRequiredLayer, UInt ui)    {m_uiNonRequiredPicFragmentOrder[uiLayer][uiNonRequiredLayer] = ui;}


  private:
    UInt    m_uiNumInfoEntriesMinus1;
    UInt    m_uiEntryDependencyId[MAX_NUM_INFO_ENTRIES];
    UInt    m_uiNumNonRequiredPicsMinus1[MAX_NUM_INFO_ENTRIES];
    UInt    m_uiNonRequiredPicDependencyId[MAX_NUM_INFO_ENTRIES][MAX_NUM_NON_REQUIRED_PICS];
    UInt    m_uiNonRequiredPicQulityLevel[MAX_NUM_INFO_ENTRIES][MAX_NUM_NON_REQUIRED_PICS];
    UInt    m_uiNonRequiredPicFragmentOrder[MAX_NUM_INFO_ENTRIES][MAX_NUM_NON_REQUIRED_PICS];
  };//shenqiu 05-09-15

  // JVT-S080 LMI {
  class H264AVCCOMMONLIB_API ScalableSeiLayersNotPresent: public SEIMessage
  {
  protected:
      ScalableSeiLayersNotPresent ();
   ~ScalableSeiLayersNotPresent();

  public:
      static ErrVal create ( ScalableSeiLayersNotPresent*&      rpcSeiMessage);
   //TMM_FIX
      ErrVal destroy ();
   //TMM_FIX
      ErrVal write         ( HeaderSymbolWriteIf  *pcWriteIf);
      ErrVal read           ( HeaderSymbolReadIf    *pcReadIf, ParameterSetMng* pcParameterSetMng);
      Void setNumLayers( UInt ui )                                        { m_uiNumLayers = ui;  }
      Void setLayerId ( UInt uiLayer, UInt uiId )                                { m_auiLayerId                              [uiLayer] = uiId; }
    Void setOutputFlag ( Bool bFlag )  { m_bOutputFlag = bFlag; }

      UInt getNumLayers() const {return m_uiNumLayers;}
      UInt getLayerId ( UInt uiLayer ) const { return m_auiLayerId[uiLayer]; }
    Bool getOutputFlag ( ) const { return m_bOutputFlag; }
      static UInt m_uiLeftNumLayers;
      static UInt m_auiLeftLayerId[MAX_SCALABLE_LAYERS];

  private:
      UInt m_uiNumLayers;
      UInt m_auiLayerId[MAX_SCALABLE_LAYERS];
    Bool m_bOutputFlag;

  };

  class H264AVCCOMMONLIB_API ScalableSeiDependencyChange: public SEIMessage
  {
  protected:
      ScalableSeiDependencyChange ();
   ~ScalableSeiDependencyChange();

  public:
      static ErrVal create ( ScalableSeiDependencyChange*&      rpcSeiMessage);
      ErrVal write         ( HeaderSymbolWriteIf  *pcWriteIf);
      ErrVal read           ( HeaderSymbolReadIf    *pcReadIf, ParameterSetMng* pcParameterSetMng);
      Void setNumLayersMinus1( UInt ui )                                        { m_uiNumLayersMinus1 = ui;  }
      Void setDependencyId ( UInt uiLayer, UInt uiId )                                { m_auiLayerId                              [uiLayer] = uiId; }
    Void setLayerDependencyInfoPresentFlag ( UInt uiLayer, Bool bFlag ) { m_abLayerDependencyInfoPresentFlag[uiLayer] = bFlag; }
      Void setNumDirectDependentLayers ( UInt uiLayer, UInt ui ) { m_auiNumDirectDependentLayers[uiLayer] = ui; }
    Void setDirectDependentLayerIdDeltaMinus1( UInt uiLayer, UInt uiDirectLayer, UInt uiIdDeltaMinus1 )  { m_auiDirectDependentLayerIdDeltaMinus1[uiLayer][uiDirectLayer] = uiIdDeltaMinus1; }
    Void setLayerDependencyInfoSrcLayerIdDeltaMinus1 ( UInt uiLayer, UInt uiIdDeltaMinus1 ) { m_auiLayerDependencyInfoSrcLayerIdDeltaMinus1[uiLayer] = uiIdDeltaMinus1; }
    Void setOutputFlag ( Bool bFlag )  { m_bOutputFlag = bFlag; }

      UInt getNumLayersMinus1() const {return m_uiNumLayersMinus1;}
      UInt getDependencyId ( UInt uiLayer ) const { return m_auiLayerId[uiLayer]; }
      UInt getNumDirectDependentLayers ( UInt uiLayer ) const { return m_auiNumDirectDependentLayers[uiLayer]; }
    UInt getDirectDependentLayerIdDeltaMinus1( UInt uiLayer, UInt uiDirectLayer ) const { return m_auiDirectDependentLayerIdDeltaMinus1[uiLayer][uiDirectLayer]; }
    UInt getLayerDependencyInfoSrcLayerIdDeltaMinus1 ( UInt uiLayer ) const { return m_auiLayerDependencyInfoSrcLayerIdDeltaMinus1[uiLayer]; }
    Bool getLayerDependencyInfoPresentFlag ( UInt uiLayer ) const { return m_abLayerDependencyInfoPresentFlag[uiLayer]; }
    Bool getOutputFlag ( ) const { return m_bOutputFlag; }

  private:
      UInt m_uiNumLayersMinus1;
      UInt m_auiLayerId[MAX_SCALABLE_LAYERS];
      UInt m_auiNumDirectDependentLayers[MAX_SCALABLE_LAYERS];
      UInt m_auiDirectDependentLayerIdDeltaMinus1[MAX_SCALABLE_LAYERS][MAX_SCALABLE_LAYERS];
      UInt m_auiLayerDependencyInfoSrcLayerIdDeltaMinus1[MAX_SCALABLE_LAYERS];
    Bool m_abLayerDependencyInfoPresentFlag[MAX_SCALABLE_LAYERS];
    Bool m_bOutputFlag;
  };

  // JVT-S080 LMI }
// JVT-T073 {
#define MAX_LREP_IN_ACCESS_UNIT 50
  class H264AVCCOMMONLIB_API ScalableNestingSei : public SEIMessage
  {
  protected:
    ScalableNestingSei()
      : SEIMessage(SCALABLE_NESTING_SEI)
      , m_bAllPicturesInAuFlag  (0)
    , m_uiNumPictures         (0)
    , m_bHasBufferingPeriod( false )
    //, m_pcSEIMessage          (NULL)
    {}

  public:
    static ErrVal create( ScalableNestingSei*&  rpcSEIMessage );
  ErrVal      destroy();
    ErrVal        write ( HeaderSymbolWriteIf*  pcWriteIf );
    ErrVal        read  ( HeaderSymbolReadIf*   pcReadIf, ParameterSetMng* pcParameterSetMng );
    ErrVal        init  ( Bool                  m_bAllPicturesInAuFlag,
                        UInt                  m_uiNumPictures,
                        UInt*                 m_auiDependencyId,
                        UInt*                 m_auiQualityLevel
            );
    Bool  bHasBufferingPeriod() const { return m_bHasBufferingPeriod; }

    Bool getAllPicturesInAuFlag()  const { return m_bAllPicturesInAuFlag; }
    UInt getNumPictures()          const { return m_uiNumPictures; }
  UInt getDependencyId( UInt uiIndex ) { return m_auiDependencyId[uiIndex]; }
  UInt getQualityId( UInt uiIndex ) { return m_auiQualityLevel[uiIndex]; }

  Void setAllPicturesInAuFlag( Bool bFlag ) { m_bAllPicturesInAuFlag = bFlag; }
  Void setNumPictures( UInt uiNum ) { m_uiNumPictures = uiNum; }
  Void setDependencyId( UInt uiIndex, UInt uiValue ) { m_auiDependencyId[uiIndex] = uiValue; }
  Void setQualityLevel( UInt uiIndex, UInt uiValue ) { m_auiQualityLevel[uiIndex] = uiValue; }

    // JVT-V068 {
    UInt getTemporalId() { return m_uiTemporalId; }
    Void setTemporalId( UInt uiValue ) { m_uiTemporalId = uiValue; }
    // JVT-V068 }
  //JVT-W062 {
  private:
    UInt  m_uiTemporalId;
  //JVT-W062 }
    Bool  m_bAllPicturesInAuFlag;
    UInt  m_uiNumPictures;
    UInt  m_auiDependencyId[MAX_LREP_IN_ACCESS_UNIT];
    UInt  m_auiQualityLevel[MAX_LREP_IN_ACCESS_UNIT];
    Bool  m_bHasBufferingPeriod;
  };


  //scene_info is taken as en example
  class H264AVCCOMMONLIB_API SceneInfoSei : public SEIMessage
  {
  protected:
    SceneInfoSei() : SEIMessage(SCENE_INFO_SEI)
    {}
  public:
    static ErrVal create( SceneInfoSei*& rpcSceneInfoSei );
    ErrVal    destroy ();
    ErrVal        write ( HeaderSymbolWriteIf*  pcWriteIf);
      ErrVal        read  ( HeaderSymbolReadIf*   pcReadIf , ParameterSetMng* pcParameterSetMng);

    Bool getSceneInfoPresentFlag() const { return m_bSceneInfoPresentFlag; }
    UInt getSceneId()              const { return m_uiSceneId; }
    UInt getSceneTransitionType()  const { return m_uiSceneTransitionType; }
    UInt getSecondSceneId()        const { return m_uiSecondSceneId; }
    Void setSceneInfoPresentFlag( Bool bFlag )          { m_bSceneInfoPresentFlag = bFlag; }
    Void setSceneId( UInt uiSceneId )                   { m_uiSceneId = uiSceneId; }
    Void setSceneTransitionType( UInt uiTransitionType) { m_uiSceneTransitionType = uiTransitionType; }
    Void setSecondSceneId( UInt uiSecondId )            { m_uiSecondSceneId = uiSecondId; }
  private:
    Bool m_bSceneInfoPresentFlag;
    UInt m_uiSceneId;
    UInt m_uiSceneTransitionType;
    UInt m_uiSecondSceneId;
  };
  // JVT-T073 }

  // JVT-V068 HRD {
  class H264AVCCOMMONLIB_API BufferingPeriod :
    public SEIMessage
  {

  public:
    class H264AVCCOMMONLIB_API SchedSel
    {
    public:
      SchedSel() : m_uiInitialCpbRemovalDelay(0), m_uiInitialCpbRemovalDelayOffset(0) {}
      SchedSel& operator = (const SchedSel& rcSchedSel)
      {
        m_uiInitialCpbRemovalDelay = rcSchedSel.m_uiInitialCpbRemovalDelay;
        m_uiInitialCpbRemovalDelayOffset = rcSchedSel.m_uiInitialCpbRemovalDelayOffset;
        return *this;
      }
      ErrVal write( HeaderSymbolWriteIf* pcWriteIf, const HRD& rcHrd );
      ErrVal read ( HeaderSymbolReadIf* pcReadIf,   const HRD& rcHrd );

      UInt getDelay() { return m_uiInitialCpbRemovalDelay; }
      UInt getDelayOffset() { return m_uiInitialCpbRemovalDelayOffset; }
      ErrVal setDelay( UInt uiInitialCpbRemovalDelay)
      {
        m_uiInitialCpbRemovalDelay = uiInitialCpbRemovalDelay;
        return Err::m_nOK;
      }
      ErrVal setDelayOffset( UInt uiInitialCpbRemovalDelayOffset)
      {
        m_uiInitialCpbRemovalDelayOffset = uiInitialCpbRemovalDelayOffset;
        return Err::m_nOK;
      }

    private:
      UInt m_uiInitialCpbRemovalDelay;
      UInt m_uiInitialCpbRemovalDelayOffset;
    };

  protected:
    BufferingPeriod( ParameterSetMng*& rpcParameterSetMng, Bool bNested, UInt uiDQId, UInt uiTId )
      : SEIMessage( BUFFERING_PERIOD )
      , m_pcParameterSetMng( rpcParameterSetMng)
      , m_bNested( bNested )
      , m_uiDQId( uiDQId )
      , m_uiTId( uiTId )
      , m_uiSeqParameterSetId( 0 ) 
    {
      m_apcHrd[0] = 0;
      m_apcHrd[1] = 0;
      m_abHrdParametersPresentFlag[0] = 0;
      m_abHrdParametersPresentFlag[1] = 0;
    }

  public:
    static ErrVal create( BufferingPeriod*& rpcBufferingPeriod, ParameterSetMng*& rpcParameterSetMng, Bool bNested = false, UInt uiDQId = 0, UInt uiTId = 0 );
    static ErrVal create( BufferingPeriod*& rpcBufferingPeriod, BufferingPeriod * pcBufferingPeriod );

    virtual ~BufferingPeriod();
    SchedSel& getSchedSel( HRD::HrdParamType eHrdParamType, UInt uiNum ) { return m_aacSchedSel[eHrdParamType].get( uiNum ); }
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf );
    ErrVal read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng );

    ErrVal setHRD( UInt uiSPSId, Bool bSubSetSPS, const HRD* apcHrd[] );

  private:
    const HRD* m_apcHrd[2];
    ParameterSetMng* m_pcParameterSetMng;

    Bool  m_bNested;
    UInt  m_uiDQId;
    UInt  m_uiTId;
    UInt m_uiSeqParameterSetId;
    Bool m_abHrdParametersPresentFlag[2];
    StatBuf <DynBuf< SchedSel >, 2>  m_aacSchedSel;
  };

  class H264AVCCOMMONLIB_API PicTiming :
    public SEIMessage
  {

  public:
    class H264AVCCOMMONLIB_API ClockTimestamp
    {

    public:
      ClockTimestamp()
        : m_bClockTimestampFlag ( false )
        , m_uiCtType            ( MSYS_UINT_MAX )
        , m_bNuitFieldBasedFlag ( false )
        , m_uiCountingType      ( MSYS_UINT_MAX )
        , m_bFullTimestampFlag  ( false )
        , m_bDiscontinuityFlag  ( false )
        , m_bCntDroppedFlag     ( false )
        , m_uiNFrames           ( MSYS_UINT_MAX )
        , m_uiSeconds           ( 0 )
        , m_uiMinutes           ( 0 )
        , m_uiHours             ( 0 )
        , m_iTimeOffset         ( 0 )
      {}

      ErrVal write( HeaderSymbolWriteIf* pcWriteIf, const HRD& rcHRD );
      ErrVal read ( HeaderSymbolReadIf* pcReadIf,   const HRD& rcHRD );
      Int  get( const VUI& rcVUI, UInt uiLayerIndex );
      Void set( const VUI& rcVUI, UInt uiLayerIndex, Int iTimestamp );
    private:
      Bool m_bClockTimestampFlag;
      UInt m_uiCtType;
      Bool m_bNuitFieldBasedFlag;
      UInt m_uiCountingType;
      Bool m_bFullTimestampFlag;
      Bool m_bDiscontinuityFlag;
      Bool m_bCntDroppedFlag;
      UInt m_uiNFrames;
      UInt m_uiSeconds;
      UInt m_uiMinutes;
      UInt m_uiHours;
      Int m_iTimeOffset;
    };

  protected:
    PicTiming( const VUI* pcVUI, UInt uiLayerIndex )
      : SEIMessage          ( PIC_TIMING )
      , m_pcVUI             ( pcVUI )
      , m_uiLayerIndex      ( uiLayerIndex )
      , m_uiCpbRemovalDelay ( MSYS_UINT_MAX )
      , m_uiDpbOutputDelay  ( MSYS_UINT_MAX )
      , m_ePicStruct        ( PS_NOT_SPECIFIED )
    {}

  public:
    static ErrVal create( PicTiming*& rpcPicTiming, const VUI* pcVUI, UInt uiLayerIndex );
    static ErrVal create( PicTiming*& rpcPicTiming, ParameterSetMng* pcParameterSetMng );
    static ErrVal create( PicTiming*& rpcPicTiming, ParameterSetMng* pcParameterSetMng, UInt uiSPSId, Bool bSubSetSPS, UInt uiLayerIndex );

    Int  getTimestamp( UInt uiNum = 0, UInt uiLayerIndex = 0 );
    ErrVal setTimestamp( UInt uiNum, UInt uiLayerIndex, Int iTimestamp );

    Int  getCpbRemovalDelay() { return m_uiCpbRemovalDelay; }
    ErrVal setCpbRemovalDelay( UInt uiCpbRemovalDelay );

    UInt  getDpbOutputDelay() { return m_uiDpbOutputDelay; }
    ErrVal setDpbOutputDelay( UInt uiDpbOutputDelay );

    UInt getNumClockTs();
    ErrVal write( HeaderSymbolWriteIf* pcWriteIf );
    ErrVal read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng );
    PicStruct getPicStruct() const               { return m_ePicStruct; }
    Void      setPicStruct(PicStruct ePicStruct) { m_ePicStruct = ePicStruct; }

  private:
    const VUI* m_pcVUI;
    UInt m_uiLayerIndex;

    UInt m_uiCpbRemovalDelay;
    UInt m_uiDpbOutputDelay;
    PicStruct m_ePicStruct;
    StatBuf< ClockTimestamp, 3 > m_acClockTimestampBuf;
  };

  class H264AVCCOMMONLIB_API AVCCompatibleHRD :
    public SEIMessage
  {
  public:

  protected:
    AVCCompatibleHRD( VUI* pcVUI )
      : SEIMessage          ( AVC_COMPATIBLE_HRD_SEI )
      , m_pcVUI             ( pcVUI )
    {}

  public:
    static ErrVal create( AVCCompatibleHRD*& rpcAVCCompatibleHRD, VUI* pcVUI );

    ErrVal write( HeaderSymbolWriteIf* pcWriteIf );
    ErrVal read ( HeaderSymbolReadIf* pcReadIf, ParameterSetMng* pcParameterSetMng );

  private:
    VUI* m_pcVUI;
  };
  // JVT-V068 HRD }

//JVT-W049 {
  class H264AVCCOMMONLIB_API RedundantPicSei: public SEIMessage
{
protected:
    RedundantPicSei ();
    ~RedundantPicSei ();

public:
    static ErrVal create  ( RedundantPicSei*&      rpcSeiMessage );
    ErrVal        write   ( HeaderSymbolWriteIf*   pcWriteIf     );
    ErrVal        read    ( HeaderSymbolReadIf*    pcReadIf , ParameterSetMng* pcParameterSetMng     );

  Void setNumDIdMinus1( UInt ui )                                                                       { m_num_dId_minus1 = ui;                                                    }
	Void setDependencyId ( UInt uidId, UInt uiId )                                                        { m_dependency_id[uidId] = uiId;                                            }
  Void setNumQIdMinus1 ( UInt uidId, UInt uiId )                                                        { m_num_qId_minus1[uidId] = uiId;                                           }
	Void setQualityId ( UInt uidId, UInt uiqId, UInt uiId )                                               { m_quality_id[uidId][uiqId] = uiId;                                        }
	Void setNumRedundantPicsMinus1 ( UInt uidId, UInt uiqId, UInt uiredupic )                             { m_num_redundant_pics_minus1[uidId][uiqId] = uiredupic;                    }
	Void setRedundantPicCntMinus1 ( UInt uidId, UInt uiqId, UInt uiredupic,UInt uiredupicnt )             { m_redundant_pic_cnt_minus1[uidId][uiqId][uiredupic] = uiredupicnt;        }
	Void setPicMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic, Bool bFlag )                           { m_pic_match_flag[uidId][uiqId][uiredupic] = bFlag;                        }
	Void setMbTypeMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic, Bool bFlag )                        { m_mb_type_match_flag[uidId][uiqId][uiredupic] = bFlag;                    }
	Void setMotionMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic, Bool bFlag )                        { m_motion_match_flag[uidId][uiqId][uiredupic] = bFlag;                     }
	Void setResidualMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic, Bool bFlag )                      { m_residual_match_flag[uidId][uiqId][uiredupic] = bFlag;                   }
	Void setIntraSamplesMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic, Bool bFlag )                  { m_intra_samples_match_flag[uidId][uiqId][uiredupic] = bFlag;              }

	UInt getNumDIdMinus1( )                                                                         const { return m_num_dId_minus1;                                                  }
	UInt getDependencyId ( UInt uidId )                                                             const { return m_dependency_id[uidId];                                            }
  UInt getNumQIdMinus1 ( UInt uidId )                                                             const { return m_num_qId_minus1[uidId];                                           }
	UInt getQualityId ( UInt uidId, UInt uiqId )                                                    const { return m_quality_id[uidId][uiqId];                                        }
	UInt getNumRedundantPicsMinus1 ( UInt uidId, UInt uiqId )                                       const { return m_num_redundant_pics_minus1[uidId][uiqId];                         }
	UInt getRedundantPicCntMinus1 ( UInt uidId, UInt uiqId, UInt uiredupic )                        const { return m_redundant_pic_cnt_minus1[uidId][uiqId][uiredupic];               }
	Bool getPicMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic )                                 const { return m_pic_match_flag[uidId][uiqId][uiredupic];                         }
	Bool getMbTypeMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic )                              const { return m_mb_type_match_flag[uidId][uiqId][uiredupic];                     }
	Bool getMotionMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic )                              const { return m_motion_match_flag[uidId][uiqId][uiredupic];                      }
	Bool getResidualMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic )                            const { return m_residual_match_flag[uidId][uiqId][uiredupic];                    }
	Bool getIntraSamplesMatchFlag ( UInt uidId, UInt uiqId, UInt uiredupic )                        const { return m_intra_samples_match_flag[uidId][uiqId][uiredupic];               }

private:
	UInt m_num_dId_minus1;
	UInt m_dependency_id[MAX_LAYERS];
  UInt m_num_qId_minus1[MAX_LAYERS];
	UInt m_quality_id[MAX_LAYERS][MAX_QUALITY_LEVELS];
	UInt m_num_redundant_pics_minus1[MAX_LAYERS][MAX_QUALITY_LEVELS];
	UInt m_redundant_pic_cnt_minus1[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
	Bool m_pic_match_flag[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
	Bool m_mb_type_match_flag[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
	Bool m_motion_match_flag[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
	Bool m_residual_match_flag[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
	Bool m_intra_samples_match_flag[MAX_LAYERS][MAX_QUALITY_LEVELS][MAX_REDUNDANT_PICTURES_NUM];
};
//JVT-W049 }

	//JVT-W052 wxwan
	class H264AVCCOMMONLIB_API IntegrityCheckSEI : public SEIMessage
	{
	protected:
		IntegrityCheckSEI() : SEIMessage(INTEGRITY_CHECK_SEI), m_uinuminfoentriesminus1( MSYS_UINT_MAX )
		{}
	public:
		static ErrVal create( IntegrityCheckSEI*& rpcIntegrityCheckSEI );
		ErrVal    destroy ();
		ErrVal        write ( HeaderSymbolWriteIf*  pcWriteIf);
		ErrVal        read  ( HeaderSymbolReadIf*   pcReadIf, ParameterSetMng* pcParameterSetMng );
		UInt					getNumInfoEntriesMinus1()							{ return m_uinuminfoentriesminus1; }
		UInt          getEntryDependencyId(UInt uilayer)		{ return m_uientrydependency_id[uilayer]; }
		UInt          getQualityLayerCRC  (UInt uilayer)		{ return m_uiquality_layer_crc [uilayer]; }
		Void          setNumInfoEntriesMinus1(UInt ui )     { m_uinuminfoentriesminus1 = ui; }
		Void					setEntryDependencyId(UInt uilayer, UInt ui)    { m_uientrydependency_id[uilayer] = ui; }
		Void					setQualityLayerCRC  (UInt uilayer, UInt ui)		 { m_uiquality_layer_crc [uilayer] = ui; }
	private:
		UInt					m_uinuminfoentriesminus1;
		UInt					m_uientrydependency_id[ MAX_LAYERS ];
		UInt					m_uiquality_layer_crc [ MAX_LAYERS ];
	};
	//JVT-W052 wxwan
  //JVT-X032 {
  class H264AVCCOMMONLIB_API TLSwitchingPointSei: public SEIMessage
{
protected:
  TLSwitchingPointSei ();
  ~TLSwitchingPointSei ();

public:
  static ErrVal create  ( TLSwitchingPointSei*&      rpcSeiMessage );
  ErrVal        write   ( HeaderSymbolWriteIf*   pcWriteIf     );
  ErrVal        read    ( HeaderSymbolReadIf*    pcReadIf, ParameterSetMng* pcParameterSetMng      );

  Void setDeltaFrameNum( UInt ui )                                                                       { m_delta_frame_num = ui;                                                    }

	Int getDeltaFrameNum( )                                                                         const { return m_delta_frame_num;                                                  }

private:
	Int m_delta_frame_num;
};
//JVT-X032 }

    //JVT-W062 {
  class H264AVCCOMMONLIB_API Tl0DepRepIdxSei : public SEIMessage
  {
  protected:
    Tl0DepRepIdxSei();
    ~Tl0DepRepIdxSei();
  public:
    static ErrVal create ( Tl0DepRepIdxSei*& rpcSeiMessage);
    ErrVal destroy       ();
    ErrVal write         ( HeaderSymbolWriteIf  *pcWriteIf);
    ErrVal read          ( HeaderSymbolReadIf   *pcReadIf, ParameterSetMng* pcParameterSetMng);
    UInt getTl0DepRepIdx()         const { return m_uiTl0DepRepIdx; }
    Void setTl0DepRepIdx           (UInt uiIdx) {m_uiTl0DepRepIdx = uiIdx;}
    UInt getEfIdrPicId()         const { return m_uiEfIdrPicId; }
    Void setEfIdrPicId           (UInt uiId) {m_uiEfIdrPicId = uiId;}

  private:
    UInt m_uiTl0DepRepIdx;
    UInt m_uiEfIdrPicId;
  };
//JVT-W062 }


  class H264AVCCOMMONLIB_API UnsupportedSei : public SEIMessage
  {
  protected:
    UnsupportedSei  ( MessageType eMessageType, UInt uiSize );
    ~UnsupportedSei ();
  public:
    static ErrVal create ( UnsupportedSei*&     rpcSeiMessage, MessageType eMessageType, UInt uiSize );
    ErrVal destroy       ();
    ErrVal write         ( HeaderSymbolWriteIf* pcWriteIf );
    ErrVal read          ( HeaderSymbolReadIf*  pcReadIf, ParameterSetMng* pcParameterSetMng );
  private:
    UInt   m_uiSize;
  };

  typedef MyList<SEIMessage*> MessageList;

  static ErrVal read  ( HeaderSymbolReadIf*   pcReadIf,
                        MessageList&          rcSEIMessageList
                        // JVT-V068 {
                        ,ParameterSetMng* pcParameterSetMng
                        // JVT-V068 }
                      );
  static ErrVal write ( HeaderSymbolWriteIf*  pcWriteIf,
                        HeaderSymbolWriteIf*  pcWriteTestIf,
                        MessageList*          rpcSEIMessageList );

  // JVT-V068 {
  static ErrVal writeScalableNestingSei( HeaderSymbolWriteIf*  pcWriteIf,
                                         HeaderSymbolWriteIf*  pcWriteTestIf,
                                         MessageList*          rpcSEIMessageList );
  // JVT-V068 }

protected:
  static ErrVal xRead               ( HeaderSymbolReadIf*   pcReadIf,
                                      SEIMessage*&          rpcSEIMessage
                                      // JVT-V068 {
                                      ,ParameterSetMng* pcParameterSetMng
                                      // JVT-V068 }
                                    );
  static ErrVal xWrite              ( HeaderSymbolWriteIf*  pcWriteIf,
                                      HeaderSymbolWriteIf*  pcWriteTestIf,
                                      SEIMessage*           pcSEIMessage );
  static ErrVal xWritePayloadHeader ( HeaderSymbolWriteIf*  pcWriteIf,
                                      MessageType           eMessageType,
                                      UInt                  uiSize );
  static ErrVal xReadPayloadHeader  ( HeaderSymbolReadIf*   pcReadIf,
                                      MessageType&          reMessageType,
                                      UInt&                 ruiSize);
  static ErrVal xCreate             ( SEIMessage*&          rpcSEIMessage,
                                      MessageType           eMessageType,
                                      // JVT-V068 {
                                      ParameterSetMng*&     rpcParameterSetMng,
                                      // JVT-V068 }
                                      UInt                  uiSize );
};

#if defined( WIN32 )
# pragma warning( default: 4251 )
#endif

H264AVC_NAMESPACE_END


#endif // !defined(AFX_SEI_H__06FFFAD0_FB36_4BF0_9392_395C7389C1F4__INCLUDED_)
