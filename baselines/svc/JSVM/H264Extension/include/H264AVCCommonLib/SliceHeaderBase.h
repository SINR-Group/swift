
#if !defined(AFX_SLICEHEADERBASE_H__2CC1FD0F_CACB_4799_84BE_FC5FC9B9C245__INCLUDED_)
#define AFX_SLICEHEADERBASE_H__2CC1FD0F_CACB_4799_84BE_FC5FC9B9C245__INCLUDED_


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "H264AVCCommonLib/SequenceParameterSet.h"
#include "H264AVCCommonLib/PictureParameterSet.h"
#include "H264AVCCommonLib/ParameterSetMng.h"
#include "H264AVCCommonLib/YuvBufferCtrl.h"
#include "H264AVCCommonLib/MbTransformCoeffs.h"
#include "H264AVCCommonLib/Tables.h"

#include "H264AVCCommonLib/HeaderSymbolWriteIf.h"
#include "H264AVCCommonLib/HeaderSymbolReadIf.h"



H264AVC_NAMESPACE_BEGIN

#include <math.h>
class YuvMbBuffer;
class QuarterPelFilter;
class FMO;

#if defined( WIN32 )
# pragma warning( disable: 4275 )
# pragma warning( disable: 4251 )
#endif



enum ReOrderingOfPicNumsIdc
{
  RPLR_NEG   = 0,
  RPLR_POS   = 1,
  RPLR_LONG  = 2,
  RPLR_END   = 3
};


class H264AVCCOMMONLIB_API RplrCommand
{
public:
	RplrCommand( ReOrderingOfPicNumsIdc   eReOrderingOfPicNumsIdc = RPLR_END,
               UInt                     uiValue                 = 0 );
  RplrCommand( const RplrCommand&       rcRplrCommand );
  virtual ~RplrCommand();

  Void    copy  ( const RplrCommand&    rcRplrCommand );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,
                  Bool&                 rbEnd )   const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,
                  Bool&                 rbEnd );

  ReOrderingOfPicNumsIdc  getCommand  ( UInt&               ruiValue )      const { ruiValue = m_uiValue; return m_eReOrderingOfPicNumsIdc; }
  ReOrderingOfPicNumsIdc  getCommand  ()                                    const { return m_eReOrderingOfPicNumsIdc; }
  UInt                    getValue    ()                                    const { return m_uiValue; }
  Bool                    isEnd       ()                                    const { return m_eReOrderingOfPicNumsIdc == RPLR_END; }
  Bool                    operator != ( const RplrCommand&  rcRplrCommand ) const { return m_eReOrderingOfPicNumsIdc != rcRplrCommand.m_eReOrderingOfPicNumsIdc || m_uiValue != rcRplrCommand.m_uiValue; }

  const RplrCommand&      operator  = ( const RplrCommand&  rcRplrCommand )       { copy( rcRplrCommand ); return *this; }

private:
  ReOrderingOfPicNumsIdc  m_eReOrderingOfPicNumsIdc;
  UInt                    m_uiValue;
};


class H264AVCCOMMONLIB_API RefPicListReOrdering : public StatBuf< RplrCommand, 33 >
{
public:
  RefPicListReOrdering();
  RefPicListReOrdering( const RefPicListReOrdering& rcRefPicListReOrdering );
  virtual ~RefPicListReOrdering();

  Void    clear ( Bool                        bRefPicListReOrderingFlag = false );
  Void    copy  ( const RefPicListReOrdering& rcRefPicListReOrdering );
  ErrVal  write ( HeaderSymbolWriteIf&        rcWriteIf )   const;
  ErrVal  read  ( HeaderSymbolReadIf&         rcReadIf,
                  UInt                        uiNumRefIdx = MSYS_UINT_MAX );

  Bool    getRefPicListReorderingFlag()              const { return m_bRefPicListReorderingFlag; }

  Void    setRefPicListReorderingFlag( Bool bFlag )        { m_bRefPicListReorderingFlag = bFlag; }

  const RefPicListReOrdering& operator = ( const RefPicListReOrdering& rcRefPicListReOrdering ) { copy( rcRefPicListReOrdering ); return *this; }

private:
  Bool    m_bRefPicListReorderingFlag;
};



enum Mmco
{
  MMCO_END                = 0,
	MMCO_SHORT_TERM_UNUSED  = 1,
	MMCO_LONG_TERM_UNUSED   = 2,
	MMCO_ASSIGN_LONG_TERM   = 3,
	MMCO_MAX_LONG_TERM_IDX  = 4,
	MMCO_RESET              = 5,
  MMCO_SET_LONG_TERM      = 6
};


class H264AVCCOMMONLIB_API MmcoCommand
{
public:
  MmcoCommand( Mmco               eMmco     = MMCO_END,
               UInt               uiValue1  = 0,
               UInt               uiValue2  = 0 );
  MmcoCommand( const MmcoCommand& rcMmcoCommand );
  virtual ~MmcoCommand();

  Void    copy  ( const MmcoCommand&    rcMmcoCommand );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,
                  Bool                  bRefBasePic,
                  Bool&                 rbEnd )   const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,
                  Bool                  bRefBasePic,
                  Bool&                 rbEnd );

  Mmco        getCommand  ( UInt&       ruiValue1,
                            UInt&       ruiValue2 ) const { ruiValue1 = m_uiValue1, ruiValue2 = m_uiValue2; return m_eMmco; }
  Mmco        getCommand  ()                        const { return m_eMmco; }
  UInt        getValue1   ()                        const { return m_uiValue1; }
  UInt        getValue2   ()                        const { return m_uiValue2; }
  Bool        isEnd       ()                        const { return m_eMmco == MMCO_END; }

  Bool                operator != ( const MmcoCommand& rcMmcoCommand  )   const { return m_eMmco != rcMmcoCommand.m_eMmco || m_uiValue1 != rcMmcoCommand.m_uiValue1 || m_uiValue2 != rcMmcoCommand.m_uiValue2; }
  const MmcoCommand&  operator  = ( const MmcoCommand& rcMmcoCommand  )         { copy( rcMmcoCommand ); return *this; }

private:
  Mmco    m_eMmco;
  UInt    m_uiValue1;
  UInt    m_uiValue2;
};


class H264AVCCOMMONLIB_API DecRefPicMarking : public StatBuf< MmcoCommand, 33 >
{
public:
  DecRefPicMarking( Bool                    bDecRefBasePicMarking = false );
  DecRefPicMarking( const DecRefPicMarking& rcDecRefPicMarking );
  virtual ~DecRefPicMarking();

  Void    clear ( Bool                      bDecRefBasePicMarking,
                  Bool                      bAdaptiveRefPicMarkingModeFlag = false );
  Void    copy  ( const DecRefPicMarking&   rcDecRefPicMarking );
  ErrVal  write ( HeaderSymbolWriteIf&      rcWriteIf ) const;
  ErrVal  read  ( HeaderSymbolReadIf&       rcReadIf );

  Bool    hasMMCO5                          ()                                  const;
  Bool    isDecRefBasePicMarking            ()                                  const   { return m_bDecRefBasePicMarking; }
  Bool    getAdaptiveRefPicMarkingModeFlag  ()                                  const   { return m_bAdaptiveRefPicMarkingModeFlag; }

  Void    setDecRefBasePicMarking           ( Bool  bDecRefBasePicMarking )             { m_bDecRefBasePicMarking           = bDecRefBasePicMarking; }
  Void    setAdaptiveRefPicMarkingModeFlag  ( Bool  bAdaptiveRefPicMarkingModeFlag  )   { m_bAdaptiveRefPicMarkingModeFlag  = bAdaptiveRefPicMarkingModeFlag; }

  const DecRefPicMarking& operator = ( const DecRefPicMarking& rcDecRefPicMarking )     { copy( rcDecRefPicMarking ); return *this; }

private:
  Bool    m_bDecRefBasePicMarking;
  Bool    m_bAdaptiveRefPicMarkingModeFlag;
};



class H264AVCCOMMONLIB_API PredWeight
{
public:
  PredWeight();
  PredWeight( const PredWeight& rcPredWeight );
  virtual ~PredWeight();

  Void    copy  ( const PredWeight&     rcPredWeight );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf ) const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,
                  UInt                  uiLumaLog2WeightDenom,
                  UInt                  uiChromaLog2WeightDenom );

  ErrVal  initRandomly  ();
  ErrVal  initWeights   ( Int iLumaWeight, Int iCbWeight, Int iCrWeight );
  ErrVal  initOffsets   ( Int iLumaOffset, Int iCbOffset, Int iCrOffset );

  ErrVal  setOffsets    ( const Double* padOffsets );
  ErrVal  setWeights    ( const Double* padWeights, Int iLumaScale, Int iChromaScale );
  ErrVal  getOffsets    ( Double*       padOffsets );
  ErrVal  getWeights    ( Double*       padWeights );

  Void    scaleL1Weight ( Int               iDistScaleFactor );
  Void    scaleL0Weight ( const PredWeight& rcPredWeightL1 );

  Bool    getLumaWeightFlag   ()  const { return m_bLumaWeightFlag; }
  Int     getLumaWeight       ()  const { return m_iLumaWeight; }
  Int     getLumaOffset       ()  const { return m_iLumaOffset; }
  Bool    getChromaWeightFlag ()  const { return m_bChromaWeightFlag; }
  Int     getChromaCbWeight   ()  const { return m_iChromaCbWeight; }
  Int     getChromaCbOffset   ()  const { return m_iChromaCbOffset; }
  Int     getChromaCrWeight   ()  const { return m_iChromaCrWeight; }
  Int     getChromaCrOffset   ()  const { return m_iChromaCrOffset; }

  Void    setLumaWeightFlag   ( Bool  bLumaWeightFlag   )   { m_bLumaWeightFlag   = bLumaWeightFlag; }
  Void    setLumaWeight       ( Int   iLumaWeight       )   { m_iLumaWeight       = iLumaWeight; }
  Void    setLumaOffset       ( Int   iLumaOffset       )   { m_iLumaOffset       = iLumaOffset; }
  Void    setChromaWeightFlag ( Bool  bChromaWeightFlag )   { m_bChromaWeightFlag = bChromaWeightFlag; }
  Void    setChromaCbWeight   ( Int   iChromaCbWeight   )   { m_iChromaCbWeight   = iChromaCbWeight; }
  Void    setChromaCbOffset   ( Int   iChromaCbOffset   )   { m_iChromaCbOffset   = iChromaCbOffset; }
  Void    setChromaCrWeight   ( Int   iChromaCrWeight   )   { m_iChromaCrWeight   = iChromaCrWeight; }
  Void    setChromaCrOffset   ( Int   iChromaCrOffset   )   { m_iChromaCrOffset   = iChromaCrOffset; }

  Bool              operator  ==  ( const PredWeight& rcPredWeight )  const;
  Bool              operator  !=  ( const PredWeight& rcPredWeight )  const;
  const PredWeight& operator   =  ( const PredWeight& rcPredWeight )          { copy( rcPredWeight ); return *this; }

private:
  Int     xRandom( Int iMin, Int iMax );

private:
  Bool    m_bLumaWeightFlag;
  Int     m_iLumaWeight;
  Int     m_iLumaOffset;
  Bool    m_bChromaWeightFlag;
  Int     m_iChromaCbWeight;
  Int     m_iChromaCbOffset;
  Int     m_iChromaCrWeight;
  Int     m_iChromaCrOffset;
};


class H264AVCCOMMONLIB_API PredWeightTable : public StatBuf< PredWeight, 32 >
{
public:
  PredWeightTable();
  PredWeightTable( const PredWeightTable& rcPredWeightTable );
  virtual ~PredWeightTable();

  ErrVal  initRandomly  ();
  ErrVal  initDefaults  ( UInt  uiLumaWeightDenom,
                          UInt  uiChromaWeightDenom );

  ErrVal  setOffsets    ( const Double (*padOffsets)[3] );
  ErrVal  setWeights    ( const Double (*padOffsets)[3], Int iLumaScale, Int iChromaScale );

  Void    clear ();
  Void    copy  ( const PredWeightTable&  rcPredWeightTable );
  ErrVal  write ( HeaderSymbolWriteIf&    rcWriteIf,  UInt uiNumRefIdxActiveMinus1 ) const;
  ErrVal  read  ( HeaderSymbolReadIf&     rcReadIf,   UInt uiNumRefIdxActiveMinus1, UInt uiLumaLog2WeightDenom, UInt uiChromaLog2WeightDenom );

  const PredWeightTable& operator = ( const PredWeightTable& rcPredWeightTable )    { copy( rcPredWeightTable ); return *this; }
};


class H264AVCCOMMONLIB_API DBFilterParameter
{
public:
  DBFilterParameter( Bool                     bInterLayerParameters = false );
  DBFilterParameter( const DBFilterParameter& rcDBFilterParameter );
  virtual ~DBFilterParameter();

  Void    clear ( Bool                      InterLayerParameters = false );
  Void    copy  ( const DBFilterParameter&  rcDBFilterParameter );
  ErrVal  write ( HeaderSymbolWriteIf&      rcWriteIf ) const;
  ErrVal  read  ( HeaderSymbolReadIf&       rcReadIf, Bool bSVCNalUnit );

  Bool    isInterLayerParameters        ()  const   { return m_bInterLayerParameters; }
  UInt    getDisableDeblockingFilterIdc ()  const   { return m_uiDisableDeblockingFilterIdc; }
  Int     getSliceAlphaC0OffsetDiv2     ()  const   { return m_iSliceAlphaC0OffsetDiv2; }
  Int     getSliceAlphaC0Offset         ()  const   { return m_iSliceAlphaC0OffsetDiv2 << 1; }
  Int     getSliceBetaOffsetDiv2        ()  const   { return m_iSliceBetaOffsetDiv2; }
  Int     getSliceBetaOffset            ()  const   { return m_iSliceBetaOffsetDiv2 << 1; }

  Void    setDisableDeblockingFilterIdc ( UInt  uiDisableDeblockingFilterIdc  )   { m_uiDisableDeblockingFilterIdc  = uiDisableDeblockingFilterIdc; }
  Void    setSliceAlphaC0OffsetDiv2     ( Int   iSliceAlphaC0OffsetDiv2       )   { m_iSliceAlphaC0OffsetDiv2       = iSliceAlphaC0OffsetDiv2; }
  Void    setSliceAlphaC0Offset         ( Int   iSliceAlphaC0Offset           )   { m_iSliceAlphaC0OffsetDiv2       = iSliceAlphaC0Offset >> 1; }
  Void    setSliceBetaOffsetDiv2        ( Int   iSliceBetaOffsetDiv2          )   { m_iSliceBetaOffsetDiv2          = iSliceBetaOffsetDiv2; }
  Void    setSliceBetaOffset            ( Int   iSliceBetaOffset              )   { m_iSliceBetaOffsetDiv2          = iSliceBetaOffset >> 1; }

  const DBFilterParameter&  operator = ( const DBFilterParameter& rcDBFilterParameter ) { copy( rcDBFilterParameter ); return *this; }

private:
  Bool  m_bInterLayerParameters;
  UInt  m_uiDisableDeblockingFilterIdc;
  Int   m_iSliceAlphaC0OffsetDiv2;
  Int   m_iSliceBetaOffsetDiv2;
};



class H264AVCCOMMONLIB_API NalUnitHeader
{
public:
  NalUnitHeader();
  NalUnitHeader( const NalUnitHeader& rcNalUnitHeader );
  virtual ~NalUnitHeader();

  Void              copy  ( const NalUnitHeader&  rcNalUnitHeader,
                            Bool                  bInclusiveSVCExtension = true );
  ErrVal            write ( HeaderSymbolWriteIf&  rcWriteIf ) const;
  ErrVal            read  ( HeaderSymbolReadIf&   rcReadIf );

  Bool              isH264AVCCompatible     ()  const { return  m_eNalUnitType != NAL_UNIT_CODED_SLICE_SCALABLE && m_eNalUnitType != NAL_UNIT_PREFIX && m_eNalUnitType != NAL_UNIT_SUBSET_SPS; }
  Bool              isRefPic                ()  const { return  m_eNalRefIdc != NAL_REF_IDC_PRIORITY_LOWEST; }
  NalRefIdc         getNalRefIdc            ()  const { return  m_eNalRefIdc;             }
  NalUnitType       getNalUnitType          ()  const { return  m_eNalUnitType;           }
  Bool              getIdrFlag              ()  const { return  m_bIdrFlag;               }
  UInt              getPriorityId           ()  const { return  m_uiPriorityId;           }
  Bool              getNoInterLayerPredFlag ()  const { return  m_bNoInterLayerPredFlag;  }
  UInt              getDependencyId         ()  const { return  m_uiDependencyId;         }
  UInt              getQualityId            ()  const { return  m_uiQualityId;            }
  UInt              getTemporalId           ()  const { return  m_uiTemporalId;           }
  Bool              getUseRefBasePicFlag    ()  const { return  m_bUseRefBasePicFlag;     }
  Bool              getDiscardableFlag      ()  const { return  m_bDiscardableFlag;       }
  Bool              getOutputFlag           ()  const { return  m_bOutputFlag;            }

  Void              setNalRefIdc            ( NalRefIdc   eNalRefIdc            ) { m_eNalRefIdc            = eNalRefIdc;             }
  Void              setNalUnitType          ( NalUnitType eNalUnitType          ) { m_eNalUnitType          = eNalUnitType;           }
  Void              setIdrFlag              ( Bool        bIdrFlag              ) { m_bIdrFlag              = bIdrFlag;               }
  Void              setPriorityId           ( UInt        uiPriorityId          ) { m_uiPriorityId          = uiPriorityId;           }
  Void              setNoInterLayerPredFlag ( Bool        bNoInterLayerPredFlag ) { m_bNoInterLayerPredFlag = bNoInterLayerPredFlag;  }
  Void              setDependencyId         ( UInt        uiDependencyId        ) { m_uiDependencyId        = uiDependencyId;         }
  Void              setQualityId            ( UInt        uiQualityId           ) { m_uiQualityId           = uiQualityId;            }
  Void              setTemporalId           ( UInt        uiTemporalId          ) { m_uiTemporalId          = uiTemporalId;           }
  Void              setUseRefBasePicFlag    ( Bool        bUseRefBasePicFlag    ) { m_bUseRefBasePicFlag    = bUseRefBasePicFlag;     }
  Void              setDiscardableFlag      ( Bool        bDiscardableFlag      ) { m_bDiscardableFlag      = bDiscardableFlag;       }
  Void              setOutputFlag           ( Bool        bOutputFlag           ) { m_bOutputFlag           = bOutputFlag;            }

private:
  Bool              m_bForbiddenZeroBit;
  NalRefIdc         m_eNalRefIdc;
  NalUnitType       m_eNalUnitType;
  Bool              m_bReservedOneBit;
  Bool              m_bIdrFlag;
  UInt              m_uiPriorityId;
  Bool              m_bNoInterLayerPredFlag;
  UInt              m_uiDependencyId;
  UInt              m_uiQualityId;
  UInt              m_uiTemporalId;
  Bool              m_bUseRefBasePicFlag;
  Bool              m_bDiscardableFlag;
  Bool              m_bOutputFlag;
  UInt              m_uiReservedThree2Bits;
};



class H264AVCCOMMONLIB_API AUDelimiter : public NalUnitHeader
{
public:
  AUDelimiter();
  AUDelimiter( const NalUnitHeader& rcNalUnitHeader );
  AUDelimiter( const AUDelimiter&   rcAUDelimiter );
  virtual ~AUDelimiter();

  Void    copy  ( const AUDelimiter&    rcAUDelimiter,  Bool bInclusiveNALUnitHeader = true   );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,      Bool bInclusiveNALUnitHeader = true   ) const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,       Bool bInclusiveNALUnitHeader = false  );

  UInt    getPrimaryPicType ()  const                   { return m_uiPrimaryPicType;  }

  Void    setPrimaryPicType ( UInt  uiPrimaryPicType  ) { m_uiPrimaryPicType  = uiPrimaryPicType; }

private:
  UInt    m_uiPrimaryPicType;
};



class H264AVCCOMMONLIB_API EndOfSequence : public NalUnitHeader
{
public:
  EndOfSequence();
  EndOfSequence( const NalUnitHeader& rcNalUnitHeader );
  EndOfSequence( const EndOfSequence& rcEndOfSequence );
  virtual ~EndOfSequence();

  Void    copy  ( const EndOfSequence&  rcEndOfSequence,  Bool bInclusiveNALUnitHeader = true   );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,        Bool bInclusiveNALUnitHeader = true   ) const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,         Bool bInclusiveNALUnitHeader = false  );
};



class H264AVCCOMMONLIB_API EndOfStream : public NalUnitHeader
{
public:
  EndOfStream();
  EndOfStream( const NalUnitHeader& rcNalUnitHeader );
  EndOfStream( const EndOfStream&   rcEndOfStream );
  virtual ~EndOfStream();

  Void    copy  ( const EndOfStream&    rcEndOfStream,    Bool bInclusiveNALUnitHeader = true   );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,        Bool bInclusiveNALUnitHeader = true   ) const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,         Bool bInclusiveNALUnitHeader = false  );
};



class H264AVCCOMMONLIB_API FillerData : public NalUnitHeader
{
public:
  FillerData();
  FillerData( const NalUnitHeader&  rcNalUnitHeader );
  FillerData( const FillerData&     rcFillerData );
  virtual ~FillerData();

  Void    copy        ( const FillerData&    rcFillerData,  Bool bInclusiveNalUnitHeader = true  );
  ErrVal  writePrefix ( HeaderSymbolWriteIf& rcWriteIf,     Bool bInclusiveNalUnitHeader = true  ) const;
  ErrVal  write       ( HeaderSymbolWriteIf& rcWriteIf,     Bool bInclusiveNalUnitHeader = true  ) const;
  ErrVal  read        ( HeaderSymbolReadIf&  rcReadIf,      Bool bInclusiveNalUnitHeader = false );

  UInt    getNumFFBytes() const   { return m_uiNumFFBytes; }

  Void    setNumFFBytes( UInt uiNumFFBytes )  { m_uiNumFFBytes = uiNumFFBytes; }

private:
  UInt  m_uiNumFFBytes;
};



class H264AVCCOMMONLIB_API PrefixHeader : public NalUnitHeader
{
public:
  PrefixHeader();
  PrefixHeader( const NalUnitHeader& rcNalUnitHeader );
  PrefixHeader( const PrefixHeader&  rcPrefixHeader  );
  virtual ~PrefixHeader();

  Void    copy  ( const PrefixHeader&   rcPrefixHeader, Bool bInclusiveNALUnitHeader = true   );
  ErrVal  write ( HeaderSymbolWriteIf&  rcWriteIf,      Bool bInclusiveNALUnitHeader = true   ) const;
  ErrVal  read  ( HeaderSymbolReadIf&   rcReadIf,       Bool bInclusiveNALUnitHeader = false  );

  Bool                    getStoreRefBasePicFlag  ()  const { return m_bStoreRefBasePicFlag;  }
  const DecRefPicMarking& getDecRefBasePicMarking ()  const { return m_cDecRefBasePicMarking; }
  DecRefPicMarking&       getDecRefBasePicMarking ()        { return m_cDecRefBasePicMarking; }

  Void                    setStoreRefBasePicFlag  ( Bool  bStoreRefBasePicFlag  ) { m_bStoreRefBasePicFlag  = bStoreRefBasePicFlag; }

protected:
  Bool              m_bStoreRefBasePicFlag;
  DecRefPicMarking  m_cDecRefBasePicMarking;
private:
  Bool              m_bPrefixNalUnitAdditionalExtensionFlag;
};



class H264AVCCOMMONLIB_API SliceHeaderSyntax : public PrefixHeader
{
public:
  SliceHeaderSyntax();
  SliceHeaderSyntax( const NalUnitHeader&         rcNalUnitHeader );
  SliceHeaderSyntax( const PrefixHeader&          rcPrefixHeader );
  SliceHeaderSyntax( const SliceHeaderSyntax&     rcSliceHeaderSyntax );
  SliceHeaderSyntax( const SequenceParameterSet&  rcSPS,
                     const PictureParameterSet&   rcPPS );
  virtual ~SliceHeaderSyntax();

  ErrVal  init        ( const SequenceParameterSet& rcSPS,
                        const PictureParameterSet&  rcPPS );
  Void    copy        ( const SliceHeaderSyntax&    rcSliceHeaderSyntax,  Bool bInclusiveNalUnitHeader = true  );
  ErrVal  writePrefix ( HeaderSymbolWriteIf&        rcWriteIf,            Bool bInclusiveNalUnitHeader = true  ) const;
  ErrVal  write       ( HeaderSymbolWriteIf&        rcWriteIf,            Bool bInclusiveNalUnitHeader = true  ) const;
  ErrVal  read        ( ParameterSetMng&            rcParameterSetMng,
                        HeaderSymbolReadIf&         rcReadIf,             Bool bInclusiveNalUnitHeader = false );

  Bool    isPSlice      ()  const   { return ( m_eSliceType % 5 ) ==  P_SLICE; }
  Bool    isBSlice      ()  const   { return ( m_eSliceType % 5 ) ==  B_SLICE; }
  Bool    isISlice      ()  const   { return ( m_eSliceType % 5 ) ==  I_SLICE; }
  Bool    isSPSlice     ()  const   { return ( m_eSliceType % 5 ) == SP_SLICE; }
  Bool    isSISlice     ()  const   { return ( m_eSliceType % 5 ) == SI_SLICE; }
  Bool    isIntraSlice  ()  const   { return isISlice() || isSISlice    (); }
  Bool    isPorSPSlice  ()  const   { return isPSlice() || isSPSlice    (); }
  Bool    isInterSlice  ()  const   { return isBSlice() || isPorSPSlice (); }
  Bool    isMbaffFrame  ()  const   { AOF( parameterSetsInitialized() ); return getSPS().getMbAdaptiveFrameFieldFlag() && !m_bFieldPicFlag; }
  Bool    isFrameMbsOnly()  const   { AOF( parameterSetsInitialized() ); return getSPS().getFrameMbsOnlyFlag(); }
  Int     getPPSQp      ()  const   { AOF( parameterSetsInitialized() ); return getPPS().getPicInitQp(); }

  UInt                            getFirstMbInSlice                     ()                    const { return ( isMbaffFrame() ? m_uiFirstMbInSlice << 1 : m_uiFirstMbInSlice ); }
  SliceType                       getSliceType                          ()                    const { return SliceType( m_eSliceType % 5 ); }
  UInt                            getPicParameterSetId                  ()                    const { return m_uiPicParameterSetId; }
  UInt                            getColourPlaneId                      ()                    const { return m_uiColourPlaneId; }
  UInt                            getFrameNum                           ()                    const { return m_uiFrameNum; }
  Bool                            getFieldPicFlag                       ()                    const { return m_bFieldPicFlag; }
  Bool                            getBottomFieldFlag                    ()                    const { return m_bBottomFieldFlag; }
  UInt                            getIdrPicId                           ()                    const { return m_uiIdrPicId; }
  UInt                            getPicOrderCntLsb                     ()                    const { return m_uiPicOrderCntLsb; }
  Int                             getDeltaPicOrderCntBottom             ()                    const { return m_iDeltaPicOrderCntBottom; }
  Int                             getDeltaPicOrderCnt0                  ()                    const { return m_iDeltaPicOrderCnt0; }
  Int                             getDeltaPicOrderCnt1                  ()                    const { return m_iDeltaPicOrderCnt1; }
  UInt                            getRedundantPicCnt                    ()                    const { return m_uiRedundantPicCnt; }
  Bool                            getDirectSpatialMvPredFlag            ()                    const { return m_bDirectSpatialMvPredFlag; }
  Bool                            getNumRefIdxActiveOverrideFlag        ()                    const { return m_bNumRefIdxActiveOverrideFlag; }
  UInt                            getNumRefIdxL0ActiveMinus1            ()                    const { return m_uiNumRefIdxL0ActiveMinus1; }
  UInt                            getNumRefIdxL0Active                  ()                    const { return m_uiNumRefIdxL0ActiveMinus1 + 1; }
  UInt                            getNumRefIdxL1ActiveMinus1            ()                    const { return m_uiNumRefIdxL1ActiveMinus1; }
  UInt                            getNumRefIdxL1Active                  ()                    const { return m_uiNumRefIdxL1ActiveMinus1 + 1; }
  UInt                            getNumRefIdxActive                    ( ListIdx eListIdx )  const { return ( eListIdx == LIST_0 ? m_uiNumRefIdxL0ActiveMinus1 + 1 : m_uiNumRefIdxL1ActiveMinus1 + 1 ); }
  const RefPicListReOrdering&     getRefPicListReorderingL0             ()                    const { return m_cRefPicListReorderingL0; }
  RefPicListReOrdering&           getRefPicListReorderingL0             ()                          { return m_cRefPicListReorderingL0; }
  const RefPicListReOrdering&     getRefPicListReorderingL1             ()                    const { return m_cRefPicListReorderingL1; }
  RefPicListReOrdering&           getRefPicListReorderingL1             ()                          { return m_cRefPicListReorderingL1; }
  const RefPicListReOrdering&     getRefPicListReordering               ( ListIdx eListIdx )  const { return ( eListIdx == LIST_0 ? m_cRefPicListReorderingL0 : m_cRefPicListReorderingL1 ); }
  RefPicListReOrdering&           getRefPicListReordering               ( ListIdx eListIdx )        { return ( eListIdx == LIST_0 ? m_cRefPicListReorderingL0 : m_cRefPicListReorderingL1 ); }
  Bool                            getBasePredWeightTableFlag            ()                    const { return m_bBasePredWeightTableFlag; }
  UInt                            getLumaLog2WeightDenom                ()                    const { return m_uiLumaLog2WeightDenom; }
  UInt                            getLumaWeightDenom                    ()                    const { return ( 1 << m_uiLumaLog2WeightDenom ); }
  UInt                            getChromaLog2WeightDenom              ()                    const { return m_uiChromaLog2WeightDenom; }
  UInt                            getChromaWeightDenom                  ()                    const { return ( 1 << m_uiChromaLog2WeightDenom ); }
  const PredWeightTable&          getPredWeightTableL0                  ()                    const { return m_cPredWeightTableL0; }
  PredWeightTable&                getPredWeightTableL0                  ()                          { return m_cPredWeightTableL0; }
  const PredWeightTable&          getPredWeightTableL1                  ()                    const { return m_cPredWeightTableL1; }
  PredWeightTable&                getPredWeightTableL1                  ()                          { return m_cPredWeightTableL1; }
  const PredWeightTable&          getPredWeightTable                    ( ListIdx eListIdx )  const { return ( eListIdx == LIST_0 ? m_cPredWeightTableL0 : m_cPredWeightTableL1 ); }
  PredWeightTable&                getPredWeightTable                    ( ListIdx eListIdx )        { return ( eListIdx == LIST_0 ? m_cPredWeightTableL0 : m_cPredWeightTableL1 ); }
  Bool                            getNoOutputOfPriorPicsFlag            ()                    const { return m_bNoOutputOfPriorPicsFlag; }
  Bool                            getLongTermReferenceFlag              ()                    const { return m_bLongTermReferenceFlag; }
  const DecRefPicMarking&         getDecRefPicMarking                   ()                    const { return m_cDecRefPicMarking; }
  DecRefPicMarking&               getDecRefPicMarking                   ()                          { return m_cDecRefPicMarking; }
  UInt                            getCabacInitIdc                       ()                    const { return m_uiCabacInitIdc; }
  Int                             getSliceQpDelta                       ()                    const { return m_iSliceQpDelta; }
  Int                             getSliceQp                            ()                    const { return m_iSliceQpDelta + getPPSQp(); }
  Bool                            getSPForSwitchFlag                    ()                    const { return m_bSPForSwitchFlag; }
  Int                             getSliceQsDelta                       ()                    const { return m_iSliceQsDelta; }
  const DBFilterParameter&        getDeblockingFilterParameter          ()                    const { return m_cDeblockingFilterParameter; }
  DBFilterParameter&              getDeblockingFilterParameter          ()                          { return m_cDeblockingFilterParameter; }
  UInt                            getSliceGroupChangeCycle              ()                    const { return m_uiSliceGroupChangeCycle; }
  UInt                            getRefLayerDQId                       ()                    const { return m_uiRefLayerDQId; }
  UInt                            getRefLayerDependencyId               ()                    const { return m_uiRefLayerDQId >> 4; }
  UInt                            getRefLayerQualityId                  ()                    const { return m_uiRefLayerDQId & 15; }
  const DBFilterParameter&        getInterLayerDeblockingFilterParameter()                    const { return m_cInterLayerDeblockingFilterParameter; }
  DBFilterParameter&              getInterLayerDeblockingFilterParameter()                          { return m_cInterLayerDeblockingFilterParameter; }
  Bool                            getConstrainedIntraResamplingFlag     ()                    const { return m_bConstrainedIntraResamplingFlag; }
  Bool                            getRefLayerChromaPhaseXPlus1Flag      ()                    const { return m_bRefLayerChromaPhaseXPlus1Flag; }
  UInt                            getRefLayerChromaPhaseXPlus1          ()                    const { return m_bRefLayerChromaPhaseXPlus1Flag ? 1 : 0; }
  UInt                            getRefLayerChromaPhaseYPlus1          ()                    const { return m_uiRefLayerChromaPhaseYPlus1; }
  Int                             getRefLayerChromaPhaseX               ()                    const { return m_bRefLayerChromaPhaseXPlus1Flag ? 0 : -1; }
  Int                             getRefLayerChromaPhaseY               ()                    const { return m_uiRefLayerChromaPhaseYPlus1 - 1; }
  Int                             getScaledRefLayerLeftOffset           ()                    const { return m_iScaledRefLayerLeftOffset; }
  Int                             getScaledRefLayerTopOffset            ()                    const { return ( isFrameMbsOnly() ? m_iScaledRefLayerTopOffset : m_iScaledRefLayerTopOffset << 1 ); }
  Int                             getScaledRefLayerRightOffset          ()                    const { return m_iScaledRefLayerRightOffset; }
  Int                             getScaledRefLayerBottomOffset         ()                    const { return ( isFrameMbsOnly() ? m_iScaledRefLayerBottomOffset : m_iScaledRefLayerBottomOffset << 1 ); }
  Bool                            getSliceSkipFlag                      ()                    const { return m_bSliceSkipFlag; }
  UInt                            getNumMbsInSliceMinus1                ()                    const { return m_uiNumMbsInSliceMinus1; }
  Bool                            getAdaptiveBaseModeFlag               ()                    const { return m_bAdaptiveBaseModeFlag; }
  Bool                            getDefaultBaseModeFlag                ()                    const { return m_bDefaultBaseModeFlag; }
  Bool                            getAdaptiveMotionPredictionFlag       ()                    const { return m_bAdaptiveMotionPredictionFlag; }
  Bool                            getDefaultMotionPredictionFlag        ()                    const { return m_bDefaultMotionPredictionFlag; }
  Bool                            getAdaptiveResidualPredictionFlag     ()                    const { return m_bAdaptiveResidualPredictionFlag; }
  Bool                            getDefaultResidualPredictionFlag      ()                    const { return m_bDefaultResidualPredictionFlag; }
  Bool                            getTCoeffLevelPredictionFlag          ()                    const { return m_bTCoeffLevelPredictionFlag; }
  UInt                            getScanIdxStart                       ()                    const { return m_uiScanIdxStart; }
  UInt                            getScanIdxStop                        ()                    const { return m_uiScanIdxStop; }
  Bool                            hasDefaultScanIdx                     ()                    const { return m_uiScanIdxStart == 0 && m_uiScanIdxStop == 16; }
  Bool                            parameterSetsInitialized              ()                    const { return m_pcPPS != 0 && m_pcSPS != 0; }
  const SequenceParameterSet&     getSPS                                ()                    const { AOF( parameterSetsInitialized() ); return *m_pcSPS; }
  SequenceParameterSet&           getSPS                                ()                          { AOF( parameterSetsInitialized() ); return const_cast<SequenceParameterSet&>(*m_pcSPS); }
  const PictureParameterSet&      getPPS                                ()                    const { AOF( parameterSetsInitialized() ); return *m_pcPPS; }
  const StatBuf<const UChar*,8>&  getScalingMatrix                      ()                    const { return m_acScalingMatrix; }
  StatBuf<const UChar*,8>&        getScalingMatrix                      ()                          { return m_acScalingMatrix; }


  Void  setFirstMbInSlice                     ( UInt                        uiFirstMbInSlice                      )  { m_uiFirstMbInSlice                     = ( isMbaffFrame() ? uiFirstMbInSlice >> 1 : uiFirstMbInSlice ); }
  Void  setSliceType                          ( SliceType                   eSliceType                            )  { m_eSliceType                           = eSliceType; }
  Void  setPicParameterSetId                  ( UInt                        uiPicParameterSetId                   )  { m_uiPicParameterSetId                  = uiPicParameterSetId; }
  Void  setColourPlaneId                      ( UInt                        uiColourPlaneId                       )  { m_uiColourPlaneId                      = uiColourPlaneId; }
  Void  setFrameNum                           ( UInt                        uiFrameNum                            )  { m_uiFrameNum                           = uiFrameNum; }
  Void  setFieldPicFlag                       ( Bool                        bFieldPicFlag                         )  { m_bFieldPicFlag                        = bFieldPicFlag; }
  Void  setBottomFieldFlag                    ( Bool                        bBottomFieldFlag                      )  { m_bBottomFieldFlag                     = bBottomFieldFlag; }
  Void  setIdrPicId                           ( UInt                        uiIdrPicId                            )  { m_uiIdrPicId                           = uiIdrPicId; }
  Void  setPicOrderCntLsb                     ( UInt                        uiPicOrderCntLsb                      )  { m_uiPicOrderCntLsb                     = uiPicOrderCntLsb; }
  Void  setDeltaPicOrderCntBottom             ( Int                         iDeltaPicOrderCntBottom               )  { m_iDeltaPicOrderCntBottom              = iDeltaPicOrderCntBottom; }
  Void  setDeltaPicOrderCnt0                  ( Int                         iDeltaPicOrderCnt0                    )  { m_iDeltaPicOrderCnt0                   = iDeltaPicOrderCnt0; }
  Void  setDeltaPicOrderCnt1                  ( Int                         iDeltaPicOrderCnt1                    )  { m_iDeltaPicOrderCnt1                   = iDeltaPicOrderCnt1; }
  Void  setRedundantPicCnt                    ( UInt                        uiRedundantPicCnt                     )  { m_uiRedundantPicCnt                    = uiRedundantPicCnt; }
  Void  setDirectSpatialMvPredFlag            ( Bool                        bDirectSpatialMvPredFlag              )  { m_bDirectSpatialMvPredFlag             = bDirectSpatialMvPredFlag; }
  Void  setNumRefIdxActiveOverrideFlag        ( Bool                        bNumRefIdxActiveOverrideFlag          )  { m_bNumRefIdxActiveOverrideFlag         = bNumRefIdxActiveOverrideFlag; }
  Void  setNumRefIdxL0ActiveMinus1            ( UInt                        uiNumRefIdxL0ActiveMinus1             )  { m_uiNumRefIdxL0ActiveMinus1            = uiNumRefIdxL0ActiveMinus1; }
  Void  setNumRefIdxL0Active                  ( UInt                        uiNumRefIdxL0Active                   )  { m_uiNumRefIdxL0ActiveMinus1            = uiNumRefIdxL0Active - 1; }
  Void  setNumRefIdxL1ActiveMinus1            ( UInt                        uiNumRefIdxL1ActiveMinus1             )  { m_uiNumRefIdxL1ActiveMinus1            = uiNumRefIdxL1ActiveMinus1; }
  Void  setNumRefIdxL1Active                  ( UInt                        uiNumRefIdxL1Active                   )  { m_uiNumRefIdxL1ActiveMinus1            = uiNumRefIdxL1Active - 1; }
  Void  setNumRefIdxActive                    ( ListIdx                     eListIdx,
                                                UInt                        uiNumRefIdxActive                     )  { if( eListIdx == LIST_0 ) m_uiNumRefIdxL0ActiveMinus1 = uiNumRefIdxActive - 1; else m_uiNumRefIdxL1ActiveMinus1 = uiNumRefIdxActive - 1; }
  Void  setRefPicListReorderingL0             ( const RefPicListReOrdering& rcRefPicListReorderingL0              )  { m_cRefPicListReorderingL0              = rcRefPicListReorderingL0; }
  Void  setRefPicListReorderingL1             ( const RefPicListReOrdering& rcRefPicListReorderingL1              )  { m_cRefPicListReorderingL1              = rcRefPicListReorderingL1; }
  Void  setBasePredWeightTableFlag            ( Bool                        bBasePredWeightTableFlag              )  { m_bBasePredWeightTableFlag             = bBasePredWeightTableFlag; }
  Void  setLumaLog2WeightDenom                ( UInt                        uiLumaLog2WeightDenom                 )  { m_uiLumaLog2WeightDenom                = uiLumaLog2WeightDenom; }
  Void  setChromaLog2WeightDenom              ( UInt                        uiChromaLog2WeightDenom               )  { m_uiChromaLog2WeightDenom              = uiChromaLog2WeightDenom; }
  Void  setPredWeightTableL0                  ( const PredWeightTable&      rcPredWeightTableL0                   )  { m_cPredWeightTableL0                   = rcPredWeightTableL0; }
  Void  setPredWeightTableL1                  ( const PredWeightTable&      rcPredWeightTableL1                   )  { m_cPredWeightTableL1                   = rcPredWeightTableL1; }
  Void  setNoOutputOfPriorPicsFlag            ( Bool                        bNoOutputOfPriorPicsFlag              )  { m_bNoOutputOfPriorPicsFlag             = bNoOutputOfPriorPicsFlag; }
  Void  setLongTermReferenceFlag              ( Bool                        bLongTermReferenceFlag                )  { m_bLongTermReferenceFlag               = bLongTermReferenceFlag; }
  Void  setDecRefPicMarking                   ( const DecRefPicMarking&     rcDecRefPicMarking                    )  { m_cDecRefPicMarking                    = rcDecRefPicMarking; }
  Void  setCabacInitIdc                       ( UInt                        uiCabacInitIdc                        )  { m_uiCabacInitIdc                       = uiCabacInitIdc; }
  Void  setSliceQpDelta                       ( Int                         iSliceQpDelta                         )  { m_iSliceQpDelta                        = iSliceQpDelta; }
  Void  setSliceHeaderQp                      ( Int                         iSliceHeaderQp                        )  { m_iSliceQpDelta                        = iSliceHeaderQp - getPPSQp(); }
  Void  setSPForSwitchFlag                    ( Bool                        bSPForSwitchFlag                      )  { m_bSPForSwitchFlag                     = bSPForSwitchFlag; }
  Void  setSliceQsDelta                       ( Int                         iSliceQsDelta                         )  { m_iSliceQsDelta                        = iSliceQsDelta; }
  Void  setDeblockingFilterParameter          ( const DBFilterParameter&    rcDeblockingFilterParameter           )  { m_cDeblockingFilterParameter           = rcDeblockingFilterParameter; }
  Void  setSliceGroupChangeCycle              ( UInt                        uiSliceGroupChangeCycle               )  { m_uiSliceGroupChangeCycle              = uiSliceGroupChangeCycle; }
  Void  setRefLayerDQId                       ( UInt                        uiRefLayerDQId                        )  { m_uiRefLayerDQId                       = uiRefLayerDQId; }
  Void  setRefLayer                           ( UInt                        uiRefLayerDependencyId,
                                                UInt                        uiRefLayerQualityId                   )  { m_uiRefLayerDQId                       = ( uiRefLayerDependencyId << 4 ) + uiRefLayerQualityId; }
  Void  setInterLayerDeblockingFilterParameter( const DBFilterParameter&    rcInterLayerDeblockingFilterParameter )  { m_cInterLayerDeblockingFilterParameter = rcInterLayerDeblockingFilterParameter; }
  Void  setConstrainedIntraResamplingFlag     ( Bool                        bConstrainedIntraResamplingFlag       )  { m_bConstrainedIntraResamplingFlag      = bConstrainedIntraResamplingFlag; }
  Void  setRefLayerChromaPhaseXPlus1Flag      ( Bool                        bRefLayerChromaPhaseXPlus1Flag        )  { m_bRefLayerChromaPhaseXPlus1Flag       = bRefLayerChromaPhaseXPlus1Flag; }
  Void  setRefLayerChromaPhaseXPlus1          ( UInt                        uiRefLayerChromaPhaseXPlus1           )  { m_bRefLayerChromaPhaseXPlus1Flag       = uiRefLayerChromaPhaseXPlus1 > 0; }
  Void  setRefLayerChromaPhaseYPlus1          ( UInt                        uiRefLayerChromaPhaseYPlus1           )  { m_uiRefLayerChromaPhaseYPlus1          = uiRefLayerChromaPhaseYPlus1; }
  Void  setRefLayerChromaPhaseX               ( Int                         iRefLayerChromaPhaseX                 )  { m_bRefLayerChromaPhaseXPlus1Flag       = iRefLayerChromaPhaseX == 0; }
  Void  setRefLayerChromaPhaseY               ( Int                         iRefLayerChromaPhaseY                 )  { m_uiRefLayerChromaPhaseYPlus1          = iRefLayerChromaPhaseY + 1; }
  Void  setScaledRefLayerLeftOffset           ( Int                         iScaledRefLayerLeftOffset             )  { m_iScaledRefLayerLeftOffset            = iScaledRefLayerLeftOffset; }
  Void  setScaledRefLayerTopOffset            ( Int                         iScaledRefLayerTopOffset              )  { m_iScaledRefLayerTopOffset             = ( isFrameMbsOnly() ? iScaledRefLayerTopOffset    : iScaledRefLayerTopOffset    >> 1 ); }
  Void  setScaledRefLayerRightOffset          ( Int                         iScaledRefLayerRightOffset            )  { m_iScaledRefLayerRightOffset           = iScaledRefLayerRightOffset; }
  Void  setScaledRefLayerBottomOffset         ( Int                         iScaledRefLayerBottomOffset           )  { m_iScaledRefLayerBottomOffset          = ( isFrameMbsOnly() ? iScaledRefLayerBottomOffset : iScaledRefLayerBottomOffset >> 1 ); }
  Void  setSliceSkipFlag                      ( Bool                        bSliceSkipFlag                        )  { m_bSliceSkipFlag                       = bSliceSkipFlag; }
  Void  setNumMbsInSliceMinus1                ( UInt                        uiNumMbsInSliceMinus1                 )  { m_uiNumMbsInSliceMinus1                = uiNumMbsInSliceMinus1; }
  Void  setAdaptiveBaseModeFlag               ( Bool                        bAdaptiveBaseModeFlag                 )  { m_bAdaptiveBaseModeFlag                = bAdaptiveBaseModeFlag; }
  Void  setDefaultBaseModeFlag                ( Bool                        bDefaultBaseModeFlag                  )  { m_bDefaultBaseModeFlag                 = bDefaultBaseModeFlag; }
  Void  setAdaptiveMotionPredictionFlag       ( Bool                        bAdaptiveMotionPredictionFlag         )  { m_bAdaptiveMotionPredictionFlag        = bAdaptiveMotionPredictionFlag; }
  Void  setDefaultMotionPredictionFlag        ( Bool                        bDefaultMotionPredictionFlag          )  { m_bDefaultMotionPredictionFlag         = bDefaultMotionPredictionFlag; }
  Void  setAdaptiveResidualPredictionFlag     ( Bool                        bAdaptiveResidualPredictionFlag       )  { m_bAdaptiveResidualPredictionFlag      = bAdaptiveResidualPredictionFlag; }
  Void  setDefaultResidualPredictionFlag      ( Bool                        bDefaultResidualPredictionFlag        )  { m_bDefaultResidualPredictionFlag       = bDefaultResidualPredictionFlag; }
  Void  setTCoeffLevelPredictionFlag          ( Bool                        bTCoeffLevelPredictionFlag            )  { m_bTCoeffLevelPredictionFlag           = bTCoeffLevelPredictionFlag; }
  Void  setScanIdxStart                       ( UInt                        uiScanIdxStart                        )  { m_uiScanIdxStart                       = uiScanIdxStart; }
  Void  setScanIdxStop                        ( UInt                        uiScanIdxStop                         )  { m_uiScanIdxStop                        = uiScanIdxStop; }

private:
  Void    xInit               ();
  Void    xCopy               ( const SliceHeaderSyntax&    rcSliceHeaderSyntax );
  ErrVal  xInitScalingMatrix  ();
  ErrVal  xInitParameterSets  ( ParameterSetMng&            rcParameterSetMng,
                                UInt                        uiPPSId,
                                Bool                        bSubSetSPS );
  ErrVal  xInitParameterSets  ( const SequenceParameterSet& rcSPS,
                                const PictureParameterSet&  rcPPS );

private:
  //===== syntax =====
  UInt                        m_uiFirstMbInSlice;
  SliceType                   m_eSliceType;
  UInt                        m_uiPicParameterSetId;
  UInt                        m_uiColourPlaneId;
  UInt                        m_uiFrameNum;
  Bool                        m_bFieldPicFlag;
  Bool                        m_bBottomFieldFlag;
  UInt                        m_uiIdrPicId;
  UInt                        m_uiPicOrderCntLsb;
  Int                         m_iDeltaPicOrderCntBottom;
  Int                         m_iDeltaPicOrderCnt0;
  Int                         m_iDeltaPicOrderCnt1;
  UInt                        m_uiRedundantPicCnt;
  Bool                        m_bDirectSpatialMvPredFlag;
  Bool                        m_bNumRefIdxActiveOverrideFlag;
  UInt                        m_uiNumRefIdxL0ActiveMinus1;
  UInt                        m_uiNumRefIdxL1ActiveMinus1;
  RefPicListReOrdering        m_cRefPicListReorderingL0;
  RefPicListReOrdering        m_cRefPicListReorderingL1;
  Bool                        m_bBasePredWeightTableFlag;
  UInt                        m_uiLumaLog2WeightDenom;
  UInt                        m_uiChromaLog2WeightDenom;
  PredWeightTable             m_cPredWeightTableL0;
  PredWeightTable             m_cPredWeightTableL1;
  Bool                        m_bNoOutputOfPriorPicsFlag;
  Bool                        m_bLongTermReferenceFlag;
  DecRefPicMarking            m_cDecRefPicMarking;
  UInt                        m_uiCabacInitIdc;
  Int                         m_iSliceQpDelta;
  Bool                        m_bSPForSwitchFlag;
  Int                         m_iSliceQsDelta;
  DBFilterParameter           m_cDeblockingFilterParameter;
  UInt                        m_uiSliceGroupChangeCycle;
  UInt                        m_uiRefLayerDQId;
  DBFilterParameter           m_cInterLayerDeblockingFilterParameter;
  Bool                        m_bConstrainedIntraResamplingFlag;
  Bool                        m_bRefLayerChromaPhaseXPlus1Flag;
  UInt                        m_uiRefLayerChromaPhaseYPlus1;
  Int                         m_iScaledRefLayerLeftOffset;
  Int                         m_iScaledRefLayerTopOffset;
  Int                         m_iScaledRefLayerRightOffset;
  Int                         m_iScaledRefLayerBottomOffset;
  Bool                        m_bSliceSkipFlag;
  UInt                        m_uiNumMbsInSliceMinus1;
  Bool                        m_bAdaptiveBaseModeFlag;
  Bool                        m_bDefaultBaseModeFlag;
  Bool                        m_bAdaptiveMotionPredictionFlag;
  Bool                        m_bDefaultMotionPredictionFlag;
  Bool                        m_bAdaptiveResidualPredictionFlag;
  Bool                        m_bDefaultResidualPredictionFlag;
  Bool                        m_bTCoeffLevelPredictionFlag;
  UInt                        m_uiScanIdxStart;
  UInt                        m_uiScanIdxStop;

  //===== derived parameters =====
  const SequenceParameterSet* m_pcSPS;
  const PictureParameterSet*  m_pcPPS;
  StatBuf< const UChar*, 8 >  m_acScalingMatrix;
};




#if defined( WIN32 )
# pragma warning( default: 4251 )
# pragma warning( default: 4275 )
#endif

H264AVC_NAMESPACE_END



#endif // !defined(AFX_SLICEHEADERBASE_H__2CC1FD0F_CACB_4799_84BE_FC5FC9B9C245__INCLUDED_)
