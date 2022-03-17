
#include "SIPParameters.h"
#include "SIPAnalyser.h"

int main(int argc, char **argv)
{
  printf( "**************    JSVM SIP Analyser   **************\n\n" );
  SIPParameters* pcSIPParameters=NULL;
  SIPAnalyser* pcSIPAnalyser=NULL;

  RNOKR(SIPParameters::create(pcSIPParameters),-1);
  RNOKR(pcSIPParameters->init(argc,argv),-2);

  RNOKR(SIPAnalyser::create(pcSIPAnalyser),-3);
  RNOKR(pcSIPAnalyser->init(pcSIPParameters),-4);

  RNOKR(pcSIPAnalyser->go(),-5);

  RNOKR(pcSIPAnalyser->destroy(),-6);
  RNOKR(pcSIPParameters->destroy(),-7);

  return 0;
}
