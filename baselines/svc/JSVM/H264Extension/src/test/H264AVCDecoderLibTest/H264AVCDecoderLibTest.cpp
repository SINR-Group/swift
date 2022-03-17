
#include "H264AVCDecoderLibTest.h"
#include "H264AVCDecoderTest.h"


int
main( int argc, char** argv)
{
#ifdef SHARP_AVC_REWRITE_OUTPUT
  printf( "JSVM %s AVC REWRITER\n\n\n", _JSVM_VERSION_ );
#else
  printf( "JSVM %s Decoder\n\n\n",      _JSVM_VERSION_ );
#endif

  H264AVCDecoderTest*   pcH264AVCDecoderTest  = 0;
  ReadBitstreamFile*    pcReadStream          = 0;
#ifdef SHARP_AVC_REWRITE_OUTPUT
  WriteBitstreamToFile* pcWriteStream         = 0;
#else
  WriteYuvToFile*       pcWriteYuv            = 0;
#endif
  DecoderParameter      cParameter;

  //===== create instances =====
  RNOKRS( ReadBitstreamFile   ::create  ( pcReadStream ),                             -1 );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOKRS( WriteBitstreamToFile::create  ( pcWriteStream ),                            -2 );
#else
  RNOKRS( WriteYuvToFile      ::create  ( pcWriteYuv ),                               -2 );
#endif
  RNOKRS( H264AVCDecoderTest  ::create  ( pcH264AVCDecoderTest ),                     -3 );

  //===== initialization =====
  RNOKRS( cParameter           .init    ( argc, argv ),                               -4 );
  RNOKRS( pcReadStream        ->init    ( cParameter.cBitstreamFile ),                -5 );
#ifdef SHARP_AVC_REWRITE_OUTPUT
  RNOKRS( pcWriteStream       ->init    ( cParameter.cYuvFile ),                      -6 );
  RNOKRS( pcH264AVCDecoderTest->init    ( &cParameter, pcReadStream, pcWriteStream ), -7 );
#else
  RNOKRS( pcWriteYuv          ->init    ( cParameter.cYuvFile ),                      -6 );
  RNOKRS( pcH264AVCDecoderTest->init    ( &cParameter, pcReadStream, pcWriteYuv ),    -7 );
#endif

  //===== run =====
  RNOKR ( pcH264AVCDecoderTest->go      (),                                           -8 );

  //===== uninit and destroy instances =====
  RNOKR ( pcH264AVCDecoderTest->uninit  (),                                           -9 );
  RNOKR ( pcH264AVCDecoderTest->destroy (),                                           -10 );
  if( pcReadStream )
  {
    RNOKR ( pcReadStream      ->uninit  (),                                           -11 );
    RNOKR ( pcReadStream      ->destroy (),                                           -12 );
  }
#ifdef SHARP_AVC_REWRITE_OUTPUT
  if( pcWriteStream )
  {
    RNOKR ( pcWriteStream     ->uninit  (),                                           -13 );
    RNOKR ( pcWriteStream     ->destroy (),                                           -14 );
  }
#else
  if( pcWriteYuv )
  {
    RNOKR ( pcWriteYuv        ->uninit  (),                                           -13 );
    RNOKR ( pcWriteYuv        ->destroy (),                                           -14 );
  }
#endif

  return 0;
}
