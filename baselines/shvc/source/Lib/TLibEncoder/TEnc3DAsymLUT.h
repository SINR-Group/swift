
#ifndef __TENC3DASYMLUT__
#define __TENC3DASYMLUT__

#include "../TLibCommon/TCom3DAsymLUT.h"
#include "../TLibCommon/TComSlice.h"
#include "../TLibCommon/CommonDef.h"
#include "../TLibCommon/TComPic.h"
#include "TEncCfg.h"
#if R0179_ENC_OPT_3DLUT_SIZE
#include "TEncCavlc.h"
#define MAX_NUM_LUT_SIZES               10   // 4+3+2+1
#define MAX_Y_SIZE                       4
#define MAX_C_SIZE                       4
#endif

#if Q0048_CGS_3D_ASYMLUT

typedef struct _ColorInfo
{
  Double YY , UU , VV;
  Double Ys , Us , Vs;  // sum of enhancement
  Double ys , us , vs;  // sum of base
  Double Yy , Yu , Yv;  // product of enhancement and base
  Double Uy , Uu , Uv;
  Double Vy , Vu , Vv;
  Double yy , yu , yv , uu , uv , vv; // product of base
  Double N; // number of pixel

public:
  _ColorInfo & operator += ( const _ColorInfo & rColorInfo )
  {
    YY += rColorInfo.YY;
    UU += rColorInfo.UU;
    VV += rColorInfo.VV;
    Ys += rColorInfo.Ys;
    Us += rColorInfo.Us;
    Vs += rColorInfo.Vs;
    ys += rColorInfo.ys;
    us += rColorInfo.us;
    vs += rColorInfo.vs;
    Yy += rColorInfo.Yy;
    Yu += rColorInfo.Yu;
    Yv += rColorInfo.Yv;
    Uy += rColorInfo.Uy;
    Uu += rColorInfo.Uu;
    Uv += rColorInfo.Uv;
    Vy += rColorInfo.Vy;
    Vu += rColorInfo.Vu;
    Vv += rColorInfo.Vv;
    yy += rColorInfo.yy;
    yu += rColorInfo.yu;
    yv += rColorInfo.yv;
    uu += rColorInfo.uu;
    uv += rColorInfo.uv;
    vv += rColorInfo.vv;
    N  += rColorInfo.N; 
    return *this;
  }

}SColorInfo;

#if R0179_ENC_OPT_3DLUT_SIZE
typedef struct _LUTSize 
{
  Int iYPartNumLog2; 
  Int iCPartNumLog2; 
} SLUTSize; 
#endif 

class TEnc3DAsymLUT : public TCom3DAsymLUT
{
public:
  TEnc3DAsymLUT();
  virtual ~TEnc3DAsymLUT();

  virtual Void  create( Int nMaxOctantDepth , Int nInputBitDepth , Int nInputBitDepthC , Int nOutputBitDepth , Int nOutputBitDepthC , Int nMaxYPartNumLog2 );
  virtual Void  destroy();
  Double derive3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS , Bool bElRapSliceTypeB );
  Double estimateDistWithCur3DAsymLUT( TComPic * pCurPic , UInt refLayerIdc );
#if R0179_ENC_OPT_3DLUT_SIZE
  Double getDistFactor( Int iSliceType, Int iLayer) { return m_dDistFactor[iSliceType][iLayer];}
  Double derive3DAsymLUT( TComSlice * pSlice , TComPic * pCurPic , UInt refLayerIdc , TEncCfg * pCfg , Bool bSignalPPS , Bool bElRapSliceTypeB, Double dFrameLambda );
  Void   update3DAsymLUTParam( TEnc3DAsymLUT * pSrc );
#endif

  Void  updatePicCGSBits( TComSlice * pcSlice , Int nPPSBit );
  Void  setPPSBit(Int n)  { m_nPPSBit = n;  }
  Int   getPPSBit()       { return m_nPPSBit;}
  Void  setDsOrigPic(TComPicYuv *pPicYuv) { m_pDsOrigPic = pPicYuv; };

protected:
  SColorInfo *** m_pColorInfo;
  SColorInfo *** m_pColorInfoC;
#if R0179_ENC_OPT_3DLUT_SIZE
  SColorInfo *** m_pMaxColorInfo;
  SColorInfo *** m_pMaxColorInfoC;
#endif 
  TComPicYuv* m_pDsOrigPic;
  SCuboid *** m_pEncCuboid;
  SCuboid *** m_pBestEncCuboid;
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  Int   m_nAccuFrameBit;                  // base + enhancement layer
  Int   m_nAccuFrameCGSBit;
  Int   m_nPrevFrameCGSPartNumLog2;
#else
  Int   m_nPrevFrameBit[3][MAX_TLAYER];                  // base + enhancement layer
  Int   m_nPrevFrameCGSBit[3][MAX_TLAYER];
  Int   m_nPrevFrameCGSPartNumLog2[3][MAX_TLAYER];
  Int   m_nPrevFrameOverWritePPS[3][MAX_TLAYER];
#endif
  Double m_dTotalFrameBit;
  Int   m_nTotalCGSBit;
  Int   m_nPPSBit;
  Int   m_nLUTBitDepth;
#if R0179_ENC_OPT_3DLUT_SIZE

  Double m_dDistFactor[3][MAX_TLAYER];         
  Int    m_nNumLUTBits[MAX_Y_SIZE][MAX_C_SIZE]; 
  Int    m_nPrevELFrameBit[3][MAX_TLAYER];   


  Int   m_nTotalLutSizes;
  SLUTSize m_sLutSizes[MAX_NUM_LUT_SIZES];
#endif 
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  Double m_dSumU;
  Double m_dSumV;
  Int    m_nNChroma;
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
  TComOutputBitstream  *m_pBitstreamRedirect;
  TEncCavlc *m_pEncCavlc;
#endif 

private:
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  Double  xxDeriveVertexPerColor( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
    Pel & rP0 , Pel & rP1 , Pel & rP3 , Pel & rP7 , Int nResQuantBit );
#else
  Double  xxDeriveVertexPerColor( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
    Int y0 , Int u0 , Int v0 , Int nLengthY , Int nLengthUV ,
    Pel & rP0 , Pel & rP1 , Pel & rP3 , Pel & rP7 , Int nResQuantBit );
#endif
  Void    xxDerivePartNumLog2( TComSlice * pSlice , TEncCfg * pcCfg , Int & rOctantDepth , Int & rYPartNumLog2 , Bool bSignalPPS , Bool bElRapSliceTypeB );
  Void    xxMapPartNum2DepthYPart( Int nPartNumLog2 , Int & rOctantDepth , Int & rYPartNumLog2 );
  Int     xxCoeff2Vertex( Double a , Double b , Double c , Double d , Int y , Int u , Int v ) { return ( ( Int )( a * y + b * u + c * v + d + 0.5 ) ); }
  Void    xxCollectData( TComPic * pCurPic , UInt refLayerIdc );

  Double  xxDeriveVertexes( Int nResQuantBit , SCuboid *** pCurCuboid );
  inline Double  xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
    Int y0 , Int u0 , Int v0 , Int nLengthY , Int nLengthUV , Pel nP0 , Pel nP1 , Pel nP3 , Pel nP7 );
  inline Double  xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
    Double a , Double b , Double c , Double d );
#if R0151_CGS_3D_ASYMLUT_IMPROVE
  inline Double  xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
    Pel nP0 , Pel nP1 , Pel nP3 , Pel nP7 );
#endif
#if R0179_ENC_OPT_3DLUT_SIZE
  Void    xxConsolidateData( SLUTSize *pCurLUTSize, SLUTSize *pMaxLUTSize );
  Void    xxGetAllLutSizes(TComSlice *pSlice);
  Void    xxCopyColorInfo( SColorInfo *** dst, SColorInfo *** src ,  SColorInfo *** dstC, SColorInfo *** srcC ); 
  Void    xxAddColorInfo( Int yIdx, Int uIdx, Int vIdx, Int iYDiffLog2, Int iCDiffLog2 );
#endif 
};

Double TEnc3DAsymLUT::xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
  Int y0 , Int u0 , Int v0 , Int nLengthY , Int nLengthUV , Pel nP0 , Pel nP1 , Pel nP3 , Pel nP7 )
{
  Double a = 1.0 * ( nP7 - nP3 ) / nLengthY;
  Double b = 1.0 * ( nP1 - nP0 ) / nLengthUV;
  Double c = 1.0 * ( nP3 - nP1 ) / nLengthUV;
  Double d = ( ( nP0 * nLengthUV + u0 * nP0 + ( v0 - u0 ) * nP1 - v0 * nP3 ) * nLengthY + y0 * nLengthUV * ( nP3 - nP7 ) ) / nLengthUV / nLengthY;
  return( xxCalEstDist( N , Ys , Yy , Yu , Yv , ys , us , vs , yy , yu , yv , uu , uv , vv , YY , a , b , c , d ) );
}

Double TEnc3DAsymLUT::xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
  Double a , Double b , Double c , Double d )
{
  Double dError = N * d * d + 2 * b * c * uv + 2 * a * c * yv + 2 * a * b * yu - 2 * c * Yv - 2 * b * Yu - 2 * a * Yy + 2 * c * d * vs + 2 * b * d * us + 2 * a * d * ys + a * a * yy + c * c * vv + b * b * uu - 2 * d * Ys + YY;
  return( dError );
};

#if R0151_CGS_3D_ASYMLUT_IMPROVE
Double TEnc3DAsymLUT::xxCalEstDist( Double N , Double Ys , Double Yy , Double Yu , Double Yv , Double ys , Double us , Double vs , Double yy , Double yu , Double yv , Double uu , Double uv , Double vv , Double YY ,
  Pel nP0 , Pel nP1 , Pel nP3 , Pel nP7 )
{
  const Int nOne = xGetNormCoeffOne();
  Double a = 1.0 * nP0 / nOne;
  Double b = 1.0 * nP1 / nOne;
  Double c = 1.0 * nP3 / nOne;
  Double d = nP7;
  Double dError = N * d * d + 2 * b * c * uv + 2 * a * c * yv + 2 * a * b * yu - 2 * c * Yv - 2 * b * Yu - 2 * a * Yy + 2 * c * d * vs + 2 * b * d * us + 2 * a * d * ys + a * a * yy + c * c * vv + b * b * uu - 2 * d * Ys + YY;
  return( dError );
};
#endif

#endif

#endif
