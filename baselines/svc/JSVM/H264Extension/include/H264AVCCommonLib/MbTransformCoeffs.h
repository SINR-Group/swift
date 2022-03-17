
#if !defined(AFX_MBTRANSFORMCOEFFS_H__B5704512_C23C_497A_A794_36691A0D01BE__INCLUDED_)
#define AFX_MBTRANSFORMCOEFFS_H__B5704512_C23C_497A_A794_36691A0D01BE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN

class YuvMbBuffer;

class H264AVCCOMMONLIB_API MbTransformCoeffs
{
public:
  ErrVal copyPredictionFrom( YuvMbBuffer &rcPred );
  ErrVal copyPredictionTo( YuvMbBuffer &rcPred );
  ErrVal clearPrediction();

  MbTransformCoeffs() ;
  TCoeff*       get   ( LumaIdx   cLumaIdx )          { return &m_aaiLevel[cLumaIdx     ][0]; }
  const TCoeff* get   ( LumaIdx   cLumaIdx )    const { return &m_aaiLevel[cLumaIdx     ][0]; }

  TCoeff*       get   ( ChromaIdx cChromaIdx )        { return &m_aaiLevel[16+cChromaIdx][0]; }
  const TCoeff* get   ( ChromaIdx cChromaIdx )  const { return &m_aaiLevel[16+cChromaIdx][0]; }

  TCoeff*       get8x8( B8x8Idx   c8x8Idx )           { return &m_aaiLevel[4*c8x8Idx.b8x8Index()][0]; }
  const TCoeff* get8x8( B8x8Idx   c8x8Idx )     const { return &m_aaiLevel[4*c8x8Idx.b8x8Index()][0]; }

  TCoeff*       getTCoeffBuffer()                     { return &m_aaiLevel[0][0]; }
  const TCoeff* getTCoeffBuffer()               const { return &m_aaiLevel[0][0]; }

  UInt calcCoeffCount( LumaIdx cLumaIdx, Bool bIs8x8, Bool bInterlaced, UInt uiStart, UInt uiStop ) const;
  UInt calcCoeffCount( ChromaIdx cChromaIdx, const UChar *pucScan, UInt uiStart, UInt uiStop ) const;
  UInt getCoeffCount( LumaIdx cLumaIdx )                   const { return m_aaucCoeffCount[cLumaIdx]; }
  UInt getCoeffCount( ChromaIdx cChromaIdx )               const { return m_aaucCoeffCount[16+cChromaIdx]; }
  Void setCoeffCount( LumaIdx cLumaIdx, UInt uiCoeffCount )      { m_aaucCoeffCount[cLumaIdx] = uiCoeffCount; }
  Void setCoeffCount( ChromaIdx cChromaIdx, UInt uiCoeffCount )  { m_aaucCoeffCount[16+cChromaIdx] = uiCoeffCount; }
  Void clear();
  Void setAllCoeffCount( UChar ucCoeffCountValue = 0 );
  Void copyFrom( const MbTransformCoeffs& rcMbTransformCoeffs );
  Void copyFrom( MbTransformCoeffs& rcMbTransformCoeffs, ChromaIdx cChromaIdx );
  Void copyCoeffCounts( const MbTransformCoeffs& rcMbTransformCoeffs );
  Void reset() {}

	Void dump ( FILE* hFile ) const;

  ErrVal  load( FILE* pFile );
  ErrVal  save( FILE* pFile );

  Void  clearAcBlk                 ( ChromaIdx cChromaIdx );
  Void  clearLumaLevels            ();
  Void  clearChromaLevels          ();
  Void  clearLumaLevels4x4         ( LumaIdx c4x4Idx );
  Void  clearLumaLevels8x8         ( B8x8Idx c8x8Idx );
  Void  clearLumaLevels8x8Block    ( B8x8Idx c8x8Idx );
  Void  clearNewLumaLevels         ( MbTransformCoeffs& rcBaseMbTCoeffs );
  Void  clearNewLumaLevels8x8      ( B8x8Idx c8x8Idx, MbTransformCoeffs& rcBaseMbTCoeffs );
  Void  clearNewLumaLevels8x8Block ( B8x8Idx c8x8Idx, MbTransformCoeffs& rcBaseMbTCoeffs );

  Void storeLevelData              ();
  Void switchLevelCoeffData        ();
  Void add                         ( MbTransformCoeffs* pcCoeffs, Bool bLuma = true, Bool bChroma = true );

  Bool  allCoeffsZero               ()  const;
  Bool  allLevelsZero               ()  const;
  Bool  allLevelsAndPredictionsZero ()  const;

protected:
  TCoeff m_aaiLevel[24][16];
  UChar  m_aaucCoeffCount[24];
};



H264AVC_NAMESPACE_END


#endif // !defined(AFX_MBTRANSFORMCOEFFS_H__B5704512_C23C_497A_A794_36691A0D01BE__INCLUDED_)
