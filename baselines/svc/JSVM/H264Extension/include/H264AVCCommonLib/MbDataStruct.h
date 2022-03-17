
#if !defined(AFX_MBDATASTRUCT_H__353D9AA9_2CC4_4959_94DB_97456E3C2454__INCLUDED_)
#define AFX_MBDATASTRUCT_H__353D9AA9_2CC4_4959_94DB_97456E3C2454__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


H264AVC_NAMESPACE_BEGIN


class H264AVCCOMMONLIB_API MbDataStruct
{
public:
  MbDataStruct();

  Void copy( const MbDataStruct& rcMbDataStruct );
  Void reset();
  Void initMbData( UChar ucQp, UChar ucQp4LF, UInt uiSliceId, UInt uiMbAddr, UInt uiMapUnit, SliceType eSliceType )
  {
    m_eSliceType    = eSliceType;
    m_uiSliceId     = uiSliceId;
    m_uiMbAddr      = uiMbAddr;
    m_uiMapUnit     = uiMapUnit;
    m_ucQp          = ucQp;
    m_ucQp4LF       = ucQp4LF;
  }
  Void copyFrom( const MbDataStruct& rcMbDataStruct );
  Void clear();
  UChar getQpLF()                                         const { return isPCM() ? 0 : m_ucQp4LF; }
  UChar getQp4LF()                                        const { return m_ucQp4LF; }
  Void  setQp4LF( UChar ucQp )                                  { m_ucQp4LF = ucQp; }
  UChar getQp()                                           const { return m_ucQp; }
  Void  setQp( UChar ucQp )                                     { m_ucQp = ucQp; }
  Void  clearIntraPredictionModes( Bool bAll );

  UInt  getFwdBwd ( LumaIdx cIdx    )                     const { return getBlockFwdBwd( Par8x8( ((cIdx.y()>>1)<<1) + (cIdx.x()>>1) ) ); }

  UInt  getSliceIdcLF()                                   const { return m_uiSliceIdcLF; }

  const Bool isInterPMb()                                 const { return m_usFwdBwd == 0x1111; }
  Bool isBlockFwdBwd  ( Par8x8 ePar8x8, ListIdx eLstIdx ) const { const UInt ui = 1<<(eLstIdx + (ePar8x8<<2)); return ( m_usFwdBwd & ui) == ui; }
  UInt getBlockFwdBwd ( Par8x8 ePar8x8 )                  const { return ( m_usFwdBwd >> (ePar8x8<<2) ) & 3; }
  BlkMode getBlkMode( Par8x8 ePar8x8 )                    const { return m_aBlkMode[ePar8x8]; }
  Void setBlkMode( Par8x8 ePar8x8, BlkMode eBlkMode )           { m_aBlkMode[ePar8x8] = eBlkMode; }
  Bool is4x4BlkCoded  ( LumaIdx cLumaIdx )                const { return (0 != ((m_uiMbCbp >> cLumaIdx) & 1)); }
  Bool is4x4BlkResidual( LumaIdx cLumaIdx )               const { return (0 != ((m_uiMbCbpResidual >> cLumaIdx) & 1)); }
  Bool isDQId0AndBlkCoded( LumaIdx cLumaIdx )             const { return (0 != ((m_uiMbCbpDQId0 >> cLumaIdx) & 1)); }
  Bool has4x4NonZeroLevels( LumaIdx cLumaIdx )            const { return (0 != ((m_uiMbCbpLevelsLF >> cLumaIdx) & 1)); }
  UInt getCbpChroma4x4()                                  const { return m_uiMbCbp >> 28; }
  UInt getMbCbp       ()                                  const { return m_uiMbCbp >> 24; }
  UInt getMbExtCbp    ()                                  const { return m_uiMbCbp; }
  Void setMbExtCbp    ( UInt uiCbp )                            { m_uiMbCbp = uiCbp; }
  Void setAndConvertMbExtCbp( UInt uiExtCbp );
  Void setMbCbp       ( UInt uiCbp );
  MbMode  getMbMode         ()                            const { return m_eMbMode; }
  Bool    isInter8x8        ()                            const { return m_eMbMode == INTRA_4X4-2 || m_eMbMode == INTRA_4X4-1; }
  Bool    isIntra4x4        ()                            const { return m_eMbMode == INTRA_4X4; }
  Bool    isIntra16x16      ()                            const { return m_eMbMode  > INTRA_4X4 && m_eMbMode < MODE_PCM; }
  Bool    isIntra           ()                            const { return m_eMbMode >= INTRA_4X4; }
  Bool    isIntraInSlice    ( UInt uiSliceId )            const
  {
    ROFRS( isIntra(),                   false );
    ROTRS( uiSliceId == MSYS_UINT_MAX,  true  ); // special value: don't check sliceId here
    return uiSliceId == m_uiSliceId;
  }
  Bool    isIntraBL         ()                            const { return m_eMbMode == INTRA_BL; } // SSUN@SHARP
  Bool    isIntraButnotIBL  ()                            const { return (m_eMbMode >= INTRA_4X4 && m_eMbMode != INTRA_BL); } // SSUN@SHARP
  Bool    isSkiped          ()                            const { return m_eMbMode == MODE_SKIP; }
  Bool    isPCM             ()                            const { return m_eMbMode == MODE_PCM; }
  Bool    isAcCoded         ()                            const { AOF_DBG(isIntra16x16()); return m_eMbMode>=(INTRA_4X4 + 13); }
  UChar   intraPredMode     ()                            const { AOF_DBG(isIntra16x16()); return (m_eMbMode-(INTRA_4X4+1)) & 3; }
  SChar&  intraPredMode(LumaIdx cIdx)                           { return m_ascIPredMode[cIdx]; }
  SChar   intraPredMode(LumaIdx cIdx)                     const { return m_ascIPredMode[cIdx]; }
  UInt    getCbpChroma16x16 ()                            const { AOF_DBG(isIntra16x16()); return m_aucACTab[(m_eMbMode-(INTRA_4X4+1))>>2 ]; }
  Void    setMbMode( MbMode eMbMode )                           { m_eMbMode = eMbMode; }
  Void    setSliceId( UInt ui )                                 { m_uiSliceId = ui; }
  UInt    getSliceId()                                    const { return m_uiSliceId;}
  UInt    getMbAddr()                                     const { return m_uiMbAddr; }
  UInt    getMapUnit()                                    const { return m_uiMapUnit; }
  SliceType getSliceType()                                const { return m_eSliceType; }
  Void      setChromaPredMode( UChar ucChromaPredMode )         { m_ucChromaPredMode = ucChromaPredMode; }
  UChar     getChromaPredMode()                           const { return m_ucChromaPredMode; }
  Void      setFwdBwd( UShort usFwdBwd )                        { m_usFwdBwd = usFwdBwd; }
  UShort    getFwdBwd()                                   const { return m_usFwdBwd; }
  Void      addFwdBwd( Par8x8 ePar8x8, UInt uiFwdBwdBlk )       { m_usFwdBwd |= uiFwdBwdBlk<<(ePar8x8*4); }
  Void      setBCBPAll( UInt uiBit )                            { m_uiBCBP = (uiBit) ? 0xffff : 0; }
  UInt      getBCBP( UInt uiPos )                         const { return ((m_uiBCBP >> uiPos) & 1); }
  Void      setBCBP( UInt uiPos, UInt uiBit, Bool bReset = false )
  {
    if( bReset )
    {
      m_uiBCBP &= ~(1 << uiPos);
    }
    m_uiBCBP   |= (uiBit << uiPos);
  }
  Void      setBCBP( UInt uiBCBP )                              { m_uiBCBP = uiBCBP; }
  UInt      getBCBP()                                     const { return m_uiBCBP; }
  Bool      getSkipFlag()                                 const { return m_bSkipFlag; }
  Void      setSkipFlag( Bool b)                                { m_bSkipFlag = b; }
  Void      setMbCbpResidual( UInt uiMbCbpResidual )            { m_uiMbCbpResidual = uiMbCbpResidual; }
  Void      setMbCbpDQId0   ( UInt uiMbCbpDQId0 )               { m_uiMbCbpDQId0 = uiMbCbpDQId0; }
  Void      setMbCbpLevelsLF( UInt uiMbCbpLevelsLF )            { m_uiMbCbpLevelsLF = uiMbCbpLevelsLF; }
  Void      setSliceIdcLF   ( UInt uiSliceIdcLF )               { m_uiSliceIdcLF = uiSliceIdcLF; }

  Bool      getResidualPredFlag   ()                      const { return m_bResidualPredFlag; }
  Bool      getInCropWindowFlag   ()                      const { return m_bInCropWindowFlag; }

  Void      setResidualPredFlag   ( Bool        bFlag  )        { m_bResidualPredFlag = bFlag; }
  Void      setInCropWindowFlag   ( Bool        bFlag  )        { m_bInCropWindowFlag = bFlag; }

  ErrVal    save( FILE* pFile );
  ErrVal    load( FILE* pFile );

  Void      setBLSkipFlag         ( Bool b )  { m_bBLSkipFlag = b; }
  Bool      getBLSkipFlag         () const    { return m_bBLSkipFlag; }

  Bool      is8x8TrafoFlagPresent( Bool bDirect8x8Inference ) const;
  Bool      isTransformSize8x8   ()                           const     { return m_bTransformSize8x8; }
  Void      setTransformSize8x8  ( Bool bTransformSize8x8)              { m_bTransformSize8x8 = bTransformSize8x8; }

	Void      setFieldFlag          ( Bool b )  { m_bFieldFlag = b; }
  Bool      getFieldFlag          () const    { return m_bFieldFlag; }

  Void      setSafeResPred        ( Bool b )  { m_bRPSafe = b; }
  Bool      getSafeResPred        () const    { return m_bRPSafe; }

protected:
  UInt      m_uiSliceId;
  UInt      m_uiMbAddr;  
  UInt      m_uiMapUnit;
  SliceType m_eSliceType;
  Bool      m_bBLSkipFlag;
  MbMode    m_eMbMode;
  UInt      m_uiMbCbp;
  UInt      m_uiBCBP;
  UShort    m_usFwdBwd;
  UChar     m_ucChromaPredMode;
  BlkMode   m_aBlkMode[4];
  SChar     m_ascIPredMode[16];
  UChar     m_ucQp;
  UChar     m_ucQp4LF;
  Bool      m_bResidualPredFlag;
  Bool      m_bTransformSize8x8;
  Bool      m_bSkipFlag;
  Bool      m_bInCropWindowFlag;  // indicates if the scaled base layer MB is inside the cropping window
  Bool      m_bFieldFlag;
	UInt      m_uiMbCbpResidual;
  UInt      m_uiMbCbpDQId0;
  UInt      m_uiMbCbpLevelsLF;
  UInt      m_uiSliceIdcLF;

  Bool      m_bRPSafe;

public:
  static const UChar m_aucACTab[7];
};


H264AVC_NAMESPACE_END

#endif // !defined(AFX_MBDATASTRUCT_H__353D9AA9_2CC4_4959_94DB_97456E3C2454__INCLUDED_)
