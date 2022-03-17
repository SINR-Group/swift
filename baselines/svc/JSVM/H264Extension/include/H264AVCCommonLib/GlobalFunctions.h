
#if !defined(AFX_GLOBALFUNCTIONS_H__85D028A1_590E_423B_9072_AB351037B1E8__INCLUDED_)
#define AFX_GLOBALFUNCTIONS_H__85D028A1_590E_423B_9072_AB351037B1E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define gMin(x,y) ((x)<(y)?(x):(y))
#define gMax(x,y) ((x)>(y)?(x):(y))

H264AVC_NAMESPACE_BEGIN

__inline
UInt gCeilLog2( const UInt uiVal )
{
  UInt uiTmp = uiVal-1;
  UInt uiRet = 0;
  AOT_DBG( uiTmp == 0);

  AOT_DBG( uiTmp == 0);

  while( uiTmp != 0 )
  {
    uiTmp >>= 1;
    uiRet++;
  }
  return uiRet;
}

__inline
Bool gBoolRandom()
{
  return 0 == (rand()&1);
}

__inline
const Int gClip( const Int iX )
{
  const Int i2 = (iX & 0xFF);
  if( i2 == iX )  { return iX; }
  if( iX < 0 )    { return 0x00; }
  else            { return 0xFF; }
}


__inline
const Int gClipMinMax( const Int iX, const Int iMin, const Int iMax )
{
  return gMax( gMin( iX, iMax ), iMin );
}

//TMM_WP
__inline
Int gIntRandom(const Int iMin, const Int iMax)
{
    Double fRange = (Double)(iMax - iMin + 1);
    Int iValue = (Int)(fRange*(Double)rand()/(Double)(RAND_MAX+1.0));

    AOT_DBG( (iValue + iMin)> iMax );
    return iValue + iMin;
}
//TMM_WP

// JVT-V068 HRD {
__inline
Int gGetNumberOfLSBZeros( UInt ui )
{
  ROTRS( ui == 0, 0);
  for( Int i = 0; ; i++)
  {
    ROTRS( (ui&1) == 1, i);
    ui >>= 1;
  }
}
// JVT-V068 HRD }

H264AVC_NAMESPACE_END


#endif // !defined(AFX_GLOBALFUNCTIONS_H__85D028A1_590E_423B_9072_AB351037B1E8__INCLUDED_)
