
#include "BStreamExtractor.h"
#include "Extractor.h"

int main( int argc, char** argv)
{
  Extractor*          pcExtractor = NULL;
  ExtractorParameter  cParameter;

  printf( "JSVM %s BitStream Extractor \n\n", _JSVM_VERSION_ );

  RNOKRS( cParameter.init       ( argc, argv ),   -2 );

  for( Int n = 0; n < 1; n++ )
  {
    RNOKR( Extractor::create    ( pcExtractor ),  -3 );

    RNOKR( pcExtractor->init    ( &cParameter ),  -4 );

    RNOKR( pcExtractor->go      (),               -5 );

    RNOKR( pcExtractor->destroy (),               -6 );
  }

  return 0;
}
