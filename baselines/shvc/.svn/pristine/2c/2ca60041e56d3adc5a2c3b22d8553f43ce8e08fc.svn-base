#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "TEnc3DAsymLUT.h"

#if Q0048_CGS_3D_ASYMLUT

TEnc3DAsymLUT::TEnc3DAsymLUT()
{
  m_pColorInfo = NULL;
  m_pColorInfoC = NULL;
  m_pEncCuboid = NULL;

  m_pBestEncCuboid = NULL;
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  m_nAccuFrameBit = 0;
  m_nAccuFrameCGSBit = 0;
  m_nPrevFrameCGSPartNumLog2 = 0;
#else
  memset( m_nPrevFrameBit , 0 , sizeof( m_nPrevFrameBit ) );
  memset( m_nPrevFrameCGSBit , 0 , sizeof( m_nPrevFrameCGSBit ) );
  memset( m_nPrevFrameCGSPartNumLog2 , 0 , sizeof( m_nPrevFrameCGSPartNumLog2 ) );
  memset( m_nPrevFrameOverWritePPS , 0 , sizeof( m_nPrevFrameOverWritePPS ) );
#endif
  m_dTotalFrameBit = 0;
  m_nTotalCGSBit = 0;
  m_nPPSBit = 0;
  m_pDsOrigPic = NULL;
#if R0179_ENC_OPT_3DLUT_SIZE
  m_pMaxColorInfo = NULL;
  m_pMaxColorInfoC = NULL;

  
  // fixed m_dDistFactor
  Double dTmpFactor[3];   
  dTmpFactor[I_SLICE] = 1.0;
  dTmpFactor[P_SLICE] = 4./3.;
  dTmpFactor[B_SLICE] = 1.5; 
  for( Int iSliceType = 0; iSliceType < 3; iSliceType++) 
  {
    for(Int iLayer = 0; iLayer < MAX_TLAYER; iLayer++)
    {
      m_dDistFactor[iSliceType][iLayer] = dTmpFactor[iSliceType]*(Double)(1<<iLayer);      
    }
  }
  // initialization with approximate number of bits to code the LUT
  m_nNumLUTBits[0][0] = 200; // 1x1x1
  m_nNumLUTBits[1][0] = 400; // 2x1x1  
  m_nNumLUTBits[1][1] = 1500; // 2x2x2  
  m_nNumLUTBits[2][0] = 800; // 4x1x1
  m_nNumLUTBits[2][1] = 3200; // 4x2x2  
  m_nNumLUTBits[2][2] = 8500; // 4x4x4  
  m_nNumLUTBits[3][0] = 1200; // 8x1x1
  m_nNumLUTBits[3][1] = 4500; // 8x2x2  
  m_nNumLUTBits[3][2] = 10000; // 8x4x4  
  m_nNumLUTBits[3][3] = 12000; // 8x8x8
#endif
}

Void TEnc3DAsymLUT::create( Int nMaxOctantDepth , Int nInputBitDepth , Int nInputBitDepthC , Int nOutputBitDepth , Int nOutputBitDepthC , Int nMaxYPartNumLog2 )
{
  if( m_pColorInfo != NULL )
  {
    destroy();
  }

  TCom3DAsymLUT::create( nMaxOctantDepth , nInputBitDepth , nInputBitDepthC, nOutputBitDepth , nOutputBitDepthC, nMaxYPartNumLog2 
#if R0151_CGS_3D_ASYMLUT_IMPROVE
    , 1 << ( nInputBitDepthC - 1 ) , 1 << ( nInputBitDepthC - 1 )
#endif
    );
  xAllocate3DArray( m_pColorInfo , xGetYSize() , xGetUSize() , xGetVSize() );
  xAllocate3DArray( m_pColorInfoC , xGetYSize() , xGetUSize() , xGetVSize() );
  xAllocate3DArray( m_pEncCuboid , xGetYSize() , xGetUSize() , xGetVSize() );
  xAllocate3DArray( m_pBestEncCuboid , xGetYSize() , xGetUSize() , xGetVSize() );
#if R0179_ENC_OPT_3DLUT_SIZE
  xAllocate3DArray( m_pMaxColorInfo , xGetYSize() , xGetUSize() , xGetVSize() );
  xAllocate3DArray( m_pMaxColorInfoC , xGetYSize() , xGetUSize() , xGetVSize() );

  m_pEncCavlc = new TEncCavlc;
  m_pBitstreamRedirect = new TComOutputBitstream;
  m_pEncCavlc->setBitstream(m_pBitstreamRedirect);
#endif
}

Void TEnc3DAsymLUT::destroy()
{
  xFree3DArray( m_pColorInfo );
  xFree3DArray( m_pColorInfoC );
  xFree3DArray( m_pEncCuboid );
  xFree3DArray( m_pBestEncCuboid );
#if R0179_ENC_OPT_3DLUT_SIZE
  xFree3DArray( m_pMaxColorInfo );
  xFree3DArray( m_pMaxColorInfoC );
  delete m_pBitstreamRedirect;
  delete m_pEncCavlc;
#endif 
  TCom3DAsymLUT::destroy();
}

TEnc3DAsymLUT::~TEnc3DAsymLUT()
{
  if( m_dTotalFrameBit != 0 )
  {
    printf( "\nTotal CGS bit: %d, %.2lf%%" , m_nTotalCGSBit , m_nTotalCGSBit * 100 / m_dTotalFrameBit );
  }

  destroy();
}

#if R0151_CGS_3D_ASYMLUT_IMPROVE
Double TEnc3DAsymLUT::xxDeriveVertexPerColor( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
  Pel & rP0 , Pel & rP1 , Pel & rP3 , Pel & rP7 , Int nResQuantBit )
{
  Int nInitP0 = rP0;
  Int nInitP1 = rP1;
  Int nInitP3 = rP3;
  Int nInitP7 = rP7;

  const Int nOne = xGetNormCoeffOne();
  Double dNorm = (N * yy * vv * uu - N * yy * uv * uv - N * yv * yv * uu - N * vv * yu * yu + 2 * N * yv * uv * yu - yy * vs * vs * uu + 2 * yy * vs * uv * us - yy * vv * us * us - 2 * vs * uv * yu * ys + uv * uv * ys * ys + vs * vs * yu * yu - 2 * yv * vs * us * yu + 2 * yv * vs * ys * uu - 2 * yv * uv * us * ys + 2 * vv * yu * ys * us - vv * uu * ys * ys + yv * yv * us * us);
  if( N > 16 && dNorm != 0 )
  {
    Double dInitA = (-N * uu * yv * Yv + N * uu * Yy * vv - N * Yy * uv * uv + N * yv * uv * Yu - N * yu * Yu * vv + N * yu * uv * Yv + yu * us * Ys * vv - vs * ys * uv * Yu - yu * vs * us * Yv - yv * uv * us * Ys - yv * vs * us * Yu - yu * uv * vs * Ys - ys * us * uv * Yv + ys * us * Yu * vv + 2 * Yy * vs * uv * us + uu * yv * vs * Ys - uu * ys * Ys * vv + uu * vs * ys * Yv + ys * Ys * uv * uv - Yy * vv * us * us + yu * Yu * vs * vs + yv * Yv * us * us - uu * Yy * vs * vs) / dNorm;
    Double dInitB = (N * yy * Yu * vv - N * yy * uv * Yv - N * Yu * yv * yv - N * yu * Yy * vv + N * uv * yv * Yy + N * yv * yu * Yv - yy * us * Ys * vv + yy * uv * vs * Ys - yy * Yu * vs * vs + yy * vs * us * Yv - uv * vs * ys * Yy - yv * yu * vs * Ys + yu * Yy * vs * vs + yu * ys * Ys * vv - uv * yv * ys * Ys + 2 * Yu * yv * vs * ys + us * ys * Yy * vv - vs * ys * yu * Yv + uv * ys * ys * Yv + us * Ys * yv * yv - Yu * ys * ys * vv - yv * ys * us * Yv - vs * us * yv * Yy) / dNorm;
    Double dInitC = -(-N * yy * Yv * uu + N * yy * uv * Yu - N * yv * yu * Yu - N * uv * yu * Yy + N * Yv * yu * yu + N * yv * Yy * uu - yy * uv * us * Ys + yy * Yv * us * us + yy * vs * Ys * uu - yy * vs * us * Yu + yv * ys * us * Yu - vs * Ys * yu * yu - yv * ys * Ys * uu + vs * us * yu * Yy + vs * ys * yu * Yu - uv * Yu * ys * ys + Yv * uu * ys * ys - yv * Yy * us * us - 2 * Yv * yu * ys * us - vs * ys * Yy * uu + uv * us * ys * Yy + uv * yu * ys * Ys + yv * yu * us * Ys) / dNorm;
    nInitP0 = ( Int )( dInitA * nOne + 0.5 ) >> nResQuantBit << nResQuantBit;
    nInitP1 = ( Int )( dInitB * nOne + 0.5 ) >> nResQuantBit << nResQuantBit;
    nInitP3 = ( Int )( dInitC * nOne + 0.5 ) >> nResQuantBit << nResQuantBit;
  }

  Int nMin = - ( 1 << ( m_nLUTBitDepth - 1 ) );
  Int nMax = - nMin - ( 1 << nResQuantBit  );
  Int nMask = ( 1 << nResQuantBit ) - 1;

  Double dMinError = MAX_DOUBLE;
  Int nTestRange = 2;
  Int nStepSize = 1 << nResQuantBit;
  for( Int i = - nTestRange ; i <= nTestRange ; i++ )
  {
    for( Int j = - nTestRange ; j <= nTestRange ; j++ )
    {
      for( Int k = - nTestRange ; k <= nTestRange ; k++ )
      {
        Int nTestP0 = Clip3( nMin , nMax , nInitP0 + i * nStepSize );
        Int nTestP1 = Clip3( nMin , nMax , nInitP1 + j * nStepSize );
        Int nTestP3 = Clip3( nMin , nMax , nInitP3 + k * nStepSize );
        Double a = 1.0 * nTestP0 / nOne;
        Double b = 1.0 * nTestP1 / nOne;
        Double c = 1.0 * nTestP3 / nOne;
        Double d = ( Ys - a * ys - b * us - c * vs ) / N;
        nInitP7 = ( ( Int )d ) >> nResQuantBit << nResQuantBit;
        for( Int m = 0 ; m < 2 ; m++ )
        {
          Int nTestP7 = Clip3( nMin , nMax , nInitP7 + m * nStepSize );
          Double dError = xxCalEstDist( N , Ys , Yy , Yu , Yv , ys , us , vs , yy , yu , yv , uu , uv , vv , YY , a , b , c , nTestP7 );
          if( dError < dMinError )
          {
            dMinError = dError;
            rP0 = ( Pel )nTestP0;
            rP1 = ( Pel )nTestP1;
            rP3 = ( Pel )nTestP3;
            rP7 = ( Pel )nTestP7;
          }
        }
      }
    }
  }
  assert( !( rP0 & nMask ) && !( rP1 & nMask ) && !( rP3 & nMask ) && !( rP7 & nMask ) );

  return( dMinError );
}
#else
Double TEnc3DAsymLUT::xxDeriveVertexPerColor( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
  Int y0 , Int u0 , Int v0 , Int nLengthY , Int nLengthUV ,
  Pel & rP0 , Pel & rP1 , Pel & rP3 , Pel & rP7 , Int nResQuantBit )
{
  Int nInitP0 = rP0;
  Int nInitP1 = rP1;
  Int nInitP3 = rP3;
  Int nInitP7 = rP7;

  Double dNorm = (N * yy * vv * uu - N * yy * uv * uv - N * yv * yv * uu - N * vv * yu * yu + 2 * N * yv * uv * yu - yy * vs * vs * uu + 2 * yy * vs * uv * us - yy * vv * us * us - 2 * vs * uv * yu * ys + uv * uv * ys * ys + vs * vs * yu * yu - 2 * yv * vs * us * yu + 2 * yv * vs * ys * uu - 2 * yv * uv * us * ys + 2 * vv * yu * ys * us - vv * uu * ys * ys + yv * yv * us * us);
  if( N > 16 && dNorm != 0 )
  {
    Double dInitA = (-N * uu * yv * Yv + N * uu * Yy * vv - N * Yy * uv * uv + N * yv * uv * Yu - N * yu * Yu * vv + N * yu * uv * Yv + yu * us * Ys * vv - vs * ys * uv * Yu - yu * vs * us * Yv - yv * uv * us * Ys - yv * vs * us * Yu - yu * uv * vs * Ys - ys * us * uv * Yv + ys * us * Yu * vv + 2 * Yy * vs * uv * us + uu * yv * vs * Ys - uu * ys * Ys * vv + uu * vs * ys * Yv + ys * Ys * uv * uv - Yy * vv * us * us + yu * Yu * vs * vs + yv * Yv * us * us - uu * Yy * vs * vs) / dNorm;
    Double dInitB = (N * yy * Yu * vv - N * yy * uv * Yv - N * Yu * yv * yv - N * yu * Yy * vv + N * uv * yv * Yy + N * yv * yu * Yv - yy * us * Ys * vv + yy * uv * vs * Ys - yy * Yu * vs * vs + yy * vs * us * Yv - uv * vs * ys * Yy - yv * yu * vs * Ys + yu * Yy * vs * vs + yu * ys * Ys * vv - uv * yv * ys * Ys + 2 * Yu * yv * vs * ys + us * ys * Yy * vv - vs * ys * yu * Yv + uv * ys * ys * Yv + us * Ys * yv * yv - Yu * ys * ys * vv - yv * ys * us * Yv - vs * us * yv * Yy) / dNorm;
    Double dInitC = -(-N * yy * Yv * uu + N * yy * uv * Yu - N * yv * yu * Yu - N * uv * yu * Yy + N * Yv * yu * yu + N * yv * Yy * uu - yy * uv * us * Ys + yy * Yv * us * us + yy * vs * Ys * uu - yy * vs * us * Yu + yv * ys * us * Yu - vs * Ys * yu * yu - yv * ys * Ys * uu + vs * us * yu * Yy + vs * ys * yu * Yu - uv * Yu * ys * ys + Yv * uu * ys * ys - yv * Yy * us * us - 2 * Yv * yu * ys * us - vs * ys * Yy * uu + uv * us * ys * Yy + uv * yu * ys * Ys + yv * yu * us * Ys) / dNorm;
    Double dInitD = (-uu * yy * vs * Yv + uu * yy * Ys * vv + uu * vs * yv * Yy - uu * ys * Yy * vv + uu * ys * yv * Yv - uu * Ys * yv * yv + yy * vs * uv * Yu + yy * us * uv * Yv - yy * Ys * uv * uv - yy * us * Yu * vv + ys * yu * Yu * vv + vs * Yv * yu * yu + ys * Yy * uv * uv - us * yu * yv * Yv + us * yu * Yy * vv + 2 * Ys * yv * uv * yu - vs * uv * yu * Yy - vs * yv * yu * Yu - Ys * vv * yu * yu - us * uv * yv * Yy - ys * yv * uv * Yu - ys * yu * uv * Yv + us * Yu * yv * yv) / dNorm;
    nInitP0 = xxCoeff2Vertex( dInitA , dInitB , dInitC , dInitD , y0 , u0 , v0 ) >> nResQuantBit  << nResQuantBit ; 
    nInitP1 = xxCoeff2Vertex( dInitA , dInitB , dInitC , dInitD , y0 , u0 + nLengthUV , v0 ) >> nResQuantBit  << nResQuantBit ;
    nInitP3 = xxCoeff2Vertex( dInitA , dInitB , dInitC , dInitD , y0 , u0 + nLengthUV , v0 + nLengthUV ) >> nResQuantBit  << nResQuantBit ;
    nInitP7 = xxCoeff2Vertex( dInitA , dInitB , dInitC , dInitD , y0 + nLengthY , u0 + nLengthUV , v0 + nLengthUV ) >> nResQuantBit  << nResQuantBit ;
  }

  Int nMin = - ( 1 << ( m_nLUTBitDepth - 1 ) );
  Int nMax = - nMin - ( 1 << nResQuantBit  );
  Int nMask = ( 1 << nResQuantBit ) - 1;

  Double dMinError = MAX_DOUBLE;
  Int testRange = 2;
  for( Int i = - testRange , nDeltaP01 = nInitP1 - nInitP0 - testRange * ( 1 << nResQuantBit  ) ; i <= testRange ; i++ , nDeltaP01 += ( 1 << nResQuantBit  ) )
  {
    for( Int j = - testRange , nDeltaP13 = nInitP3 - nInitP1 - testRange * ( 1 << nResQuantBit  ) ; j <= testRange ; j++ , nDeltaP13 += ( 1 << nResQuantBit  ) )
    {
      for( Int k = - testRange , nDeltaP37 = nInitP7 - nInitP3 - testRange * ( 1 << nResQuantBit  ) ; k <= testRange ; k++ , nDeltaP37 += ( 1 << nResQuantBit  ) )
      {
        Double a = 1.0 * nDeltaP37 / nLengthY;
        Double b = 1.0 * nDeltaP01 / nLengthUV;
        Double c = 1.0 * nDeltaP13 / nLengthUV;
        Double d = ( Ys - a * ys - b * us - c * vs ) / N;
        Int nP0 = xxCoeff2Vertex( a , b , c , d , y0 , u0 , v0 ) >> nResQuantBit  << nResQuantBit ;
        nP0 = Clip3( nMin , nMax , nP0 );
        Int nP1 = Clip3( nMin , nMax , nP0 + nDeltaP01 );
        Int nP3 = Clip3( nMin , nMax , nP1 + nDeltaP13 );
        Int nP7 = Clip3( nMin , nMax , nP3 + nDeltaP37 );
        if ( nP0 & nMask )
        {
          nP0 -= ( nP0 & nMask );
        }
        if ( nP1 & nMask )
        {
          nP1 -= ( nP1 & nMask );
        }
        if ( nP3 & nMask )
        {
          nP3 -= ( nP3 & nMask );
        }
        if ( nP7 & nMask )
        {
          nP7 -= ( nP7 & nMask );
        }
        assert( !( nP0 & nMask ) && !( nP1 & nMask ) && !( nP3 & nMask ) && !( nP7 & nMask ) );
        Double dError = xxCalEstDist( N , Ys , Yy , Yu , Yv , ys , us , vs , yy , yu , yv , uu , uv , vv , YY , y0 , u0 , v0 , nLengthY , nLengthUV , nP0 , nP1 , nP3 , nP7 );
        if( dError < dMinError )
        {
          dMinError = dError;
          rP0 = ( Pel )nP0;
          rP1 = ( Pel )nP1;
          rP3 = ( Pel )nP3;
          rP7 = ( Pel )nP7;
          assert( nMin <= rP0 && rP0 <= nMax && nMin <= rP1 && rP1 <= nMax  && nMin <= rP3 && rP3 <= nMax && nMin <= rP7 && rP7 <= nMax );
        }
      }
    }
  }

  return( dMinError );
}
#endif

#if R0151_CGS_3D_ASYMLUT_IMPROVE
Double TEnc3DAsymLUT::estimateDistWithCur3DAsymLUT( TComPic * pCurPic , UInt refLayerIdc )
{
  xxCollectData( pCurPic , refLayerIdc );

  Double dErrorLuma = 0 , dErrorChroma = 0;
  Int nYSize = 1 << ( getCurOctantDepth() + getCurYPartNumLog2() );
  Int nUVSize = 1 << getCurOctantDepth();
  for( Int yIdx = 0 ; yIdx < nYSize ; yIdx++ )
  {
    for( Int uIdx = 0 ; uIdx < nUVSize ; uIdx++ )
    {
      for( Int vIdx = 0 ; vIdx < nUVSize ; vIdx++ )
      {
        SColorInfo & rCuboidColorInfo = m_pColorInfo[yIdx][uIdx][vIdx];
        SColorInfo & rCuboidColorInfoC = m_pColorInfoC[yIdx][uIdx][vIdx];
        SCuboid & rCuboid = xGetCuboid( yIdx , uIdx , vIdx );
        if( rCuboidColorInfo.N > 0 )
        {
          dErrorLuma += xxCalEstDist( rCuboidColorInfo.N , rCuboidColorInfo.Ys , rCuboidColorInfo.Yy , rCuboidColorInfo.Yu , rCuboidColorInfo.Yv , rCuboidColorInfo.ys , rCuboidColorInfo.us , rCuboidColorInfo.vs , rCuboidColorInfo.yy , rCuboidColorInfo.yu , rCuboidColorInfo.yv , rCuboidColorInfo.uu , rCuboidColorInfo.uv , rCuboidColorInfo.vv , rCuboidColorInfo.YY ,
            rCuboid.P[0].Y , rCuboid.P[1].Y , rCuboid.P[2].Y , rCuboid.P[3].Y );
        }
        if( rCuboidColorInfoC.N > 0 )
        {
          dErrorChroma += xxCalEstDist( rCuboidColorInfoC.N , rCuboidColorInfoC.Us , rCuboidColorInfoC.Uy , rCuboidColorInfoC.Uu , rCuboidColorInfoC.Uv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.UU ,
            rCuboid.P[0].U , rCuboid.P[1].U , rCuboid.P[2].U , rCuboid.P[3].U );
          dErrorChroma += xxCalEstDist( rCuboidColorInfoC.N , rCuboidColorInfoC.Vs , rCuboidColorInfoC.Vy , rCuboidColorInfoC.Vu , rCuboidColorInfoC.Vv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.VV ,
            rCuboid.P[0].V , rCuboid.P[1].V , rCuboid.P[2].V , rCuboid.P[3].V );
        }
      }
    }
  }

  return( dErrorLuma + dErrorChroma);
}
#else
Double TEnc3DAsymLUT::estimateDistWithCur3DAsymLUT( TComPic * pCurPic , UInt refLayerIdc )
{
  xxCollectData( pCurPic , refLayerIdc );

  Double dErrorLuma = 0 , dErrorChroma = 0;
  Int nYSize = 1 << ( getCurOctantDepth() + getCurYPartNumLog2() );
  Int nUVSize = 1 << getCurOctantDepth();
  Int nLengthY = 1 << ( getInputBitDepthY() - getCurOctantDepth() - getCurYPartNumLog2() );
  Int nLengthUV = 1 << ( getInputBitDepthC() - getCurOctantDepth() );
  for( Int yIdx = 0 ; yIdx < nYSize ; yIdx++ )
  {
    for( Int uIdx = 0 ; uIdx < nUVSize ; uIdx++ )
    {
      for( Int vIdx = 0 ; vIdx < nUVSize ; vIdx++ )
      {
        SColorInfo & rCuboidColorInfo = m_pColorInfo[yIdx][uIdx][vIdx];
        SColorInfo & rCuboidColorInfoC = m_pColorInfoC[yIdx][uIdx][vIdx];
        SCuboid & rCuboid = xGetCuboid( yIdx , uIdx , vIdx );
        Int y0 = yIdx << xGetYShift2Idx();
        Int u0 = uIdx << xGetUShift2Idx();
        Int v0 = vIdx << xGetVShift2Idx();
        if( rCuboidColorInfo.N > 0 )
        {
          dErrorLuma += xxCalEstDist( rCuboidColorInfo.N , rCuboidColorInfo.Ys , rCuboidColorInfo.Yy , rCuboidColorInfo.Yu , rCuboidColorInfo.Yv , rCuboidColorInfo.ys , rCuboidColorInfo.us , rCuboidColorInfo.vs , rCuboidColorInfo.yy , rCuboidColorInfo.yu , rCuboidColorInfo.yv , rCuboidColorInfo.uu , rCuboidColorInfo.uv , rCuboidColorInfo.vv , rCuboidColorInfo.YY ,
            y0 , u0 , v0 , nLengthY , nLengthUV , rCuboid.P[0].Y , rCuboid.P[1].Y , rCuboid.P[2].Y , rCuboid.P[3].Y );
        }
        if( rCuboidColorInfoC.N > 0 )
        {
          dErrorChroma += xxCalEstDist( rCuboidColorInfoC.N , rCuboidColorInfoC.Us , rCuboidColorInfoC.Uy , rCuboidColorInfoC.Uu , rCuboidColorInfoC.Uv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.UU ,
            y0 , u0 , v0 , nLengthY , nLengthUV , rCuboid.P[0].U , rCuboid.P[1].U , rCuboid.P[2].U , rCuboid.P[3].U );
          dErrorChroma += xxCalEstDist( rCuboidColorInfoC.N , rCuboidColorInfoC.Vs , rCuboidColorInfoC.Vy , rCuboidColorInfoC.Vu , rCuboidColorInfoC.Vv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.VV ,
            y0 , u0 , v0 , nLengthY , nLengthUV , rCuboid.P[0].V , rCuboid.P[1].V , rCuboid.P[2].V , rCuboid.P[3].V );
        }
      }
    }
  }

  return( dErrorLuma + dErrorChroma);
}
#endif

#if R0151_CGS_3D_ASYMLUT_IMPROVE
#if R0179_ENC_OPT_3DLUT_SIZE
Double TEnc3DAsymLUT::derive3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS , Bool bElRapSliceTypeB, Double dFrameLambda )
{
  m_nLUTBitDepth = pCfg->getCGSLUTBit();

  Int nBestAdaptCThresholdU = 1 << ( getInputBitDepthC() - 1 );
  Int nBestAdaptCThresholdV = 1 << ( getInputBitDepthC() - 1 );
  Int nAdaptCThresholdU, nAdaptCThresholdV;

  Int nTmpLutBits[MAX_Y_SIZE][MAX_C_SIZE] ;
  memset(nTmpLutBits, 0, sizeof(nTmpLutBits)); 

  SLUTSize sMaxLutSize;  

  // collect stats for the most partitions 
  Int nCurYPartNumLog2 = 0 , nCurOctantDepth = 0; 
  Int nMaxPartNumLog2 = xGetMaxPartNumLog2();

  xxMapPartNum2DepthYPart( nMaxPartNumLog2 , nCurOctantDepth , nCurYPartNumLog2 ); 
  xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2, nBestAdaptCThresholdU, nBestAdaptCThresholdV ); 
  xxCollectData( pCurPic , refLayerIdc );
  xxCopyColorInfo(m_pMaxColorInfo, m_pColorInfo, m_pMaxColorInfoC, m_pColorInfoC); 
 
  sMaxLutSize.iCPartNumLog2 = nCurOctantDepth; 
  sMaxLutSize.iYPartNumLog2 = nCurOctantDepth + nCurYPartNumLog2; 

  m_pBitstreamRedirect->clear();

  // find the best partition based on RD cost 
  Int i; 
  Double dMinCost, dCurCost;

  Int iBestLUTSizeIdx = 0;   
  Int nBestResQuanBit = 0;
  Double dCurError, dMinError; 
  Int iNumBitsCurSize; 
  Int iNumBitsCurSizeSave = m_pEncCavlc->getNumberOfWrittenBits(); 
  Double dDistFactor = getDistFactor(pSlice->getSliceType(), pSlice->getDepth());

  // check all LUT sizes 
  xxGetAllLutSizes(pSlice);  
  if (m_nTotalLutSizes == 0) // return if no valid size is found, LUT will not be updated
  {
    nCurOctantDepth = sMaxLutSize.iCPartNumLog2;
    nCurYPartNumLog2 = sMaxLutSize.iYPartNumLog2-nCurOctantDepth; 
    xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2, nBestAdaptCThresholdU, nBestAdaptCThresholdV ); 
    return MAX_DOUBLE; 
  }

  dMinCost = MAX_DOUBLE; dMinError = MAX_DOUBLE;
  for (i = 0; i < m_nTotalLutSizes; i++)
  {
    // add up the stats
    nCurOctantDepth = m_sLutSizes[i].iCPartNumLog2;
    nCurYPartNumLog2 = m_sLutSizes[i].iYPartNumLog2-nCurOctantDepth; 
    xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2, nBestAdaptCThresholdU, nBestAdaptCThresholdV ); 
    xxConsolidateData( &m_sLutSizes[i], &sMaxLutSize );
  
    dCurError = xxDeriveVertexes(nBestResQuanBit, m_pEncCuboid);

    setResQuantBit( nBestResQuanBit );
    xSaveCuboids( m_pEncCuboid ); 
    m_pEncCavlc->xCode3DAsymLUT( this ); 
    iNumBitsCurSize = m_pEncCavlc->getNumberOfWrittenBits();
    dCurCost = dCurError/dDistFactor + dFrameLambda*(Double)(iNumBitsCurSize-iNumBitsCurSizeSave);  
    nTmpLutBits[m_sLutSizes[i].iYPartNumLog2][m_sLutSizes[i].iCPartNumLog2] = iNumBitsCurSize-iNumBitsCurSizeSave; // store LUT size 
    iNumBitsCurSizeSave = iNumBitsCurSize;
    if(dCurCost < dMinCost )
    {
      SCuboid *** tmp = m_pBestEncCuboid;
      m_pBestEncCuboid = m_pEncCuboid;
      m_pEncCuboid = tmp;
      dMinCost = dCurCost; 
      dMinError = dCurError;
      iBestLUTSizeIdx = i; 
    }
  }

  nCurOctantDepth = m_sLutSizes[iBestLUTSizeIdx].iCPartNumLog2;
  nCurYPartNumLog2 = m_sLutSizes[iBestLUTSizeIdx].iYPartNumLog2-nCurOctantDepth; 

  xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2, nBestAdaptCThresholdU, nBestAdaptCThresholdV ); 

  Bool bUseNewColorInfo = false; 
  if( pCfg->getCGSAdaptChroma() && nCurOctantDepth <= 1 ) // if the best size found so far has depth = 0 or 1, then check AdaptC U/V thresholds
  {
    nAdaptCThresholdU = ( Int )( m_dSumU / m_nNChroma + 0.5 );
    nAdaptCThresholdV = ( Int )( m_dSumV / m_nNChroma + 0.5 );
    if( !(nAdaptCThresholdU == nBestAdaptCThresholdU && nAdaptCThresholdV == nBestAdaptCThresholdV ) ) 
    {
      nCurOctantDepth = 1;
      if( nCurOctantDepth + nCurYPartNumLog2 > getMaxYPartNumLog2()+getMaxOctantDepth() )
        nCurYPartNumLog2 = getMaxYPartNumLog2()+getMaxOctantDepth()-nCurOctantDepth; 
      xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2 , nAdaptCThresholdU , nAdaptCThresholdV );
      xxCollectData( pCurPic , refLayerIdc );

      dCurError = xxDeriveVertexes( nBestResQuanBit , m_pEncCuboid ) ;
      setResQuantBit( nBestResQuanBit );
      xSaveCuboids( m_pEncCuboid ); 
      m_pEncCavlc->xCode3DAsymLUT( this ); 
      iNumBitsCurSize = m_pEncCavlc->getNumberOfWrittenBits();
      dCurCost = dCurError/dDistFactor + dFrameLambda*(Double)(iNumBitsCurSize-iNumBitsCurSizeSave);  
      iNumBitsCurSizeSave = iNumBitsCurSize;
      if(dCurCost < dMinCost )
      {
        SCuboid *** tmp = m_pBestEncCuboid;
        m_pBestEncCuboid = m_pEncCuboid;
        m_pEncCuboid = tmp;
        dMinCost = dCurCost; 
        dMinError = dCurError;
        nBestAdaptCThresholdU = nAdaptCThresholdU;
        nBestAdaptCThresholdV = nAdaptCThresholdV;
        bUseNewColorInfo = true; 
      }
    }
  }

  xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2, nBestAdaptCThresholdU, nBestAdaptCThresholdV ); 

  // check res_quant_bits only for the best table size and best U/V threshold
  if( !bUseNewColorInfo ) 
    xxConsolidateData( &m_sLutSizes[iBestLUTSizeIdx], &sMaxLutSize );

  //    xxCollectData( pCurPic , refLayerIdc );
  for( Int nResQuanBit = 1 ; nResQuanBit < 4 ; nResQuanBit++ )
  {
    dCurError = xxDeriveVertexes( nResQuanBit , m_pEncCuboid );

    setResQuantBit( nResQuanBit );
    xSaveCuboids( m_pEncCuboid ); 
    m_pEncCavlc->xCode3DAsymLUT( this ); 
    iNumBitsCurSize = m_pEncCavlc->getNumberOfWrittenBits();
    dCurCost = dCurError/dDistFactor + dFrameLambda*(Double)(iNumBitsCurSize-iNumBitsCurSizeSave);   

    iNumBitsCurSizeSave = iNumBitsCurSize;
    if(dCurCost < dMinCost)
    {
      nBestResQuanBit = nResQuanBit;
      SCuboid *** tmp = m_pBestEncCuboid;
      m_pBestEncCuboid = m_pEncCuboid;
      m_pEncCuboid = tmp;
      dMinCost = dCurCost; 
      dMinError = dCurError;
    }
    else
    {
      break;
    }
  }
    
  setResQuantBit( nBestResQuanBit );
  xSaveCuboids( m_pBestEncCuboid );

  // update LUT size stats 
  for(Int iLutSizeY = 0; iLutSizeY < MAX_Y_SIZE; iLutSizeY++)
  {
    for(Int iLutSizeC = 0; iLutSizeC < MAX_C_SIZE; iLutSizeC++) 
    {
      if(nTmpLutBits[iLutSizeY][iLutSizeC] != 0) 
        m_nNumLUTBits[iLutSizeY][iLutSizeC] =  (m_nNumLUTBits[iLutSizeY][iLutSizeC] + nTmpLutBits[iLutSizeY][iLutSizeC]*3+2)>>2; // update with new stats
    }
  }

  // return cost rather than error
  return( dMinCost );
}
#endif 


Double TEnc3DAsymLUT::derive3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS , Bool bElRapSliceTypeB )
{
  m_nLUTBitDepth = pCfg->getCGSLUTBit();
  Int nCurYPartNumLog2 = 0 , nCurOctantDepth = 0; 
  xxDerivePartNumLog2( pSlice , pCfg , nCurOctantDepth , nCurYPartNumLog2 , bSignalPPS , bElRapSliceTypeB );

  Int nBestResQuanBit = 0;
  Int nBestAdaptCThresholdU = 1 << ( getInputBitDepthC() - 1 );
  Int nBestAdaptCThresholdV = 1 << ( getInputBitDepthC() - 1 );
  Int nBestOctantDepth = nCurOctantDepth;
  Int nBestYPartNumLog2 = nCurYPartNumLog2;
  Int nTargetLoop = 1 + ( pCfg->getCGSAdaptChroma() && ( nCurOctantDepth == 1 || ( nCurOctantDepth * 3 + nCurYPartNumLog2 ) >= 5 ) );
  Double dMinError = MAX_DOUBLE;
  for( Int nLoop = 0 ; nLoop < nTargetLoop ; nLoop++ )
  {
    Int nAdaptCThresholdU = 1 << ( getInputBitDepthC() - 1 );
    Int nAdaptCThresholdV = 1 << ( getInputBitDepthC() - 1 );
    if( nLoop > 0 )
    {
      nAdaptCThresholdU = ( Int )( m_dSumU / m_nNChroma + 0.5 );
      nAdaptCThresholdV = ( Int )( m_dSumV / m_nNChroma + 0.5 );
      if( nCurOctantDepth > 1 )
      {
        nCurOctantDepth = 1;
        nCurYPartNumLog2 = 2;
      }
      if( nAdaptCThresholdU == nBestAdaptCThresholdU && nAdaptCThresholdV == nBestAdaptCThresholdV 
        && nCurOctantDepth == nBestOctantDepth && nCurYPartNumLog2 == nBestYPartNumLog2 )
        break;
    }

    xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2 , nAdaptCThresholdU , nAdaptCThresholdV );
    xxCollectData( pCurPic , refLayerIdc );
    for( Int nResQuanBit = 0 ; nResQuanBit < 4 ; nResQuanBit++ )
    {
      Double dError = xxDeriveVertexes( nResQuanBit , m_pEncCuboid ) / ( 1 + ( nResQuanBit > 0 ) * 0.001 * ( pSlice->getDepth() + 1 ) );
      if( dError <= dMinError )
      {
        nBestResQuanBit = nResQuanBit;
        nBestAdaptCThresholdU = nAdaptCThresholdU;
        nBestAdaptCThresholdV = nAdaptCThresholdV;
        nBestOctantDepth = nCurOctantDepth;
        nBestYPartNumLog2 = nCurYPartNumLog2;
        SCuboid *** tmp = m_pBestEncCuboid;
        m_pBestEncCuboid = m_pEncCuboid;
        m_pEncCuboid = tmp;
        dMinError = dError;
      }
      else
      {
        break;
      }
    }
  }

  setResQuantBit( nBestResQuanBit );
  xUpdatePartitioning( nBestOctantDepth , nBestYPartNumLog2 , nBestAdaptCThresholdU , nBestAdaptCThresholdV );

  xSaveCuboids( m_pBestEncCuboid );
  return( dMinError );
}
#else
Double TEnc3DAsymLUT::derive3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS , Bool bElRapSliceTypeB )
{
  m_nLUTBitDepth = pCfg->getCGSLUTBit();
  Int nCurYPartNumLog2 = 0 , nCurOctantDepth = 0; 
  xxDerivePartNumLog2( pSlice , pCfg , nCurOctantDepth , nCurYPartNumLog2 , bSignalPPS , bElRapSliceTypeB );
  xUpdatePartitioning( nCurOctantDepth , nCurYPartNumLog2 );
  xxCollectData( pCurPic , refLayerIdc );
  Int nBestResQuanBit = 0;
  Double dError0 = xxDeriveVertexes( nBestResQuanBit , m_pBestEncCuboid );
  Double dCurError = dError0;
  Double dFactor = 1 + 0.001 * ( pSlice->getDepth() + 1 );
  for( Int nResQuanBit = 1 ; nResQuanBit < 4 ; nResQuanBit++ )
  {
    Double dError = xxDeriveVertexes( nResQuanBit , m_pEncCuboid );
    if( dError < dError0 * dFactor )
    {
      nBestResQuanBit = nResQuanBit;
      SCuboid *** tmp = m_pBestEncCuboid;
      m_pBestEncCuboid = m_pEncCuboid;
      m_pEncCuboid = tmp;
      dCurError = dError;
    }
    else
    {
      break;
    }
  }
  setResQuantBit( nBestResQuanBit );
  xSaveCuboids( m_pBestEncCuboid );
  return( dCurError );
}
#endif

Double TEnc3DAsymLUT::xxDeriveVertexes( Int nResQuanBit , SCuboid *** pCurCuboid )
{
  Double dErrorLuma = 0 , dErrorChroma = 0;
  Int nYSize = 1 << ( getCurOctantDepth() + getCurYPartNumLog2() );
  Int nUVSize = 1 << getCurOctantDepth();
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
  Int nLengthY = 1 << ( getInputBitDepthY() - getCurOctantDepth() - getCurYPartNumLog2() );
  Int nLengthUV = 1 << ( getInputBitDepthC() - getCurOctantDepth() );
#endif
  for( Int yIdx = 0 ; yIdx < nYSize ; yIdx++ )
  {
    for( Int uIdx = 0 ; uIdx < nUVSize ; uIdx++ )
    {
      for( Int vIdx = 0 ; vIdx < nUVSize ; vIdx++ )
      {
        SColorInfo & rCuboidColorInfo = m_pColorInfo[yIdx][uIdx][vIdx];
        SColorInfo & rCuboidColorInfoC = m_pColorInfoC[yIdx][uIdx][vIdx];
        SCuboid & rCuboid = pCurCuboid[yIdx][uIdx][vIdx];
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
        Int y0 = yIdx << xGetYShift2Idx();
        Int u0 = uIdx << xGetUShift2Idx();
        Int v0 = vIdx << xGetVShift2Idx();
#endif
        for( Int idxVertex = 0 ; idxVertex < 4 ; idxVertex++ )
        {
          rCuboid.P[idxVertex] = xGetCuboidVertexPredAll( yIdx , uIdx , vIdx , idxVertex , pCurCuboid );
        }

        if( rCuboidColorInfo.N > 0 )
        {
          dErrorLuma += xxDeriveVertexPerColor( rCuboidColorInfo.N , rCuboidColorInfo.Ys , rCuboidColorInfo.Yy , rCuboidColorInfo.Yu , rCuboidColorInfo.Yv , rCuboidColorInfo.ys , rCuboidColorInfo.us , rCuboidColorInfo.vs , rCuboidColorInfo.yy , rCuboidColorInfo.yu , rCuboidColorInfo.yv , rCuboidColorInfo.uu , rCuboidColorInfo.uv , rCuboidColorInfo.vv , rCuboidColorInfo.YY ,
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
            y0 , u0 , v0 , nLengthY , nLengthUV , 
#endif
            rCuboid.P[0].Y , rCuboid.P[1].Y , rCuboid.P[2].Y , rCuboid.P[3].Y , nResQuanBit );
        }
        if( rCuboidColorInfoC.N > 0 )
        {
          dErrorChroma += xxDeriveVertexPerColor( rCuboidColorInfoC.N , rCuboidColorInfoC.Us , rCuboidColorInfoC.Uy , rCuboidColorInfoC.Uu , rCuboidColorInfoC.Uv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.UU ,
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
            y0 , u0 , v0 , nLengthY , nLengthUV , 
#endif
            rCuboid.P[0].U , rCuboid.P[1].U , rCuboid.P[2].U , rCuboid.P[3].U , nResQuanBit );
          dErrorChroma += xxDeriveVertexPerColor( rCuboidColorInfoC.N , rCuboidColorInfoC.Vs , rCuboidColorInfoC.Vy , rCuboidColorInfoC.Vu , rCuboidColorInfoC.Vv , rCuboidColorInfoC.ys , rCuboidColorInfoC.us , rCuboidColorInfoC.vs , rCuboidColorInfoC.yy , rCuboidColorInfoC.yu , rCuboidColorInfoC.yv , rCuboidColorInfoC.uu , rCuboidColorInfoC.uv , rCuboidColorInfoC.vv , rCuboidColorInfoC.VV ,
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
            y0 , u0 , v0 , nLengthY , nLengthUV , 
#endif
            rCuboid.P[0].V , rCuboid.P[1].V , rCuboid.P[2].V , rCuboid.P[3].V , nResQuanBit );
        }

        if( nResQuanBit > 0 )
        {
          // check quantization
          for( Int idxVertex = 0 ; idxVertex < 4 ; idxVertex++ )
          {
            SYUVP sPred = xGetCuboidVertexPredAll( yIdx , uIdx , vIdx , idxVertex , pCurCuboid );
            assert( ( ( rCuboid.P[idxVertex].Y - sPred.Y ) >> nResQuanBit << nResQuanBit ) == rCuboid.P[idxVertex].Y - sPred.Y );
            assert( ( ( rCuboid.P[idxVertex].U - sPred.U ) >> nResQuanBit << nResQuanBit ) == rCuboid.P[idxVertex].U - sPred.U );
            assert( ( ( rCuboid.P[idxVertex].V - sPred.V ) >> nResQuanBit << nResQuanBit ) == rCuboid.P[idxVertex].V - sPred.V );
          }
        }
      }
    }
  }

  return( dErrorLuma + dErrorChroma );
}

Void TEnc3DAsymLUT::xxCollectData( TComPic * pCurPic , UInt refLayerIdc )
{
  Pel * pSrcY = m_pDsOrigPic->getLumaAddr();
  Pel * pSrcU = m_pDsOrigPic->getCbAddr();
  Pel * pSrcV = m_pDsOrigPic->getCrAddr();
  Int nStrideSrcY = m_pDsOrigPic->getStride();
  Int nStrideSrcC = m_pDsOrigPic->getCStride();
  TComPicYuv *pRecPic = pCurPic->getSlice(pCurPic->getCurrSliceIdx())->getBaseColPic(refLayerIdc)->getPicYuvRec();
  Pel * pIRLY = pRecPic->getLumaAddr();
  Pel * pIRLU = pRecPic->getCbAddr();
  Pel * pIRLV = pRecPic->getCrAddr();
  Int nStrideILRY = pRecPic->getStride();
  Int nStrideILRC = pRecPic->getCStride();
#if R0179_ENC_OPT_3DLUT_SIZE
  xReset3DArray( m_pColorInfo  , getMaxYSize() , getMaxCSize() , getMaxCSize() );
  xReset3DArray( m_pColorInfoC , getMaxYSize() , getMaxCSize() , getMaxCSize() );
#else
  xReset3DArray( m_pColorInfo , xGetYSize() , xGetUSize() , xGetVSize() );
  xReset3DArray( m_pColorInfoC , xGetYSize() , xGetUSize() , xGetVSize() );
#endif

  //alignment padding
  pRecPic->setBorderExtension( false );
  pRecPic->extendPicBorder();

  TComSlice * pSlice = pCurPic->getSlice(pCurPic->getCurrSliceIdx());
  UInt refLayerId = pSlice->getVPS()->getRefLayerId(pSlice->getLayerId(), refLayerIdc);
#if MOVE_SCALED_OFFSET_TO_PPS
  const Window &scalEL = pSlice->getPPS()->getScaledRefLayerWindowForLayer(refLayerId); 
#else
  const Window &scalEL = pSlice->getSPS()->getScaledRefLayerWindowForLayer(refLayerId); 
#endif
  TComPicYuv *pcRecPicBL = pSlice->getBaseColPic(refLayerIdc)->getPicYuvRec();
  // borders of down-sampled picture
  Int leftDS =  (scalEL.getWindowLeftOffset()*g_posScalingFactor[refLayerIdc][0]+(1<<15))>>16;
  Int rightDS = pcRecPicBL->getWidth() - 1 + (((scalEL.getWindowRightOffset())*g_posScalingFactor[refLayerIdc][0]+(1<<15))>>16);
  Int topDS = (((scalEL.getWindowTopOffset())*g_posScalingFactor[refLayerIdc][1]+(1<<15))>>16);
  Int bottomDS = pcRecPicBL->getHeight() - 1 + (((scalEL.getWindowBottomOffset())*g_posScalingFactor[refLayerIdc][1]+(1<<15))>>16);
  // overlapped region
  Int left = max( 0 , leftDS );
  Int right = min( pcRecPicBL->getWidth() - 1 , rightDS );
  Int top = max( 0 , topDS );
  Int bottom = min( pcRecPicBL->getHeight() - 1 , bottomDS );
  // since we do data collection only for overlapped region, the border extension is good enough

#if R0151_CGS_3D_ASYMLUT_IMPROVE
  m_dSumU = m_dSumV = 0;
  m_nNChroma = 0;
#endif
  for( Int i = top ; i <= bottom ; i++ )
  {
    Int iDS = i-topDS;
    Int jDS = left-leftDS;
    Int posSrcY = iDS * nStrideSrcY + jDS;
    Int posIRLY = i * nStrideILRY + left;
    Int posSrcUV = ( iDS >> 1 ) * nStrideSrcC + (jDS>>1);
    Int posIRLUV = ( i >> 1 ) * nStrideILRC + (left>>1);
    for( Int j = left ; j <= right ; j++ , posSrcY++ , posIRLY++ , posSrcUV += !( j & 0x01 ) , posIRLUV += !( j & 0x01 ) )
    {
      Int Y = pSrcY[posSrcY];
      Int y = pIRLY[posIRLY];
      Int U = pSrcU[posSrcUV];
      Int u = pIRLU[posIRLUV];
      Int V = pSrcV[posSrcUV];
      Int v = pIRLV[posIRLUV];

      // alignment
      //filtering u, v for luma;
      Int posIRLUVN =  posIRLUV + ((i&1)? nStrideILRC : -nStrideILRC);
      if((j&1))
      {
        u = (pIRLU[posIRLUVN] + pIRLU[posIRLUVN+1] +(u + pIRLU[posIRLUV+1])*3 +4)>>3;
        v = (pIRLV[posIRLUVN] + pIRLV[posIRLUVN+1] +(v + pIRLV[posIRLUV+1])*3 +4)>>3;
      }
      else
      { 
        u = (pIRLU[posIRLUVN] +u*3 +2)>>2;
        v = (pIRLV[posIRLUVN] +v*3 +2)>>2;
      }

#if R0151_CGS_3D_ASYMLUT_IMPROVE
      m_dSumU += u;
      m_dSumV += v;
      m_nNChroma++;
#endif
      SColorInfo sColorInfo;
#if R0151_CGS_3D_ASYMLUT_IMPROVE
      SColorInfo & rCuboidColorInfo = m_pColorInfo[xGetYIdx(y)][xGetUIdx(u)][xGetVIdx(v)];
#else
      SColorInfo & rCuboidColorInfo = m_pColorInfo[y>>xGetYShift2Idx()][u>>xGetUShift2Idx()][v>>xGetVShift2Idx()];
#endif
      memset(&sColorInfo, 0, sizeof(SColorInfo));
      sColorInfo.Ys = Y;
      sColorInfo.ys = y;
      sColorInfo.us = u;
      sColorInfo.vs = v;
      sColorInfo.Yy = Y * y;
      sColorInfo.Yu = Y * u;
      sColorInfo.Yv = Y * v;
      sColorInfo.yy = y * y;
      sColorInfo.yu = y * u;
      sColorInfo.yv = y * v;
      sColorInfo.uu = u * u;
      sColorInfo.uv = u * v;
      sColorInfo.vv = v * v;
      sColorInfo.YY = Y * Y;
      sColorInfo.N  = 1;

      rCuboidColorInfo += sColorInfo;

      if(!((i&1) || (j&1)))
      {
        // alignment
        y =  (pIRLY[posIRLY] + pIRLY[posIRLY+nStrideILRY] + 1)>>1;

        u = pIRLU[posIRLUV];
        v = pIRLV[posIRLUV];
#if R0151_CGS_3D_ASYMLUT_IMPROVE
        SColorInfo & rCuboidColorInfoC = m_pColorInfoC[xGetYIdx(y)][xGetUIdx(u)][xGetVIdx(v)];
#else
        SColorInfo & rCuboidColorInfoC = m_pColorInfoC[y>>xGetYShift2Idx()][u>>xGetUShift2Idx()][v>>xGetVShift2Idx()];
#endif
        sColorInfo.Us = U;
        sColorInfo.Vs = V;
        sColorInfo.ys = y;
        sColorInfo.us = u;
        sColorInfo.vs = v;

        sColorInfo.Uy = U * y;
        sColorInfo.Uu = U * u;
        sColorInfo.Uv = U * v;
        sColorInfo.Vy = V * y;
        sColorInfo.Vu = V * u;
        sColorInfo.Vv = V * v;
        sColorInfo.yy = y * y;
        sColorInfo.yu = y * u;
        sColorInfo.yv = y * v;
        sColorInfo.uu = u * u;
        sColorInfo.uv = u * v;
        sColorInfo.vv = v * v;
        sColorInfo.UU = U * U;
        sColorInfo.VV = V * V;
        sColorInfo.N  = 1;

        rCuboidColorInfoC += sColorInfo;
      }
    }
  }
}

Void TEnc3DAsymLUT::xxDerivePartNumLog2( TComSlice * pSlice , TEncCfg * pcCfg , Int & rOctantDepth , Int & rYPartNumLog2 , Bool bSignalPPS , Bool bElRapSliceTypeB )
{
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
  Int nSliceType = pSlice->getSliceType();
  // update slice type as what will be done later
  if( pSlice->getActiveNumILRRefIdx() == 0 && pSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP && pSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA )
  {
    nSliceType = I_SLICE;
  }
  else if( !bElRapSliceTypeB )
  {
    if( (pSlice->getNalUnitType() >= NAL_UNIT_CODED_SLICE_BLA_W_LP) &&
      (pSlice->getNalUnitType() <= NAL_UNIT_CODED_SLICE_CRA) &&
      pSlice->getSliceType() == B_SLICE )
    {
      nSliceType = P_SLICE;
    }
  }

  const Int nSliceTempLevel = pSlice->getDepth();
#endif
  Int nPartNumLog2 = 4;
  if( pSlice->getBaseColPic( pSlice->getInterLayerPredLayerIdc( 0 ) )->getSlice( 0 )->isIntra() )
  {
    nPartNumLog2 = xGetMaxPartNumLog2();
  }
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  if( m_nAccuFrameBit && pSlice->getPPS()->getCGSFlag() ) 
  {
    Double dBitCost = 1.0 * m_nAccuFrameCGSBit / m_nAccuFrameBit;
    nPartNumLog2 = m_nPrevFrameCGSPartNumLog2;
#else
  if( m_nPrevFrameBit[nSliceType][nSliceTempLevel] && pSlice->getPPS()->getCGSFlag() ) 
  {
    Double dBitCost = 1.0 * m_nPrevFrameCGSBit[nSliceType][nSliceTempLevel] / m_nPrevFrameBit[nSliceType][nSliceTempLevel];
    nPartNumLog2 = m_nPrevFrameCGSPartNumLog2[nSliceType][nSliceTempLevel];
#endif
    Double dBitCostT = 0.03;
    if( dBitCost < dBitCostT / 6.0 )
    {
      nPartNumLog2++;
    }
    else if( dBitCost >= dBitCostT )
    {
      nPartNumLog2--;
    }
  }
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
  else
  {
    nPartNumLog2 -= nSliceTempLevel;
  }
#endif
  nPartNumLog2 = Clip3( 0 , xGetMaxPartNumLog2()  , nPartNumLog2 );
  xxMapPartNum2DepthYPart( nPartNumLog2 , rOctantDepth , rYPartNumLog2 );
}

Void TEnc3DAsymLUT::xxMapPartNum2DepthYPart( Int nPartNumLog2 , Int & rOctantDepth , Int & rYPartNumLog2 )
{
  for( Int y = getMaxYPartNumLog2() ; y >= 0 ; y-- )
  {
    for( Int depth = ( nPartNumLog2 - y ) >> 1 ; depth >= 0 ; depth-- )
    {
      if( y + 3 * depth == nPartNumLog2 )
      {
        rOctantDepth = depth;
        rYPartNumLog2 = y;
        return;
      }
    }
  }
  rOctantDepth = min( getMaxOctantDepth() , nPartNumLog2 / 3 );
  rYPartNumLog2 = min( getMaxYPartNumLog2() , nPartNumLog2 - 3 * rOctantDepth );
}

Void TEnc3DAsymLUT::updatePicCGSBits( TComSlice * pcSlice , Int nPPSBit )
{
#if !R0151_CGS_3D_ASYMLUT_IMPROVE
  const Int nSliceType = pcSlice->getSliceType();
  const Int nSliceTempLevel = pcSlice->getDepth();
#endif
  for( Int i = 0; i < pcSlice->getActiveNumILRRefIdx(); i++ )
  {
    UInt refLayerIdc = pcSlice->getInterLayerPredLayerIdc(i);
#if R0151_CGS_3D_ASYMLUT_IMPROVE
    m_nAccuFrameBit += pcSlice->getPic()->getFrameBit() + pcSlice->getBaseColPic(refLayerIdc)->getFrameBit();
#else
    m_nPrevFrameBit[nSliceType][nSliceTempLevel] = pcSlice->getPic()->getFrameBit() + pcSlice->getBaseColPic(refLayerIdc)->getFrameBit();
#endif
    m_dTotalFrameBit += pcSlice->getPic()->getFrameBit() + pcSlice->getBaseColPic(refLayerIdc)->getFrameBit();
  }
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  m_nAccuFrameCGSBit += nPPSBit;
  m_nTotalCGSBit += nPPSBit;
  m_nPrevFrameCGSPartNumLog2 = getCurOctantDepth() * 3 + getCurYPartNumLog2();
#else
  m_nPrevFrameOverWritePPS[nSliceType][nSliceTempLevel] = pcSlice->getCGSOverWritePPS();
  m_nPrevFrameCGSBit[nSliceType][nSliceTempLevel] = nPPSBit;
  m_nTotalCGSBit += nPPSBit;
  m_nPrevFrameCGSPartNumLog2[nSliceType][nSliceTempLevel] = getCurOctantDepth() * 3 + getCurYPartNumLog2();
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
  Int nCurELFrameBit = pcSlice->getPic()->getFrameBit();
  const Int nSliceType = pcSlice->getSliceType();
  const Int nSliceTempLevel = pcSlice->getDepth();
  m_nPrevELFrameBit[nSliceType][nSliceTempLevel] = m_nPrevELFrameBit[nSliceType][nSliceTempLevel] == 0 ? nCurELFrameBit:((m_nPrevELFrameBit[nSliceType][nSliceTempLevel]+nCurELFrameBit)>>1);
#endif 
}

#if R0179_ENC_OPT_3DLUT_SIZE

Void TEnc3DAsymLUT::xxGetAllLutSizes(TComSlice *pSlice)
{
  Int iMaxYPartNumLog2, iMaxCPartNumLog2; 
  Int iCurYPartNumLog2, iCurCPartNumLog2; 
  Int iMaxAddYPartNumLog2; 
  Int iNumELFrameBits = m_nPrevELFrameBit[pSlice->getSliceType()][pSlice->getDepth()];

  xxMapPartNum2DepthYPart( xGetMaxPartNumLog2() , iMaxCPartNumLog2 , iMaxYPartNumLog2 );
  iMaxAddYPartNumLog2 = iMaxYPartNumLog2; 
  iMaxYPartNumLog2 += iMaxCPartNumLog2; 

  //m_sLutSizes[0].iYPartNumLog2 = iMaxYPartNumLog2; 
  //m_sLutSizes[0].iCPartNumLog2 = iMaxCPartNumLog2; 
  m_nTotalLutSizes = 0; 


  for(iCurYPartNumLog2 = iMaxYPartNumLog2; iCurYPartNumLog2 >= 0; iCurYPartNumLog2--) 
  {
    for(iCurCPartNumLog2 = iMaxCPartNumLog2; iCurCPartNumLog2 >= 0; iCurCPartNumLog2--) 
    {
       // try more sizes
      if(iCurCPartNumLog2 <= iCurYPartNumLog2  && 
         (m_nNumLUTBits[iCurYPartNumLog2][iCurCPartNumLog2] < (iNumELFrameBits>>1)) && 
         m_nTotalLutSizes < MAX_NUM_LUT_SIZES)
      {
        m_sLutSizes[m_nTotalLutSizes].iYPartNumLog2 = iCurYPartNumLog2; 
        m_sLutSizes[m_nTotalLutSizes].iCPartNumLog2 = iCurCPartNumLog2; 
        m_nTotalLutSizes ++; 
      }
    }
  }

}

Void TEnc3DAsymLUT::xxCopyColorInfo( SColorInfo *** dst, SColorInfo *** src ,  SColorInfo *** dstC, SColorInfo *** srcC )
{
  Int yIdx, uIdx, vIdx; 

  // copy from pColorInfo to pMaxColorInfo
  for(yIdx = 0; yIdx < xGetYSize(); yIdx++)
  {
    for(uIdx = 0; uIdx < xGetUSize(); uIdx++)
    {
      for(vIdx = 0; vIdx < xGetVSize(); vIdx++)
      {
        dst [yIdx][uIdx][vIdx] = src [yIdx][uIdx][vIdx];
        dstC[yIdx][uIdx][vIdx] = srcC[yIdx][uIdx][vIdx];
      }
    }
  }
}

Void TEnc3DAsymLUT::xxAddColorInfo( Int yIdx, Int uIdx, Int vIdx, Int iYDiffLog2, Int iCDiffLog2 )
{
  SColorInfo & rCuboidColorInfo  = m_pColorInfo [yIdx][uIdx][vIdx];
  SColorInfo & rCuboidColorInfoC = m_pColorInfoC[yIdx][uIdx][vIdx];
  
  for( Int i = 0; i < (1<<iYDiffLog2); i++)
  {
    for (Int j = 0; j < (1<<iCDiffLog2); j++)
    {
      for(Int k = 0; k < (1<<iCDiffLog2); k++)
      {
        rCuboidColorInfo  += m_pMaxColorInfo [(yIdx<<iYDiffLog2)+i][(uIdx<<iCDiffLog2)+j][(vIdx<<iCDiffLog2)+k];
        rCuboidColorInfoC += m_pMaxColorInfoC[(yIdx<<iYDiffLog2)+i][(uIdx<<iCDiffLog2)+j][(vIdx<<iCDiffLog2)+k];
      }
    }
  }
}

Void TEnc3DAsymLUT::xxConsolidateData( SLUTSize *pCurLUTSize, SLUTSize *pMaxLUTSize )
{
  Int yIdx, uIdx, vIdx; 
  Int iYDiffLog2, iCDiffLog2;
  Int nYSize = 1<< pMaxLUTSize->iYPartNumLog2;
  Int nCSize = 1<< pMaxLUTSize->iCPartNumLog2;

  iYDiffLog2 = pMaxLUTSize->iYPartNumLog2-pCurLUTSize->iYPartNumLog2;
  iCDiffLog2 = pMaxLUTSize->iCPartNumLog2-pCurLUTSize->iCPartNumLog2;

  //assert(pMaxLUTSize->iCPartNumLog2 >= pCurLUTSize->iCPartNumLog2 && pMaxLUTSize->iYPartNumLog2 >= pCurLUTSize->iYPartNumLog2); 
  if (iYDiffLog2 == 0 && iCDiffLog2 == 0) // shouldn't have to do anything 
  {
    xxCopyColorInfo(m_pColorInfo, m_pMaxColorInfo, m_pColorInfoC, m_pMaxColorInfoC);
    return; 
  }

  xReset3DArray( m_pColorInfo  ,  1<<pMaxLUTSize->iYPartNumLog2, 1<<pMaxLUTSize->iCPartNumLog2, 1<<pMaxLUTSize->iCPartNumLog2 );
  xReset3DArray( m_pColorInfoC ,  1<<pMaxLUTSize->iYPartNumLog2, 1<<pMaxLUTSize->iCPartNumLog2, 1<<pMaxLUTSize->iCPartNumLog2 );

  for(yIdx = 0; yIdx < nYSize; yIdx++)
  {
    for(uIdx = 0; uIdx < nCSize; uIdx++)
    {
      for(vIdx = 0; vIdx < nCSize; vIdx++)
      {
        const SColorInfo & rCuboidSrc   = m_pMaxColorInfo [yIdx][uIdx][vIdx];
        const SColorInfo & rCuboidSrcC  = m_pMaxColorInfoC[yIdx][uIdx][vIdx];
        
        Int yIdx2, uIdx2, vIdx2; 
        yIdx2 = yIdx>>iYDiffLog2; 
        uIdx2 = uIdx>>iCDiffLog2;
        vIdx2 = vIdx>>iCDiffLog2; 

        m_pColorInfo [yIdx2][uIdx2][vIdx2] += rCuboidSrc;
        m_pColorInfoC[yIdx2][uIdx2][vIdx2] += rCuboidSrcC;
      }
    }
  }
}

Void TEnc3DAsymLUT::update3DAsymLUTParam( TEnc3DAsymLUT * pSrc )
{
  assert( pSrc->getMaxOctantDepth() == getMaxOctantDepth() && pSrc->getMaxYPartNumLog2() == getMaxYPartNumLog2() );
  xUpdatePartitioning( pSrc->getCurOctantDepth() , pSrc->getCurYPartNumLog2() 
#if R0151_CGS_3D_ASYMLUT_IMPROVE
    , pSrc->getAdaptChromaThresholdU() , pSrc->getAdaptChromaThresholdV()
#endif
    );
  setResQuantBit( pSrc->getResQuantBit() );
}

#endif 
#endif
