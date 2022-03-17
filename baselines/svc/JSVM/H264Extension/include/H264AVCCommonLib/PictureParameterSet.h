
#if !defined(AFX_PICTUREPARAMETERSET_H__8ED333BE_D213_4BFF_A379_67DDDA7F090C__INCLUDED_)
#define AFX_PICTUREPARAMETERSET_H__8ED333BE_D213_4BFF_A379_67DDDA7F090C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/HeaderSymbolReadIf.h"
#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"
#include "H264AVCCommonLib/ScalingMatrix.h"


//--ICU/ETRI FMO Implementation
#include <math.h>

H264AVC_NAMESPACE_BEGIN


//--ICU/ETRI FMO Implementation
const unsigned MAXNumSliceGroupsMinus1 =8; //it is also defined at cfmo.h


class H264AVCCOMMONLIB_API PictureParameterSet
{
protected:
  PictureParameterSet         ();
  virtual ~PictureParameterSet();

public:
  static ErrVal create  ( PictureParameterSet*& rpcPPS );
  ErrVal        destroy ();

  Bool                  referencesSubsetSPS                     ()            const { return m_bReferencesSubsetSPS; }
  NalUnitType           getNalUnitType                          ()            const { return m_eNalUnitType; }
  UInt                  getDependencyId                              ()            const { return m_uiDependencyId; }
  UInt                  getPicParameterSetId                    ()            const { return m_uiPicParameterSetId; }
  UInt                  getSeqParameterSetId                    ()            const { return m_uiSeqParameterSetId; }
  Bool                  getEntropyCodingModeFlag                ()            const { return m_bEntropyCodingModeFlag; }
  Bool                  getPicOrderPresentFlag                  ()            const { return m_bPicOrderPresentFlag; }
  UInt                  getNumRefIdxActive                      ( ListIdx e ) const { return m_auiNumRefIdxActive[e]; }
  Bool                  getWeightedPredFlag                     ()            const { return m_bWeightedPredFlag; }
  UInt                  getWeightedBiPredIdc                    ()            const { return m_uiWeightedBiPredIdc; }
  UInt                  getPicInitQp                            ()            const { return m_uiPicInitQp; }
  Int                   getChromaQpIndexOffset                  ()            const { return m_iChromaQpIndexOffset; }
  Bool                  getDeblockingFilterParametersPresentFlag()            const { return m_bDeblockingFilterParametersPresentFlag; }
  Bool                  getConstrainedIntraPredFlag             ()            const { return m_bConstrainedIntraPredFlag; }
  Bool                  getRedundantPicCntPresentFlag           ()            const { return m_bRedundantPicCntPresentFlag; } //JVT-Q054 Red. Picture
  Bool                  getRedundantKeyPicCntPresentFlag        ()            const { return m_bRedundantKeyPicCntPresentFlag; } //JVT-W049
  Bool                  getEnableRedundantKeyPicCntPresentFlag  ()            const { return m_bEnableRedundantKeyPicCntPresentFlag; } //JVT-W049
	Bool                  getTransform8x8ModeFlag                 ()            const { return m_bTransform8x8ModeFlag; }
  Bool                  getPicScalingMatrixPresentFlag          ()            const { return m_bPicScalingMatrixPresentFlag; }
  const ScalingMatrix&  getPicScalingMatrix                     ()            const { return m_cPicScalingMatrix; }
  ScalingMatrix&        getPicScalingMatrix                     ()                  { return m_cPicScalingMatrix; }
  Int                   get2ndChromaQpIndexOffset               ()            const { return m_iSecondChromaQpIndexOffset; }


  //--ICU/ETRI FMO Implementation : FMO stuff start
  UInt          getNumSliceGroupsMinus1() const {return m_uiNumSliceGroupsMinus1;}
  UInt          getSliceGroupMapType() const {return  m_uiSliceGroupMapType;  }
  UInt          getRunLengthMinus1 (Int i) const {return m_uiRunLengthMinus1[i];}
  UInt          getTopLeft (Int i) const {return m_uiTopLeft[i];}
  UInt          getBottomRight (Int i) const {return m_uiBottomRight[i];}
  Bool          getSliceGroupChangeDirection_flag () const {return m_bSliceGroupChangeDirection_flag;}
  UInt          getSliceGroupChangeRateMinus1 () const {return m_uiSliceGroupChangeRateMinus1;}
  UInt          getNumSliceGroupMapUnitsMinus1() const {return m_uiNumSliceGroupMapUnitsMinus1;}
  UInt          getSliceGroupId(Int i) const { AOF(i<(Int)m_uiSliceGroupIdArraySize); return m_pauiSliceGroupId[i];}
  UInt*         getArrayRunLengthMinus1 () const {return (UInt*)m_uiRunLengthMinus1;}
  UInt*         getArrayTopLeft () const {return (UInt*)m_uiTopLeft;}
  UInt*         getArrayBottomRight () const {return (UInt*)m_uiBottomRight;}
  UInt*         getArraySliceGroupId() const {return m_pauiSliceGroupId;}
  UInt          getSliceGroupChangeCycle() const {return m_uiSliceGroupChangeCycle;}
  UInt          getLog2MaxSliceGroupChangeCycle(UInt uiPicSizeInMapUnits) const 
  {
    UInt uiTmp = ( uiPicSizeInMapUnits + m_uiSliceGroupChangeRateMinus1 ) / ( m_uiSliceGroupChangeRateMinus1 + 1 ) + 1;
    for( UInt uiMaxLog2 = 0; uiMaxLog2 < 32; uiMaxLog2++ )
    {
      if( uiTmp <= UInt( 1 << uiMaxLog2 ) )
      {
        return uiMaxLog2;
      }
    }
    return MSYS_UINT_MAX;
  };
  //--ICU/ETRI FMO Implementation : FMO stuff end


  Void  setReferencesSubsetSPS                  ( Bool b )                  { m_bReferencesSubsetSPS                    = b; }
  Void  setNalUnitType                          ( NalUnitType e )           { m_eNalUnitType                            = e; }
  Void  setDependencyId                         ( UInt        ui )          { m_uiDependencyId                          = ui; }
  Void  setPicParameterSetId                    ( UInt        ui )          { m_uiPicParameterSetId                     = ui; }
  Void  setSeqParameterSetId                    ( UInt        ui )          { m_uiSeqParameterSetId                     = ui; }
  Void  setEntropyCodingModeFlag                ( Bool        b )           { m_bEntropyCodingModeFlag                  = b; }
  Void  setPicOrderPresentFlag                  ( Bool        b )           { m_bPicOrderPresentFlag                    = b; }
  Void  setNumRefIdxActive                      ( ListIdx     e, UInt ui )  { m_auiNumRefIdxActive[e]                   = ui; }
  Void  setWeightedPredFlag                     ( Bool        b )           { m_bWeightedPredFlag                       = b; }
  Void  setWeightedBiPredIdc                    ( UInt        ui )          { m_uiWeightedBiPredIdc                     = ui; }
  Void  setPicInitQp                            ( UInt        ui )          { m_uiPicInitQp                             = ui; }
  Void  setChromaQpIndexOffset                  ( Int         i )           { m_iChromaQpIndexOffset                    = i; }
  Void  setDeblockingFilterParametersPresentFlag( Bool        b )           { m_bDeblockingFilterParametersPresentFlag  = b; }
  Void  setConstrainedIntraPredFlag             ( Bool        b )           { m_bConstrainedIntraPredFlag               = b; }
  Void  setRedundantPicCntPresentFlag           ( Bool        b )           { m_bRedundantPicCntPresentFlag             = b; }  // JVT-Q054 Red. Picture
  Void  setRedundantKeyPicCntPresentFlag        ( Bool        b )           { m_bRedundantKeyPicCntPresentFlag          = b; }  // JVT-W049
  Void  setEnableRedundantKeyPicCntPresentFlag  ( Bool        b )           { m_bEnableRedundantKeyPicCntPresentFlag    = b; }  // JVT-W049
	Void  setTransform8x8ModeFlag                 ( Bool        b )           { m_bTransform8x8ModeFlag                   = b; }
  Void  setPicScalingMatrixPresentFlag          ( Bool        b )           { m_bPicScalingMatrixPresentFlag            = b; }
  Void  set2ndChromaQpIndexOffset               ( Int         i )           { m_iSecondChromaQpIndexOffset              = i; }


  //--ICU/ETRI FMO Implementation : FMO stuff start
  Void setNumSliceGroupsMinus1(UInt   uiNumSliceGroupsMinus1) {m_uiNumSliceGroupsMinus1 =uiNumSliceGroupsMinus1;}
  Void setSliceGroupMapType(UInt          uiSliceGroupMapType) {m_uiSliceGroupMapType =uiSliceGroupMapType;  }
  Void setRunLengthMinus1 (UInt        uiRunLengthMinus1,  Int i) {m_uiRunLengthMinus1[i] = uiRunLengthMinus1;}
  Void setTopLeft (UInt          uiTopLeft, Int i) {m_uiTopLeft[i] = uiTopLeft;}
  Void setBottomRight (UInt          uiBottomRight, Int i){m_uiBottomRight[i] = uiBottomRight;}
  Void setSliceGroupChangeDirection_flag (Bool         SliceGroupChangeDirection_flag){m_bSliceGroupChangeDirection_flag = SliceGroupChangeDirection_flag;}
  Void setSliceGroupChangeRateMinus1 (UInt         SliceGroupChangeRateMinus1 ){m_uiSliceGroupChangeRateMinus1 = SliceGroupChangeRateMinus1;}
  Void setNumSliceGroupMapUnitsMinus1 (UInt         uiNumSliceGroupMapUnitsMinus1){ m_uiNumSliceGroupMapUnitsMinus1 = uiNumSliceGroupMapUnitsMinus1;}
  Void setSliceGroupId(UInt         uiSliceGroupId, Int i) { AOF(i<(Int)m_uiSliceGroupIdArraySize); m_pauiSliceGroupId[i] = uiSliceGroupId;}
  Void setArrayRunLengthMinus1 (UInt*        uiRunLengthMinus1)
  {
    for(UInt i=0;i<=getNumSliceGroupsMinus1();i++)
      m_uiRunLengthMinus1[i] = uiRunLengthMinus1[i];
  }
  Void setArrayTopLeft (UInt*          uiTopLeft)
  {
    for(UInt i=0;i<getNumSliceGroupsMinus1();i++)
      m_uiTopLeft[i] = uiTopLeft[i];
  }
  Void setArrayBottomRight (UInt*          uiBottomRight)
  {
    for(UInt i=0;i<getNumSliceGroupsMinus1();i++)
    m_uiBottomRight[i] = uiBottomRight[i];
  }
  Void setArraySliceGroupId( UInt* uiSliceGroupId )
  {
    ROFVS( uiSliceGroupId );
    if( m_uiSliceGroupIdArraySize <= getNumSliceGroupMapUnitsMinus1() )
    {
      delete [] m_pauiSliceGroupId;
      m_uiSliceGroupIdArraySize = getNumSliceGroupMapUnitsMinus1() + 1;
      m_pauiSliceGroupId        = new UInt [m_uiSliceGroupIdArraySize];
    }
    for( UInt i=0; i<=getNumSliceGroupMapUnitsMinus1(); i++)
    {
      m_pauiSliceGroupId[i] = uiSliceGroupId[i];
    }
  }
  Void setSliceGroupChangeCycle(UInt SliceGroupChangeCycle){ m_uiSliceGroupChangeCycle = SliceGroupChangeCycle;}
  //--ICU/ETRI FMO Implementation : FMO stuff end

  ErrVal write      ( HeaderSymbolWriteIf*  pcWriteIf ) const;
  ErrVal read       ( HeaderSymbolReadIf*   pcReadIf,
                      NalUnitType           eNalUnitType );

protected:
  ErrVal xWriteFrext( HeaderSymbolWriteIf*  pcWriteIf ) const;
  ErrVal xReadFrext ( HeaderSymbolReadIf*   pcReadIf );

protected:
  NalUnitType   m_eNalUnitType;
  UInt          m_uiDependencyId;
  UInt          m_uiPicParameterSetId;
  UInt          m_uiSeqParameterSetId;
  Bool          m_bEntropyCodingModeFlag;
  Bool          m_bPicOrderPresentFlag;


  //--ICU/ETRI FMO Implementation : FMO stuff start
  UInt          m_uiNumSliceGroupsMinus1;
  UInt          m_uiSliceGroupMapType;
  UInt          m_uiRunLengthMinus1[MAXNumSliceGroupsMinus1];
  UInt          m_uiTopLeft[MAXNumSliceGroupsMinus1];
  UInt          m_uiBottomRight[MAXNumSliceGroupsMinus1];
  Bool          m_bSliceGroupChangeDirection_flag;
  UInt          m_uiSliceGroupChangeRateMinus1;
  UInt          m_uiNumSliceGroupMapUnitsMinus1;
  UInt          m_uiSliceGroupChangeCycle;
  UInt          m_uiSliceGroupIdArraySize;
  UInt*         m_pauiSliceGroupId;
  //--ICU/ETRI FMO Implementation : FMO stuff end

  UInt          m_auiNumRefIdxActive[2];
  Bool          m_bWeightedPredFlag;
  UInt          m_uiWeightedBiPredIdc;
  UInt          m_uiPicInitQp;
  Int           m_iChromaQpIndexOffset;
  Bool          m_bDeblockingFilterParametersPresentFlag;
  Bool          m_bConstrainedIntraPredFlag;
  Bool          m_bTransform8x8ModeFlag;
  Bool          m_bPicScalingMatrixPresentFlag;
  ScalingMatrix m_cPicScalingMatrix;
  Int           m_iSecondChromaQpIndexOffset;
  Bool          m_bRedundantPicCntPresentFlag;  //JVT-Q054 Red. Picture u(1)
	Bool          m_bRedundantKeyPicCntPresentFlag;  //JVT-W049
  Bool          m_bEnableRedundantKeyPicCntPresentFlag;  //JVT-W049

  Bool          m_bReferencesSubsetSPS;
};


H264AVC_NAMESPACE_END


#endif // !defined(AFX_PICTUREPARAMETERSET_H__8ED333BE_D213_4BFF_A379_67DDDA7F090C__INCLUDED_)
