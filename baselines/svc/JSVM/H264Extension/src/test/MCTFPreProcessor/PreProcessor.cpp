
#include "PreProcessorParameter.h"
#include "MCTFPreProcessorTest.h"


int main(int argc, char **argv)
{
  printf( "JSVM MCTF Pre-Processor\n\n" );

  PreProcessorParameter*  pcParameter = 0;
  RNOKR( PreProcessorParameter::create( pcParameter ),            -1 );
  RNOKS( pcParameter->init( argc, argv ) );

  MCTFPreProcessorTest*   pcMCTFPreProcessorTest = 0;
  RNOKR( MCTFPreProcessorTest::create( pcMCTFPreProcessorTest ),  -2 );
  RNOKR( pcMCTFPreProcessorTest->init( pcParameter ),             -3 );
  RNOKR( pcMCTFPreProcessorTest->go(),                            -4 );
  RNOKR( pcMCTFPreProcessorTest->destroy(),                       -5 );
  RNOKR( pcParameter           ->destroy(),                       -6 );

  return 0;
}

