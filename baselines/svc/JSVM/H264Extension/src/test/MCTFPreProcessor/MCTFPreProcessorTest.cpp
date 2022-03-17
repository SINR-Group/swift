
#include "MCTFPreProcessorTest.h"
#include "H264AVCCommonLib.h"
#include "H264AVCCommonLib/CommonDefs.h"

using namespace h264;


MCTFPreProcessorTest::MCTFPreProcessorTest()
: m_pcParameter             ( 0 )
, m_pcMCTFPreProcessor      ( 0 )
, m_pcWriteYuv              ( 0 )
, m_pcReadYuv               ( 0 )
, m_uiPicSize               ( 0 )
, m_uiLumOffset             ( 0 )
, m_uiCbOffset              ( 0 )
, m_uiCrOffset              ( 0 )
, m_uiHeight                ( 0 )
, m_uiWidth                 ( 0 )
, m_uiStride                ( 0 )
{
}


MCTFPreProcessorTest::~MCTFPreProcessorTest()
{
}


ErrVal
MCTFPreProcessorTest::create( MCTFPreProcessorTest*& rpcMCTFPreProcessorTest )
{
  rpcMCTFPreProcessorTest = new MCTFPreProcessorTest;
  ROF( rpcMCTFPreProcessorTest );
  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::destroy()
{
  if( m_pcMCTFPreProcessor )
  {
    RNOK( m_pcMCTFPreProcessor->uninit() );
    RNOK( m_pcMCTFPreProcessor->destroy() );
  }
  if( m_pcWriteYuv )
  {
    RNOK( m_pcWriteYuv->destroy() );
  }
  if( m_pcReadYuv )
  {
    RNOK( m_pcReadYuv->uninit() );
    RNOK( m_pcReadYuv->destroy() );
  }

  AOF( m_cActivePicBufferList.empty() );

  //===== delete picture buffer =====
  PicBufferList::iterator iter;
  for( iter = m_cUnusedPicBufferList.begin(); iter != m_cUnusedPicBufferList.end(); iter++ )
  {
    delete (*iter)->getBuffer();
    delete (*iter);
  }
  for( iter = m_cActivePicBufferList.begin(); iter != m_cActivePicBufferList.end(); iter++ )
  {
    delete (*iter)->getBuffer();
    delete (*iter);
  }

  delete this;
  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::init( PreProcessorParameter* pcParameter )
{
  ROF( pcParameter );
  m_pcParameter = pcParameter;

  //----- init coding parameters -----
  RNOK( xInitCodingParameter() );

  //----- init YUV reader and writer -----
  RNOKS( WriteYuvToFile::create( m_pcWriteYuv ) );
  RNOKS( m_pcWriteYuv  ->init  ( m_pcParameter->getOutputFileName() ) );
  RNOKS( ReadYuvFile   ::create( m_pcReadYuv  ) );
  RNOKS( m_pcReadYuv   ->init  ( m_pcParameter->getInputFileName(),
                                 m_pcParameter->getFrameHeight  (),
                                 m_pcParameter->getFrameWidth   () ) );

  //----- creater MCTF pre processor -----
  RNOK( MCTFPreProcessor::create( m_pcMCTFPreProcessor ) );

  //----- set frame buffer parameters -----
  UInt  uiMbX     = m_pcParameter->getFrameWidth  () >> 4;
  UInt  uiMbY     = m_pcParameter->getFrameHeight () >> 4;
  UInt  uiSize    = ((uiMbY<<4)+2*YUV_Y_MARGIN)*((uiMbX<<4)+2*YUV_X_MARGIN);
  m_uiPicSize     = ((uiMbX<<4)+2*YUV_X_MARGIN)*((uiMbY<<4)+2*YUV_Y_MARGIN)*3/2;
  m_uiLumOffset   = ((uiMbX<<4)+2*YUV_X_MARGIN)* YUV_Y_MARGIN   + YUV_X_MARGIN;
  m_uiCbOffset    = ((uiMbX<<3)+  YUV_X_MARGIN)* YUV_Y_MARGIN/2 + YUV_X_MARGIN/2 + uiSize;
  m_uiCrOffset    = ((uiMbX<<3)+  YUV_X_MARGIN)* YUV_Y_MARGIN/2 + YUV_X_MARGIN/2 + 5*uiSize/4;
  m_uiHeight      =   uiMbY<<4;
  m_uiWidth       =   uiMbX<<4;
  m_uiStride      =  (uiMbX<<4)+ 2*YUV_X_MARGIN;

  return Err::m_nOK;
}



ErrVal
MCTFPreProcessorTest::go()
{
  PicBuffer*    pcOriginalPicBuffer;
  PicBuffer*    pcReconstructPicBuffer;
  PicBufferList cPicBufferOutputList;
  PicBufferList cPicBufferUnusedList;

  //===== initialization =====
  RNOK( m_pcMCTFPreProcessor->init( m_pcParameter, &m_cCodingParameter ) );

  //===== loop over frames =====
  for( UInt uiFrame = 0; uiFrame < m_pcParameter->getNumFrames(); uiFrame++ )
  {
    //===== get picture buffers and read original pictures =====
    RNOK( xGetNewPicBuffer( pcReconstructPicBuffer, m_uiPicSize ) );
    RNOK( xGetNewPicBuffer( pcOriginalPicBuffer,    m_uiPicSize ) );

    RNOK( m_pcReadYuv->readFrame( *pcOriginalPicBuffer + m_uiLumOffset,
                                  *pcOriginalPicBuffer + m_uiCbOffset,
                                  *pcOriginalPicBuffer + m_uiCrOffset,
                                  m_uiHeight, m_uiWidth, m_uiStride ) );

    RNOK( m_pcMCTFPreProcessor->process( pcOriginalPicBuffer,
                                         pcReconstructPicBuffer,
                                         cPicBufferOutputList,
                                         cPicBufferUnusedList ) );

    RNOK( xWrite          ( cPicBufferOutputList ) );
    RNOK( xRemovePicBuffer( cPicBufferUnusedList ) );
  }

  RNOK( m_pcMCTFPreProcessor->finish( cPicBufferOutputList,
                                     cPicBufferUnusedList ) );

  RNOK( xWrite          ( cPicBufferOutputList ) );
  RNOK( xRemovePicBuffer( cPicBufferUnusedList ) );

  printf("\n\n");
  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::xInitCodingParameter()
{
  m_cCodingParameter.getMotionVectorSearchParams().setSearchMode( 4 );
  m_cCodingParameter.getMotionVectorSearchParams().setFullPelDFunc( 3 );
  m_cCodingParameter.getMotionVectorSearchParams().setSubPelDFunc( 2 );
  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::xGetNewPicBuffer( PicBuffer*&  rpcPicBuffer,
                                        UInt         uiSize )
{
  if( m_cUnusedPicBufferList.empty() )
  {
    rpcPicBuffer = new PicBuffer( new UChar[ uiSize ] );
  }
  else
  {
    rpcPicBuffer = m_cUnusedPicBufferList.popFront();
  }
  m_cActivePicBufferList.push_back( rpcPicBuffer );

  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::xRemovePicBuffer( PicBufferList&  rcList )
{
  while( ! rcList.empty() )
  {
    PicBuffer*  pcBuffer = rcList.popFront();
    if( NULL != pcBuffer )
    {
      PicBufferList::iterator begin = m_cActivePicBufferList.begin();
      PicBufferList::iterator end   = m_cActivePicBufferList.end  ();
      PicBufferList::iterator iter  = std::find( begin, end, pcBuffer );

      ROT( iter == end ); // there is something wrong if the address is not in the active list

      AOT_DBG( (*iter)->isUsed() );
      m_cUnusedPicBufferList.push_back( *iter );
      m_cActivePicBufferList.erase    (  iter );
    }
  }
  return Err::m_nOK;
}


ErrVal
MCTFPreProcessorTest::xWrite( PicBufferList&  rcPicBufferList )
{
  UInt auiCropping[4] = {0,0,0,0};

  while( ! rcPicBufferList.empty() )
  {
    PicBuffer*  pcBuffer  = rcPicBufferList.popFront();
    Pel*        pcBuf     = pcBuffer->getBuffer();
    RNOK( m_pcWriteYuv->writeFrame( pcBuf + m_uiLumOffset,
                                    pcBuf + m_uiCbOffset,
                                    pcBuf + m_uiCrOffset,
                                    m_uiHeight,
                                    m_uiWidth,
                                    m_uiStride, auiCropping ) );
  }
  return Err::m_nOK;
}


