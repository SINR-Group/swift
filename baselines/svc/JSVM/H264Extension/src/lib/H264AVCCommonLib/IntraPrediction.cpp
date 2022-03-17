
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/IntraPrediction.h"


H264AVC_NAMESPACE_BEGIN


const UChar g_aaucAvailableBlkMask[16][8] =
{
  {0x0,0x0,0x0,0x0,  0x0,0x8,0x0,0x08  }, // L, A, AL, AR
  {0x1,0x0,0x0,0x0,  0x5,0x8,0x0,0x08  }, //    A, AL, AR
  {0xA,0xE,0xE,0x6,  0x0,0x8,0x0,0x08  }, // L,    AL, AR
  {0xB,0xE,0xE,0x6,  0x5,0x8,0x0,0x08  }, //       AL, AR

  {0x4,0x0,0x0,0x0,  0x0,0x8,0x0,0x08  }, // L, A,     AR
  {0x5,0x0,0x0,0x0,  0x5,0x8,0x0,0x08  }, //    A,     AR
  {0xE,0xE,0xE,0x6,  0x0,0x8,0x0,0x08  }, // L,        AR
  {0xF,0xE,0xE,0x6,  0x5,0x8,0x0,0x08  }, //           AR

  {0x0,0x0,0x0,0x8,  0x0,0x8,0x0,0x08  }, // L, A, AL
  {0x1,0x0,0x0,0x8,  0x5,0x8,0x0,0x08  }, //    A, AL
  {0xA,0xE,0xE,0xE,  0x0,0x8,0x0,0x08  }, // L,    AL
  {0xB,0xE,0xE,0xE,  0x5,0x8,0x0,0x08  }, //       AL

  {0x4,0x0,0x0,0x8,  0x0,0x8,0x0,0x08  }, // L, A,
  {0x5,0x0,0x0,0x8,  0x5,0x8,0x0,0x08  }, //    A,
  {0xE,0xE,0xE,0xE,  0x0,0x8,0x0,0x08  }, // L,
  {0xF,0xE,0xE,0xE,  0x5,0x8,0x0,0x08  }  //
};


IntraPrediction::IntraPrediction()
{
  m_uiAvailableMaskMb=0;
  m_uiAvailable=0;
  m_bSpecial=false;
  ::memset(m_ac8x8Pred,0,26*sizeof(XPel));

  m_pucHor = m_ac8x8Pred+1;
  m_pucVer = m_ac8x8Pred+18;
}

IntraPrediction::~IntraPrediction()
{
}


ErrVal IntraPrediction::create( IntraPrediction*& rpcIntraPrediction )
{
  rpcIntraPrediction = new IntraPrediction;

  ROT( NULL == rpcIntraPrediction );

  return Err::m_nOK;
}


ErrVal IntraPrediction::destroy()
{
  delete this;

  return Err::m_nOK;
}


ErrVal IntraPrediction::init()
{
  return Err::m_nOK;
}


ErrVal IntraPrediction::uninit()
{
  return Err::m_nOK;
}


Void IntraPrediction::setAvailableMaskMb( UInt uiAvailableMaskMb )
{
  AOF_DBG( uiAvailableMaskMb < 256 );
  m_uiAvailableMaskMb = uiAvailableMaskMb;
}



ErrVal IntraPrediction::predictLumaMb( Pel* puc, Int iStride, UInt uiPredMode )
{
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  switch( uiPredMode & 0xf )
  {
  case 0:
    {
      xPred16x16IMode0Vert  ( puc, iStride );
      break;
    }
  case 1:
    {
      xPred16x16IMode1Hori  ( puc, iStride );
      break;
    }
  case 2:
    {
      xPred16x16IMode2DC    ( puc, iStride );
      break;
    }
  case 3:
    {
      xPred16x16IMode3Plane ( puc, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }
  return Err::m_nOK;
}


ErrVal IntraPrediction::predictChromaBlock( Pel* pucCb, Pel* pucCr, Int iStride, UInt uiPredMode )
{
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  switch( uiPredMode )
  {
  case 0:
    {
      xPred8x8IMode0DC    ( pucCb, iStride );
      xPred8x8IMode0DC    ( pucCr, iStride );
      break;
    }
  case 1:
    {
      xPred8x8IMode1Hori  ( pucCb, iStride );
      xPred8x8IMode1Hori  ( pucCr, iStride );
      break;
    }
  case 2:
    {
      xPred8x8IMode2Vert  ( pucCb, iStride );
      xPred8x8IMode2Vert  ( pucCr, iStride );
      break;
    }
  case 3:
    {
      xPred8x8IMode3Plane ( pucCb, iStride );
      xPred8x8IMode3Plane ( pucCr, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}

ErrVal IntraPrediction::predictLumaBlock( Pel* puc, Int iStride, UInt uiPredMode, LumaIdx cIdx )
{
  m_uiAvailable = xGetAvailableMask( cIdx );

  switch( uiPredMode )
  {
  case 0:
    {
      xPredMode0Vert( puc, iStride );
      break;
    }
  case 1:
    {
      xPredMode1Horiz( puc, iStride );
      break;
    }
  case 2:
    {
      xPredMode2Dc( puc, iStride );
      break;
    }
  case 3:
    {
      xPredMode3DiagDownLeft( puc, iStride );
      break;
    }
  case 4:
    {
      xPredMode4DiagDownRight( puc, iStride );
      break;
    }
  case 5:
    {
      xPredMode5VertRight( puc, iStride );
      break;
    }
  case 6:
    {
      xPredMode6HorizDown( puc, iStride );
      break;
    }
  case 7:
    {
      xPredMode7VertLeft( puc, iStride );
      break;
    }
  case 8:
    {
      xPredMode8HorizUp( puc, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}


__inline Void IntraPrediction::xLoadPredictorsABCD( Pel* puc, Int iStride, UInt& A, UInt& B, UInt& C, UInt& D )
{
  A = puc[-iStride];
  B = puc[1-iStride];
  C = puc[2-iStride];
  D = puc[3-iStride];
}

__inline Void IntraPrediction::xLoadPredictorsEFGH( Pel* puc, Int iStride, UInt& E, UInt& F, UInt& G, UInt& H )
{
  if( ! xIsAboveRightRef() )
  {
    E = F = G = H = puc[3-iStride];
  }
  else
  {
    E = puc[4-iStride];
    F = puc[5-iStride];
    G = puc[6-iStride];
    H = puc[7-iStride];
  }
}
__inline Void IntraPrediction::xLoadPredictorsIJKL( Pel* puc, Int iStride, UInt& I, UInt& J, UInt& K, UInt& L )
{
  I = puc[-1];
  J = puc[iStride-1];
  K = puc[2*iStride-1];
  L = puc[3*iStride-1];
}

__inline Void IntraPrediction::xLoadPredictorsX   ( Pel* puc, Int iStride, UInt& X )
{
  X = puc[-1-iStride];
}



__inline Void IntraPrediction::xLoadPredictorsABCD( XPel* puc, Int iStride, UInt& A, UInt& B, UInt& C, UInt& D )
{
  A = puc[-iStride];
  B = puc[1-iStride];
  C = puc[2-iStride];
  D = puc[3-iStride];
}

__inline Void IntraPrediction::xLoadPredictorsEFGH( XPel* puc, Int iStride, UInt& E, UInt& F, UInt& G, UInt& H )
{
  if( ! xIsAboveRightRef() )
  {
    E = F = G = H = puc[3-iStride];
  }
  else
  {
    E = puc[4-iStride];
    F = puc[5-iStride];
    G = puc[6-iStride];
    H = puc[7-iStride];
  }
}
__inline Void IntraPrediction::xLoadPredictorsIJKL( XPel* puc, Int iStride, UInt& I, UInt& J, UInt& K, UInt& L )
{
  I = puc[-1];
  J = puc[iStride-1];
  K = puc[2*iStride-1];
  L = puc[3*iStride-1];
}

__inline Void IntraPrediction::xLoadPredictorsX   ( XPel* puc, Int iStride, UInt& X )
{
  X = puc[-1-iStride];
}




UInt IntraPrediction::xGetAvailableMask( LumaIdx cIdx )
{
  if(cIdx >= 8)
  {
    return g_aaucAvailableBlkMask[m_uiAvailableMaskMb >> 4][cIdx & 0x7];
  }

  return g_aaucAvailableBlkMask[m_uiAvailableMaskMb & 0xF][cIdx];
}

Void IntraPrediction::xPredMode0Vert( Pel* puc, Int iStride ) // Vertical prediction
{
  //If A,B,C,D are inside the picture,
  AOF( xIsAboveRef() );

  ::memcpy( puc, puc - iStride, 4 );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4 );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4 );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4 );
}


Void IntraPrediction::xPredMode1Horiz( Pel* puc, Int iStride ) // Horizontal prediction
{
  // If E,F,G,H are inside the picture
  AOF( xIsLeftRef() );

  ::memset( puc, puc[-1], 4 );
  puc += iStride;

  ::memset( puc, puc[-1], 4 );
  puc += iStride;

  ::memset( puc, puc[-1], 4 );
  puc += iStride;

  ::memset( puc, puc[-1], 4 );
}


Void IntraPrediction::xPredMode2Dc( Pel* puc, Int iStride ) // DC Prediction
{
  UInt uiDcValue;

  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      uiDcValue = 0x80;
    }
    else
    {
//      uiDcValue = (A + B + C + D + 2) >> 2;
      uiDcValue   = puc[-iStride];
      uiDcValue  += puc[1-iStride];
      uiDcValue  += puc[2-iStride];
      uiDcValue  += puc[3-iStride];
      uiDcValue  += 2;
      uiDcValue >>= 2;
    }
  }
  else
  {
    if( ! xIsAboveRef() )
    {
//      uiDcValue = (I + J + K + L + 2) >> 2;
      uiDcValue   = puc[-1];
      uiDcValue  += puc[iStride-1];
      uiDcValue  += puc[2*iStride-1];
      uiDcValue  += puc[3*iStride-1];
      uiDcValue  += 2;
      uiDcValue >>= 2;
    }
    else
    {
//      uiDcValue = ( A + B + C + D + I + J + K + L + 4) >> 3;
      uiDcValue   = puc[-iStride];
      uiDcValue  += puc[1-iStride];
      uiDcValue  += puc[2-iStride];
      uiDcValue  += puc[3-iStride];
      uiDcValue  += puc[-1];
      uiDcValue  += puc[iStride-1];
      uiDcValue  += puc[2*iStride-1];
      uiDcValue  += puc[3*iStride-1];
      uiDcValue  += 4;
      uiDcValue >>= 3;
    }
  }

  ::memset( puc, uiDcValue, 4 );
  puc += iStride;

  ::memset( puc, uiDcValue, 4 );
  puc += iStride;

  ::memset( puc, uiDcValue, 4 );
  puc += iStride;

  ::memset( puc, uiDcValue, 4 );
}



Void IntraPrediction::xPredMode3DiagDownLeft( Pel* puc, Int iStride ) // Diagonal Down Left Pred
{
  AOF( xIsAboveRef() );
  UInt A,B,C,D;
  UInt E,F,G,H;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsEFGH ( puc, iStride, E, F, G, H );

  puc[0]                                                         = (A + 2*B + C + 2) >> 2;
  puc[1] = puc[iStride+0]                                        = (B + 2*C + D + 2) >> 2;
  puc[2] = puc[iStride+1] = puc[2*iStride+0]                     = (C + 2*D + E + 2) >> 2;
  puc[3] = puc[iStride+2] = puc[2*iStride+1] = puc[3*iStride+0]  = (D + 2*E + F + 2) >> 2;
           puc[iStride+3] = puc[2*iStride+2] = puc[3*iStride+1]  = (E + 2*F + G + 2) >> 2;
                            puc[2*iStride+3] = puc[3*iStride+2]  = (F + 2*G + H + 2) >> 2;
                                               puc[3*iStride+3]  = (G + 2*H + H + 2) >> 2;
}

Void IntraPrediction::xPredMode4DiagDownRight( Pel* puc, Int iStride ) // Diagonal Down Right Pred
{
  AOF( xIsAllLeftAboveRef() )
  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );

  puc[3]                                                        = (B + 2 * C + D + 2)/4;
  puc[2] = puc[iStride+3]                                       = (A + 2 * B + C + 2)/4;
  puc[1] = puc[iStride+2] = puc[2*iStride+3]                    = (X + 2 * A + B + 2)/4;
  puc[0] = puc[iStride+1] = puc[2*iStride+2] = puc[3*iStride+3] = (A + 2 * X + I + 2)/4;
           puc[iStride+0] = puc[2*iStride+1] = puc[3*iStride+2] = (X + 2 * I + J + 2)/4;
                            puc[2*iStride+0] = puc[3*iStride+1] = (I + 2 * J + K + 2)/4;
                                               puc[3*iStride+0] = (J + 2 * K + L + 2)/4;
}


Void IntraPrediction::xPredMode5VertRight( Pel* puc, Int iStride )  // Vertical Right Pred
{
  AOF( xIsAllLeftAboveRef() )

  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );


  puc[0]         = puc[2*iStride+1] = (X + A + 1) >> 1;
  puc[1]         = puc[2*iStride+2] = (A + B + 1) >> 1;
  puc[2]         = puc[2*iStride+3] = (B + C + 1) >> 1;
  puc[3]                            = (C + D + 1) >> 1;

  puc[iStride+0] = puc[3*iStride+1] = (I + 2*X + A + 2) >> 2;
  puc[iStride+1] = puc[3*iStride+2] = (X + 2*A + B + 2) >> 2;
  puc[iStride+2] = puc[3*iStride+3] = (A + 2*B + C + 2) >> 2;
  puc[iStride+3]                    = (B + 2*C + D + 2) >> 2;

  puc[2*iStride] = (X + 2*I + J + 2) >> 2;
  puc[3*iStride] = (I + 2*J + K + 2) >> 2;
}


Void IntraPrediction::xPredMode6HorizDown( Pel* puc, Int iStride ) // Horizontal DownPred
{
  AOF( xIsAllLeftAboveRef() )

  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );

  puc[0] = puc[iStride+2] = (X + I + 1) >> 1;
  puc[1] = puc[iStride+3] = (I + 2*X + A + 2) >> 2;
  puc[2] =                  (X + 2*A + B + 2) >> 2;
  puc[3] =                  (A + 2*B + C + 2) >> 2;

  puc[iStride+0]   = puc[2*iStride+2] = (I + J + 1) >> 1;
  puc[iStride+1]   = puc[2*iStride+3] = (X + 2*I + J + 2) >> 2;
  puc[2*iStride+0] = puc[3*iStride+2] = (J + K + 1) >> 1;
  puc[2*iStride+1] = puc[3*iStride+3] = (I + 2*J + K + 2) >> 2;

  puc[3*iStride+0] = (K + L + 1) >> 1;
  puc[3*iStride+1] = (J + 2*K + L + 2) >> 2;

}



Void IntraPrediction::xPredMode7VertLeft( Pel* puc, Int iStride ) // Vertical Left Pred
{
  AOF( xIsAboveRef() ); //!!!

  UInt A,B,C,D;
  UInt E,F,G,H;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsEFGH ( puc, iStride, E, F, G, H );

  puc[0] = (A + B + 1) >> 1;

  puc[1] = puc[2*iStride+0] = (B + C + 1) >> 1;
  puc[2] = puc[2*iStride+1] = (C + D + 1) >> 1;
  puc[3] = puc[2*iStride+2] = (D + E + 1) >> 1;
           puc[2*iStride+3] = (E + F + 1) >> 1;

  puc[iStride+0]                    = (A + 2*B + C + 2) >> 2;
  puc[iStride+1] = puc[3*iStride+0] = (B + 2*C + D + 2) >> 2;
  puc[iStride+2] = puc[3*iStride+1] = (C + 2*D + E + 2) >> 2;
  puc[iStride+3] = puc[3*iStride+2] = (D + 2*E + F + 2) >> 2;
                   puc[3*iStride+3] = (E + 2*F + G + 2) >> 2;
}


Void IntraPrediction::xPredMode8HorizUp( Pel* puc, Int iStride ) // Horizontal Up Pred
{
  AOF( xIsLeftRef() ); //!!!

  UInt I,J,K,L;

  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );

  puc[0] = (I + J + 1) >> 1;
  puc[1] = (I + 2*J + K + 2) >> 2;
  puc[2] = puc[iStride + 0] = (J + K + 1) >> 1;
  puc[3] = puc[iStride + 1] = (J + 2*K + L + 2) >> 2;

  puc[iStride + 2] = puc[2*iStride + 0] = (K + L + 1) >> 1;
  puc[iStride + 3] = puc[2*iStride + 1] = (K + 2*L + L + 2) >> 2;

  puc[2*iStride + 2] = puc[3*iStride + 0] =
  puc[2*iStride + 3] = puc[3*iStride + 1] =
  puc[3*iStride + 2] = puc[3*iStride + 3] = L;
}



Void IntraPrediction::xPred16x16IMode0Vert( Pel* puc, Int iStride ) // vertical
{
  AOF( xIsAboveRef() );

  for( Int n = 0; n < 16; n++ )
  {
    ::memcpy( puc, puc - iStride, 16 );
    puc += iStride;
  }
}



Void IntraPrediction::xPred16x16IMode1Hori( Pel* puc, Int iStride ) // horizontal
{
  AOF( xIsLeftRef() );

  for( Int n = 0; n < 16; n++ )
  {
   ::memset( puc, puc[-1], 16 );
    puc += iStride;
  }
}


Void IntraPrediction::xPred16x16IMode2DC( Pel* puc, Int iStride ) // DC prediction
{
  Int n, m;
  UInt uiDcValue = 0;

  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      uiDcValue = 0x80;
    }
    else
    {
      for( n = 0; n < 16; n++ )
      {
        uiDcValue += puc[n - iStride];
      }
      uiDcValue += 8;
      uiDcValue >>= 4;
    }
  }
  else
  {
    for( m = -1, n = 0; n < 16; n++, m+= iStride )
    {
      uiDcValue += puc[m];
    }

    if( ! xIsAboveRef() )
    {
      uiDcValue += 8;
      uiDcValue >>= 4;
    }
    else
    {
      for( n = 0; n < 16; n++ )
      {
        uiDcValue += puc[n - iStride];
      }
      uiDcValue += 16;
      uiDcValue >>= 5;
    }
  }


  for( n = 0; n < 16; n++ )
  {
   ::memset( puc, uiDcValue, 16 );
    puc += iStride;
  }
}


Void IntraPrediction::xPred16x16IMode3Plane( Pel* puc, Int iStride ) // plane prediction
{
  Int n, m;
  Int iH = 0;
  Int iV = 0;

  AOF( xIsAllLeftAboveRef() );

  Pel* pucDes = puc;

  puc += 7 - iStride;
  for( n = 1; n < 9; n++ )
  {
    iH += n * (Int)(puc[n] - puc[-n]);
  }

  puc += (iStride << 3) - 8;
  for( m = iStride, n = 1; n < 9; n++, m += iStride)
  {
    iV += n * (Int)(puc[m] - puc[-m]);
  }
  puc -= 7 * iStride - 1;


  Int iB =  (5 * iH + 32) >> 6;
  Int iC =  (5 * iV + 32) >> 6;
  Int iA = 16 * (Int)(puc[(iStride << 4) - (iStride + 1)] + puc[ 15 - iStride ]);
  Int x, y;

  for( y = 0; y < 16; y++ )
  {
    Int iYSum = iA + (y-7) * iC + 16;
    for( x = 0; x < 16; x++ )
    {
      pucDes[x] = gClip((iYSum + (x-7) * iB) >> 5);
    }
    pucDes += iStride;
  }
}





Void IntraPrediction::xPred8x8IMode2Vert( Pel* puc, Int iStride ) // vertical
{
  AOF( xIsAboveRef() );

  for( Int n = 0; n < 8; n++ )
  {
    ::memcpy( puc, puc - iStride, 8 );
    puc += iStride;
  }
}


Void IntraPrediction::xPred8x8IMode1Hori( Pel* puc, Int iStride ) // horizontal
{
  AOF( xIsLeftRef() );

  for( Int n = 0; n < 8; n++ )
  {
   ::memset( puc, puc[-1], 8 );
    puc += iStride;
  }
}


Void IntraPrediction::xPred8x8IMode0DC( Pel* puc, Int iStride ) // DC prediction
{
  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      xChroma0PredAllOutside( puc, iStride );
    }
    else
    {
      xChroma0PredNoLeftRef( puc, iStride );
    }
  }
  else
  {
    if( ! xIsAboveRef() )
    {
      xChroma0PredNoAboveRef( puc, iStride );
    }
    else
    {
      xChroma0PredAllInside( puc, iStride );
    }
  }
}


Void IntraPrediction::xPred8x8IMode3Plane( Pel* puc, Int iStride ) // plane prediction
{
  Int n, m;
  Int iH = 0;
  Int iV = 0;

  AOF( xIsAllLeftAboveRef() );

  Pel* pucDes = puc;

  puc += 3 - iStride;
  for( n = 1; n < 5; n++ )
  {
    iH += n * (Int)(puc[n] - puc[-n]);
  }

  puc += (iStride << 2) - 4;
  for( m = iStride, n = 1; n < 5; n++, m += iStride)
  {
    iV += n * (Int)(puc[m] - puc[-m]);
  }
  puc -= 3 * iStride - 1;

  Int iB =  (17 * iH + 16) >> 5;
  Int iC =  (17 * iV + 16) >> 5;
  Int iA = 16 * (Int)(puc[(iStride << 3) - (iStride + 1)] + puc[ 7 - iStride ]);
  Int x, y;

  for( y = 0; y < 8; y++ )
  {
    Int iYSum = iA + (y-3) * iC + 16;
    for( x = 0; x < 8; x++ )
    {
      pucDes[x] = gClip((iYSum + (x-3) * iB) >> 5);
    }
    pucDes += iStride;
  }
}









Void IntraPrediction::xPred16x16IMode0Vert( XPel* puc, Int iStride ) // vertical
{
  AOF( xIsAboveRef() );

  for( Int n = 0; n < 16; n++ )
  {
    ::memcpy( puc, puc - iStride, 16*sizeof(XPel) );
    puc += iStride;
  }
}

Void IntraPrediction::xPred16x16IMode1Hori( XPel* puc, Int iStride ) // horizontal
{
  AOF( xIsLeftRef() );

  for( Int n = 0; n < 16; n++ )
  {
    for( Int m = 0; m < 16; m++ )
    {
      puc[m] = puc[m-1];
    }
    puc += iStride;
  }
}


Void IntraPrediction::xPred16x16IMode2DC( XPel* puc, Int iStride ) // DC prediction
{
  Int n, m;
  UInt uiDcValue = 0;

  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      uiDcValue = 0x80;
    }
    else
    {
      for( n = 0; n < 16; n++ )
      {
        uiDcValue += puc[n - iStride];
      }
      uiDcValue += 8;
      uiDcValue >>= 4;
    }
  }
  else
  {
    for( m = -1, n = 0; n < 16; n++, m+= iStride )
    {
      uiDcValue += puc[m];
    }

    if( ! xIsAboveRef() )
    {
      uiDcValue += 8;
      uiDcValue >>= 4;
    }
    else
    {
      for( n = 0; n < 16; n++ )
      {
        uiDcValue += puc[n - iStride];
      }
      uiDcValue += 16;
      uiDcValue >>= 5;
    }
  }


  for( n = 0; n < 16; n++ )
  {
    for( m = 0; m < 16; m++ )
    {
      puc[m] = uiDcValue;
    }
    puc += iStride;
  }
}


Void IntraPrediction::xPred16x16IMode3Plane( XPel* puc, Int iStride ) // plane prediction
{
  Int n, m;
  Int iH = 0;
  Int iV = 0;

  AOF( xIsAllLeftAboveRef() );

  XPel* pucDes = puc;

  puc += 7 - iStride;
  for( n = 1; n < 9; n++ )
  {
    iH += n * (Int)(puc[n] - puc[-n]);
  }

  puc += (iStride << 3) - 8;
  for( m = iStride, n = 1; n < 9; n++, m += iStride)
  {
    iV += n * (Int)(puc[m] - puc[-m]);
  }
  puc -= 7 * iStride - 1;


  Int iB =  (5 * iH + 32) >> 6;
  Int iC =  (5 * iV + 32) >> 6;
  Int iA = 16 * (Int)(puc[(iStride << 4) - (iStride + 1)] + puc[ 15 - iStride ]);
  Int x, y;

  for( y = 0; y < 16; y++ )
  {
    Int iYSum = iA + (y-7) * iC + 16;
    for( x = 0; x < 16; x++ )
    {
      pucDes[x] = gClip((iYSum + (x-7) * iB) >> 5);
    }
    pucDes += iStride;
  }
}





Void IntraPrediction::xPredMode0Vert( XPel* puc, Int iStride ) // Vertical prediction
{
  //If A,B,C,D are inside the picture,
  AOF( xIsAboveRef() );

  ::memcpy( puc, puc - iStride, 4*sizeof(XPel) );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4*sizeof(XPel) );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4*sizeof(XPel) );
  puc += iStride;

  ::memcpy( puc, puc - iStride, 4*sizeof(XPel) );
}


Void IntraPrediction::xPredMode1Horiz( XPel* puc, Int iStride ) // Horizontal prediction
{
  // If E,F,G,H are inside the picture
  AOF( xIsLeftRef() );

  Int n;

  for( n=0; n<4; n++ )  puc[n] = puc[n-1];
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = puc[n-1];
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = puc[n-1];
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = puc[n-1];
}


Void IntraPrediction::xPredMode2Dc( XPel* puc, Int iStride ) // DC Prediction
{
  UInt uiDcValue;

  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      uiDcValue = 0x80;
    }
    else
    {
//      uiDcValue = (A + B + C + D + 2) >> 2;
      uiDcValue   = puc[-iStride];
      uiDcValue  += puc[1-iStride];
      uiDcValue  += puc[2-iStride];
      uiDcValue  += puc[3-iStride];
      uiDcValue  += 2;
      uiDcValue >>= 2;
    }
  }
  else
  {
    if( ! xIsAboveRef() )
    {
//      uiDcValue = (I + J + K + L + 2) >> 2;
      uiDcValue   = puc[-1];
      uiDcValue  += puc[iStride-1];
      uiDcValue  += puc[2*iStride-1];
      uiDcValue  += puc[3*iStride-1];
      uiDcValue  += 2;
      uiDcValue >>= 2;
    }
    else
    {
//      uiDcValue = ( A + B + C + D + I + J + K + L + 4) >> 3;
      uiDcValue   = puc[-iStride];
      uiDcValue  += puc[1-iStride];
      uiDcValue  += puc[2-iStride];
      uiDcValue  += puc[3-iStride];
      uiDcValue  += puc[-1];
      uiDcValue  += puc[iStride-1];
      uiDcValue  += puc[2*iStride-1];
      uiDcValue  += puc[3*iStride-1];
      uiDcValue  += 4;
      uiDcValue >>= 3;
    }
  }

  Int n;

  for( n=0; n<4; n++ )  puc[n] = uiDcValue;
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = uiDcValue;
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = uiDcValue;
  puc += iStride;

  for( n=0; n<4; n++ )  puc[n] = uiDcValue;
}



Void IntraPrediction::xPredMode3DiagDownLeft( XPel* puc, Int iStride ) // Diagonal Down Left Pred
{
  AOF( xIsAboveRef() );
  UInt A,B,C,D;
  UInt E,F,G,H;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsEFGH ( puc, iStride, E, F, G, H );

  puc[0]                                                         = (A + 2*B + C + 2) >> 2;
  puc[1] = puc[iStride+0]                                        = (B + 2*C + D + 2) >> 2;
  puc[2] = puc[iStride+1] = puc[2*iStride+0]                     = (C + 2*D + E + 2) >> 2;
  puc[3] = puc[iStride+2] = puc[2*iStride+1] = puc[3*iStride+0]  = (D + 2*E + F + 2) >> 2;
           puc[iStride+3] = puc[2*iStride+2] = puc[3*iStride+1]  = (E + 2*F + G + 2) >> 2;
                            puc[2*iStride+3] = puc[3*iStride+2]  = (F + 2*G + H + 2) >> 2;
                                               puc[3*iStride+3]  = (G + 2*H + H + 2) >> 2;
}


Void IntraPrediction::xPredMode4DiagDownRight( XPel* puc, Int iStride ) // Diagonal Down Right Pred
{
  AOF( xIsAllLeftAboveRef() )
  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );

  puc[3]                                                        = (B + 2 * C + D + 2)/4;
  puc[2] = puc[iStride+3]                                       = (A + 2 * B + C + 2)/4;
  puc[1] = puc[iStride+2] = puc[2*iStride+3]                    = (X + 2 * A + B + 2)/4;
  puc[0] = puc[iStride+1] = puc[2*iStride+2] = puc[3*iStride+3] = (A + 2 * X + I + 2)/4;
           puc[iStride+0] = puc[2*iStride+1] = puc[3*iStride+2] = (X + 2 * I + J + 2)/4;
                            puc[2*iStride+0] = puc[3*iStride+1] = (I + 2 * J + K + 2)/4;
                                               puc[3*iStride+0] = (J + 2 * K + L + 2)/4;
}


Void IntraPrediction::xPredMode5VertRight( XPel* puc, Int iStride )  // Vertical Right Pred
{
  AOF( xIsAllLeftAboveRef() )

  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );


  puc[0]         = puc[2*iStride+1] = (X + A + 1) >> 1;
  puc[1]         = puc[2*iStride+2] = (A + B + 1) >> 1;
  puc[2]         = puc[2*iStride+3] = (B + C + 1) >> 1;
  puc[3]                            = (C + D + 1) >> 1;

  puc[iStride+0] = puc[3*iStride+1] = (I + 2*X + A + 2) >> 2;
  puc[iStride+1] = puc[3*iStride+2] = (X + 2*A + B + 2) >> 2;
  puc[iStride+2] = puc[3*iStride+3] = (A + 2*B + C + 2) >> 2;
  puc[iStride+3]                    = (B + 2*C + D + 2) >> 2;

  puc[2*iStride] = (X + 2*I + J + 2) >> 2;
  puc[3*iStride] = (I + 2*J + K + 2) >> 2;
}



Void IntraPrediction::xPredMode6HorizDown( XPel* puc, Int iStride ) // Horizontal DownPred
{
  AOF( xIsAllLeftAboveRef() )

  UInt A,B,C,D;
  UInt I,J,K,L;
  UInt X;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );
  xLoadPredictorsX    ( puc, iStride, X );

  puc[0] = puc[iStride+2] = (X + I + 1) >> 1;
  puc[1] = puc[iStride+3] = (I + 2*X + A + 2) >> 2;
  puc[2] =                  (X + 2*A + B + 2) >> 2;
  puc[3] =                  (A + 2*B + C + 2) >> 2;

  puc[iStride+0]   = puc[2*iStride+2] = (I + J + 1) >> 1;
  puc[iStride+1]   = puc[2*iStride+3] = (X + 2*I + J + 2) >> 2;
  puc[2*iStride+0] = puc[3*iStride+2] = (J + K + 1) >> 1;
  puc[2*iStride+1] = puc[3*iStride+3] = (I + 2*J + K + 2) >> 2;

  puc[3*iStride+0] = (K + L + 1) >> 1;
  puc[3*iStride+1] = (J + 2*K + L + 2) >> 2;

}


Void IntraPrediction::xPredMode7VertLeft( XPel* puc, Int iStride ) // Vertical Left Pred
{
  AOF( xIsAboveRef() ); //!!!

  UInt A,B,C,D;
  UInt E,F,G,H;

  xLoadPredictorsABCD ( puc, iStride, A, B, C, D );
  xLoadPredictorsEFGH ( puc, iStride, E, F, G, H );

  puc[0] = (A + B + 1) >> 1;

  puc[1] = puc[2*iStride+0] = (B + C + 1) >> 1;
  puc[2] = puc[2*iStride+1] = (C + D + 1) >> 1;
  puc[3] = puc[2*iStride+2] = (D + E + 1) >> 1;
           puc[2*iStride+3] = (E + F + 1) >> 1;

  puc[iStride+0]                    = (A + 2*B + C + 2) >> 2;
  puc[iStride+1] = puc[3*iStride+0] = (B + 2*C + D + 2) >> 2;
  puc[iStride+2] = puc[3*iStride+1] = (C + 2*D + E + 2) >> 2;
  puc[iStride+3] = puc[3*iStride+2] = (D + 2*E + F + 2) >> 2;
                   puc[3*iStride+3] = (E + 2*F + G + 2) >> 2;
}


Void IntraPrediction::xPredMode8HorizUp( XPel* puc, Int iStride ) // Horizontal Up Pred
{
  AOF( xIsLeftRef() ); //!!!

  UInt I,J,K,L;

  xLoadPredictorsIJKL ( puc, iStride, I, J, K, L );

  puc[0] = (I + J + 1) >> 1;
  puc[1] = (I + 2*J + K + 2) >> 2;
  puc[2] = puc[iStride + 0] = (J + K + 1) >> 1;
  puc[3] = puc[iStride + 1] = (J + 2*K + L + 2) >> 2;

  puc[iStride + 2] = puc[2*iStride + 0] = (K + L + 1) >> 1;
  puc[iStride + 3] = puc[2*iStride + 1] = (K + 2*L + L + 2) >> 2;

  puc[2*iStride + 2] = puc[3*iStride + 0] =
  puc[2*iStride + 3] = puc[3*iStride + 1] =
  puc[3*iStride + 2] = puc[3*iStride + 3] = L;
}


Void IntraPrediction::xPred8x8IMode2Vert( XPel* puc, Int iStride ) // vertical
{
  AOF( xIsAboveRef() );

  for( Int n = 0; n < 8; n++ )
  {
    ::memcpy( puc, puc - iStride, 8*sizeof(XPel) );
    puc += iStride;
  }
}


Void IntraPrediction::xPred8x8IMode1Hori( XPel* puc, Int iStride ) // horizontal
{
  AOF( xIsLeftRef() );

  for( Int n = 0; n < 8; n++ )
  {
    for( Int m = 0; m < 8; m++ )
    {
      puc[m] = puc[m-1];
    }
    puc += iStride;
  }
}


Void IntraPrediction::xPred8x8IMode0DC( XPel* puc, Int iStride ) // DC prediction
{
  UInt uiA, uiB, uiC, uiD;

  if( ! xIsAboveRef() )
  {
    if( m_uiAvailableMaskMb & 0x1 )  // top
    {
      uiA = uiB = 0x80;
    }
    else
  {
      uiA = uiB = (xGetS2( puc, iStride ) + 2) / 4;
    }

    if( m_uiAvailableMaskMb & 0x10 ) // bot
    {
      uiC = uiD = 0x80;
    }
    else
    {
      uiC = uiD = (xGetS3( puc, iStride ) + 2) / 4;
    }
    }
  else
  {
    UInt uiS0 = xGetS0( puc, iStride );
    UInt uiS1 = xGetS1( puc, iStride );

    if( m_uiAvailableMaskMb & 0x1 )  // top
    {
      uiA = (uiS0 + 2)/4;
      uiB = (uiS1 + 2)/4;
  }
  else
  {
      UInt uiS2 = xGetS2( puc, iStride );
      uiA = (uiS0 + uiS2 + 4)/8;
      uiB = (uiS1 + 2)/4;
    }

    if( m_uiAvailableMaskMb & 0x10 ) // bot
    {
      uiC = (uiS0 + 2)/4;
      uiD = (uiS1 + 2)/4;
    }
    else
    {
      UInt uiS3 = xGetS3( puc, iStride );
      uiC = (uiS3 + 2)/4;
      uiD = (uiS1 + uiS3 + 4)/8;
    }
  }

  Int pos;
  XPel* pucDes = puc;
  for( pos = 0; pos < 4; pos++)
  {
    pucDes[pos]      = uiA;
    pucDes[pos + 4]  = uiB;
  }


  for( Int n1 = 0; n1 < 3; n1 ++ )
  {
    memcpy( pucDes + iStride, pucDes, 8 * sizeof( XPel ) );
    pucDes += iStride;
  }

  pucDes += iStride;
  for( pos = 0; pos < 4; pos++)
  {
    pucDes[pos]     = uiC;
    pucDes[pos + 4] = uiD;
    }

  for( Int n2 = 0; n2 < 3; n2 ++ )
  {
    memcpy( pucDes + iStride, pucDes, 8 * sizeof( XPel ) );
    pucDes += iStride;
  }

}


Void IntraPrediction::xPred8x8IMode3Plane( XPel* puc, Int iStride ) // plane prediction
{
  Int n, m;
  Int iH = 0;
  Int iV = 0;

  AOF( xIsAllLeftAboveRef() );

  XPel* pucDes = puc;

  puc += 3 - iStride;
  for( n = 1; n < 5; n++ )
  {
    iH += n * (Int)(puc[n] - puc[-n]);
  }

  puc += (iStride << 2) - 4;
  for( m = iStride, n = 1; n < 5; n++, m += iStride)
  {
    iV += n * (Int)(puc[m] - puc[-m]);
  }
  puc -= 3 * iStride - 1;

  Int iB =  (17 * iH + 16) >> 5;
  Int iC =  (17 * iV + 16) >> 5;
  Int iA = 16 * (Int)(puc[(iStride << 3) - (iStride + 1)] + puc[ 7 - iStride ]);
  Int x, y;

  for( y = 0; y < 8; y++ )
  {
    Int iYSum = iA + (y-3) * iC + 16;
    for( x = 0; x < 8; x++ )
    {
      pucDes[x] = gClip((iYSum + (x-3) * iB) >> 5);
    }
    pucDes += iStride;
  }
}




ErrVal IntraPrediction::predictLumaBlock( XPel* puc, Int iStride, UInt uiPredMode, LumaIdx cIdx )
{
  m_uiAvailable = xGetAvailableMask( cIdx );

  switch( uiPredMode )
  {
  case 0:
    {
      xPredMode0Vert( puc, iStride );
      break;
    }
  case 1:
    {
      xPredMode1Horiz( puc, iStride );
      break;
    }
  case 2:
    {
      xPredMode2Dc( puc, iStride );
      break;
    }
  case 3:
    {
      xPredMode3DiagDownLeft( puc, iStride );
      break;
    }
  case 4:
    {
      xPredMode4DiagDownRight( puc, iStride );
      break;
    }
  case 5:
    {
      xPredMode5VertRight( puc, iStride );
      break;
    }
  case 6:
    {
      xPredMode6HorizDown( puc, iStride );
      break;
    }
  case 7:
    {
      xPredMode7VertLeft( puc, iStride );
      break;
    }
  case 8:
    {
      xPredMode8HorizUp( puc, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}




ErrVal IntraPrediction::predictLumaMb( XPel* puc, Int iStride, UInt uiPredMode )
{
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  switch( uiPredMode & 0xf )
  {
  case 0:
    {
      xPred16x16IMode0Vert  ( puc, iStride );
      break;
    }
  case 1:
    {
      xPred16x16IMode1Hori  ( puc, iStride );
      break;
    }
  case 2:
    {
      xPred16x16IMode2DC    ( puc, iStride );
      break;
    }
  case 3:
    {
      xPred16x16IMode3Plane ( puc, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }
  return Err::m_nOK;
}



ErrVal IntraPrediction::predictChromaBlock( XPel* pucCb, XPel* pucCr, Int iStride, UInt uiPredMode )
{
  m_uiAvailable = ( m_uiAvailableMaskMb >> 4 ) | m_uiAvailableMaskMb;

  switch( uiPredMode )
  {
  case 0:
    {
      xPred8x8IMode0DC    ( pucCb, iStride );
      xPred8x8IMode0DC    ( pucCr, iStride );
      break;
    }
  case 1:
    {
      xPred8x8IMode1Hori  ( pucCb, iStride );
      xPred8x8IMode1Hori  ( pucCr, iStride );
      break;
    }
  case 2:
    {
      xPred8x8IMode2Vert  ( pucCb, iStride );
      xPred8x8IMode2Vert  ( pucCr, iStride );
      break;
    }
  case 3:
    {
      xPred8x8IMode3Plane ( pucCb, iStride );
      xPred8x8IMode3Plane ( pucCr, iStride );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nERR;
    }
  }

  return Err::m_nOK;
}









Void IntraPrediction::xSet8x8AvailableMask( B8x8Idx cIdx )
{
  B4x4Idx c4x4Idx( cIdx );
  UInt uiM1 = xGetAvailableMask( c4x4Idx );
  c4x4Idx++;
  UInt uiM2 = xGetAvailableMask( c4x4Idx );

  m_uiAvailable  = 0;
  m_uiAvailable |= uiM2&8;
  m_uiAvailable |= uiM1&7;

  m_bSpecial = ( cIdx.b8x8Index() == 1 );
}


ErrVal IntraPrediction::predictLuma8x8Block( XPel* puc, Int iStride, UInt uiPredMode, B8x8Idx cIdx )
{
  xSet8x8AvailableMask( cIdx );

  switch( uiPredMode )
  {
  case 0:
    {
      xPredLum8x8Mode0Vert( puc, iStride );
      ROFRS( xIsAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 1:
    {
      xPredLum8x8Mode1Horiz( puc, iStride );
      ROFRS( xIsLeftRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 2:
    {
      xPredLum8x8Mode2Dc( puc, iStride );
      break;
    }
  case 3:
    {
      xPredLum8x8Mode3DiagDownLeft( puc, iStride );
      ROFRS( xIsAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 4:
    {
      xPredLum8x8Mode4DiagDownRight( puc, iStride );
      ROFRS( xIsAllLeftAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 5:
    {
      xPredLum8x8Mode5VertRight( puc, iStride );
      ROFRS( xIsAllLeftAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 6:
    {
      xPredLum8x8Mode6HorizDown( puc, iStride );
      ROFRS( xIsAllLeftAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 7:
    {
      xPredLum8x8Mode7VertLeft( puc, iStride );
      ROFRS( xIsAboveRef(), Err::m_nDataNotAvailable );
      break;
    }
  case 8:
    {
      xPredLum8x8Mode8HorizUp( puc, iStride );
      ROFRS( xIsLeftRef(), Err::m_nDataNotAvailable );
      break;
    }
  default:
    {
      assert( 0 );
      return Err::m_nInvalidParameter;
    }
  }

  return Err::m_nOK;
}



Void IntraPrediction::xLoadHorPred8x8( XPel* puc, Int iStride )
{
  if( xIsAboveLeftRef() )
  {
    m_pucHor[0] = (puc[-iStride-1] + 2*puc[-iStride] + puc[-iStride+1] + 2) >> 2;
  }
  else
  {
    m_pucHor[0] = (3*puc[-iStride] + puc[-iStride+1] + 2) >> 2;
  }

  for( Int x = 1; x < 7; x++ )
  {
    const Int iOffset = x-iStride;
    m_pucHor[x] = (puc[iOffset-1] + 2*puc[iOffset] + puc[iOffset+1] + 2) >> 2;
  }

  if( ! xIsAboveRightRef() )
  {
    m_pucHor[7] = (puc[6-iStride] + 3*puc[7-iStride] + 2) >> 2;

    for( Int x = 8; x < 16; x++ )
    {
      m_pucHor[x] = puc[7-iStride];
    }
  }
  else
  {
    m_pucHor[7] = (puc[6-iStride] + 2*puc[7-iStride] + puc[8-iStride] + 2) >> 2;

    AOT( ! xIsAboveRef() )

    if( ! m_bSpecial)
    {
      m_pucHor[ 8] = (puc[ 7-iStride] + 2*puc[ 8-iStride] + puc[ 9-iStride] + 2) >> 2;
      m_pucHor[ 9] = (puc[ 8-iStride] + 2*puc[ 9-iStride] + puc[10-iStride] + 2) >> 2;
      m_pucHor[10] = (puc[ 9-iStride] + 2*puc[10-iStride] + puc[11-iStride] + 2) >> 2;
      m_pucHor[11] = (puc[10-iStride] + 2*puc[11-iStride] + puc[12-iStride] + 2) >> 2;
      m_pucHor[12] = (puc[11-iStride] + 2*puc[12-iStride] + puc[13-iStride] + 2) >> 2;
      m_pucHor[13] = (puc[12-iStride] + 2*puc[13-iStride] + puc[14-iStride] + 2) >> 2;
      m_pucHor[14] = (puc[13-iStride] + 2*puc[14-iStride] + puc[15-iStride] + 2) >> 2;

      m_pucHor[15] = (puc[14-iStride] + 3*puc[15-iStride] + 2) >> 2;
    }
    else
    {
      m_pucHor[ 8] = (puc[ 7-iStride] + 2*puc[ 8-iStride] + puc[ 9-iStride] + 2) >> 2;
      m_pucHor[ 9] = (puc[ 8-iStride] + 2*puc[ 9-iStride] + puc[10-iStride] + 2) >> 2;
      m_pucHor[10] = (puc[ 9-iStride] + 2*puc[10-iStride] + puc[11-iStride] + 2) >> 2;
      m_pucHor[11] = (puc[10-iStride] + 2*puc[11-iStride] + puc[8         ] + 2) >> 2;
      m_pucHor[12] = (puc[11-iStride] + 2*puc[8         ] + puc[9         ] + 2) >> 2;
      m_pucHor[13] = (puc[8         ] + 2*puc[9         ] + puc[10        ] + 2) >> 2;
      m_pucHor[14] = (puc[9         ] + 2*puc[10        ] + puc[11        ] + 2) >> 2;

      m_pucHor[15] = (puc[10] + 3*puc[11] + 2) >> 2;
    }
  }
}


Void IntraPrediction::xLoadXPred8x8( XPel* puc, Int iStride )
{
  if( xIsAboveLeftRef() )
  {
    if( xIsAboveRef() )
    {
      if( xIsLeftRef() )
      {
        m_pucHor[-1] = m_pucVer[-1] = (puc[-1] + 2 * puc[-iStride-1] + puc[-iStride] + 2 )>>2;
      }
      else
      {
        m_pucHor[-1] = m_pucVer[-1] = (3 * puc[-iStride-1] + puc[-iStride] + 2 )>>2;
      }
    }
    else
    {
      if( xIsLeftRef() )
      {
        m_pucHor[-1] = m_pucVer[-1] = (puc[-1] + 3 * puc[-iStride-1] )>>2;
      }
      else
      {
        m_pucHor[-1] = m_pucVer[-1] = puc[-iStride-1];
      }
    }
  }
}


Void IntraPrediction::xLoadVerPred8x8( XPel* puc, Int iStride )
{
  if( xIsLeftRef() )
  {
    if( xIsAboveLeftRef() )
    {
      m_pucVer[0] = (puc[-iStride-1] + 2 * puc[-1] + puc[iStride-1] + 2 )>>2;
    }
    else
    {
      m_pucVer[0] = ( 3 * puc[-1] + puc[iStride-1] + 2 )>>2;
    }

    for( Int y = 1; y < 7; y++ )
    {
      const Int iOffset = y*iStride-1;
      m_pucVer[y] = (puc[iOffset-iStride] + 2*puc[iOffset] + puc[iOffset+iStride] + 2) >> 2;
    }

    m_pucVer[7] = ( puc[6*iStride-1] + 3*puc[7*iStride-1] + 2 )>>2;
  }
}


Void IntraPrediction::xPredLum8x8Mode0Vert( XPel* puc, Int iStride )
{
  AOF_DBG( xIsAboveRef() );

  xLoadHorPred8x8( puc, iStride );

  for( Int n = 0; n < 8; n++ )
  {
    ::memcpy( puc, m_pucHor, 8*sizeof(XPel) );
    puc += iStride;
  }
}


Void IntraPrediction::xPredLum8x8Mode1Horiz( XPel* puc, Int iStride )
{
  AOF_DBG( xIsLeftRef() );

  xLoadVerPred8x8( puc, iStride );

  for( Int n = 0; n < 8; n++ )
  {
    for( Int m = 0; m < 8; m++ )
    {
      puc[m] = m_pucVer[n];
    }
    puc += iStride;
  }
}



Void IntraPrediction::xPredLum8x8Mode2Dc( XPel* puc, Int iStride )
{
  UInt uiDcValue = 4;

  if( ! xIsLeftRef() )
  {
    if( ! xIsAboveRef() )
    {
      uiDcValue = 0x80;
    }
    else
    {
      xLoadHorPred8x8( puc, iStride );
      for( Int x = 0; x < 8; x++ )
      {
        uiDcValue  += m_pucHor[x];
      }
      uiDcValue >>= 3;
    }
  }
  else
  {
    xLoadVerPred8x8( puc, iStride );

    if( ! xIsAboveRef() )
    {
      for( Int y = 0; y < 8; y++ )
      {
        uiDcValue  += m_pucVer[y];
      }
      uiDcValue >>= 3;
    }
    else
    {
      xLoadHorPred8x8( puc, iStride );

      uiDcValue = 8;
      for( Int x = 0; x < 8; x++ )
      {
        uiDcValue  += m_pucHor[x];
      }

      for( Int y = 0; y < 8; y++ )
      {
        uiDcValue  += m_pucVer[y];
      }
      uiDcValue >>= 4;

    }
  }

  for( Int n = 0; n < 8; n++ )
  {
    for( Int m = 0; m < 8; m++ )
    {
      puc[m] = uiDcValue;
    }
    puc += iStride;
  }
}


Void IntraPrediction::xPredLum8x8Mode3DiagDownLeft( XPel* puc, Int iStride ) // Diagonal Down Left Pred
{
  AOF_DBG( xIsAboveRef() );

  xLoadHorPred8x8( puc, iStride );

  for( Int y = 0; y < 8; y++ )
  {
    for( Int x = 0; x < 8; x++ )
    {
      puc[x] = (m_pucHor[x+y] + 2*m_pucHor[x+y+1] + m_pucHor[x+y+2] + 2 ) >> 2;
    }
    puc += iStride;
  }
  puc[7-iStride] = (m_pucHor[14] + 3*m_pucHor[15] + 2)>>2;
}


Void IntraPrediction::xPredLum8x8Mode4DiagDownRight( XPel* puc, Int iStride ) // Diagonal Down Right Pred
{
  AOF_DBG( xIsAllLeftAboveRef() );

  xLoadHorPred8x8( puc, iStride );
  xLoadVerPred8x8( puc, iStride );
  xLoadXPred8x8  ( puc, iStride );

  for( Int y = 0; y < 8; y++ )
  {
    for( Int x = 0; x < 8; x++ )
    {
      if(x > y)
      {
        const Int iOffset = x-y;
        puc[x] = (m_pucHor[iOffset-2] + 2*m_pucHor[iOffset-1] + m_pucHor[iOffset] + 2)>>2;
      }
      else if(x < y)
      {
        const Int iOffset = y-x;
        puc[x] = (m_pucVer[iOffset-2] + 2*m_pucVer[iOffset-1] + m_pucVer[iOffset] + 2)>>2;
      }
      else
      {
        puc[x] = (m_pucHor[0] + 2*m_pucVer[-1] + m_pucVer[0] + 2)>>2;
      }
    }
    puc += iStride;
  }
}



Void IntraPrediction::xPredLum8x8Mode5VertRight( XPel* puc, Int iStride )  // Vertical Right Pred
{
  AOF_DBG( xIsAllLeftAboveRef() );

  xLoadHorPred8x8( puc, iStride );
  xLoadVerPred8x8( puc, iStride );
  xLoadXPred8x8  ( puc, iStride );

  for( Int y = 0; y < 8; y++ )
  {
    for( Int x = 0; x < 8; x++ )
    {
      Int iZVR = 2 * x - y;
      if( iZVR >= 0 )
      {
        Int iOffset = x-(y>>1);
        if( iZVR & 1)
        {
          puc[x] = ( m_pucHor[iOffset-2] + 2*m_pucHor[iOffset-1] + m_pucHor[iOffset] + 2 ) >> 2;
        }
        else
        {
          puc[x] = ( m_pucHor[iOffset-1] + m_pucHor[iOffset] + 1 ) >> 1;
        }
      }
      else
      {
        if( iZVR == -1)
        {
          puc[x] = (m_pucHor[0] + 2*m_pucVer[-1] + m_pucVer[0] + 2)>>2;
        }
        else
        {
          Int iOffset = y-2*x;
          puc[x] = (m_pucVer[iOffset-1] + 2*m_pucVer[iOffset-2] + m_pucVer[iOffset-3] + 2)>>2;
        }
      }
    }
    puc += iStride;
  }
}



Void IntraPrediction::xPredLum8x8Mode6HorizDown( XPel* puc, Int iStride ) // Horizontal DownPred
{
  AOF_DBG( xIsAllLeftAboveRef() );

  xLoadXPred8x8( puc, iStride );
  xLoadHorPred8x8( puc, iStride );
  xLoadVerPred8x8( puc, iStride );

  for( Int y = 0; y < 8; y++ )
  {
    for( Int x = 0; x < 8; x++ )
    {
      Int iZHD = 2 * y - x;
      if( iZHD >= 0 )
      {
        Int iOffset = y-(x>>1);
        if( iZHD & 1)
        {
          puc[x] = ( m_pucVer[iOffset-2] + 2*m_pucVer[iOffset-1] + m_pucVer[iOffset] + 2 ) >> 2;
        }
        else
        {
          puc[x] = ( m_pucVer[iOffset-1] + m_pucVer[iOffset] + 1 ) >> 1;
        }
      }
      else
      {
        if( iZHD == -1)
        {
          puc[x] = (m_pucHor[0] + 2*m_pucVer[-1] + m_pucVer[0] + 2)>>2;
        }
        else
        {
          Int iOffset = x - 2*y;
          puc[x] = ( m_pucHor[iOffset-1] + 2*m_pucHor[iOffset-2] + m_pucHor[iOffset-3] + 2 ) >> 2;
        }
      }
    }
    puc += iStride;
  }
}


Void IntraPrediction::xPredLum8x8Mode7VertLeft( XPel* puc, Int iStride ) // Vertical Left Pred
{
  AOF_DBG( xIsAboveRef() ); //!!!

  xLoadHorPred8x8( puc, iStride );

  for( Int y = 0; y < 8; y+=2 )
  {
    for( Int x1 = 0; x1 < 8; x1++ )
    {
      Int iOffset = x1 + ( y >> 1 );
      puc[x1] = ( m_pucHor[iOffset] + m_pucHor[iOffset+1] + 1 ) >> 1;
    }
    puc += iStride;

    for( Int x2 = 0; x2 < 8; x2++ )
    {
      Int iOffset = x2 + ( y >> 1 );
      puc[x2] = ( m_pucHor[iOffset] + 2*m_pucHor[iOffset+1] + m_pucHor[iOffset+2] + 2 ) >> 2;
    }
    puc += iStride;
  }
}



Void IntraPrediction::xPredLum8x8Mode8HorizUp( XPel* puc, Int iStride ) // Horizontal Up Pred
{
  AOF_DBG( xIsLeftRef() ); //!!!

  xLoadVerPred8x8( puc, iStride );

  UInt uiEqual13 = (m_pucVer[6]+ 3*m_pucVer[7] + 2) >> 2;
  UInt uiGreater13 = m_pucVer[7];

  for( Int y = 0; y < 8; y++ )
  {
    for( Int x = 0; x < 8; x++ )
    {
      Int iZHU = x + 2 * y;
      if( iZHU < 13 )
      {
        Int iOffset = y+(x>>1);
        if( iZHU & 1)
        {
          puc[x] = ( m_pucVer[iOffset] + 2*m_pucVer[iOffset+1] + m_pucVer[iOffset+2] + 2 ) >> 2;
        }
        else
        {
          puc[x] = ( m_pucVer[iOffset] + m_pucVer[iOffset+1] + 1 ) >> 1;
        }
      }
      else
      {
        if( iZHU == 13 )
        {
          puc[x] = uiEqual13;
        }
        else
        {
          puc[x] = uiGreater13;
        }
      }
    }
    puc += iStride;
  }
}




H264AVC_NAMESPACE_END
