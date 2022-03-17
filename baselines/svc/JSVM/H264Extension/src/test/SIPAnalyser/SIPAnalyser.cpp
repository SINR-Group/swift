
#include "SIPAnalyser.h"

#define MAX_KNAPSACK (0x40000)

SIPAnalyser::SIPAnalyser():
m_pcSIPParameters(NULL),
m_aaaiFrameBits(NULL),
m_aiTotalBits(NULL),
m_aaiSIPDecision(NULL)
{

}

SIPAnalyser::~SIPAnalyser()
{

}

ErrVal SIPAnalyser::create( SIPAnalyser*& rpcSIPAnalyser)
{
  rpcSIPAnalyser = new SIPAnalyser;
  ROT( NULL == rpcSIPAnalyser );
  return Err::m_nOK;
}


ErrVal SIPAnalyser::destroy()
{
  RNOK(xUninitData());

  delete this;

  return Err::m_nOK;
}

ErrVal SIPAnalyser::init(SIPParameters *pcSIPParameters)
{
  ROF(pcSIPParameters);

  m_pcSIPParameters=pcSIPParameters;

  RNOK(xInitData());
  RNOK(xReadData());

  return Err::m_nOK;
}

ErrVal SIPAnalyser::go()
{
  ROF(m_pcSIPParameters);

  for(UInt i=1;i<m_pcSIPParameters->getLayerNum();i++)
  {
    RNOK(xProcessLayer(i));
    RNOK(xDumpLayer(i));
  }

  return Err::m_nOK;
}

ErrVal SIPAnalyser::xInitData()
{
  ROF(m_pcSIPParameters);

  ROF(m_aaaiFrameBits=new Int**[m_pcSIPParameters->getLayerNum()]);
  UInt i;
  for(i=0;i<m_pcSIPParameters->getLayerNum();i++)
  {
    ROF(m_aaaiFrameBits[i]=new Int*[m_pcSIPParameters->getFrameNum()]);
    for(UInt j=0;j<m_pcSIPParameters->getFrameNum();j++)
    {
      ROF(m_aaaiFrameBits[i][j]=new Int[2]);
      m_aaaiFrameBits[i][j][0]=m_aaaiFrameBits[i][j][1]=0;
    }
  }

  //dummy for layer 0
  ROF(m_aiTotalBits  =  new int[m_pcSIPParameters->getLayerNum()]);
  memset(m_aiTotalBits,0,sizeof(int)*m_pcSIPParameters->getLayerNum());

  ROF(m_aaiSIPDecision  =  new Int*[m_pcSIPParameters->getLayerNum()]);
  for(i=0;i<m_pcSIPParameters->getLayerNum();i++)
  {
    ROF(m_aaiSIPDecision[i]=new Int[m_pcSIPParameters->getFrameNum()]);
    memset(m_aaiSIPDecision[i],0,sizeof(int)*m_pcSIPParameters->getFrameNum());
  }

  return Err::m_nOK;
}

ErrVal SIPAnalyser::xReadData()
{
  ROF(m_pcSIPParameters);
  ROF(m_aaaiFrameBits);

  FILE *pFileWith,*pFileWithout;
  pFileWith=pFileWithout=NULL;

  for(UInt i=0;i<m_pcSIPParameters->getLayerNum();i++)
  {
    pFileWith    =  fopen(m_pcSIPParameters->getLayerParameter(i)->m_strInputFileWithInterPred.c_str(),"rt");
    pFileWithout  =  fopen(m_pcSIPParameters->getLayerParameter(i)->m_strInputFileWithoutInterPred.c_str(),"rt");
    if(!pFileWith||!pFileWithout)
    {
      printf("\nCan't find input files\n");
      exit(-8);
    }

    int iStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(i)->m_uiFps;

    for(UInt j=0;j<m_pcSIPParameters->getFrameNum();j+=iStep)
    {
      if(fscanf(pFileWith,"%d",&m_aaaiFrameBits[i][j][0])!=1)
      {
        printf("\nError when reading input files\n");
        return Err::m_nERR;
      }
      if(fscanf(pFileWithout,"%d",&m_aaaiFrameBits[i][j][1])!=1)
      {
        printf("\nError when reading input files\n");
        return Err::m_nERR;
      }

      for(UInt k=i;k<m_pcSIPParameters->getLayerNum();k++)
          m_aiTotalBits[k]+=m_aaaiFrameBits[i][j][0];
    }
  }

  fclose(pFileWith);
  fclose(pFileWithout);

  return Err::m_nOK;
}

ErrVal SIPAnalyser::xUninitData()
{
  if(m_aaaiFrameBits!=NULL)
  {
    for(UInt i=0;i<m_pcSIPParameters->getLayerNum();i++)
    {
      for(UInt j=0;j<m_pcSIPParameters->getFrameNum();j++)
        delete[] m_aaaiFrameBits[i][j];

      delete[] m_aaaiFrameBits[i];
    }
    delete[] m_aaaiFrameBits;
    m_aaaiFrameBits=NULL;
  }

  if(m_aiTotalBits!=NULL)
  {
    delete[] m_aiTotalBits;
    m_aiTotalBits=NULL;
  }

  if(m_aaiSIPDecision!=NULL)
  {
    for(UInt i=0;i<m_pcSIPParameters->getLayerNum();i++)
      delete[] m_aaiSIPDecision[i];
    delete[] m_aaiSIPDecision;
    m_aaiSIPDecision=NULL;
  }

  return Err::m_nOK;
}

//Solve the knapsack problem using dynamic programming method.
ErrVal SIPAnalyser::xProcessKnapsack(int iNumber, int *piWeight, int *piPrice, int iBagCubage, int *piDecision)
{
  int **ppiM;
  ROF(ppiM=new int* [iNumber]);

  int i;
  for(i=0;i<iNumber;i++)
  {
    ppiM[i]=new int[iBagCubage+1];
    ROF(ppiM[i]);
  }

  int iMax=gMin(piWeight[iNumber-1]-1,iBagCubage);

  for(i=0;i<=iMax;i++)
    ppiM[iNumber-1][i]=0;
  for(i=piWeight[iNumber-1];i<=iBagCubage;i++)
    ppiM[iNumber-1][i]=piPrice[iNumber-1];

  for(i=iNumber-2;i>0;i--)
  {
    iMax=gMin(piWeight[i]-1,iBagCubage);
    int j;
    for(j=0;j<=iMax;j++)
      ppiM[i][j]=ppiM[i+1][j];
    for(j=piWeight[i];j<=iBagCubage;j++)
      ppiM[i][j]=gMax(ppiM[i+1][j],ppiM[i+1][j-piWeight[i]]+piPrice[i]);
  }

  ppiM[0][iBagCubage]=ppiM[1][iBagCubage];
  if(iBagCubage>=piWeight[0])
    ppiM[0][iBagCubage]=gMax(ppiM[0][iBagCubage],ppiM[1][iBagCubage-piWeight[0]]+piPrice[0]);

  for(i=0;i<iNumber-1;i++)
  {
    if(ppiM[i][iBagCubage]==ppiM[i+1][iBagCubage])
      piDecision[i]=0;
    else
    {
      piDecision[i]=1;
      iBagCubage-=piWeight[i];
    }
  }

  piDecision[iNumber-1]=ppiM[iNumber-1][iBagCubage]?1:0;

  for(i=0;i<iNumber;i++)
    delete [] ppiM[i];
  delete[] ppiM;

  return Err::m_nOK;
}

ErrVal SIPAnalyser::xProcessLayer(int iLayer)
{
  ROF(m_pcSIPParameters);
  ROFS(iLayer);

  int* aiWeight,*aiPrice,*aiPOCMap,*aiDecision;
  ROF(aiWeight  =  new int[m_pcSIPParameters->getFrameNum()]);
  ROF(aiPrice    =  new int[m_pcSIPParameters->getFrameNum()]);
  ROF(aiPOCMap  =  new int[m_pcSIPParameters->getFrameNum()]);
  ROF(aiDecision  =  new int[m_pcSIPParameters->getFrameNum()]);

  memset(aiWeight,0,sizeof(int)*m_pcSIPParameters->getFrameNum());
  memset(aiPrice,0,sizeof(int)*m_pcSIPParameters->getFrameNum());
  memset(aiPOCMap,0,sizeof(int)*m_pcSIPParameters->getFrameNum());
  memset(aiDecision,0,sizeof(int)*m_pcSIPParameters->getFrameNum());

  int iLowStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(iLayer-1)->m_uiFps;
  int iHighStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(iLayer)->m_uiFps;

  int iTolerated=int(float(m_aiTotalBits[iLayer])*m_pcSIPParameters->getLayerParameter(iLayer)->m_fTolerableRatio+0.5);

  int iIndex=0;

  UInt i;
  for(i=0;i<m_pcSIPParameters->getFrameNum();i+=iHighStep)
  {
    if(i%iLowStep!=0)//there is no corresponding low resolution frame.
      iTolerated-=m_aaaiFrameBits[iLayer][i][0];
    else
    {
      int  iDelta=m_aaaiFrameBits[iLayer][i][1]-m_aaaiFrameBits[iLayer][i][0];

      if(iDelta<=0)//don't use interlayer prediction
      {
        m_aaiSIPDecision[iLayer][i]=1;
        iTolerated-=m_aaaiFrameBits[iLayer][i][1]+m_aaaiFrameBits[iLayer-1][i][0];
      }
      else if(iDelta>=m_aaaiFrameBits[iLayer-1][i][1])//use interlayer prediction
      {
        m_aaiSIPDecision[iLayer][i]=0;
        iTolerated-=m_aaaiFrameBits[iLayer][i][0]+m_aaaiFrameBits[iLayer-1][i][0];
      }
      else//prepare for the knapsack algorithm
      {
        iTolerated-=m_aaaiFrameBits[iLayer][i][0]+m_aaaiFrameBits[iLayer-1][i][0];
        aiPOCMap[iIndex]=i;
        aiWeight[iIndex]=iDelta;
        aiPrice[iIndex++]=m_aaaiFrameBits[iLayer-1][i][1]-iDelta;
      }
    }
  }

  if(iTolerated<0)
  {
    printf("\n Warning : The constrain is too tight on layer %d\n",iLayer);
    iTolerated=0;
  }

  //If the bag cubage is too big, the algorithm complexity will be too hard.
  //So we cut down precision to decrease complexity.MAX_KNAPSACK should be set
  //according to the available computing capability.
  {
    int iShift=0;
    while(iTolerated>MAX_KNAPSACK)
    {
      iTolerated>>=1;
      iShift++;
    }
    if(iShift!=0)
      for(int j=0;j<iIndex;j++)
      {
        aiWeight[j]>>=iShift;
        aiPrice[j]>>=iShift;
      }
  }

  if(iIndex>0)
    RNOK(xProcessKnapsack(iIndex,aiWeight,aiPrice,iTolerated,aiDecision));

  for(int j=0;j<iIndex;j++)
    m_aaiSIPDecision[iLayer][aiPOCMap[j]]=aiDecision[j];

  RNOK(xPrintLayer(iLayer));

  for(i=0;i<m_pcSIPParameters->getFrameNum();i+=iLowStep)
  {
    m_aaaiFrameBits[iLayer][i][1]=m_aaiSIPDecision[iLayer][i]*m_aaaiFrameBits[iLayer][i][1]+(1-m_aaiSIPDecision[iLayer][i])*m_aaaiFrameBits[iLayer][i][0];
    m_aaaiFrameBits[iLayer][i][0]=m_aaaiFrameBits[iLayer][i][1]+m_aaaiFrameBits[iLayer-1][i][0];
    m_aaaiFrameBits[iLayer][i][1]=m_aaaiFrameBits[iLayer][i][1]+(1-m_aaiSIPDecision[iLayer][i])*m_aaaiFrameBits[iLayer-1][i][1];
  }

  delete[]  aiWeight;
  delete[]  aiPrice;
  delete[]  aiPOCMap;
  delete[]  aiDecision;

  return Err::m_nOK;
}

ErrVal SIPAnalyser::xDumpLayer(int iLayer)
{
  ROF(m_pcSIPParameters);
  ROFS(iLayer);

  FILE* pFile    =  fopen(m_pcSIPParameters->getLayerParameter(iLayer)->m_strOutputFile.c_str(),"wt");
  if(!pFile)
  {
    printf("\nCan't open output files\n");
    exit(-8);
  }

  int iLowStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(iLayer-1)->m_uiFps;

  for(UInt i=0;i<m_pcSIPParameters->getFrameNum();i+=iLowStep)
    if(m_aaiSIPDecision[iLayer][i]==1)
      fprintf(pFile,"%d\n",i);

  fclose(pFile);
  return Err::m_nOK;
}

ErrVal SIPAnalyser::xPrintLayer(int iLayer)
{
  ROF(m_pcSIPParameters);
  ROFS(iLayer);
  ROF(m_aiTotalBits);
  ROF(m_aaaiFrameBits);
  ROF(m_aaiSIPDecision);

  int iBitsWithMA=0;
  int iBitsWithoutMA=0;

  int iLowStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(iLayer-1)->m_uiFps;
  int iHighStep  =  m_pcSIPParameters->getInFps()/m_pcSIPParameters->getLayerParameter(iLayer)->m_uiFps;

  for(UInt i=0;i<m_pcSIPParameters->getFrameNum();i+=iHighStep)
  {
    if(i%iLowStep!=0)
    {
      iBitsWithMA+=m_aaaiFrameBits[iLayer][i][0];
      iBitsWithoutMA+=m_aaaiFrameBits[iLayer][i][0];
    }
    else
    {
      if(m_aaiSIPDecision[iLayer][i]==1)
      {
        iBitsWithMA+=m_aaaiFrameBits[iLayer][i][1]+m_aaaiFrameBits[iLayer-1][i][0];
        iBitsWithoutMA+=m_aaaiFrameBits[iLayer][i][1];
      }
      else
      {
        iBitsWithMA+=m_aaaiFrameBits[iLayer][i][0]+m_aaaiFrameBits[iLayer-1][i][0];
        iBitsWithoutMA+=m_aaaiFrameBits[iLayer][i][0]+m_aaaiFrameBits[iLayer-1][i][1];
      }
    }
  }

  //display
  {
    printf("\n");
    for(int i=0;i<60;i++)
      printf("#");

    printf("\n Layer : %d\n",iLayer);
    printf(" Anchor bits : %d\n",m_aiTotalBits[iLayer]);
    printf(" Tolerated loss as requiring : %.2f%%\n",100*m_pcSIPParameters->getLayerParameter(iLayer)->m_fTolerableRatio);
    printf(" Theoretical bits with MA : %d\n",iBitsWithMA);
    printf(" Ratio (theoretical bits with MA/anchor bits) : %.2f%%\n",100*(float)iBitsWithMA/(float)m_aiTotalBits[iLayer] );
    printf(" Theoretical bits without MA : %d\n",iBitsWithoutMA);
    printf(" Ratio (theoretical bits without MA/anchor bits) : %.2f%%\n",100*(float)iBitsWithoutMA/(float)m_aiTotalBits[iLayer] );
  }

  return Err::m_nOK;
}
