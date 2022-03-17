
#include "H264AVCEncoderLibTest.h"
#include "H264AVCEncoderTest.h"
#include "H264AVCCommonLib/CommonBuffers.h"


int
main( int argc, char** argv)
{
  printf("JSVM %s Encoder\n\n",_JSVM_VERSION_);

  H264AVCEncoderTest*               pcH264AVCEncoderTest = NULL;
  RNOK( H264AVCEncoderTest::create( pcH264AVCEncoderTest ) );

  RNOKS( pcH264AVCEncoderTest->init   ( argc, argv ) );
  RNOK ( pcH264AVCEncoderTest->go     () );
  RNOK ( pcH264AVCEncoderTest->destroy() );

  return 0;
}
