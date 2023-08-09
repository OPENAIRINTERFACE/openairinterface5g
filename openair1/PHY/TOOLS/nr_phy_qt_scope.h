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
 * Authors and copyright: Bo Zhao, Marwan Hammouda, Thomas Schlichter (Fraunhofer IIS)
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

#ifndef QT_SCOPE_MAINWINDOW_H
#define QT_SCOPE_MAINWINDOW_H

#include <QtCharts>

extern "C" {
#include <simple_executable.h>
#include <common/utils/system.h>
#include "common/ran_context.h"
#include <openair1/PHY/defs_gNB.h>
#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_RU.h"
#include "executables/softmodem-common.h"
#include "phy_scope_interface.h"
#include <openair2/LAYER2/NR_MAC_gNB/mac_proto.h>
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"

extern RAN_CONTEXT_t RC;
}

/// This is an enum class for the different gNB plot types
enum class PlotTypeGnb {
  empty,
  waterFall,
  CIR,
  puschLLR,
  puschIQ,
  puschSNR,
  puschBLER,
  puschMCS,
  puschRETX,
  puschThroughput,
  pdschSNR,
  pdschBLER,
  pdschMCS,
  pdschRETX,
  pdschThroughput,
  pdschRBs,
  config
};

/// This is an enum class fpr the different UE plot types
enum class PlotTypeUE {
  empty,
  waterFall,
  CIR,
  pbchLLR,
  pbchIQ,
  pdcchLLR,
  pdcchIQ,
  pdschLLR,
  pdschIQ,
  pdschSNR,
  pdschBLER,
  pdschMCS,
  pdschThroughput,
  pdschRBs,
  frequencyOffset,
  timingAdvance,
  config
};

/// This abstract class defines an interface how the KPIPlot class can access values for the different KPI plot types
class ValueProvider {
 public:
  /// This pure virtual function is meant to provide the KPI value to be plotted
  virtual float getValue() = 0;
};

class ValueProviderUE : public ValueProvider {
 public:
  /// This pure virtual function is meant to provide the graph values to be plotted
  virtual scopeGraphData_t *getPlotValue()
  {
    return nullptr;
  }
};

/// Class for emitting a queued signal to update plots
class PlotUpdater : public QObject {
  Q_OBJECT

 signals:
  void updatePlot(int numElt);
};

/// An editable GUI field for a dialog box to set certain KPI configurations
class ConfigBoxFloat : public QLineEdit {
  Q_OBJECT

 public:
  /// Constructor
  /// \param valuePtr Pointer to a float value, which can be edited through this ConfigBoxFloat
  /// \param parent Optional pointer to parent QWidget
  ConfigBoxFloat(float *valuePtr, QWidget *parent = nullptr);

 public slots:
  /// This function converts the value in the editable GUI field to float and writes it back to valuePtr
  void readText();

 private:
  /// Member variable keeping the pointer to the float value
  float *valuePtr;
};

/// Dialog box for configuring gNB KPI Limits
class KPIConfigGnb : public QWidget {
  Q_OBJECT

 public:
  /// Constructor
  /// \param parent Optional pointer to parent QWidget
  explicit KPIConfigGnb(QWidget *parent = nullptr);
};

/// Dialog box for configuring UE KPI Limits
class KPIConfigUE : public QWidget {
  Q_OBJECT

 public:
  /// Constructor
  /// \param parent Optional pointer to parent QWidget
  explicit KPIConfigUE(QWidget *parent = nullptr);
};

/// drop-down list gNB
class KPIListSelectGnb : public QComboBox {
  Q_OBJECT

 public:
  /// Constructor
  /// \param parent Optional pointer to parent QWidget
  explicit KPIListSelectGnb(QWidget *parent = nullptr);
};

/// drop-down list UE
class KPIListSelectUE : public QComboBox {
  Q_OBJECT

 public:
  /// Constructor
  /// \param parent Optional pointer to parent QWidget
  explicit KPIListSelectUE(QWidget *parent = nullptr);
};

/// Waterfall plot of RX signal power
class WaterFall : public QWidget {
  Q_OBJECT

 public:
  /// Constructor
  /// \param values Pointer to the digital I/Q samples
  /// \param frame_parms Pointer to the NR_DL_FRAME_PARMS
  /// \param parent Optional pointer to parent QWidget
  WaterFall(complex16 *values, NR_DL_FRAME_PARMS *frame_parms, QWidget *parent = nullptr);

 protected:
  /// This function is triggered when the own timer expires. It reads data from values and updates the image
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

  /// This function is used to draw the image on the GUI
  /// \param event Pointer to the paint event
  virtual void paintEvent(QPaintEvent *event) override;

  /// This function is called to change the WaterFall size
  /// \param event Pointer to the resize event
  virtual void resizeEvent(QResizeEvent *event) override;

 private:
  /// Pointer to the digital I/Q samples
  complex16 *values;

  /// Pointer to the NR_DL_FRAME_PARMS
  NR_DL_FRAME_PARMS *frame_parms;

  /// Pointer to the image
  QImage *image;

  /// Counter of the drawn lines
  int iteration;

  /// pointer to an array storing per-row average power values
  double *waterFallAvg;
};

/// Chart class for plotting the Channel Impulse Response
class CIRPlot : public QChart {
  Q_OBJECT

 public:
  /// Constructor
  /// \param data Pointer to the CIR data
  /// \param len Length of the CIR data
  CIRPlot(complex16 *data, int len);

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted CIR
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

  /// Pointer to the CIR data
  complex16 *data;

  /// Length of the CIR data
  int len;

  /// Line series used to plot the CIR in the chart
  QLineSeries *series;

  /// Horizontal axis of the chart
  QValueAxis *axisX;

  /// Vertical axis of the chart
  QValueAxis *axisY;
};

class CIRPlotUE : public CIRPlot {
  Q_OBJECT

 public:
  CIRPlotUE(complex16 *data, int len, ValueProviderUE *valueProvider) : CIRPlot(data, len), valueProvider(valueProvider)
  {
  }

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted CIR
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

 private:
  ValueProviderUE *valueProvider;
};

/// Chart class for plotting LLRs
class LLRPlot : public QChart {
  Q_OBJECT

 public:
  /// Constructor
  /// \param data Pointer to the LLR data
  /// \param len Length of the LLR data
  /// \param interval update interval in ms (0 means no timer-triggered updates)
  /// \param plotUpdater pointer to a PlotUpdater for update notifications
  LLRPlot(int16_t *data, int len, int interval = 1000, PlotUpdater *plotUpdater = nullptr);

  /// Destructor
  ~LLRPlot();

 public slots:
  /// This function updates the plotted LLR
  void updatePlot(int len);

 protected:
  /// This function is triggered when the own timer expires. It calls updatePlot() to update the plotted LLR
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

  /// Pointer to the LLR data
  int16_t *data;

  /// Length of the LLR data
  int len;

  /// Pointer to a PlotUpdater for update notifications
  PlotUpdater *plotUpdater;

  /// Scatter series used to plot the LLR in the chart
  QScatterSeries *series;

  /// Horizontal axis of the chart
  QValueAxis *axisX;

  /// Vertical axis of the chart
  QValueAxis *axisY;
};

class LLRPlotUE : public LLRPlot {
  Q_OBJECT

 public:
  LLRPlotUE(int16_t *data, int len, ValueProviderUE *valueProvider) : LLRPlot(data, len), valueProvider(valueProvider)
  {
  }

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted I/Q constellation diagram
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

 private:
  ValueProviderUE *valueProvider;
};

/// Chart class for plotting the I/Q constellation diagram
class IQPlot : public QChart {
  Q_OBJECT

 public:
  /// Constructor
  /// \param data Pointer to the complex I/Q data
  /// \param len Length of the I/Q data
  /// \param interval update interval in ms (0 means no timer-triggered updates)
  /// \param plotUpdater pointer to a PlotUpdater for update notifications
  IQPlot(complex16 *data, int len, int interval = 1000, PlotUpdater *plotUpdater = nullptr);

  /// Destructor
  ~IQPlot();

 public slots:
  /// This function updates the plotted I/Q constellation diagram
  void updatePlot(int len);

 protected:
  /// This function is triggered when the own timer expires. It calls updatePlot() to update the plotted I/Q constellation diagram
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

  /// Pointer to the I/Q data
  complex16 *data;

  /// Length of the I/Q data
  int len;

  /// Pointer to a PlotUpdater for update notifications
  PlotUpdater *plotUpdater;

  /// Scatter series used to plot the I/Q constellation diagram
  QScatterSeries *series;

  /// Horizontal axis of the chart
  QValueAxis *axisX;

  /// Vertical axis of the chart
  QValueAxis *axisY;
};

class IQPlotUE : public IQPlot {
  Q_OBJECT

 public:
  IQPlotUE(complex16 *data, int len, ValueProviderUE *valueProvider) : IQPlot(data, len), valueProvider(valueProvider)
  {
  }

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted I/Q constellation diagram
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

 private:
  /// Pointer to an instance of a class that implements the ValueProvider interface
  ValueProviderUE *valueProvider;
};

/// Generic class for plotting KPI values with min., max. and average bars
class KPIPlot : public QChart {
  Q_OBJECT

 public:
  /// Constructor
  /// \param valueProvider Pointer to an instance of a class that implements the ValueProvider interface
  /// \param limits Optional parameter pointing to an array of two floating point values indicating lower and upper bounds
  KPIPlot(ValueProvider *valueProvider, float *limits = nullptr);

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted KPI chart
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

 private:
  /// Pointer to an instance of a class that implements the ValueProvider interface
  ValueProvider *valueProvider;

  /// Pointer to an array of two floating point values indicating lower and upper bounds
  float *limits;

  /// smallest observed KPI value
  float minValue;

  /// biggest observed KPI value
  float maxValue;

  /// Accumulated KPI valus, used to compute the average
  float sumValue;

  /// Index (horizontal position) of the last plotted KPI value
  int plotIdx;

  /// Line series used to plot the KPI values
  QLineSeries *series;

  /// Line series used to plot an indication of the smallest observed KPI value
  QLineSeries *seriesMin;

  /// Line series used to plot an indication of the biggest observed KPI value
  QLineSeries *seriesMax;

  /// Line series used to plot an indication of the average KPI value
  QLineSeries *seriesAvg;

  /// Horizontal axis of the chart
  QValueAxis *axisX;

  /// Vertical axis of the chart
  QValueAxis *axisY;
};

/// Crart class for plotting HARQ retransmission counters
class RTXPlot : public QChart {
  Q_OBJECT

 public:
  /// Constructor
  /// \param rounds Pointer to the HARQ round counters
  RTXPlot(uint64_t *rounds);

 protected:
  /// This function is triggered when the own timer expires. It updates the plotted HARQ retransmission counters
  /// \param event Pointer to the timer event
  virtual void timerEvent(QTimerEvent *event) override;

 private:
  /// Pointer to the HARQ round counters
  uint64_t *rounds;

  /// Last stored HARQ round counters
  uint64_t lastRounds[4];

  /// Maximum observed HARQ retransmissions
  int maxValue;

  /// Index (horizontal position) of the last plotted HARQ retransmission value
  int plotIdx;

  /// Line series used to plot the four HARQ retransmission counters
  QLineSeries *series[4];

  /// Horizontal axis of the chart
  QValueAxis *axisX;

  /// Vertical axis of the chart
  QValueAxis *axisY;
};

/// Widget showing one selectable gNB KPI
class PainterWidgetGnb : public QWidget, public ValueProvider {
  Q_OBJECT

 public:
  /// Constructor
  /// \param config Pointer to the dialog box for configuring gNB KPI Limits
  /// \param comboBox Pointer to the drop-down list selecting the KPI to be shown here
  /// \param p Pointer to the gNB parameters
  PainterWidgetGnb(QWidget *config, QComboBox *comboBox, scopeData_t *p);

  /// This function provides the current KPI value to be plotted
  virtual float getValue() override;

 protected:
  /// This function is called to change the widget size
  /// \param event Pointer to the resize event
  virtual void resizeEvent(QResizeEvent *event) override;

 public slots:
  /// This function is called when a different KPI is selected
  /// \param type selected KPI type
  void makeConnections(int type);

 private:
  /// Pointer to the dialog box for configuring gNB KPI Limits
  QWidget *config;

  /// Pointer to the drop-down list selecting the KPI to be shown here
  QComboBox *comboBox;

  /// Pointer to the gNB parameters
  scopeData_t *p;

  /// Pointer to a class to view all QChart based KPIs
  QChartView *chartView;

  /// Pointer to the waterfall diagram
  WaterFall *waterFall;

  /// Currently plotted KPI type
  PlotTypeGnb plotType;
};

/// Widget showing one selectable UE KPI
class PainterWidgetUE : public QWidget, public ValueProviderUE {
  Q_OBJECT

 public:
  /// Constructor
  /// \param config Pointer to the dialog box for configuring UE KPI Limits
  /// \param comboBox Pointer to the drop-down list selecting the KPI to be shown here
  /// \param ue Pointer to the UE parameters
  PainterWidgetUE(QWidget *config, QComboBox *comboBox, PHY_VARS_NR_UE *ue);

  /// This function provides the current KPI value to be plotted
  virtual float getValue() override;

  virtual scopeGraphData_t *getPlotValue() override;

 protected:
  /// This function is called to change the widget size
  /// \param event Pointer to the resize event
  virtual void resizeEvent(QResizeEvent *event) override;

 public slots:
  /// This function is called when a different KPI is selected
  /// \param type selected KPI type
  void makeConnections(int type);

 private:
  /// Pointer to the dialog box for configuring UE KPI Limits
  QWidget *config;

  /// Pointer to the drop-down list selecting the KPI to be shown here
  QComboBox *comboBox;

  /// Pointer to the UE parameters
  PHY_VARS_NR_UE *ue;

  /// Pointer to a class to view all QChart based KPIs
  QChartView *chartView;

  /// Pointer to the waterfall diagram
  WaterFall *waterFall;

  /// Currently plotted KPI type
  PlotTypeUE plotType;
};

#endif // QT_SCOPE_MAINWINDOW_H
