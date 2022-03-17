
#include "H264AVCEncoderLib.h"
#include "Distortion.h"

#include "H264AVCCommonLib/YuvPicBuffer.h"

#include <math.h>

#define Abs(x) abs(x)

H264AVC_NAMESPACE_BEGIN


XDistortion::XDistortion()
{
}

XDistortion::~XDistortion()
{
}

ErrVal XDistortion::create( XDistortion*& rpcDistortion )
{
  XDistortion* pcDistortion = new XDistortion;

  rpcDistortion = pcDistortion;

  ROT( NULL == pcDistortion );

  return Err::m_nOK;
}

ErrVal XDistortion::destroy()
{
  delete this;
  return Err::m_nOK;
}


Void XDistortion::loadOrgMbPelData( const YuvPicBuffer* pcOrgYuvBuffer, YuvMbBuffer*& rpcOrgMbBuffer )
{
  m_cOrgData.loadBuffer( pcOrgYuvBuffer );
  rpcOrgMbBuffer = &m_cOrgData;
}


ErrVal XDistortion::init()
{
  m_aiRows[0x0] =  0;
  m_aiRows[0x1] = 16;
  m_aiRows[0x2] =  8;
  m_aiRows[0x3] = 16;
  m_aiRows[0x4] =  8;
  m_aiRows[0x5] =  0;
  m_aiRows[0x6] =  0;
  m_aiRows[0x7] =  0;
  m_aiRows[0x8] =  8;
  m_aiRows[0x9] =  4;
  m_aiRows[0xa] =  8;
  m_aiRows[0xb] =  4;

  m_aiCols[0x0] =  0;
  m_aiCols[0x1] = 16;
  m_aiCols[0x2] = 16;
  m_aiCols[0x3] =  8;
  m_aiCols[0x4] =  8;
  m_aiCols[0x5] =  0;
  m_aiCols[0x6] =  0;
  m_aiCols[0x7] =  0;
  m_aiCols[0x8] =  8;
  m_aiCols[0x9] =  8;
  m_aiCols[0xa] =  4;
  m_aiCols[0xb] =  4;

  m_aaafpDistortionFunc[0][0][0x0] = NULL;
  m_aaafpDistortionFunc[0][0][0x1] = XDistortion::xGetSAD16x;
  m_aaafpDistortionFunc[0][0][0x2] = XDistortion::xGetSAD16x;
  m_aaafpDistortionFunc[0][0][0x3] = XDistortion::xGetSAD8x;
  m_aaafpDistortionFunc[0][0][0x4] = XDistortion::xGetSAD8x;
  m_aaafpDistortionFunc[0][0][0x5] = NULL;
  m_aaafpDistortionFunc[0][0][0x6] = NULL;
  m_aaafpDistortionFunc[0][0][0x7] = NULL;
  m_aaafpDistortionFunc[0][0][0x8] = XDistortion::xGetSAD8x;
  m_aaafpDistortionFunc[0][0][0x9] = XDistortion::xGetSAD8x;
  m_aaafpDistortionFunc[0][0][0xa] = XDistortion::xGetSAD4x;
  m_aaafpDistortionFunc[0][0][0xb] = XDistortion::xGetSAD4x;

  m_aaafpDistortionFunc[0][1][0x0] = NULL;
  m_aaafpDistortionFunc[0][1][0x1] = XDistortion::xGetSSE16x;
  m_aaafpDistortionFunc[0][1][0x2] = XDistortion::xGetSSE16x;
  m_aaafpDistortionFunc[0][1][0x3] = XDistortion::xGetSSE8x;
  m_aaafpDistortionFunc[0][1][0x4] = XDistortion::xGetSSE8x;
  m_aaafpDistortionFunc[0][1][0x5] = NULL;
  m_aaafpDistortionFunc[0][1][0x6] = NULL;
  m_aaafpDistortionFunc[0][1][0x7] = NULL;
  m_aaafpDistortionFunc[0][1][0x8] = XDistortion::xGetSSE8x;
  m_aaafpDistortionFunc[0][1][0x9] = XDistortion::xGetSSE8x;
  m_aaafpDistortionFunc[0][1][0xa] = XDistortion::xGetSSE4x;
  m_aaafpDistortionFunc[0][1][0xb] = XDistortion::xGetSSE4x;

  m_aaafpDistortionFunc[0][2][0x0] = NULL;
  m_aaafpDistortionFunc[0][2][0x1] = XDistortion::xGetHAD16x;
  m_aaafpDistortionFunc[0][2][0x2] = XDistortion::xGetHAD16x;
  m_aaafpDistortionFunc[0][2][0x3] = XDistortion::xGetHAD8x;
  m_aaafpDistortionFunc[0][2][0x4] = XDistortion::xGetHAD8x;
  m_aaafpDistortionFunc[0][2][0x5] = NULL;
  m_aaafpDistortionFunc[0][2][0x6] = NULL;
  m_aaafpDistortionFunc[0][2][0x7] = NULL;
  m_aaafpDistortionFunc[0][2][0x8] = XDistortion::xGetHAD8x;
  m_aaafpDistortionFunc[0][2][0x9] = XDistortion::xGetHAD8x;
  m_aaafpDistortionFunc[0][2][0xa] = XDistortion::xGetHAD4x;
  m_aaafpDistortionFunc[0][2][0xb] = XDistortion::xGetHAD4x;

  m_aaafpDistortionFunc[0][3][0x0] = NULL;
  m_aaafpDistortionFunc[0][3][0x1] = XDistortion::xGetYuvSAD16x;
  m_aaafpDistortionFunc[0][3][0x2] = XDistortion::xGetYuvSAD16x;
  m_aaafpDistortionFunc[0][3][0x3] = XDistortion::xGetYuvSAD8x;
  m_aaafpDistortionFunc[0][3][0x4] = XDistortion::xGetYuvSAD8x;
  m_aaafpDistortionFunc[0][3][0x5] = NULL;
  m_aaafpDistortionFunc[0][3][0x6] = NULL;
  m_aaafpDistortionFunc[0][3][0x7] = NULL;
  m_aaafpDistortionFunc[0][3][0x8] = XDistortion::xGetYuvSAD8x;
  m_aaafpDistortionFunc[0][3][0x9] = XDistortion::xGetYuvSAD8x;
  m_aaafpDistortionFunc[0][3][0xa] = XDistortion::xGetYuvSAD4x;
  m_aaafpDistortionFunc[0][3][0xb] = XDistortion::xGetYuvSAD4x;



  m_aaafpDistortionFunc[1][0][0]  = NULL;
  m_aaafpDistortionFunc[1][0][1]  = XDistortion::xGetBiSAD16x;
  m_aaafpDistortionFunc[1][0][2]  = XDistortion::xGetBiSAD16x;
  m_aaafpDistortionFunc[1][0][3]  = XDistortion::xGetBiSAD8x;
  m_aaafpDistortionFunc[1][0][4]  = XDistortion::xGetBiSAD8x;
  m_aaafpDistortionFunc[1][0][5]  = NULL;
  m_aaafpDistortionFunc[1][0][6]  = NULL;
  m_aaafpDistortionFunc[1][0][7]  = NULL;
  m_aaafpDistortionFunc[1][0][8]  = XDistortion::xGetBiSAD8x;
  m_aaafpDistortionFunc[1][0][9]  = XDistortion::xGetBiSAD8x;
  m_aaafpDistortionFunc[1][0][10] = XDistortion::xGetBiSAD4x;
  m_aaafpDistortionFunc[1][0][11] = XDistortion::xGetBiSAD4x;

  m_aaafpDistortionFunc[1][1][0]  = NULL;
  m_aaafpDistortionFunc[1][1][1]  = XDistortion::xGetBiSSE16x;
  m_aaafpDistortionFunc[1][1][2]  = XDistortion::xGetBiSSE16x;
  m_aaafpDistortionFunc[1][1][3]  = XDistortion::xGetBiSSE8x;
  m_aaafpDistortionFunc[1][1][4]  = XDistortion::xGetBiSSE8x;
  m_aaafpDistortionFunc[1][1][5]  = NULL;
  m_aaafpDistortionFunc[1][1][6]  = NULL;
  m_aaafpDistortionFunc[1][1][7]  = NULL;
  m_aaafpDistortionFunc[1][1][8]  = XDistortion::xGetBiSSE8x;
  m_aaafpDistortionFunc[1][1][9]  = XDistortion::xGetBiSSE8x;
  m_aaafpDistortionFunc[1][1][10] = XDistortion::xGetBiSSE4x;
  m_aaafpDistortionFunc[1][1][11] = XDistortion::xGetBiSSE4x;

  m_aaafpDistortionFunc[1][2][0]  = NULL;
  m_aaafpDistortionFunc[1][2][1]  = XDistortion::xGetBiHAD16x;
  m_aaafpDistortionFunc[1][2][2]  = XDistortion::xGetBiHAD16x;
  m_aaafpDistortionFunc[1][2][3]  = XDistortion::xGetBiHAD8x;
  m_aaafpDistortionFunc[1][2][4]  = XDistortion::xGetBiHAD8x;
  m_aaafpDistortionFunc[1][2][5]  = NULL;
  m_aaafpDistortionFunc[1][2][6]  = NULL;
  m_aaafpDistortionFunc[1][2][7]  = NULL;
  m_aaafpDistortionFunc[1][2][8]  = XDistortion::xGetBiHAD8x;
  m_aaafpDistortionFunc[1][2][9]  = XDistortion::xGetBiHAD8x;
  m_aaafpDistortionFunc[1][2][10] = XDistortion::xGetBiHAD4x;
  m_aaafpDistortionFunc[1][2][11] = XDistortion::xGetBiHAD4x;

  m_aaafpDistortionFunc[1][3][0]  = NULL;
  m_aaafpDistortionFunc[1][3][1]  = XDistortion::xGetBiYuvSAD16x;
  m_aaafpDistortionFunc[1][3][2]  = XDistortion::xGetBiYuvSAD16x;
  m_aaafpDistortionFunc[1][3][3]  = XDistortion::xGetBiYuvSAD8x;
  m_aaafpDistortionFunc[1][3][4]  = XDistortion::xGetBiYuvSAD8x;
  m_aaafpDistortionFunc[1][3][5]  = NULL;
  m_aaafpDistortionFunc[1][3][6]  = NULL;
  m_aaafpDistortionFunc[1][3][7]  = NULL;
  m_aaafpDistortionFunc[1][3][8]  = XDistortion::xGetBiYuvSAD8x;
  m_aaafpDistortionFunc[1][3][9]  = XDistortion::xGetBiYuvSAD8x;
  m_aaafpDistortionFunc[1][3][10] = XDistortion::xGetBiYuvSAD4x;
  m_aaafpDistortionFunc[1][3][11] = XDistortion::xGetBiYuvSAD4x;

  return Err::m_nOK;
}



UInt XDistortion::get8x8Cb( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( BLK_8x8, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getMbCbAddr();
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  return cDSS.Func( &cDSS );
}

UInt XDistortion::get8x8Cr( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( BLK_8x8, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getMbCrAddr();
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  return cDSS.Func( &cDSS );
}


UInt XDistortion::getLum4x4( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( BLK_4x4, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getLumBlk();
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  return cDSS.Func( &cDSS );
}


UInt XDistortion::getLum16x16( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( MODE_16x16, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getMbLumAddr();  // i do not like this
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  return cDSS.Func( &cDSS );
}

UInt XDistortion::getLum16x16RP( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( MODE_16x16, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getMbLumAddr();  // i do not like this
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  if(checkLargeDistortion(cDSS.pYOrg, cDSS.pYSearch, iStride))
    return 10*cDSS.Func( &cDSS );

  return cDSS.Func( &cDSS );
}

UInt XDistortion::checkLargeDistortion( XPel *pOrg, XPel *pPel, Int iStride )
{
  int i, j, m, n;
  XPel a[16];

  for(i=0; i<4; i++)
    for(j=0; j<4; j++)
    {
      for(m=0; m<4; m++)
        for(n=0; n<4; n++)
        {
          int offset = (i*4+m)*iStride + j*4 + n;
          a[m*4+n] = abs(pOrg[offset]-pPel[offset]);
        }

      int ave=0;
      for(m=0; m<16; m++)
        ave += a[m];
      ave = (ave+8)/16;

      for(m=0; m<16; m++)
        a[m] = a[m]>2*ave? 1:0;
      for(m=0,n=0; m<4; m++, n+=4)
      {
        if(a[n]+a[n+1]+a[n+2]+a[n+3]>=3)
          return 1;
        if(a[m]+a[m+4]+a[m+8]+a[m+12]>=3)
          return 1;
      }
    }
  return 0;
}


UInt XDistortion::getLum8x8( XPel *pPel, Int iStride, DFunc eDFunc )
{
  XDistSearchStruct cDSS;
  getDistStruct( BLK_8x8, eDFunc, false, cDSS );
  cDSS.pYOrg    = m_cOrgData.getLumBlk();
  cDSS.pYSearch = pPel;
  cDSS.iYStride = iStride;
  return cDSS.Func( &cDSS );
}



#define X_OFFSET 1


UInt XDistortion::xCalcHadamard4x4( XPel *pucOrg, XPel *pPel, Int iStride )
{
  XPel* pucCur = pPel;
  UInt uiSum = 0;
  Int aai[4][4];
  Int ai[4];
  Int n;

  for( n = 0; n < 4; n++ )
  {
    aai[n][0] = pucOrg[0] - pucCur[0];
    aai[n][1] = pucOrg[1] - pucCur[1];
    aai[n][2] = pucOrg[2] - pucCur[2];
    aai[n][3] = pucOrg[3] - pucCur[3];
    pucCur += iStride;
    pucOrg += MB_BUFFER_WIDTH;
  }


  for( n = 0; n < 4; n++ )
  {
    ai[0] = aai[0][n] + aai[3][n];
    ai[1] = aai[1][n] + aai[2][n];
    ai[2] = aai[1][n] - aai[2][n];
    ai[3] = aai[0][n] - aai[3][n];

    aai[0][n] = ai[0] + ai[1];
    aai[2][n] = ai[0] - ai[1];
    aai[1][n] = ai[3] + ai[2];
    aai[3][n] = ai[3] - ai[2];
  }

  for( n = 0; n < 4; n++ )
  {
    Int iTemp1;
    Int iTemp2;
    iTemp1 = aai[n][0] + aai[n][3];
    iTemp2 = aai[n][1] + aai[n][2];

    uiSum += Abs( iTemp1 + iTemp2 );
    uiSum += Abs( iTemp1 - iTemp2 );

    iTemp1 = aai[n][1] - aai[n][2];
    iTemp2 = aai[n][0] - aai[n][3];

    uiSum += Abs( iTemp1 + iTemp2 );
    uiSum += Abs( iTemp1 - iTemp2 );
  }
  return uiSum/2;
}


UInt XDistortion::xCalcBiHadamard4x4( XPel *pucOrg, XPel *pPelFix, XPel *pPel, Int iStride )
{
  XPel* pucCur = pPel;

  UInt uiSum = 0;
  Int aai[4][4];
  Int ai[4];
  Int n;

  for( n = 0; n < 4; n++ )
  {
    aai[n][0] = pucOrg[0] - ((pucCur[0] + pPelFix[0] + 1)>>1);
    aai[n][1] = pucOrg[1] - ((pucCur[1] + pPelFix[1] + 1)>>1);
    aai[n][2] = pucOrg[2] - ((pucCur[2] + pPelFix[2] + 1)>>1);
    aai[n][3] = pucOrg[3] - ((pucCur[3] + pPelFix[3] + 1)>>1);
    pucCur += iStride;
    pucOrg  += MB_BUFFER_WIDTH;
    pPelFix += 24;
  }

  for( n = 0; n < 4; n++ )
  {
    ai[0] = aai[0][n] + aai[3][n];
    ai[1] = aai[1][n] + aai[2][n];
    ai[2] = aai[1][n] - aai[2][n];
    ai[3] = aai[0][n] - aai[3][n];

    aai[0][n] = ai[0] + ai[1];
    aai[2][n] = ai[0] - ai[1];
    aai[1][n] = ai[2] + ai[3];
    aai[3][n] = ai[3] - ai[2];
  }

  for( n = 0; n < 4; n++ )
  {
    ai[0] = aai[n][0] + aai[n][3];
    ai[1] = aai[n][1] + aai[n][2];
    ai[2] = aai[n][1] - aai[n][2];
    ai[3] = aai[n][0] - aai[n][3];

    uiSum += Abs( ai[0] + ai[1] );
    uiSum += Abs( ai[0] - ai[1] );
    uiSum += Abs( ai[2] + ai[3] );
    uiSum += Abs( ai[3] - ai[2] );
  }

  return uiSum/2;

}





UInt XDistortion::xGetSAD16x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += Abs( pucOrg[0x0] - pucCur[0x0] );
    uiSum += Abs( pucOrg[0x1] - pucCur[0x1] );
    uiSum += Abs( pucOrg[0x2] - pucCur[0x2] );
    uiSum += Abs( pucOrg[0x3] - pucCur[0x3] );
    uiSum += Abs( pucOrg[0x4] - pucCur[0x4] );
    uiSum += Abs( pucOrg[0x5] - pucCur[0x5] );
    uiSum += Abs( pucOrg[0x6] - pucCur[0x6] );
    uiSum += Abs( pucOrg[0x7] - pucCur[0x7] );
    uiSum += Abs( pucOrg[0x8] - pucCur[0x8] );
    uiSum += Abs( pucOrg[0x9] - pucCur[0x9] );
    uiSum += Abs( pucOrg[0xa] - pucCur[0xa] );
    uiSum += Abs( pucOrg[0xb] - pucCur[0xb] );
    uiSum += Abs( pucOrg[0xc] - pucCur[0xc] );
    uiSum += Abs( pucOrg[0xd] - pucCur[0xd] );
    uiSum += Abs( pucOrg[0xe] - pucCur[0xe] );
    uiSum += Abs( pucOrg[0xf] - pucCur[0xf] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSum;
}





UInt XDistortion::xGetYuvSAD16x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;
  UInt  uiSumY  = 0;
  UInt  uiSumU  = 0;
  UInt  uiSumV  = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSumY  += Abs( pucOrg[0x0] - pucCur[0x0] );
    uiSumY  += Abs( pucOrg[0x1] - pucCur[0x1] );
    uiSumY  += Abs( pucOrg[0x2] - pucCur[0x2] );
    uiSumY  += Abs( pucOrg[0x3] - pucCur[0x3] );
    uiSumY  += Abs( pucOrg[0x4] - pucCur[0x4] );
    uiSumY  += Abs( pucOrg[0x5] - pucCur[0x5] );
    uiSumY  += Abs( pucOrg[0x6] - pucCur[0x6] );
    uiSumY  += Abs( pucOrg[0x7] - pucCur[0x7] );
    uiSumY  += Abs( pucOrg[0x8] - pucCur[0x8] );
    uiSumY  += Abs( pucOrg[0x9] - pucCur[0x9] );
    uiSumY  += Abs( pucOrg[0xa] - pucCur[0xa] );
    uiSumY  += Abs( pucOrg[0xb] - pucCur[0xb] );
    uiSumY  += Abs( pucOrg[0xc] - pucCur[0xc] );
    uiSumY  += Abs( pucOrg[0xd] - pucCur[0xd] );
    uiSumY  += Abs( pucOrg[0xe] - pucCur[0xe] );
    uiSumY  += Abs( pucOrg[0xf] - pucCur[0xf] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  pucCur  = pcDSS->pUSearch;
  pucOrg  = pcDSS->pUOrg;
  iStride = pcDSS->iCStride;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumU  += Abs( pucOrg[0x0] - pucCur[0x0] );
    uiSumU  += Abs( pucOrg[0x1] - pucCur[0x1] );
    uiSumU  += Abs( pucOrg[0x2] - pucCur[0x2] );
    uiSumU  += Abs( pucOrg[0x3] - pucCur[0x3] );
    uiSumU  += Abs( pucOrg[0x4] - pucCur[0x4] );
    uiSumU  += Abs( pucOrg[0x5] - pucCur[0x5] );
    uiSumU  += Abs( pucOrg[0x6] - pucCur[0x6] );
    uiSumU  += Abs( pucOrg[0x7] - pucCur[0x7] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  pucCur  = pcDSS->pVSearch;
  pucOrg  = pcDSS->pVOrg;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumV  += Abs( pucOrg[0x0] - pucCur[0x0] );
    uiSumV  += Abs( pucOrg[0x1] - pucCur[0x1] );
    uiSumV  += Abs( pucOrg[0x2] - pucCur[0x2] );
    uiSumV  += Abs( pucOrg[0x3] - pucCur[0x3] );
    uiSumV  += Abs( pucOrg[0x4] - pucCur[0x4] );
    uiSumV  += Abs( pucOrg[0x5] - pucCur[0x5] );
    uiSumV  += Abs( pucOrg[0x6] - pucCur[0x6] );
    uiSumV  += Abs( pucOrg[0x7] - pucCur[0x7] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}






UInt XDistortion::xGetSSE16x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;
  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp = pucOrg[0x0] - pucCur[0x0];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x1] - pucCur[0x1];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x2] - pucCur[0x2];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x3] - pucCur[0x3];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x4] - pucCur[0x4];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x5] - pucCur[0x5];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x6] - pucCur[0x6];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x7] - pucCur[0x7];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x8] - pucCur[0x8];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0x9] - pucCur[0x9];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xa] - pucCur[0xa];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xb] - pucCur[0xb];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xc] - pucCur[0xc];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xd] - pucCur[0xd];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xe] - pucCur[0xe];
    uiSum += iTemp * iTemp;
    iTemp = pucOrg[0xf] - pucCur[0xf];
    uiSum += iTemp * iTemp;
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }
  return uiSum;
}


UInt XDistortion::xGetSAD8x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += Abs( pucOrg[0] - pucCur[0] );
    uiSum += Abs( pucOrg[1] - pucCur[1] );
    uiSum += Abs( pucOrg[2] - pucCur[2] );
    uiSum += Abs( pucOrg[3] - pucCur[3] );
    uiSum += Abs( pucOrg[4] - pucCur[4] );
    uiSum += Abs( pucOrg[5] - pucCur[5] );
    uiSum += Abs( pucOrg[6] - pucCur[6] );
    uiSum += Abs( pucOrg[7] - pucCur[7] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSum;
}





UInt XDistortion::xGetYuvSAD8x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;
  UInt  uiSumY  = 0;
  UInt  uiSumU  = 0;
  UInt  uiSumV  = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSumY  += Abs( pucOrg[0] - pucCur[0] );
    uiSumY  += Abs( pucOrg[1] - pucCur[1] );
    uiSumY  += Abs( pucOrg[2] - pucCur[2] );
    uiSumY  += Abs( pucOrg[3] - pucCur[3] );
    uiSumY  += Abs( pucOrg[4] - pucCur[4] );
    uiSumY  += Abs( pucOrg[5] - pucCur[5] );
    uiSumY  += Abs( pucOrg[6] - pucCur[6] );
    uiSumY  += Abs( pucOrg[7] - pucCur[7] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  pucCur  = pcDSS->pUSearch;
  pucOrg  = pcDSS->pUOrg;
  iStride = pcDSS->iCStride;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumU  += Abs( pucOrg[0] - pucCur[0] );
    uiSumU  += Abs( pucOrg[1] - pucCur[1] );
    uiSumU  += Abs( pucOrg[2] - pucCur[2] );
    uiSumU  += Abs( pucOrg[3] - pucCur[3] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  pucCur  = pcDSS->pVSearch;
  pucOrg  = pcDSS->pVOrg;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumV  += Abs( pucOrg[0] - pucCur[0] );
    uiSumV  += Abs( pucOrg[1] - pucCur[1] );
    uiSumV  += Abs( pucOrg[2] - pucCur[2] );
    uiSumV  += Abs( pucOrg[3] - pucCur[3] );
    pucOrg  += MB_BUFFER_WIDTH;
    pucCur  += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}





UInt XDistortion::xGetSSE8x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;
  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp   = pucOrg[0] - pucCur[0];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[1] - pucCur[1];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[2] - pucCur[2];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[3] - pucCur[3];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[4] - pucCur[4];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[5] - pucCur[5];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[6] - pucCur[6];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[7] - pucCur[7];
    uiSum  += iTemp * iTemp;
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSum;
}




UInt XDistortion::xGetSAD4x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum  += Abs( pucOrg[0] - pucCur[0] );
    uiSum  += Abs( pucOrg[1] - pucCur[1] );
    uiSum  += Abs( pucOrg[2] - pucCur[2] );
    uiSum  += Abs( pucOrg[3] - pucCur[3] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSum;
}






UInt XDistortion::xGetYuvSAD4x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;
  UInt  uiSumY  = 0;
  UInt  uiSumU  = 0;
  UInt  uiSumV  = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSumY += Abs( pucOrg[0] - pucCur[0] );
    uiSumY += Abs( pucOrg[1] - pucCur[1] );
    uiSumY += Abs( pucOrg[2] - pucCur[2] );
    uiSumY += Abs( pucOrg[3] - pucCur[3] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  pucCur  = pcDSS->pUSearch;
  pucOrg  = pcDSS->pUOrg;
  iStride = pcDSS->iCStride;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumU += Abs( pucOrg[0] - pucCur[0] );
    uiSumU += Abs( pucOrg[1] - pucCur[1] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  pucCur  = pcDSS->pVSearch;
  pucOrg  = pcDSS->pVOrg;
  iRows   = pcDSS->iRows / 2;

  for( ; iRows != 0; iRows-- )
  {
    uiSumV += Abs( pucOrg[0] - pucCur[0] );
    uiSumV += Abs( pucOrg[1] - pucCur[1] );
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}




UInt XDistortion::xGetSSE4x( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt uiSum = 0;
  Int  iTemp;

  for( ; iRows != 0; iRows-- )
  {
    iTemp   = pucOrg[0] - pucCur[0];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[1] - pucCur[1];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[2] - pucCur[2];
    uiSum  += iTemp * iTemp;
    iTemp   = pucOrg[3] - pucCur[3];
    uiSum  += iTemp * iTemp;
    pucOrg += MB_BUFFER_WIDTH;
    pucCur += iStride;
  }

  return uiSum;
}



UInt XDistortion::xGetHAD16x ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcHadamard4x4( pucOrg+0x0, pucCur+0x0, iStride );
    uiSum += xCalcHadamard4x4( pucOrg+0x4, pucCur+0x4, iStride );
    uiSum += xCalcHadamard4x4( pucOrg+0x8, pucCur+0x8, iStride );
    uiSum += xCalcHadamard4x4( pucOrg+0xc, pucCur+0xc, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
  }
  return uiSum;
}


UInt XDistortion::xGetHAD8x  ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcHadamard4x4( pucOrg+0x0, pucCur+0x0, iStride );
    uiSum += xCalcHadamard4x4( pucOrg+0x4, pucCur+0x4, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
  }
  return uiSum;
}

UInt XDistortion::xGetHAD4x  ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcHadamard4x4( pucOrg+0x0, pucCur+0x0, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
  }
  return uiSum;
}



UInt XDistortion::xGetBiSAD16x( XDistSearchStruct* pcDSS )
{
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  x;
  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  iStride -= 16;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 16; x != 0; x-- )
    {
      uiSum += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-16;
    uiOffset2 += iStride;
  }

  return uiSum;
}



UInt XDistortion::xGetBiYuvSAD16x( XDistSearchStruct* pcDSS )
{
  UInt  x;
  XPel* pucSrc1   = pcDSS->pYSearch;
  XPel* pucSrc2   = pcDSS->pYFix;
  XPel* pucOrg    = pcDSS->pYOrg;
  Int   iStride   = pcDSS->iYStride - 16;
  Int   iRows     = pcDSS->iRows;
  UInt  uiSumY    = 0;
  UInt  uiSumU    = 0;
  UInt  uiSumV    = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 16; x != 0; x-- )
    {
      uiSumY   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-16;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pUSearch;
  pucSrc2   = pcDSS->pUFix;
  pucOrg    = pcDSS->pUOrg;
  iStride   = pcDSS->iCStride - 8;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 8; x != 0; x-- )
    {
      uiSumU   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-8;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pVSearch;
  pucSrc2   = pcDSS->pVFix;
  pucOrg    = pcDSS->pVOrg;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 8; x != 0; x-- )
    {
      uiSumV   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-8;
    uiOffset2 += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}



UInt XDistortion::xGetBiSAD8x( XDistSearchStruct* pcDSS )
{
  UInt  x;
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  iStride -= 8;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 8; x != 0; x-- )
    {
      uiSum += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-8;
    uiOffset2 += iStride;
  }

  return uiSum;
}


UInt XDistortion::xGetBiYuvSAD8x( XDistSearchStruct* pcDSS )
{
  UInt  x;
  XPel* pucSrc1   = pcDSS->pYSearch;
  XPel* pucSrc2   = pcDSS->pYFix;
  XPel* pucOrg    = pcDSS->pYOrg;
  Int   iStride   = pcDSS->iYStride - 8;
  Int   iRows     = pcDSS->iRows;
  UInt  uiSumY    = 0;
  UInt  uiSumU    = 0;
  UInt  uiSumV    = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 8; x != 0; x-- )
    {
      uiSumY   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-8;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pUSearch;
  pucSrc2   = pcDSS->pUFix;
  pucOrg    = pcDSS->pUOrg;
  iStride   = pcDSS->iCStride - 4;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 4; x != 0; x-- )
    {
      uiSumU   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-4;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pVSearch;
  pucSrc2   = pcDSS->pVFix;
  pucOrg    = pcDSS->pVOrg;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 4; x != 0; x-- )
    {
      uiSumV   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-4;
    uiOffset2 += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}



UInt XDistortion::xGetBiSAD4x( XDistSearchStruct* pcDSS )
{
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  x;
  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  iStride -= 4;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 4; x != 0; x-- )
    {
      uiSum += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-4;
    uiOffset2 += iStride;
  }

  return uiSum;
}



UInt XDistortion::xGetBiYuvSAD4x( XDistSearchStruct* pcDSS )
{
  UInt  x;
  XPel* pucSrc1   = pcDSS->pYSearch;
  XPel* pucSrc2   = pcDSS->pYFix;
  XPel* pucOrg    = pcDSS->pYOrg;
  Int   iStride   = pcDSS->iYStride - 4;
  Int   iRows     = pcDSS->iRows;
  UInt  uiSumY    = 0;
  UInt  uiSumU    = 0;
  UInt  uiSumV    = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 4; x != 0; x-- )
    {
      uiSumY   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-4;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pUSearch;
  pucSrc2   = pcDSS->pUFix;
  pucOrg    = pcDSS->pUOrg;
  iStride   = pcDSS->iCStride - 2;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 2; x != 0; x-- )
    {
      uiSumU   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-2;
    uiOffset2 += iStride;
  }

  pucSrc1   = pcDSS->pVSearch;
  pucSrc2   = pcDSS->pVFix;
  pucOrg    = pcDSS->pVOrg;
  iRows     = pcDSS->iRows / 2;
  uiOffset1 = 0;
  uiOffset2 = 0;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 2; x != 0; x-- )
    {
      uiSumV   += Abs( pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) );
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-2;
    uiOffset2 += iStride;
  }

  return uiSumY+uiSumU+uiSumV;
}



UInt XDistortion::xGetBiSSE16x( XDistSearchStruct* pcDSS )
{
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  x;
  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;
  Int   iTemp;

  iStride -= 16;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 16; x != 0; x-- )
    {
      iTemp = pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) ;
      uiSum += iTemp * iTemp;
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-16;
    uiOffset2 += iStride;
  }

  return uiSum;
}


UInt XDistortion::xGetBiSSE8x( XDistSearchStruct* pcDSS )
{
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  x;
  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;
  Int   iTemp;

  iStride -= 8;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 8; x != 0; x-- )
    {
      iTemp = pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) ;
      uiSum += iTemp * iTemp;
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-8;
    uiOffset2 += iStride;
  }

  return uiSum;
}


UInt XDistortion::xGetBiSSE4x( XDistSearchStruct* pcDSS )
{
  XPel* pucSrc1 = pcDSS->pYSearch;
  XPel* pucSrc2 = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows;

  UInt  x;
  UInt  uiSum = 0;
  UInt  uiOffset1 = 0;
  UInt  uiOffset2 = 0;
  Int   iTemp;

  iStride -= 4;

  for( ; iRows != 0; iRows-- )
  {
    for( x = 4; x != 0; x-- )
    {
      iTemp = pucOrg[uiOffset1] - ((pucSrc1[uiOffset2] + pucSrc2[uiOffset1] + 1) >> 1) ;
      uiSum += iTemp * iTemp;
      uiOffset1++;
      uiOffset2++;
    }
    uiOffset1 += MB_BUFFER_WIDTH-4;
    uiOffset2 += iStride;
  }

  return uiSum;
}



UInt XDistortion::xGetBiHAD16x ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucFix  = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcBiHadamard4x4( pucOrg+0x0, pucFix+0x0, pucCur+0x0, iStride );
    uiSum += xCalcBiHadamard4x4( pucOrg+0x4, pucFix+0x4, pucCur+0x4, iStride );
    uiSum += xCalcBiHadamard4x4( pucOrg+0x8, pucFix+0x8, pucCur+0x8, iStride );
    uiSum += xCalcBiHadamard4x4( pucOrg+0xc, pucFix+0xc, pucCur+0xc, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
    pucFix += 4*MB_BUFFER_WIDTH;
  }
  return uiSum;
}


UInt XDistortion::xGetBiHAD8x  ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucFix  = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcBiHadamard4x4( pucOrg+0x0, pucFix+0x0, pucCur+0x0, iStride );
    uiSum += xCalcBiHadamard4x4( pucOrg+0x4, pucFix+0x4, pucCur+0x4, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
    pucFix += 4*MB_BUFFER_WIDTH;
  }
  return uiSum;
}

UInt XDistortion::xGetBiHAD4x  ( XDistSearchStruct* pcDSS )
{
  XPel* pucCur  = pcDSS->pYSearch;
  XPel* pucFix  = pcDSS->pYFix;
  XPel* pucOrg  = pcDSS->pYOrg;
  Int   iStride = pcDSS->iYStride;
  Int   iRows   = pcDSS->iRows>>2;

  UInt uiSum = 0;

  for( ; iRows != 0; iRows-- )
  {
    uiSum += xCalcBiHadamard4x4( pucOrg+0x0, pucFix+0x0, pucCur+0x0, iStride );
    pucOrg += 4*MB_BUFFER_WIDTH;
    pucCur += 4*iStride;
    pucFix += 4*MB_BUFFER_WIDTH;
  }
  return uiSum;
}

//TMM_WP
Void XDistortion::xGetWeight(XPel *pucRef, XPel *pucOrg, const UInt uiStride,
                             const UInt uiHeight, const UInt uiWidth,
                             Double &dDCOrg, Double &dDCRef)
{
    /* get dc of org & ref frame */
    for (UInt y = 0; y < uiHeight; y++)
    {
        for (UInt x = 0; x < uiWidth; x++)
        {
            dDCOrg += (Double)pucOrg[x];
            dDCRef += (Double)pucRef[x];
        }

        pucOrg += uiStride;
        pucRef += uiStride;
    }
}



ErrVal XDistortion::getLumaWeight( YuvPicBuffer* pcOrgPicBuffer,
                                   YuvPicBuffer* pcRefPicBuffer, Double& rfWeight,
                                   UInt uiLumaLog2WeightDenom)
{
  ROT( NULL == pcRefPicBuffer );
  ROT( NULL == pcOrgPicBuffer );

  const Int iStride = pcRefPicBuffer->getLStride();
  const Int iHeight = pcRefPicBuffer->getLHeight();
  const Int iWidth  = pcRefPicBuffer->getLWidth();

  AOT_DBG( iStride != pcOrgPicBuffer->getLStride() );

  XPel* pucRef = pcRefPicBuffer->getLumOrigin();
  XPel* pucOrg = pcOrgPicBuffer->getLumOrigin();

  Double dDCOrg = 0;
  Double dDCRef = 0;
  xGetWeight( pucRef, pucOrg, iStride, iHeight, iWidth, dDCOrg, dDCRef);

  if(dDCRef)
  {
      rfWeight = (Int) (rfWeight * dDCOrg / dDCRef + 0.5);

      if(rfWeight < -64 || rfWeight > 127)
          rfWeight = 1 << uiLumaLog2WeightDenom;
  }

  return Err::m_nOK;
}


ErrVal XDistortion::getChromaWeight( YuvPicBuffer* pcOrgPicBuffer,
                                     YuvPicBuffer* pcRefPicBuffer,
                                     Double& rfWeight, UInt uiChromaLog2WeightDenom, Bool bCb )
{
  ROT( NULL == pcRefPicBuffer );
  ROT( NULL == pcOrgPicBuffer );

  /* no weights for chroma */
  rfWeight = 1 << uiChromaLog2WeightDenom;

  return Err::m_nOK;
}

ErrVal XDistortion::getLumaOffsets( YuvPicBuffer* pcOrgPicBuffer,
                                    YuvPicBuffer* pcRefPicBuffer, Double& rfOffset )
{
  ROT( NULL == pcRefPicBuffer );
  ROT( NULL == pcOrgPicBuffer );

  const Int iStride = pcRefPicBuffer->getLStride();
  const Int iHeight = pcRefPicBuffer->getLHeight();
  const Int iWidth  = pcRefPicBuffer->getLWidth();

  AOT_DBG( iStride != pcOrgPicBuffer->getLStride() );

  XPel* pucRef = pcRefPicBuffer->getLumOrigin();
  XPel* pucOrg = pcOrgPicBuffer->getLumOrigin();

  Double dDCOrg = 0;
  Double dDCRef = 0;
  xGetWeight( pucRef, pucOrg, iStride, iHeight, iWidth, dDCOrg, dDCRef);

  rfOffset = (Int) ( ( (dDCOrg - dDCRef)/ (iHeight * iWidth) ) + 0.5);

  rfOffset = (rfOffset < -128) ? -128 : (rfOffset > 127 ? 127 : rfOffset);

  return Err::m_nOK;
}


ErrVal XDistortion::getChromaOffsets( YuvPicBuffer* pcOrgPicBuffer,
                                      YuvPicBuffer* pcRefPicBuffer,
                                      Double& rfOffset, Bool bCb )
{
  ROT( NULL == pcRefPicBuffer );
  ROT( NULL == pcOrgPicBuffer );

  /* no offsets for chroma */
  rfOffset = 0;

  return Err::m_nOK;
}
//TMM_WP



H264AVC_NAMESPACE_END
