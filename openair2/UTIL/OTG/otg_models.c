/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file otg_models.c
* \brief function containing the OTG TX traffic generation functions
* \author M. Laner and navid nikaein
* \date 2013
* \version 0.1
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#include "UTIL/MATH/oml.h"
#include "otg_models.h"
#include "UTIL/LOG/log.h"

double backgroundRateRnd(void);


/*all the TARMA stuff:
  this is based on the modelling framework provided in
  "M. Laner, P. Svoboda and M. Rupp, Modeling Randomness in Network Traffic, SIGMETRICS/Performance'12, London, UK, 2012".

*/

/* this function must be called for each random process  and each time it shall produce a new random sample,
 which is the return value
 input:   - inputSamples: an array of length TARMA_NUM_PROCESSES with iid gaussian samples
        which must be the same for xcorrelated processes
      - proc: pointer to tarmaProcess_t
 output:  - the next value in the random process
*/
double tarmaCalculateSample( double inputSamples[], tarmaProcess_t *proc)
{
  /* struct with all definitions of the random process and its history */
  int cnt;
  double X, Y, Ypower, Z;

  /* calculate the current sample X' by weighting the input samples */
  X=0;

  for( cnt=0; cnt<TARMA_NUM_PROCESSES ; cnt++) {
    X += proc->inputWeight[cnt]*inputSamples[cnt];
  }

  /* update the MA history: shift everything back for one lag */
  for( cnt=TARMA_NUM_MA_MAX-1; cnt>0; cnt--) {
    proc->maHist[cnt] = proc->maHist[cnt-1];
  }

  proc->maHist[0]=X;  /* update the MA history with the current filter input value */

  /* update the AR history: shift everything back for one lag */
  for( cnt=TARMA_NUM_AR_MAX-1; cnt>0; cnt--) {
    proc->arHist[cnt] = proc->arHist[cnt-1];
  }

  proc->arHist[0]=0;  /* caution: arHist[0] must not be used for the filter processing! */

  /* filter processing */
  Y=0;

  for( cnt=0; cnt<TARMA_NUM_MA_MAX; cnt++) { /* moving average part, start at lag 0 = current X */
    Y += proc->maWeight[cnt] * proc->maHist[cnt];
  }

  for( cnt=1; cnt<TARMA_NUM_AR_MAX; cnt++) { /* auto regressive part, start at lag 1 = former Y */
    Y += -proc->arWeight[cnt] * proc->arHist[cnt];
  }

  proc->arHist[0]=Y;  /* update the AR history with the current filter output value */

  /* polynomial transformation */
  Z=0;
  Ypower=1;

  for(cnt=0; cnt<TARMA_NUM_POLY_MAX; cnt++) {
    Z += proc->polyWeight[cnt]*Ypower;
    Ypower *= Y;
  }

  LOG_D(OTG,"TARMA_DEBUG: tarmaCalculateSamples called: Z=%f\n",Z);
  //tarmaPrintProc(proc);
  return Z;
}

/*
 this function must be called each time before size and itd are generated for the respective packet.
  it updates the input samples stored in the structure stream,
  ps: calling it more often than for each packet is also fine
 input:   - the stream to be updated
 output:
 */
void tarmaUpdateInputSample (tarmaStream_t *stream)
{
  int cnt;
  LOG_T(OTG,"TARMA_DEBUG: tarmaUpdateInputSample(%p)\n", stream);

  if(stream) {
    for(cnt=0; cnt<TARMA_NUM_PROCESSES; cnt++) {
      stream->tarma_input_samples[cnt]=gaussian_dist(10000,1)-10000;
      LOG_D(OTG,"TARMA_DEBUG:   %f\n",stream->tarma_input_samples[cnt]);
    }
  }
}

/*
 this function initializes the given stream, it allocates the respective memory if necessary (stream=0)
  and sets all the respective values to zero.
 input:   - steam to be initialized (or zero if memory shall be allocated)
 output:  - the same stream
 */
tarmaStream_t *tarmaInitStream(tarmaStream_t *stream)
{
  if(stream==0) {
    stream=(tarmaStream_t*) malloc(sizeof(tarmaStream_t));
  }

  tarmaProcess_t *proc;
  int cntvar, cntp, cntma, cntar, cntpy;

  for(cntvar=0; cntvar<2; cntvar++) {
    if(cntvar==0) {
      proc=&(stream->tarma_idt);
    } else {
      proc=&(stream->tarma_size);
    }

    for(cntp=0; cntp<TARMA_NUM_PROCESSES; cntp++) {
      proc->inputWeight[cntp]=0;
    }

    for(cntma=0; cntma<TARMA_NUM_MA_MAX; cntma++) {
      proc->maWeight[cntma]=0;
      proc->maHist[cntma]=0;
    }

    for(cntar=0; cntar<TARMA_NUM_AR_MAX; cntar++) {
      proc->arWeight[cntar]=0;
      proc->arHist[cntar]=0;
    }

    for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
      proc->polyWeight[cntpy]=0;
    }
  }

  LOG_D(OTG,"TARMA_DEBUG: tarmaInitStream(%p) called\n", stream);
  return stream;
}

/*
 this function initializes the input stream according to the openarena traffic modelling
 input:   - stream to be initialized
 output:
 */
void tarmaSetupOpenarenaDownlink(tarmaStream_t *stream)
{
  stream->tarma_size.inputWeight[0]=1;
  stream->tarma_size.maWeight[0]=0.6;
  stream->tarma_size.maWeight[1]=-1.04;
  stream->tarma_size.maWeight[2]=0.44;
  stream->tarma_size.arWeight[0]=1;
  stream->tarma_size.arWeight[1]=-1.971;
  stream->tarma_size.arWeight[2]=0.971;
  stream->tarma_size.polyWeight[0]=157;
  stream->tarma_size.polyWeight[1]=67.2;
  stream->tarma_size.polyWeight[2]=-11.2;
  stream->tarma_size.polyWeight[3]=-1.9;
  stream->tarma_size.polyWeight[4]=18.6;
  stream->tarma_size.polyWeight[5]=1.8;
  //tarmaPrintStreamInit(stream);
}

/*
 this function prints the actual history of a random process
 input:   - random process
 output:
 */
void tarmaPrintProc(tarmaProcess_t *proc)
{
  char prefix[]="OTG TARMA DEBUG: ";
  int cntma, cntar;

  printf("%s tarmaPrintProc(%p) called\n", prefix, proc);
  printf("%s     ma history:\n",prefix);

  for(cntma=0; cntma<TARMA_NUM_MA_MAX; cntma++) {
    printf("%s       ma[%d]: %f\n",prefix,cntma,proc->maHist[cntma]);
  }

  printf("%s     ar history:\n",prefix);

  for(cntar=0; cntar<TARMA_NUM_AR_MAX; cntar++) {
    printf("%s       ar[%d]: %f\n",prefix,cntar,proc->arHist[cntar]);
  }
}

/*
 this function prints the fixed (weigths) parameters of the input stream
 input:   - stream to be printed
 output:
 */
void tarmaPrintStreamInit(tarmaStream_t *stream)
{
  char prefix[]="OTG TARMA DEBUG: ";
  tarmaProcess_t *proc;
  int cntvar, cntp, cntma, cntar, cntpy;

  printf("%s tarmaPrintStreamInit(%p) called\n", prefix, stream);

  for(cntvar=0; cntvar<2; cntvar++) {
    if(cntvar==0) {
      proc=&(stream->tarma_idt);
      printf("%s     variable: idt\n",prefix);
    } else {
      proc=&(stream->tarma_size);
      printf("%s     variable: size\n",prefix);
    }

    printf("%s       input processes\n",prefix);

    for(cntp=0; cntp<TARMA_NUM_PROCESSES; cntp++) {
      printf("%s         w_pr[%d]: %f\n",prefix,cntp,proc->inputWeight[cntp]);
    }

    printf("%s       moving average weights\n",prefix);

    for(cntma=0; cntma<TARMA_NUM_MA_MAX; cntma++) {
      printf("%s         w_ma[%d]: %f\n",prefix,cntma,proc->maWeight[cntma]);
    }

    printf("%s       auto-regressive weights\n",prefix);

    for(cntar=0; cntar<TARMA_NUM_AR_MAX; cntar++) {
      printf("%s         w_ar[%d]: %f\n",prefix,cntar,proc->arWeight[cntar]);
    }

    printf("%s       polynomial factors\n",prefix);

    for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
      printf("%s         w_py[%d]: %f\n",prefix,cntpy,proc->polyWeight[cntpy]);
    }
  }
}


/*
 everything about video modeling which is an extended version of tarma modeling
 */

/*
 this function calculates the packet size (frame size) for each video frame,
  the idt is always fixed to the framerate (i.e., between 20 and 50 fps, default=25)
 input:   - vidoe stream
 output:  - frame size
 */
double tarmaCalculateVideoSample(tarmaVideo_t *video)
{
  double size=0;
  double inputsamples[TARMA_NUM_PROCESSES];
  tarmaProcess_t *proc;
  int frameidx;
  int cntp, cntpy;

  if(video) {
    proc=&(video->tarma_size);
    frameidx=video->tarmaVideoGopStructure[video->tarmaVideoFrameNumber];
    LOG_D(OTG,"TARMA_DEBUG: tarmaCalculateVideoSample(%p) called\n", video);
    LOG_D(OTG,"TARMA_DEBUG:     frameidx=%d\n",frameidx);

    if(frameidx>=0 && frameidx<TARMA_NUM_FRAME_TYPES) {
      for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
        proc->polyWeight[cntpy]=video->polyWeightFrame[frameidx][cntpy];
      }

      for(cntp=0; cntp<TARMA_NUM_PROCESSES; cntp++) {
        inputsamples[cntp]=gaussian_dist(10000,1)-10000;
      }

      size=tarmaCalculateSample(inputsamples,proc);
    }

    /*get ready for the next frame*/
    video->tarmaVideoFrameNumber++;

    if(video->tarmaVideoFrameNumber>=TARMA_NUM_GOP_MAX) {
      video->tarmaVideoFrameNumber=0;
    }

    if(video->tarmaVideoGopStructure[video->tarmaVideoFrameNumber]<0 ||
        video->tarmaVideoGopStructure[video->tarmaVideoFrameNumber]>TARMA_NUM_FRAME_TYPES) {
      video->tarmaVideoFrameNumber=0;
    }
  }

  return size;
}

/*
 this function initializes a video stream (and allocates memory if needed)
 input:   - video stream (or zero if memory should be allocated)
 output:  - the same video stream
 */
tarmaVideo_t *tarmaInitVideo(tarmaVideo_t *video)
{
  tarmaProcess_t *proc;
  int cntp, cntma, cntar, cntpy, cntgop, cnttype;

  LOG_D(OTG,"TARMA_DEBUG: tarmaInitVideo(%p) called\n", video);

  if(video==0) {
    video=(tarmaVideo_t*) malloc(sizeof(tarmaVideo_t));
  }

  proc=&(video->tarma_size);

  for(cntp=0; cntp<TARMA_NUM_PROCESSES; cntp++) {
    proc->inputWeight[cntp]=0;
  }

  for(cntma=0; cntma<TARMA_NUM_MA_MAX; cntma++) {
    proc->maWeight[cntma]=0;
    proc->maHist[cntma]=0;
  }

  for(cntar=0; cntar<TARMA_NUM_AR_MAX; cntar++) {
    proc->arWeight[cntar]=0;
    proc->arHist[cntar]=0;
  }

  for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
    proc->polyWeight[cntpy]=0;
  }

  video->tarmaVideoFrameNumber=0;

  for(cntgop=0; cntgop<TARMA_NUM_GOP_MAX; cntgop++) {
    video->tarmaVideoGopStructure[cntgop]=-1;
  }

  for(cnttype=0; cnttype<TARMA_NUM_FRAME_TYPES; cnttype++) {
    for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
      video->polyWeightFrame[cnttype][cntpy]=0;
    }
  }

  return video;
}

/*
 this function initializes a video stream according to a tarma model
 input:   - video to be initialized
      - compression rate: 1...dvd quality
 output:
 */
void tarmaSetupVideoGop12(tarmaVideo_t *video, double compression)
{
  if(video) {
    video->tarma_size.inputWeight[0]=1;
    video->tarma_size.maWeight[0]=0.47;
    video->tarma_size.maWeight[1]=-0.829;
    video->tarma_size.maWeight[2]=0.358;
    video->tarma_size.arWeight[0]=1;
    video->tarma_size.arWeight[1]=-1.984;
    video->tarma_size.arWeight[2]=0.984;
    video->tarmaVideoGopStructure[0]=0; /*i frame*/
    video->tarmaVideoGopStructure[1]=2; /*b frame*/
    video->tarmaVideoGopStructure[2]=2;
    video->tarmaVideoGopStructure[3]=1; /*p frame*/
    video->tarmaVideoGopStructure[4]=2;
    video->tarmaVideoGopStructure[5]=2;
    video->tarmaVideoGopStructure[6]=1;
    video->tarmaVideoGopStructure[7]=2;
    video->tarmaVideoGopStructure[8]=2;
    video->tarmaVideoGopStructure[9]=1;
    video->tarmaVideoGopStructure[10]=2;
    video->tarmaVideoGopStructure[11]=2;

    if(compression<1) {
      compression=1;
    }

    video->polyWeightFrame[0][0]=55400/compression; /*i frame*/
    video->polyWeightFrame[0][1]=32300/compression;
    video->polyWeightFrame[0][2]=10400/compression;
    video->polyWeightFrame[0][3]=-100/compression;
    video->polyWeightFrame[0][4]=-800/compression;
    video->polyWeightFrame[0][5]=74/compression;
    video->polyWeightFrame[1][0]=20900/compression; /*p frame*/
    video->polyWeightFrame[1][1]=19100/compression;
    video->polyWeightFrame[1][2]=7150/compression;
    video->polyWeightFrame[1][3]=-740/compression;
    video->polyWeightFrame[1][4]=-130/compression;
    video->polyWeightFrame[1][5]=20/compression;
    video->polyWeightFrame[2][0]=11700/compression; /*b frame*/
    video->polyWeightFrame[2][1]=10300/compression;
    video->polyWeightFrame[2][2]=4320/compression;
    video->polyWeightFrame[2][3]=700/compression;
    video->polyWeightFrame[2][4]=-90/compression;
    video->polyWeightFrame[2][5]=-2/compression;
    tarmaPrintVideoInit(video);
  }
}

/*
 this function prints the fixed (weigths) parameters of the input video stream
 input:   - video stream to be printed
 output:
 */
void tarmaPrintVideoInit(tarmaVideo_t *video)
{
  char prefix[]="OTG TARMA DEBUG: ";
  tarmaProcess_t *proc;
  int cntp, cntma, cntar, cntpy, cntgop, cnttype;

  printf("%s tarmaPrintVideoInit(%p) called\n", prefix, video);
  proc=&(video->tarma_size);
  printf("%s      input process weights\n",prefix);

  for(cntp=0; cntp<TARMA_NUM_PROCESSES; cntp++) {
    printf("%s        w[%d]=%f\n",prefix,cntp,proc->inputWeight[cntp]);
  }

  printf("%s      ma weights\n",prefix);

  for(cntma=0; cntma<TARMA_NUM_MA_MAX; cntma++) {
    printf("%s        ma[%d]=%f\n",prefix,cntma,proc->maWeight[cntma]);
  }

  printf("%s      ar weights\n",prefix);

  for(cntar=0; cntar<TARMA_NUM_AR_MAX; cntar++) {
    printf("%s        ar[%d]=%f\n",prefix,cntar,proc->arWeight[cntar]);
  }

  printf("%s      polynomial weights\n",prefix);

  for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
    printf("%s        p[%d]=%f\n",prefix,cntpy,proc->polyWeight[cntpy]);
  }

  printf("%s      video frame number=%d\n",prefix,video->tarmaVideoFrameNumber);
  printf("%s      gop structure\n",prefix);

  for(cntgop=0; cntgop<TARMA_NUM_GOP_MAX; cntgop++) {
    printf("%s        gop[%d]=%d\n",prefix,cntgop,video->tarmaVideoGopStructure[cntgop]);
  }

  printf("%s      frame poly weights\n",prefix);

  for(cntpy=0; cntpy<TARMA_NUM_POLY_MAX; cntpy++) {
    printf("%s      [%d] ",prefix,cntpy);

    for(cnttype=0; cnttype<TARMA_NUM_FRAME_TYPES; cnttype++) {
      printf("     %05.3f",video->polyWeightFrame[cnttype][cntpy]);
    }

    printf("\n");
  }
}





/*
 everything about background traffic modeling according to
 "M. Laner, P. Svoboda, S. Schwarz, M. Rupp, Users in Cells: a Data Traffic Analysis, WCNC'12, Paris, France, 2012".
 */

/*
 this function creates a random realization of the mean data rate of a background-session
 input:
 output:  - random realization of R_s (B/s)
 */
double backgroundRateRnd(void)
{
  double rate;
  rate=pow(10,lognormal_dist(1.3525, 0.1954))/8; /*Byte/s*/;

  if(rate>BACKGROUND_RATE_MAX) {
    rate=BACKGROUND_RATE_MAX;
  } else if(rate<0) {
    rate=0;
  }

  return rate;
}

/*
 this function creates a random realization of the session duration of a background-session
 input:
 output:  - random realization of D_s (ms)
 */
int backgroundSessionDurationRnd(void)
{
  int duration;
  duration = ceil(pow(10,exponential_dist(1/0.3591))*1000); /*ms*/

  if(duration<0) {
    duration=0;
  } else if(duration>1<<30) {
    duration=1<<30;
  }

  return duration;
}

/*
 this function initializes a background stream and allocates memory if needed
 input:   - the bg-stream (or zero if memory shall be allocated)
      - the mean number of users (sessions) to be expected for this stream
 output:  - the same bg-stream
 */
backgroundStream_t *backgroundStreamInit(backgroundStream_t *stream, double lambda_n)
{
  int cnts, numactivenow;

  if(stream==0) {
    stream=(backgroundStream_t*) malloc(sizeof(backgroundStream_t));
  }

  stream->meanNumSessions=lambda_n;
  stream->lastUpdateTime=0;
  numactivenow=poisson_dist(lambda_n); /*how many sessions are active (i.e., started before simulation)*/

  for(cnts=0; cnts<BACKGROUND_NUM_ACTIVE_MAX; cnts++) {
    if(cnts<numactivenow) {
      /*these sessions are already active for unknown time and have therefore a random phaseshift in ds*/
      stream->activeSessions[cnts].meanSessionRate=backgroundRateRnd();
      stream->activeSessions[cnts].endTime=ceil((double)backgroundSessionDurationRnd()*uniform_rng()); /*ms*/
    } else {
      stream->activeSessions[cnts].meanSessionRate=0;
      stream->activeSessions[cnts].endTime=-1;
    }
  }

  LOG_D(OTG,"BACKGROUND_USERS DEBUG: backgroundStreamInit(%p) called\n", stream);
  backgroundPrintStream (stream);
  return stream;
}

/*
 this function updates a given background stream
 input:   - bg-stream
      - current time
 output:
 */
void backgroundUpdateStream(backgroundStream_t *stream, int ctime)
{
  int numNewSessions, cnts, period;

  LOG_D(OTG,"BACKGROUND DEBUG: backgroundUpdateStream(stream*=%p,ctime=%d) called\n", stream, ctime);

  if(stream) {
    period=ctime-stream->lastUpdateTime;
    numNewSessions=poisson_dist(stream->meanNumSessions/5710*period);

    for(cnts=0; cnts<BACKGROUND_NUM_ACTIVE_MAX; cnts++) {
      if(stream->activeSessions[cnts].endTime<ctime) {
        if(numNewSessions>0) {
          stream->activeSessions[cnts].meanSessionRate=backgroundRateRnd();
          stream->activeSessions[cnts].endTime=
            ctime-period*uniform_rng()+backgroundSessionDurationRnd(); /*ms*/
          numNewSessions--;
        } else {
          stream->activeSessions[cnts].meanSessionRate=0;
          stream->activeSessions[cnts].endTime=-1;
        }
      }
    }

    stream->lastUpdateTime=ctime;
    backgroundPrintStream (stream);
  }
}

/*
 this function generates the packet size for a given bg-stream and a given idt,
  the distribution of the idt can thereby be arbitrary (const idt=20ms recommended)
 input:   - bg-stream
        - current time
        - idt of the current packet
 output:  - packet size of the current packet
 */
double backgroundCalculateSize(backgroundStream_t *stream, int ctime, int idt)
{
  int cnts, cntact=0;
  double size=0;
  double mrate=0;

  backgroundUpdateStream(stream, ctime);
  LOG_D(OTG,"BACKGROUND DEBUG: backgroundCalculateSize(stream*=%p,idt=%d,ctime=%d) called\n", stream, idt, ctime);

  if(stream) {
    for(cnts=0; cnts<BACKGROUND_NUM_ACTIVE_MAX; cnts++) {
      if(stream->activeSessions[cnts].endTime>ctime) {
        mrate+=stream->activeSessions[cnts].meanSessionRate;
        cntact++;
      }
    }

    size=mrate*idt/1000;
    LOG_D(OTG,"BACKGROUND DEBUG:     cntact=%02d, idt=%05d, agg_mrate=%05.1f, size=%04.1f\n", cntact, idt, mrate,size);
  }

  return size;
}

/*
 this function prints the current status of the bg-stream
 input:   - bg-stream
 output:
 */
void backgroundPrintStream(backgroundStream_t *stream)
{
  int cnts;

  LOG_D(OTG,"BACKGROUND DEBUG: backgroundPrintStream(%p)\n", stream);

  if(stream) {
    LOG_D(OTG,"BACKGROUND DEBUG:     meanNumSessions(lambda_n)=%f\n",stream->meanNumSessions);

    for(cnts=0; cnts<BACKGROUND_NUM_ACTIVE_MAX; cnts++) {
      LOG_D(OTG,"BACKGROUND DEBUG:       session[%d] -> mrate=%06.3f, etime=%05d\n",
            cnts, stream->activeSessions[cnts].meanSessionRate, stream->activeSessions[cnts].endTime);
    }
  }
}



