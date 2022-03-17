
#include "QualityLevelParameter.h"
#include "QualityLevelAssigner.h"

int main(int argc, char **argv)
{
  printf( "JSVM Quality Assigner\n\n" );
  printf( "Info: This tool relies on the scalable SEI message\n"
          "      This tool requires prefix NAL units in front of each H.264 NAL unit\n"
          "      This tool assumes a fixed GOP size (non-AGS) throughout a sequence\n\n\n" );

  QualityLevelParameter*  pcParameter = 0;
  RNOKR( QualityLevelParameter::create( pcParameter ),            -1 );
  RNOKS( pcParameter->init( argc, argv ) );


  QualityLevelAssigner*   pcQualityLevelAssigner = 0;
  RNOKR( QualityLevelAssigner::create( pcQualityLevelAssigner ),  -2 );
  RNOKR( pcQualityLevelAssigner->init( pcParameter ),             -3 );
  RNOKR( pcQualityLevelAssigner->go(),                            -4 );
  RNOKR( pcQualityLevelAssigner->destroy(),                       -5 );

  //manu.mathew@samsung : memory leak fix
  RNOKS( pcParameter->destroy() );
  //--
  return 0;
}

