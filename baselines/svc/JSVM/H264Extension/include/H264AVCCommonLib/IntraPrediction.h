
#if !defined(AFX_INTRAPREDICTION_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
#define AFX_INTRAPREDICTION_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

H264AVC_NAMESPACE_BEGIN


class H264AVCCOMMONLIB_API IntraPrediction
{
protected:
  IntraPrediction();
  virtual ~IntraPrediction();

public:
  static ErrVal create( IntraPrediction*& rpcIntraPrediction );
  ErrVal destroy();
  ErrVal init();
  ErrVal uninit();

public:
  ErrVal predictLumaBlock   ( Pel* puc,               Int iStride, UInt uiPredMode, LumaIdx cIdx );
  ErrVal predictLumaMb      ( Pel* puc,               Int iStride, UInt uiPredMode );
  ErrVal predictChromaBlock ( Pel* pucCb, Pel* pucCr, Int iStride, UInt uiPredMode );
  Void   setAvailableMaskMb ( UInt uiAvailableMaskMb);

  ErrVal predictLumaBlock         ( XPel* puc,                Int iStride, UInt uiPredMode, LumaIdx cIdx );
  ErrVal predictLumaMb            ( XPel* puc,                Int iStride, UInt uiPredMode );
  ErrVal predictChromaBlock       ( XPel* pucCb, XPel* pucCr, Int iStride, UInt uiPredMode );
  ErrVal predictLuma8x8Block      ( XPel* puc,                Int iStride, UInt uiPredMode, B8x8Idx cIdx );

protected:
  Void xPredMode0Vert         ( Pel* puc, Int iStride );
  Void xPredMode1Horiz        ( Pel* puc, Int iStride );
  Void xPredMode2Dc           ( Pel* puc, Int iStride );
  Void xPredMode3DiagDownLeft ( Pel* puc, Int iStride );
  Void xPredMode4DiagDownRight( Pel* puc, Int iStride );
  Void xPredMode5VertRight    ( Pel* puc, Int iStride );
  Void xPredMode6HorizDown    ( Pel* puc, Int iStride );
  Void xPredMode7VertLeft     ( Pel* puc, Int iStride );
  Void xPredMode8HorizUp      ( Pel* puc, Int iStride );

  Void xPred16x16IMode0Vert   ( Pel* puc, Int iStride );
  Void xPred16x16IMode1Hori   ( Pel* puc, Int iStride );
  Void xPred16x16IMode2DC     ( Pel* puc, Int iStride );
  Void xPred16x16IMode3Plane  ( Pel* puc, Int iStride );

  Void xPred8x8IMode0DC       ( Pel* puc, Int iStride );
  Void xPred8x8IMode1Hori     ( Pel* puc, Int iStride );
  Void xPred8x8IMode2Vert     ( Pel* puc, Int iStride );
  Void xPred8x8IMode3Plane    ( Pel* puc, Int iStride );

  Void xPredMode0Vert         ( XPel* puc, Int iStride );
  Void xPredMode1Horiz        ( XPel* puc, Int iStride );
  Void xPredMode2Dc           ( XPel* puc, Int iStride );
  Void xPredMode3DiagDownLeft ( XPel* puc, Int iStride );
  Void xPredMode4DiagDownRight( XPel* puc, Int iStride );
  Void xPredMode5VertRight    ( XPel* puc, Int iStride );
  Void xPredMode6HorizDown    ( XPel* puc, Int iStride );
  Void xPredMode7VertLeft     ( XPel* puc, Int iStride );
  Void xPredMode8HorizUp      ( XPel* puc, Int iStride );

  Void xPred16x16IMode0Vert   ( XPel* puc, Int iStride );
  Void xPred16x16IMode1Hori   ( XPel* puc, Int iStride );
  Void xPred16x16IMode2DC     ( XPel* puc, Int iStride );
  Void xPred16x16IMode3Plane  ( XPel* puc, Int iStride );

  Void xPred8x8IMode0DC       ( XPel* puc, Int iStride );
  Void xPred8x8IMode1Hori     ( XPel* puc, Int iStride );
  Void xPred8x8IMode2Vert     ( XPel* puc, Int iStride );
  Void xPred8x8IMode3Plane    ( XPel* puc, Int iStride );


  Bool xIsLeftRef         ()  { return ( m_uiAvailable & 0x1 ) == 0; }
  Bool xIsAboveRef        ()  { return ( m_uiAvailable & 0x2 ) == 0; }
  Bool xIsAboveLeftRef    ()  { return ( m_uiAvailable & 0x4 ) == 0; }
  Bool xIsAboveRightRef   ()  { return ( m_uiAvailable & 0x8 ) == 0; }
  Bool xIsAllLeftAboveRef ()  { return ( m_uiAvailable & 0x7 ) == 0; }

  UInt xGetAvailableMask  ( LumaIdx cIdx );


  Void xSet8x8AvailableMask         ( B8x8Idx cIdx );

  Void xPredLum8x8Mode0Vert         ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode1Horiz        ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode2Dc           ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode3DiagDownLeft ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode4DiagDownRight( XPel* puc, Int iStride );
  Void xPredLum8x8Mode5VertRight    ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode6HorizDown    ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode7VertLeft     ( XPel* puc, Int iStride );
  Void xPredLum8x8Mode8HorizUp      ( XPel* puc, Int iStride );


private:
  __inline Void xChroma0PredAllInside ( Pel* puc, Int iStride);
  __inline Void xChroma0PredAllOutside( Pel* puc, Int iStride);
  __inline Void xChroma0PredNoLeftRef ( Pel* puc, Int iStride);
  __inline Void xChroma0PredNoAboveRef( Pel* puc, Int iStride);

  __inline UInt xGetS0( Pel* puc, Int iStride );
  __inline UInt xGetS1( Pel* puc, Int iStride );
  __inline UInt xGetS2( Pel* puc, Int iStride );
  __inline UInt xGetS3( Pel* puc, Int iStride );

  __inline Void xLoadPredictorsABCD( Pel* puc, Int iStride, UInt& A, UInt& B, UInt& C, UInt& D );
  __inline Void xLoadPredictorsEFGH( Pel* puc, Int iStride, UInt& E, UInt& F, UInt& G, UInt& H );
  __inline Void xLoadPredictorsIJKL( Pel* puc, Int iStride, UInt& I, UInt& J, UInt& K, UInt& L );
  __inline Void xLoadPredictorsX   ( Pel* puc, Int iStride, UInt& X );

  __inline UInt xGetS0( XPel* puc, Int iStride );
  __inline UInt xGetS1( XPel* puc, Int iStride );
  __inline UInt xGetS2( XPel* puc, Int iStride );
  __inline UInt xGetS3( XPel* puc, Int iStride );

  __inline Void xLoadPredictorsABCD( XPel* puc, Int iStride, UInt& A, UInt& B, UInt& C, UInt& D );
  __inline Void xLoadPredictorsEFGH( XPel* puc, Int iStride, UInt& E, UInt& F, UInt& G, UInt& H );
  __inline Void xLoadPredictorsIJKL( XPel* puc, Int iStride, UInt& I, UInt& J, UInt& K, UInt& L );
  __inline Void xLoadPredictorsX   ( XPel* puc, Int iStride, UInt& X );

  __inline Void xChroma0PredAllInside ( XPel* puc, Int iStride);
  __inline Void xChroma0PredAllOutside( XPel* puc, Int iStride);
  __inline Void xChroma0PredNoLeftRef ( XPel* puc, Int iStride);
  __inline Void xChroma0PredNoAboveRef( XPel* puc, Int iStride);

  Void xLoadHorPred8x8( XPel* puc, Int iStride);
  Void xLoadVerPred8x8( XPel* puc, Int iStride);
  Void xLoadXPred8x8  ( XPel* puc, Int iStride);

protected:
  UInt    m_uiAvailableMaskMb;
  UInt    m_uiAvailable;

  XPel*   m_pucHor;
  XPel*   m_pucVer;
  XPel    m_ac8x8Pred[26];
  Bool    m_bSpecial;
};


__inline Void IntraPrediction::xChroma0PredAllInside( Pel* puc, Int iStride)
{

  UInt uiS0 = xGetS0( puc, iStride );
  UInt uiS1 = xGetS1( puc, iStride );
  UInt uiS2 = xGetS2( puc, iStride );
  UInt uiS3 = xGetS3( puc, iStride );

  UInt uiA = (uiS0 + uiS2 + 4)/8;
  UInt uiB = (uiS1 + 2)/4;

  ::memset( puc, uiA, 4 );
  ::memset( puc + 4, uiB, 4 );

  for( Int n1 = 0; n1 < 3; n1 ++ )
  {
    memcpy( puc + iStride, puc, 8 );
    puc += iStride;
  }

  UInt uiC = (uiS3 + 2)/4;
  UInt uiD = (uiS1 + uiS3 + 4)/8;

  puc += iStride;

  ::memset( puc, uiC, 4 );
  ::memset( puc + 4, uiD, 4 );

  for( Int n2 = 0; n2 < 3; n2 ++ )
  {
    memcpy( puc + iStride, puc, 8 );
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredAllOutside( Pel* puc, Int iStride)
{
  for( Int n = 0; n < 8; n ++ )
  {
    ::memset( puc, 0x80, 8);
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredNoLeftRef( Pel* puc, Int iStride)
{
  //If only S0 and S1 are inside the frame:
  UInt uiAC = (xGetS0( puc, iStride ) + 2) / 4;
  UInt uiBD = (xGetS1( puc, iStride ) + 2) / 4;

  ::memset( puc, uiAC, 4 );
  ::memset( puc + 4, uiBD, 4 );

  for( Int n = 0; n < 7; n ++ )
  {
    memcpy( puc + iStride, puc, 8 );
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredNoAboveRef( Pel* puc, Int iStride)
{
  //If only S2 and S3 are inside the frame:
  UInt uiAB = (xGetS2( puc, iStride ) + 2) / 4;
  UInt uiCD = (xGetS3( puc, iStride ) + 2) / 4;

  for( Int n1 = 0; n1 < 4; n1 ++ )
  {
    ::memset( puc, uiAB, 8);
    puc += iStride;
  }

  for( Int n2 = 0; n2 < 4; n2 ++ )
  {
    ::memset( puc, uiCD, 8);
    puc += iStride;
  }
}


__inline UInt IntraPrediction::xGetS0( Pel* puc, Int iStride )
{
  puc -= iStride;
  return puc[0] + puc[1] + puc[2] + puc[3];
}

__inline UInt IntraPrediction::xGetS1( Pel* puc, Int iStride )
{
  puc -= iStride;
  return puc[4] + puc[5] + puc[6] + puc[7];
}

__inline UInt IntraPrediction::xGetS2( Pel* puc, Int iStride )
{
  puc--;
  return puc[0] + puc[iStride] + puc[2*iStride] + puc[3*iStride];
}

__inline UInt IntraPrediction::xGetS3( Pel* puc, Int iStride )
{
  puc += 4 * iStride - 1;
  return puc[0] + puc[iStride] + puc[2*iStride] + puc[3*iStride];
}




__inline UInt IntraPrediction::xGetS0( XPel* puc, Int iStride )
{
  puc -= iStride;
  return puc[0] + puc[1] + puc[2] + puc[3];
}
__inline UInt IntraPrediction::xGetS1( XPel* puc, Int iStride )
{
  puc -= iStride;
  return puc[4] + puc[5] + puc[6] + puc[7];
}
__inline UInt IntraPrediction::xGetS2( XPel* puc, Int iStride )
{
  puc--;
  return puc[0] + puc[iStride] + puc[2*iStride] + puc[3*iStride];
}
__inline UInt IntraPrediction::xGetS3( XPel* puc, Int iStride )
{
  puc += 4 * iStride - 1;
  return puc[0] + puc[iStride] + puc[2*iStride] + puc[3*iStride];
}


__inline Void IntraPrediction::xChroma0PredAllInside( XPel* puc, Int iStride)
{

  UInt uiS0 = xGetS0( puc, iStride );
  UInt uiS1 = xGetS1( puc, iStride );
  UInt uiS2 = xGetS2( puc, iStride );
  UInt uiS3 = xGetS3( puc, iStride );

  UInt uiA = (uiS0 + uiS2 + 4)/8;
  UInt uiB = (uiS1 + 2)/4;

  Int n;
  for( n = 0; n < 4; n++ )  puc[n] = uiA;
  for( n = 4; n < 8; n++ )  puc[n] = uiB;

  for( Int n1 = 0; n1 < 3; n1 ++ )
  {
    memcpy( puc + iStride, puc, 8*sizeof(XPel) );
    puc += iStride;
  }

  UInt uiC = (uiS3 + 2)/4;
  UInt uiD = (uiS1 + uiS3 + 4)/8;

  puc += iStride;

  for( n = 0; n < 4; n++ )  puc[n] = uiC;
  for( n = 4; n < 8; n++ )  puc[n] = uiD;

  for( Int n2 = 0; n2 < 3; n2 ++ )
  {
    memcpy( puc + iStride, puc, 8*sizeof(XPel) );
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredAllOutside( XPel* puc, Int iStride)
{
  for( Int n = 0; n < 8; n ++ )
  {
    for( Int m = 0; m < 8; m++ )
    {
      puc[m] = 0x80;
    }
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredNoLeftRef( XPel* puc, Int iStride)
{
  //If only S0 and S1 are inside the frame:
  UInt uiAC = (xGetS0( puc, iStride ) + 2) / 4;
  UInt uiBD = (xGetS1( puc, iStride ) + 2) / 4;

  Int m;
  for( m = 0; m < 4; m++ )  puc[m] = uiAC;
  for( m = 4; m < 8; m++ )  puc[m] = uiBD;

  for( Int n = 0; n < 7; n ++ )
  {
    memcpy( puc + iStride, puc, 8*sizeof(XPel) );
    puc += iStride;
  }
}

__inline Void IntraPrediction::xChroma0PredNoAboveRef( XPel* puc, Int iStride)
{
  //If only S2 and S3 are inside the frame:
  UInt uiAB = (xGetS2( puc, iStride ) + 2) / 4;
  UInt uiCD = (xGetS3( puc, iStride ) + 2) / 4;

  for( Int n1 = 0; n1 < 4; n1 ++ )
  {
    for( Int m = 0; m < 8; m++ )  puc[m] = uiAB;
    puc += iStride;
  }

  for( Int n2 = 0; n2 < 4; n2 ++ )
  {
    for( Int m = 0; m < 8; m++ )  puc[m] = uiCD;
    puc += iStride;
  }
}




H264AVC_NAMESPACE_END


#endif // !defined(AFX_INTRAPREDICTION_H__CBFE313E_2382_4ECC_9D41_416668E3507D__INCLUDED_)
