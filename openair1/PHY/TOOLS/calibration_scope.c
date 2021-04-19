#include <stdlib.h>
#include <openair1/PHY/impl_defs_top.h>
#include "executables/softmodem-common.h"
#include "executables/nr-softmodem-common.h"
#include <forms.h>
#include <openair1/PHY/TOOLS/calibration_scope.h>

#define TPUT_WINDOW_LENGTH 100
#define ScaleZone 4

const FL_COLOR rx_antenna_colors[4] = {FL_RED,FL_BLUE,FL_GREEN,FL_YELLOW};
const FL_COLOR water_colors[4] = {FL_BLUE,FL_GREEN,FL_YELLOW,FL_RED};

typedef struct {
  int16_t r;
  int16_t i;
} scopeSample_t;
#define SquaredNorm(VaR) ((VaR).r*(VaR).r+(VaR).i*(VaR).i)
typedef struct {
  void ** samplesRx;
  openair0_device *rfdevice;
} calibData_t;

typedef struct OAIgraph {
  FL_OBJECT *graph;
  FL_OBJECT *text;
  float maxX;
  float maxY;
  float minX;
  float minY;
  int x;
  int y;
  int w;
  int h;
  int waterFallh;
  double *waterFallAvg;
  boolean_t initDone;
  int iteration;
  void (*funct) (struct OAIgraph *graph, calibData_t *);
} OAIgraph_t;


/* Forms and Objects */
typedef struct {
  calibData_t * context;
  FL_FORM    *phy_scope;
  OAIgraph_t graph[20];
  FL_OBJECT *button_0;
} OAI_phy_scope_t;

typedef struct {
  FL_FORM    *stats_form;
  void       *vdata;
  char       *cdata;
  long        ldata;
  FL_OBJECT *stats_text;
  FL_OBJECT *stats_button;
} FD_stats_form;

static void drawsymbol(FL_OBJECT *obj, int id,
                       FL_POINT *p, int n, int w, int h) {
  fl_points( p, n, FL_YELLOW);
}

#define WATERFALL 10000

static void commonGraph(OAIgraph_t *graph, int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label, FL_COLOR pointColor) {
  if (type==WATERFALL) {
    graph->waterFallh=h-15;
    graph->waterFallAvg=malloc(sizeof(*graph->waterFallAvg) * graph->waterFallh);

    for (int i=0; i< graph->waterFallh; i++)
      graph->waterFallAvg[i]=0;

    graph->graph=fl_add_canvas(FL_NORMAL_CANVAS, x, y, w, graph->waterFallh, label);
    graph->text=fl_add_text(FL_NORMAL_TEXT, x, y+graph->waterFallh, w, 15, label);
    fl_set_object_lcolor(graph->text,FL_WHITE);
    fl_set_object_color(graph->text, FL_BLACK, FL_BLACK);
    fl_set_object_lalign(graph->text, FL_ALIGN_CENTER );
  } else {
    graph->graph=fl_add_xyplot(type, x, y, w, h, label);
    fl_set_object_lcolor(graph->graph, FL_WHITE ); // Label color
    fl_set_object_color(graph->graph, FL_BLACK, pointColor);

    for (int i=0; i< FL_MAX_XYPLOTOVERLAY; i++)
      fl_set_xyplot_symbol(graph->graph, i, drawsymbol);
  }

  graph->x=x;
  graph->y=y;
  graph->w=w;
  graph->h=h;
  graph->maxX=0;
  graph->maxY=0;
  graph->minX=0;
  graph->minY=0;
  graph->initDone=false;
  graph->iteration=0;
}

static OAIgraph_t calibrationCommonGraph( void (*funct) (OAIgraph_t *graph, calibData_t *context),
                                  int type, FL_Coord x, FL_Coord y, FL_Coord w, FL_Coord h, const char *label, FL_COLOR pointColor) {
  OAIgraph_t graph;
  commonGraph(&graph, type, x, y, w, h, label, pointColor);
  graph.funct=funct;
  return graph;
}


static void setRange(OAIgraph_t *graph, float minX, float maxX, float minY, float maxY) {
  if ( maxX > graph->maxX ||  minX < graph->minX ||
       abs(maxX-graph->maxX)>abs(graph->maxX)/2 ||
       abs(maxX-graph->maxX)>abs(graph->maxX)/2 ) {
    graph->maxX/=2;
    graph->minX/=2;
    graph->maxX=max(graph->maxX,maxX);
    graph->minX=min(graph->minX,minX);
    fl_set_xyplot_xbounds(graph->graph, graph->minX*1.2, graph->maxX*1.2);
  }

  if ( maxY > graph->maxY || minY < graph->minY ||
       abs(maxY-graph->maxY)>abs(graph->maxY)/2 ||
       abs(maxY-graph->maxY)>abs(graph->maxY)/2 ) {
    graph->maxY/=2;
    graph->minY/=2;
    graph->maxY=max(graph->maxY,maxY);
    graph->minY=min(graph->minY,minY);
    fl_set_xyplot_ybounds(graph->graph, graph->minY*1.2, graph->maxY*1.2);
  }
}

static void oai_xygraph_getbuff(OAIgraph_t *graph, float **x, float **y, int len, int layer) {
  float *old_x;
  float *old_y;
  int old_len=-1;

  if (graph->iteration >1)
    fl_get_xyplot_data_pointer(graph->graph, layer, &old_x, &old_y, &old_len);

  if (old_len != len) {
    LOG_W(HW,"allocating graph of %d scope\n", len);
    float values[len];
    float time[len];

    // make time in case we will use it
    for (int i=0; i<len; i++)
      time[i] = values[i] = i;

    if (layer==0)
      fl_set_xyplot_data(graph->graph,time,values,len,"","","");
    else
      fl_add_xyplot_overlay(graph->graph,layer,time,values,len,rx_antenna_colors[layer]);

    fl_get_xyplot_data_pointer(graph->graph, layer, &old_x, &old_y, &old_len);
    AssertFatal(old_len==len,"");
  }

  *x=old_x;
  *y=old_y;
}

static void oai_xygraph(OAIgraph_t *graph, float *x, float *y, int len, int layer, boolean_t NoAutoScale) {
  fl_redraw_object(graph->graph);

  if ( NoAutoScale && graph->iteration%NoAutoScale == 0) {
    float maxX=0, maxY=0, minX=0, minY=0;

    for (int k=0; k<len; k++) {
      maxX=max(maxX,x[k]);
      minX=min(minX,x[k]);
      maxY=max(maxY,y[k]);
      minY=min(minY,y[k]);
    }

    setRange(graph, minX-5, maxX+5, minY-5, maxY+5);
  }

  graph->iteration++;
}

static void genericWaterFall (OAIgraph_t *graph, scopeSample_t *values, const int datasize, const int divisions, const char *label) {
  if ( values == NULL )
     return;
  fl_winset(FL_ObjWin(graph->graph));
  const int samplesPerPixel=datasize/graph->w;
  int displayPart=graph->waterFallh-ScaleZone;
  int row=graph->iteration%displayPart;
  double avg=0;

  for (int i=0; i < displayPart; i++)
    avg+=graph->waterFallAvg[i];

  avg/=displayPart;
  graph->waterFallAvg[row]=0;

  for (int pix=0; pix<graph->w; pix++) {
    scopeSample_t *end=values+(pix+1)*samplesPerPixel;
    end-=2;
    AssertFatal(end <= values+datasize,"diff : %ld", end-values+datasize);
    double val=0;

    for (scopeSample_t *s=values+(pix)*samplesPerPixel;
         s <end;
         s++)
      val += SquaredNorm(*s);

    val/=samplesPerPixel;
    graph->waterFallAvg[row]+=val/graph->w;
    int col=0;

    if (val > avg*2 )
      col=1;

    if (val > avg*10 )
      col=2;

    if (val > avg*100 )
      col=3;

    fl_point(pix, graph->iteration%displayPart, water_colors[col]);
  }

  if (graph->initDone==false) {
    for ( int i=0; i < graph->waterFallh; i++ )
      for ( int j = 0 ; j < graph->w ; j++ )
        fl_point(j, i, FL_BLACK);

    for ( int i=1; i<divisions; i++)
      for (int j= displayPart; j<graph->waterFallh; j++)
        fl_point(i*(graph->w/divisions),j, FL_WHITE);

    graph->initDone=true;
  }

  fl_set_object_label_f(graph->text, "%s, avg I/Q pow: %4.1f", label, sqrt(avg));
  graph->iteration++;
}

static void genericPowerPerAntena(OAIgraph_t  *graph, const int nb_ant, const scopeSample_t **data, const int len) {
  float *values, *time;
  oai_xygraph_getbuff(graph, &time, &values, len, 0);

  for (int ant=0; ant<nb_ant; ant++) {
    if (data[ant] != NULL) {
      for (int i=0; i<len; i++) {
        values[i] = SquaredNorm(data[ant][i]);
      }

      oai_xygraph(graph,time,values, len, ant, 10);
    }
  }
}

static void gNBWaterFall (OAIgraph_t *graph, calibData_t *context) {
  //use 1st antenna
  genericWaterFall(graph, (scopeSample_t *)context->samplesRx[0],
                   0, 0,
                   "X axis:one frame in time");
}
static void gNBfreqWaterFall  (OAIgraph_t *graph, calibData_t *context) {
  //use 1st antenna
  genericWaterFall(graph, (scopeSample_t *)context->samplesRx[0],
                   0, 0,
                   "X axis:one frame in time");
}
static void timeResponse (OAIgraph_t *graph, calibData_t *context) {
  #if 0
  const int len=2*phy_vars_gnb->frame_parms.ofdm_symbol_size;
  float *values, *time;
  oai_xygraph_getbuff(graph, &time, &values, len, 0);
  const int ant=0; // display antenna 0 for each UE

  for (int ue=0; ue<nb_UEs; ue++) {
    scopeSample_t *data= (scopeSample_t *)phy_vars_gnb->pusch_vars[ue]->ul_ch_estimates_time[ant];

    if (data != NULL) {
      for (int i=0; i<len; i++) {
        values[i] = SquaredNorm(data[i]);
      }

      oai_xygraph(graph,time,values, len, ue, 10);
    }
  }
  #endif
}

static void puschIQ (OAIgraph_t *graph, calibData_t *context) {
  #if 0
  NR_DL_FRAME_PARMS *frame_parms=&phy_vars_gnb->frame_parms;
  int sz=frame_parms->N_RB_UL*12*frame_parms->symbols_per_slot;

  for (int ue=0; ue<nb_UEs; ue++) {
    scopeSample_t *pusch_comp = (scopeSample_t *) phy_vars_gnb->pusch_vars[ue]->rxdataF_comp[0];
    float *I, *Q;
    oai_xygraph_getbuff(graph, &I, &Q, sz, ue);

    if (pusch_comp) {
      for (int k=0; k<sz; k++ ) {
        I[k] = pusch_comp[k].r;
        Q[k] = pusch_comp[k].i;
      }

      oai_xygraph(graph,I,Q,sz,ue,10);
    }
  }
  #endif
}


static OAI_phy_scope_t *createScopeCalibration(calibData_t * context) {
  FL_OBJECT *obj;
  OAI_phy_scope_t *fdui = calloc(( sizeof *fdui ),1);
  fdui->context=context;
  // Define form
  fdui->phy_scope = fl_bgn_form( FL_NO_BOX, 800, 800 );
  // This the whole UI box
  obj = fl_add_box( FL_BORDER_BOX, 0, 0, 800, 800, "" );
  fl_set_object_color( obj, FL_BLACK, FL_WHITE );
  int curY=0,x,y,w,h;
  // Received signal
  fdui->graph[0] = calibrationCommonGraph( gNBWaterFall, WATERFALL, 0, curY, 400, 100,
                                   "Received Signal (Time-Domain, one frame)", FL_RED );
  
  // Time-domain channel response
  //fdui->graph[1] = calibrationCommonGraph( timeResponse, FL_NORMAL_XYPLOT, 410, curY, 400, 100, "SRS Frequency Response (samples, abs)", FL_RED );
  fl_get_object_bbox(fdui->graph[0].graph,&x, &y,&w, &h);
  curY+=h;
  // Frequency-domain channel response
  fdui->graph[1] = calibrationCommonGraph( gNBfreqWaterFall, WATERFALL, 0, curY, 800, 100,
                                   "Channel Frequency domain (RE, one frame)", FL_RED );
  fl_get_object_bbox(fdui->graph[1].graph,&x, &y,&w, &h);
  curY+=h+20;
  // LLR of PUSCH
  //fdui->graph[3] = calibrationCommonGraph( puschLLR, FL_POINTS_XYPLOT, 0, curY, 500, 200, "PUSCH Log-Likelihood Ratios (LLR, mag)", FL_YELLOW );
  // I/Q PUSCH comp
  fdui->graph[2] = calibrationCommonGraph( puschIQ, FL_POINTS_XYPLOT, 500, curY, 300, 200,
                                   "PUSCH I/Q of MF Output", FL_YELLOW );
  fl_get_object_bbox(fdui->graph[2].graph,&x, &y,&w, &h);
  curY+=h;
  //fl_get_object_bbox(fdui->graph[6].graph,&x, &y,&w, &h);
  curY+=h;
  fdui->graph[3].graph=NULL;
  fl_end_form( );
  fdui->phy_scope->fdui = fdui;
  fl_show_form (fdui->phy_scope, FL_PLACE_HOTSPOT, FL_FULLBORDER, "LTE UL SCOPE gNB");
  return fdui;
}

void calibrationScope(OAI_phy_scope_t  *form) {
  int i=0;

  while (form->graph[i].graph) {
    form->graph[i].funct(form->graph+i, form->context);
    i++;
  }

  //fl_check_forms();
}

static void *scopeThread(void *arg) {
  calibData_t * context = (calibData_t *)arg;
  size_t stksize=0;
  pthread_attr_t atr;
  pthread_attr_init(&atr);
  pthread_attr_getstacksize(&atr, &stksize);
  pthread_attr_setstacksize(&atr,32*1024*1024 );
  sleep(3); // no clean interthread barriers
  int fl_argc=1;
  char *name="Calibration-scope";
  fl_initialize (&fl_argc, &name, NULL, 0, 0);
  OAI_phy_scope_t  *form = createScopeCalibration(context);

  while (!oai_exit) {
    calibrationScope(form);
    usleep(99*1000);
  }

  return NULL;
}

void CalibrationInitScope(void ** samplesRx,openair0_device *rfdevice) {
  pthread_t forms_thread;
  calibData_t * tmp=(calibData_t *) malloc(sizeof(*tmp));
  tmp->samplesRx=samplesRx;
  tmp->rfdevice=rfdevice;
  threadCreate(&forms_thread, scopeThread, (void*) tmp, "scope", -1, OAI_PRIORITY_RT_LOW);
}
