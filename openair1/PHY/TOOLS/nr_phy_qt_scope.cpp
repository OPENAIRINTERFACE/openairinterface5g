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

#include <QApplication>
#include <QtWidgets>
#include <QPainter>
#include <QtGui>
#include <QLineEdit>
#include <QFormLayout>
#include <QtCharts>
#include <QValueAxis>

#include <iostream>
#include <cassert>
#include <cmath>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nr_phy_qt_scope.h"

extern "C" {
#include "PHY/CODING/nrPolar_tools/nr_polar_defs.h"
#include <openair1/PHY/CODING/nrPolar_tools/nr_polar_defs.h>
}

#define ScaleZone 4;
#define SquaredNorm(VaR) ((VaR).r * (VaR).r + (VaR).i * (VaR).i)

/*
@gNB: These are the (default) lower and upper threshold values for BLER and Throughput at the gNB side.
These threshold values can be further updated in run-time through the option 'Configs' in the drop-down list.
*/
float Limits_KPI_gNB[4][2] = {
    // {lower Limit, Upper Limit}
    {0.0, 0.8}, // UL BLER
    {0.2, 10}, // UL Throughput in Mbs
    {0.0, 0.8}, // DL BLER
    {0.2, 10} // DL Throughput in Mbs
};

/*
@UE: These are the (default) lower and upper threshold values for BLER and Throughput at the UE side.
These threshold values can be further updated in run-time through the option 'Configs' in the drop-down list
*/
float Limits_KPI_ue[2][2] = {
    // {lower Limit, Upper Limit}
    {0.0, 0.8}, // DL BLER
    {0.2, 10} // Throughput in Mbs
};

// Plot updater
PlotUpdater puschLlrUpdater;
PlotUpdater puschIqUpdater;

void scopeUpdaterGnb(enum PlotTypeGnbIf plotType, int numElt)
{
  switch (plotType) {
    case puschLLRe:
      puschLlrUpdater.updatePlot(numElt);
      break;
    case puschIQe:
      puschIqUpdater.updatePlot(numElt);
      break;
  }
}

/* This class creates the window when choosing the option 'Configs' to configure the threshold values. */
ConfigBoxFloat::ConfigBoxFloat(float *valuePtr, QWidget *parent) : QLineEdit(parent), valuePtr(valuePtr)
{
  this->setText(QString::number(*valuePtr));
  connect(this, &ConfigBoxFloat::editingFinished, this, &ConfigBoxFloat::readText);
}

/* This function reads the input config values, entered by user, and update the Limits_KPI_* accordignly. */
void ConfigBoxFloat::readText()
{
  QString text_e1 = this->text();
  *this->valuePtr = text_e1.toFloat();
}

/* @gNB: create configuration window */
KPIConfigGnb::KPIConfigGnb(QWidget *parent) : QWidget(parent)
{
  this->resize(300, 300);
  this->setWindowTitle("gNB Configs");

  ConfigBoxFloat *configItem1 = new ConfigBoxFloat(&Limits_KPI_gNB[0][0]);
  ConfigBoxFloat *configItem2 = new ConfigBoxFloat(&Limits_KPI_gNB[0][1]);
  ConfigBoxFloat *configItem3 = new ConfigBoxFloat(&Limits_KPI_gNB[1][0]);
  ConfigBoxFloat *configItem4 = new ConfigBoxFloat(&Limits_KPI_gNB[1][1]);
  ConfigBoxFloat *configItem5 = new ConfigBoxFloat(&Limits_KPI_gNB[2][0]);
  ConfigBoxFloat *configItem6 = new ConfigBoxFloat(&Limits_KPI_gNB[2][1]);
  ConfigBoxFloat *configItem7 = new ConfigBoxFloat(&Limits_KPI_gNB[3][0]);
  ConfigBoxFloat *configItem8 = new ConfigBoxFloat(&Limits_KPI_gNB[3][1]);

  QFormLayout *flo = new QFormLayout(this);
  flo->addRow("U-BLER lower", configItem1);
  flo->addRow("U-BLER upper", configItem2);
  flo->addRow("U-Throughput lower[Mbs]", configItem3);
  flo->addRow("U-Throughput upper[Mbs]", configItem4);
  flo->addRow("D-BLER lower", configItem5);
  flo->addRow("D-BLER upper", configItem6);
  flo->addRow("D-Throughput lower[Mbs]", configItem7);
  flo->addRow("D-Throughput upper[Mbs]", configItem8);
}

/* @UE: create configuration window */
KPIConfigUE::KPIConfigUE(QWidget *parent) : QWidget(parent)
{
  this->resize(300, 300);
  this->setWindowTitle("UE Configs");

  ConfigBoxFloat *configItem1 = new ConfigBoxFloat(&Limits_KPI_ue[0][0]);
  ConfigBoxFloat *configItem2 = new ConfigBoxFloat(&Limits_KPI_ue[0][1]);
  ConfigBoxFloat *configItem3 = new ConfigBoxFloat(&Limits_KPI_ue[1][0]);
  ConfigBoxFloat *configItem4 = new ConfigBoxFloat(&Limits_KPI_ue[1][1]);

  QFormLayout *flo = new QFormLayout(this);
  flo->addRow("BLER lower", configItem1);
  flo->addRow("BLER upper", configItem2);
  flo->addRow("Throughput lower[Mbs]", configItem3);
  flo->addRow("Throughput upper[Mbs]", configItem4);
}

/* @gNB: This class creates the drop-down list at the gNB side. Each item correspinds to an implemented KPI. */
KPIListSelectGnb::KPIListSelectGnb(QWidget *parent) : QComboBox(parent)
{
  this->addItem("- empty -", static_cast<int>(PlotTypeGnb::empty));
  this->addItem("RX Signal-Time", static_cast<int>(PlotTypeGnb::waterFall));
  this->addItem("Channel Response", static_cast<int>(PlotTypeGnb::CIR));

  this->addItem("LLR PUSCH", static_cast<int>(PlotTypeGnb::puschLLR));
  this->addItem("I/Q PUSCH", static_cast<int>(PlotTypeGnb::puschIQ));
  this->addItem("UL SNR", static_cast<int>(PlotTypeGnb::puschSNR));
  this->addItem("UL BLER", static_cast<int>(PlotTypeGnb::puschBLER));
  this->addItem("UL MCS", static_cast<int>(PlotTypeGnb::puschMCS));
  this->addItem("UL Retrans.", static_cast<int>(PlotTypeGnb::puschRETX));
  this->addItem("UL Throughput", static_cast<int>(PlotTypeGnb::puschThroughput));

  this->addItem("DL SNR (CQI)", static_cast<int>(PlotTypeGnb::pdschSNR));
  this->addItem("DL BLER", static_cast<int>(PlotTypeGnb::pdschBLER));
  this->addItem("DL MCS", static_cast<int>(PlotTypeGnb::pdschMCS));
  this->addItem("DL Retrans.", static_cast<int>(PlotTypeGnb::pdschRETX));
  this->addItem("DL Throughput", static_cast<int>(PlotTypeGnb::pdschThroughput));
  this->addItem("Nof Sched. RBs", static_cast<int>(PlotTypeGnb::pdschRBs));

  this->addItem("Configs", static_cast<int>(PlotTypeGnb::config));
}

/* @UE: This class creates the drop-down list at the UE side. Each item correspinds to an implemented KPI. */
KPIListSelectUE::KPIListSelectUE(QWidget *parent) : QComboBox(parent)
{
  this->addItem("- empty -", static_cast<int>(PlotTypeUE::empty));
  this->addItem("RX Signal-Time", static_cast<int>(PlotTypeUE::waterFall));
  this->addItem("Channel Response", static_cast<int>(PlotTypeUE::CIR));

  this->addItem("LLR PBCH", static_cast<int>(PlotTypeUE::pbchLLR));
  this->addItem("I/Q PBCH", static_cast<int>(PlotTypeUE::pbchIQ));
  this->addItem("LLR PDCCH", static_cast<int>(PlotTypeUE::pdcchLLR));
  this->addItem("I/Q PDCCH", static_cast<int>(PlotTypeUE::pdcchIQ));
  this->addItem("LLR PDSCH", static_cast<int>(PlotTypeUE::pdschLLR));
  this->addItem("I/Q PDSCH", static_cast<int>(PlotTypeUE::pdschIQ));

  this->addItem("DL SNR", static_cast<int>(PlotTypeUE::pdschSNR));
  this->addItem("DL BLER", static_cast<int>(PlotTypeUE::pdschBLER));
  this->addItem("DL MCS", static_cast<int>(PlotTypeUE::pdschMCS));
  this->addItem("Throughput", static_cast<int>(PlotTypeUE::pdschThroughput));
  this->addItem("Nof Sched. RBs", static_cast<int>(PlotTypeUE::pdschRBs));
  this->addItem("Freq. Offset", static_cast<int>(PlotTypeUE::frequencyOffset));
  this->addItem("Time Adv.", static_cast<int>(PlotTypeUE::timingAdvance));

  this->addItem("Configs", static_cast<int>(PlotTypeUE::config));
}

WaterFall::WaterFall(complex16 *values, NR_DL_FRAME_PARMS *frame_parms, QWidget *parent)
    : QWidget(parent), values(values), frame_parms(frame_parms)
{
  this->iteration = 0;
  this->image = nullptr;
  this->waterFallAvg = nullptr;

  startTimer(100);
}

/* this function to plot the waterfall graph for the RX signal in time domain for one frame. x-axis shows the frame divided into
   slots and the y-axis is a color map depending on the SquaredNorm of the received signal at the correspoinding slot. */
void WaterFall::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  const int datasize = frame_parms->samples_per_frame;
  const int samplesPerPixel = datasize / this->width();
  const int displayPart = this->height() - ScaleZone;

  if (!this->image) {
    this->image = new QImage(this->width(), this->height(), QImage::Format_RGB32);
    this->image->fill(QColor(240, 240, 240));
    this->iteration = 0;
    this->waterFallAvg = (double *)realloc(this->waterFallAvg, this->height() * sizeof(double));
    memset(this->waterFallAvg, 0, this->height() * sizeof(double));

    // Plot vertical Lines
    QRgb *pixels = (QRgb *)this->image->bits();
    for (int slot = 1; slot < frame_parms->slots_per_frame; slot++) {
      int lineX = frame_parms->get_samples_slot_timestamp(slot, frame_parms, 0) / samplesPerPixel;
      for (int row = displayPart; row < this->height(); row++)
        pixels[row * this->width() + lineX] = 0xFF000000; // black
    }
    this->update();
  }

  QRgb *pixels = (QRgb *)this->image->bits();

  double avg = 0;
  for (int i = 0; i < displayPart; i++)
    avg += this->waterFallAvg[i];
  avg /= displayPart;

  const int row = this->iteration % displayPart;

  this->waterFallAvg[row] = 0;
  for (int pix = 0; pix < this->width(); pix++) {
    complex16 *end = values + (pix + 1) * samplesPerPixel;
    end -= 2;

    double val = 0;
    for (complex16 *s = values + pix * samplesPerPixel; s < end; s++)
      val += SquaredNorm(*s);
    val /= samplesPerPixel;
    this->waterFallAvg[row] += val;

    QRgb col;
    if (val > avg * 100)
      col = 0xFFFF0000; // red
    else if (val > avg * 10)
      col = 0xFFFFFF00; // yellow
    else if (val > avg)
      col = 0xFF00FF00; // green
    else
      col = 0xFF0000FF; // blue
    pixels[row * this->width() + pix] = col;
  }
  this->waterFallAvg[row] /= this->width();

  this->iteration++;
  this->update(0, row, this->width(), 1);
}

void WaterFall::paintEvent(QPaintEvent *event)
{
  if (!this->image)
    return;

  QPainter painter(this);
  painter.drawImage(event->rect(), *this->image, event->rect()); // paint image on widget
}

void WaterFall::resizeEvent(QResizeEvent *event)
{
  if (this->image) {
    delete this->image;
    this->image = nullptr;
  }
}

CIRPlot::CIRPlot(complex16 *data, int len) : data(data), len(len)
{
  this->legend()->hide();

  // add new series to the chart
  this->series = new QLineSeries();
  this->series->setColor(Qt::blue);
  this->addSeries(series);

  // add new X axis
  this->axisX = new QValueAxis();
  this->axisX->setLabelFormat("%d");
  this->axisX->setRange(-len / 2, len / 2);
  this->addAxis(this->axisX, Qt::AlignBottom);
  this->series->attachAxis(this->axisX);

  // add new Y axis
  this->axisY = new QValueAxis();
  this->axisY->setLabelFormat("%.1e");
  this->addAxis(this->axisY, Qt::AlignLeft);
  this->series->attachAxis(this->axisY);

  startTimer(1000);
}

void CIRPlot::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  QVector<QPointF> points(this->len);
  float maxY = this->axisY->max();

  for (int i = 0; i < this->len / 2; i++) {
    float value = SquaredNorm(this->data[i + this->len / 2]);
    points[i] = QPointF(i - this->len / 2, value);
    maxY = std::max(maxY, value);
  }
  for (int i = 0; i < this->len / 2; i++) {
    float value = SquaredNorm(this->data[i]);
    points[i + this->len / 2] = QPointF(i, value);
    maxY = std::max(maxY, value);
  }

  this->axisY->setMax(maxY);
  this->series->replace(points);
}

void CIRPlotUE::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  scopeGraphData_t *data = this->valueProvider->getPlotValue();
  if (data) {
    this->data = (complex16 *)(data + 1);
    this->len = data->lineSz;
    QVector<QPointF> points(this->len);
    float maxY = this->axisY->max();

    for (int i = 0; i < this->len / 2; i++) {
      float value = SquaredNorm(this->data[i + this->len / 2]);
      points[i] = QPointF(i - this->len / 2, value);
      maxY = std::max(maxY, value);
    }
    for (int i = 0; i < this->len / 2; i++) {
      float value = SquaredNorm(this->data[i]);
      points[i + this->len / 2] = QPointF(i, value);
      maxY = std::max(maxY, value);
    }

    this->axisX->setRange(-this->len / 2, this->len / 2);
    this->axisY->setMax(maxY);
    this->series->replace(points);
  }
}

LLRPlot::LLRPlot(int16_t *data, int len, int interval, PlotUpdater *plotUpdater) : data(data), len(len), plotUpdater(plotUpdater)
{
  this->legend()->hide();

  // add new series to the chart
  this->series = new QScatterSeries();
  this->series->setUseOpenGL();
  this->series->setMarkerSize(3);
  this->series->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
  this->series->setColor(Qt::blue);
  this->series->setPen(Qt::NoPen);
  this->addSeries(series);

  // add new X axis
  this->axisX = new QValueAxis();
  this->axisX->setLabelFormat("%d");
  this->addAxis(this->axisX, Qt::AlignBottom);
  this->series->attachAxis(this->axisX);

  // add new Y axis
  this->axisY = new QValueAxis();
  this->addAxis(this->axisY, Qt::AlignLeft);
  this->series->attachAxis(this->axisY);

  if (interval)
    startTimer(interval);

  if (plotUpdater)
    connect(plotUpdater, &PlotUpdater::updatePlot, this, &LLRPlot::updatePlot, Qt::QueuedConnection);
}

LLRPlot::~LLRPlot()
{
  if (this->plotUpdater)
    disconnect(this->plotUpdater, &PlotUpdater::updatePlot, this, &LLRPlot::updatePlot);
}

void LLRPlot::updatePlot(int len)
{
  this->len = len;

  if (!this->isVisible())
    return;

  QVector<QPointF> points(len);
  int maxY = this->axisY->max();

  for (int i = 0; i < len; i++) {
    points[i] = QPointF(i, this->data[i]);
    maxY = std::max(maxY, abs(this->data[i]));
  }

  this->axisX->setRange(0, len);
  this->axisY->setRange(-maxY, maxY);
  this->series->replace(points);
}

void LLRPlot::timerEvent(QTimerEvent *event)
{
  this->updatePlot(this->len);
}

void LLRPlotUE::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  scopeGraphData_t *data = this->valueProvider->getPlotValue();
  if (data) {
    this->data = (int16_t *)(data + 1);
    this->len = data->lineSz;
    QVector<QPointF> points(this->len);
    int maxY = this->axisY->max();

    for (int i = 0; i < this->len; i++) {
      points[i] = QPointF(i, this->data[i]);
      maxY = std::max(maxY, abs(this->data[i]));
    }

    this->axisX->setRange(0, this->len);
    this->axisY->setRange(-maxY, maxY);
    this->series->replace(points);
  }
}

IQPlot::IQPlot(complex16 *data, int len, int interval, PlotUpdater *plotUpdater) : data(data), len(len), plotUpdater(plotUpdater)
{
  this->legend()->hide();

  // add new series to the chart
  this->series = new QScatterSeries();
  this->series->setUseOpenGL();
  this->series->setMarkerSize(3);
  this->series->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
  this->series->setColor(Qt::blue);
  this->series->setPen(Qt::NoPen);
  this->addSeries(series);

  // add new X axis
  this->axisX = new QValueAxis();
  this->addAxis(this->axisX, Qt::AlignBottom);
  this->series->attachAxis(this->axisX);

  // add new Y axis
  this->axisY = new QValueAxis();
  this->addAxis(this->axisY, Qt::AlignLeft);
  this->series->attachAxis(this->axisY);

  if (interval)
    startTimer(interval);

  if (plotUpdater)
    connect(plotUpdater, &PlotUpdater::updatePlot, this, &IQPlot::updatePlot, Qt::QueuedConnection);
}

IQPlot::~IQPlot()
{
  if (this->plotUpdater)
    disconnect(this->plotUpdater, &PlotUpdater::updatePlot, this, &IQPlot::updatePlot);
}

void IQPlot::updatePlot(int len)
{
  this->len = len;

  if (!this->isVisible())
    return;

  QVector<QPointF> points(len);
  int maxX = this->axisX->max();
  int maxY = this->axisY->max();

  for (int i = 0; i < len; i++) {
    points[i] = QPointF(this->data[i].r, this->data[i].i);

    maxX = std::max(maxX, abs(this->data[i].r));
    maxY = std::max(maxY, abs(this->data[i].i));
  }

  this->axisX->setRange(-maxX, maxX);
  this->axisY->setRange(-maxY, maxY);
  this->series->replace(points);
}

void IQPlot::timerEvent(QTimerEvent *event)
{
  this->updatePlot(this->len);
}

void IQPlotUE::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  scopeGraphData_t *data = this->valueProvider->getPlotValue();
  if (data) {
    this->data = (complex16 *)(data + 1);
    this->len = data->lineSz;
    QVector<QPointF> points(this->len);
    int maxX = this->axisX->max();
    int maxY = this->axisY->max();

    for (int i = 0; i < this->len; i++) {
      points[i] = QPointF(this->data[i].r, this->data[i].i);

      maxX = std::max(maxX, abs(this->data[i].r));
      maxY = std::max(maxY, abs(this->data[i].i));
    }

    this->axisX->setRange(-maxX, maxX);
    this->axisY->setRange(-maxY, maxY);
    this->series->replace(points);
  }
}

KPIPlot::KPIPlot(ValueProvider *valueProvider, float *limits) : valueProvider(valueProvider), limits(limits)
{
  this->series = new QLineSeries();
  this->series->setColor(Qt::blue);
  this->addSeries(series);

  this->seriesMin = new QLineSeries();
  this->seriesMin->setColor(Qt::red);
  this->addSeries(seriesMin);

  this->seriesMax = new QLineSeries();
  this->seriesMax->setColor(Qt::red);
  this->addSeries(seriesMax);

  this->seriesAvg = new QLineSeries();
  this->seriesAvg->setColor(Qt::green);
  this->seriesAvg->setName("Average");
  this->addSeries(seriesAvg);

  if (limits) {
    this->seriesMin->setName("Upper Limit");
    this->seriesMax->setName("Lower Limit");
    this->minValue = limits[0];
    this->maxValue = limits[1];
  } else {
    this->seriesMin->setName("Minimum");
    this->seriesMax->setName("Maximum");
    this->minValue = 0;
    this->maxValue = 0;
  }

  this->sumValue = 0;
  this->plotIdx = 0;

  // add new X axis
  this->axisX = new QValueAxis();
  this->axisX->setLabelFormat("%d");
  this->axisX->setRange(0, 300);
  this->addAxis(this->axisX, Qt::AlignBottom);

  this->series->attachAxis(this->axisX);
  this->seriesMin->attachAxis(this->axisX);
  this->seriesMax->attachAxis(this->axisX);
  this->seriesAvg->attachAxis(this->axisX);

  // add new Y axis
  this->axisY = new QValueAxis();
  this->addAxis(this->axisY, Qt::AlignLeft);
  this->series->attachAxis(this->axisY);
  this->seriesMin->attachAxis(this->axisY);
  this->seriesMax->attachAxis(this->axisY);
  this->seriesAvg->attachAxis(this->axisY);

  startTimer(1000);
}

void KPIPlot::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  if (this->plotIdx >= 300) {
    this->series->clear();
    this->sumValue = 0;
    this->plotIdx = 0;
  }

  float value = this->valueProvider->getValue();
  this->series->append(this->plotIdx++, value);

  this->minValue = std::min(this->minValue, value);
  this->maxValue = std::max(this->maxValue, value);

  this->seriesMin->clear();
  this->seriesMax->clear();
  this->seriesAvg->clear();

  if (this->limits) {
    this->seriesMin->append(0, this->limits[0]);
    this->seriesMin->append(300, this->limits[0]);

    this->seriesMax->append(0, this->limits[1]);
    this->seriesMax->append(300, this->limits[1]);
  } else {
    this->seriesMin->append(0, this->minValue);
    this->seriesMin->append(300, this->minValue);

    this->seriesMax->append(0, this->maxValue);
    this->seriesMax->append(300, this->maxValue);
  }

  this->sumValue += value;
  float average = this->sumValue / this->plotIdx;
  this->seriesAvg->append(0, average);
  this->seriesAvg->append(300, average);

  this->axisY->setRange(this->minValue, this->maxValue);
}

RTXPlot::RTXPlot(uint64_t *rounds) : rounds(rounds)
{
  for (int i = 0; i < 4; i++)
    this->lastRounds[i] = rounds[i];

  this->maxValue = 0;
  this->plotIdx = 0;

  this->series[0] = new QLineSeries();
  this->series[0]->setColor(Qt::blue);
  this->series[0]->setName("round 1");
  this->addSeries(this->series[0]);

  this->series[1] = new QLineSeries();
  this->series[1]->setColor(Qt::green);
  this->series[1]->setName("round 2");
  this->addSeries(this->series[1]);

  this->series[2] = new QLineSeries();
  this->series[2]->setColor(Qt::yellow);
  this->series[2]->setName("round 3");
  this->addSeries(this->series[2]);

  this->series[3] = new QLineSeries();
  this->series[3]->setColor(Qt::red);
  this->series[3]->setName("round 4");
  this->addSeries(this->series[3]);

  // add new X axis
  this->axisX = new QValueAxis();
  this->axisX->setLabelFormat("%d");
  this->axisX->setRange(0, 300);
  this->addAxis(this->axisX, Qt::AlignBottom);

  this->series[0]->attachAxis(this->axisX);
  this->series[1]->attachAxis(this->axisX);
  this->series[2]->attachAxis(this->axisX);
  this->series[3]->attachAxis(this->axisX);

  // add new Y axis
  this->axisY = new QValueAxis();
  this->addAxis(this->axisY, Qt::AlignLeft);
  this->series[0]->attachAxis(this->axisY);
  this->series[1]->attachAxis(this->axisY);
  this->series[2]->attachAxis(this->axisY);
  this->series[3]->attachAxis(this->axisY);

  startTimer(1000);
}

void RTXPlot::timerEvent(QTimerEvent *event)
{
  if (!this->isVisible())
    return;

  if (this->plotIdx >= 300) {
    this->series[0]->clear();
    this->series[1]->clear();
    this->series[2]->clear();
    this->series[3]->clear();
    this->plotIdx = 0;
  }

  for (int i = 0; i < 4; i++) {
    int value = this->rounds[i] - this->lastRounds[i];
    this->lastRounds[i] += value;
    this->maxValue = std::max(this->maxValue, value);
    this->series[i]->append(this->plotIdx, value);
  }

  this->plotIdx++;

  this->axisY->setRange(0, this->maxValue);
}

/* @gNB: This is the main function of the gNB sub-widgets, i.e., for each KPI. This function will be called
   only once when the the sub-widget is created, and it mainly initializes the widget variables and structures. */
PainterWidgetGnb::PainterWidgetGnb(QWidget *config, QComboBox *comboBox, scopeData_t *p) : config(config), comboBox(comboBox), p(p)
{
  this->chartView = new QChartView(this);
  this->chartView->hide();

  this->plotType = PlotTypeGnb::empty;

  makeConnections(this->comboBox->currentIndex());
  connect(this->comboBox,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &PainterWidgetGnb::makeConnections);
}

float PainterWidgetGnb::getValue()
{
  NR_DL_FRAME_PARMS *frame_parms = &this->p->gNB->frame_parms;
  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &targetUE->UE_sched_ctrl;
  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;

  switch (this->plotType) {
    case PlotTypeGnb::puschSNR:
      return sched_ctrl->pusch_snrx10 / 10.0;

    case PlotTypeGnb::puschBLER:
      return sched_ctrl->ul_bler_stats.bler;

    case PlotTypeGnb::puschMCS:
      return sched_ctrl->ul_bler_stats.mcs;

    case PlotTypeGnb::puschThroughput: {
      double slotDuration = 10.0 / (double)frame_parms->slots_per_frame;
      double blerTerm = 1.0 - (double)sched_ctrl->ul_bler_stats.bler;
      double blockSizeBits = (double)(targetUE->mac_stats.ul.current_bytes << 3);
      double ThrouputKBitSec = blerTerm * blockSizeBits / slotDuration;
      return (float)(ThrouputKBitSec / 1000); // Throughput in MBit/sec
    }
    case PlotTypeGnb::pdschSNR:
      return sched_ctrl->CSI_report.cri_ri_li_pmi_cqi_report.wb_cqi_1tb;

    case PlotTypeGnb::pdschBLER:
      return sched_ctrl->dl_bler_stats.bler;

    case PlotTypeGnb::pdschMCS:
      return sched_ctrl->dl_bler_stats.mcs;

    case PlotTypeGnb::pdschThroughput: {
      double slotDuration = 10.0 / (double)frame_parms->slots_per_frame;
      double blerTerm = 1.0 - (double)sched_ctrl->dl_bler_stats.bler;
      double blockSizeBits = (double)(targetUE->mac_stats.dl.current_bytes << 3);
      double ThrouputKBitSec = blerTerm * blockSizeBits / slotDuration;
      return (float)(ThrouputKBitSec / 1000); // Throughput in MBit/sec
    }
    case PlotTypeGnb::pdschRBs:
      return sched_ctrl->harq_processes[sched_pdsch->dl_harq_pid].sched_pdsch.rbSize;

    default:
      return 0;
  }
}

/* @gNB Class: this function is called when a resize event is detected, then the widget will be adjusted accordignly. */
void PainterWidgetGnb::resizeEvent(QResizeEvent *event)
{
  if (this->waterFall)
    this->waterFall->resize(event->size());
  this->chartView->resize(event->size());
}

/* @gNB: this function is to check which KPI to plot in the current widget based on the drop-down list selection
   Then the widget will be connected with the correspinding KPI function. */
void PainterWidgetGnb::makeConnections(int type)
{
  const PlotTypeGnb plotType = static_cast<PlotTypeGnb>(type);

  if (plotType == this->plotType)
    return;

  if (plotType == PlotTypeGnb::config) {
    config->show();
    this->comboBox->setCurrentIndex(static_cast<int>(this->plotType));
    return;
  }

  this->plotType = plotType;
  this->chartView->hide();
  QChart *prevChart = this->chartView->chart();

  if (plotType == PlotTypeGnb::waterFall) {
    this->chartView->setChart(new QChart);
    this->waterFall = new WaterFall((complex16 *)this->p->ru->common.rxdata[0], &this->p->gNB->frame_parms, this);
    this->waterFall->resize(this->size());
    this->waterFall->show();
    delete prevChart;
    return;
  }

  if (this->waterFall) {
    this->waterFall->hide();
    delete this->waterFall;
    this->waterFall = nullptr;
  }

  NR_DL_FRAME_PARMS *frame_parms = &this->p->gNB->frame_parms;
  gNB_MAC_INST *gNBMac = (gNB_MAC_INST *)RC.nrmac[0];
  NR_UE_info_t *targetUE = gNBMac->UE_info.list[0];

  QChart *newChart = nullptr;

  switch (plotType) {
    case PlotTypeGnb::empty: {
      newChart = new QChart();
      break;
    }
    case PlotTypeGnb::CIR: {
      newChart = new CIRPlot((complex16 *)p->gNB->pusch_vars[0].ul_ch_estimates_time[0], frame_parms->ofdm_symbol_size);
      break;
    }

    case PlotTypeGnb::puschLLR: {
      int init_coded_bits_per_codeword = 100;
      newChart = new LLRPlot((int16_t *)p->gNB->pusch_vars[0].llr, init_coded_bits_per_codeword, 0, &puschLlrUpdater);
      break;
    }
    case PlotTypeGnb::puschIQ: {
      int init_num_re = 100;
      newChart = new IQPlot((complex16 *)p->gNB->pusch_vars[0].rxdataF_comp[0], init_num_re, 0, &puschIqUpdater);
      break;
    }
    case PlotTypeGnb::puschSNR: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeGnb::puschBLER: {
      newChart = new KPIPlot(this, Limits_KPI_gNB[0]);
      break;
    }
    case PlotTypeGnb::puschMCS: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeGnb::puschRETX: {
      newChart = new RTXPlot(targetUE->mac_stats.ul.rounds);
      break;
    }
    case PlotTypeGnb::puschThroughput: {
      newChart = new KPIPlot(this, Limits_KPI_gNB[1]);
      break;
    }

    case PlotTypeGnb::pdschSNR: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeGnb::pdschBLER: {
      newChart = new KPIPlot(this, Limits_KPI_gNB[2]);
      break;
    }
    case PlotTypeGnb::pdschMCS: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeGnb::pdschRETX: {
      newChart = new RTXPlot(targetUE->mac_stats.dl.rounds);
      break;
    }
    case PlotTypeGnb::pdschThroughput: {
      newChart = new KPIPlot(this, Limits_KPI_gNB[3]);
      break;
    }
    case PlotTypeGnb::pdschRBs: {
      newChart = new KPIPlot(this);
      break;
    }

    default:
      break;
  }

  this->chartView->setChart(newChart);
  this->chartView->show();
  delete prevChart;
}

/* @UE: This is the main function of the UE sub-widgets, i.e., for each KPI. This function will be called
   only once when the the sub-widget is created, and it mainly initializes the widget variables and structures. */
PainterWidgetUE::PainterWidgetUE(QWidget *config, QComboBox *comboBox, PHY_VARS_NR_UE *ue)
    : config(config), comboBox(comboBox), ue(ue)
{
  this->chartView = new QChartView(this);
  this->chartView->hide();

  this->plotType = PlotTypeUE::empty;

  makeConnections(this->comboBox->currentIndex());
  connect(this->comboBox,
          static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
          this,
          &PainterWidgetUE::makeConnections);
}

float PainterWidgetUE::getValue()
{
  NR_DL_FRAME_PARMS *frame_parms = &this->ue->frame_parms;

  switch (this->plotType) {
    case PlotTypeUE::pdschSNR:
      return (float)this->ue->measurements.wideband_cqi_avg[0];

    case PlotTypeUE::pdschBLER: {
      if (getKPIUE()->nb_total == 0)
        return 0;
      return (float)getKPIUE()->nb_nack / (float)getKPIUE()->nb_total;
    }
    case PlotTypeUE::pdschMCS:
      return (float)getKPIUE()->dl_mcs;

    case PlotTypeUE::pdschThroughput: {
      if (getKPIUE()->nb_total == 0)
        return 0;
      double slotDuration = 10.0 / (double)frame_parms->slots_per_frame;
      double blerTerm = 1.0 - (double)(getKPIUE()->nb_nack) / (double)getKPIUE()->nb_total;
      double blockSizeBits = (double)getKPIUE()->blockSize;
      double ThrouputKBitSec = blerTerm * blockSizeBits / slotDuration;
      return (float)(ThrouputKBitSec / 1000); // Throughput in MBit/sec
    }
    case PlotTypeUE::pdschRBs:
      return (float)getKPIUE()->nofRBs;

    case PlotTypeUE::frequencyOffset:
      return (float)this->ue->common_vars.freq_offset;

    case PlotTypeUE::timingAdvance:
      return (float)this->ue->timing_advance;

    default:
      return 0;
  }
}

scopeGraphData_t *PainterWidgetUE::getPlotValue()
{
  scopeData_t *scope = (scopeData_t *)this->ue->scopeData;
  scopeGraphData_t **data = (scopeGraphData_t **)scope->liveData;
  switch (this->plotType) {
    case PlotTypeUE::CIR:
      return data[pbchDlChEstimateTime];

    case PlotTypeUE::pbchLLR:
      return data[pbchLlr];
    case PlotTypeUE::pbchIQ:
      return data[pbchRxdataF_comp];

    case PlotTypeUE::pdcchLLR:
      return data[pdcchLlr];

    case PlotTypeUE::pdcchIQ:
      return data[pdcchRxdataF_comp];

    case PlotTypeUE::pdschLLR:
      return data[pdschLlr];

    case PlotTypeUE::pdschIQ:
      return data[pdschRxdataF_comp];

    default:
      return nullptr;
  }
}

/* @UE Class: this function is called when a resize event is detected, then the widget will be adjusted accordignly. */
void PainterWidgetUE::resizeEvent(QResizeEvent *event)
{
  if (this->waterFall)
    this->waterFall->resize(event->size());
  this->chartView->resize(event->size());
}

/* @UE: this function is to check which KPI to plot in the current widget based on the drop-down list selection.
   Then the widget will be connected with the correspinding KPI function. */
void PainterWidgetUE::makeConnections(int type)
{
  const PlotTypeUE plotType = static_cast<PlotTypeUE>(type);

  if (plotType == this->plotType)
    return;

  if (plotType == PlotTypeUE::config) {
    config->show();
    this->comboBox->setCurrentIndex(static_cast<int>(this->plotType));
    return;
  }

  this->plotType = plotType;
  this->chartView->hide();
  QChart *prevChart = this->chartView->chart();

  if (plotType == PlotTypeUE::waterFall) {
    this->chartView->setChart(new QChart);
    this->waterFall = new WaterFall((complex16 *)this->ue->common_vars.rxdata[0], &this->ue->frame_parms, this);
    this->waterFall->resize(this->size());
    this->waterFall->show();
    delete prevChart;
    return;
  }

  if (this->waterFall) {
    this->waterFall->hide();
    delete this->waterFall;
    this->waterFall = nullptr;
  }

  scopeData_t *scope = (scopeData_t *)this->ue->scopeData;
  scopeGraphData_t **data = (scopeGraphData_t **)scope->liveData;

  QChart *newChart = nullptr;

  switch (plotType) {
    case PlotTypeUE::empty: {
      newChart = new QChart();
      break;
    }
    case PlotTypeUE::CIR: {
      if (!data[pbchDlChEstimateTime]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new CIRPlot((complex16 *)(data[pbchDlChEstimateTime] + 1), data[pbchDlChEstimateTime]->lineSz);
      break;
    }

    case PlotTypeUE::pbchLLR: {
      if (!data[pbchLlr]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new LLRPlotUE((int16_t *)(data[pbchLlr] + 1), data[pbchLlr]->lineSz, this);
      break;
    }
    case PlotTypeUE::pbchIQ: {
      if (!data[pbchRxdataF_comp]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new IQPlotUE((complex16 *)(data[pbchRxdataF_comp] + 1), data[pbchRxdataF_comp]->lineSz, this);
      break;
    }
    case PlotTypeUE::pdcchLLR: {
      if (!data[pdcchLlr]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new LLRPlotUE((int16_t *)(data[pdcchLlr] + 1), data[pdcchLlr]->lineSz, this);
      break;
    }
    case PlotTypeUE::pdcchIQ: {
      if (!data[pdcchRxdataF_comp]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new IQPlotUE((complex16 *)(data[pdcchRxdataF_comp] + 1), data[pdcchRxdataF_comp]->lineSz, this);
      break;
    }
    case PlotTypeUE::pdschLLR: {
      if (!data[pdschLlr]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new LLRPlotUE((int16_t *)(data[pdschLlr] + 1), data[pdschLlr]->lineSz, this);
      break;
    }
    case PlotTypeUE::pdschIQ: {
      if (!data[pdschRxdataF_comp]) {
        newChart = new QChart();
        this->plotType = PlotTypeUE::empty;
        this->comboBox->setCurrentIndex(static_cast<int>(PlotTypeUE::empty));
        break;
      }
      newChart = new IQPlotUE((complex16 *)(data[pdschRxdataF_comp] + 1), data[pdschRxdataF_comp]->lineSz, this);
      break;
    }

    case PlotTypeUE::pdschSNR: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeUE::pdschBLER: {
      newChart = new KPIPlot(this, Limits_KPI_ue[0]);
      break;
    }
    case PlotTypeUE::pdschMCS: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeUE::pdschThroughput: {
      newChart = new KPIPlot(this, Limits_KPI_ue[1]);
      break;
    }
    case PlotTypeUE::pdschRBs: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeUE::frequencyOffset: {
      newChart = new KPIPlot(this);
      break;
    }
    case PlotTypeUE::timingAdvance: {
      newChart = new KPIPlot(this);
      break;
    }

    default:
      break;
  }

  this->chartView->setChart(newChart);
  this->chartView->show();
  delete prevChart;
}

// main thread of gNB
void *nrgNBQtscopeThread(void *arg)
{
  scopeData_t *p = (scopeData_t *)arg;

  sleep(3);

  int argc = 1;
  char *argv[] = {(char *)"nrqt_scopegNB"};
  QApplication a(argc, argv);

  // Create a main window (widget)
  QWidget window;
  window.resize(800, 800);
  window.setWindowTitle("gNB Scope");

  // Create gNB configuration window
  KPIConfigGnb config;

  // Main layout
  QGridLayout mainLayout(&window);

  KPIListSelectGnb combo1;
  combo1.setCurrentIndex(static_cast<int>(PlotTypeGnb::waterFall));
  PainterWidgetGnb pwidgetGnbCombo1(&config, &combo1, p);

  mainLayout.addWidget(&combo1, 0, 0);
  mainLayout.addWidget(&pwidgetGnbCombo1, 1, 0);

  KPIListSelectGnb combo2;
  combo2.setCurrentIndex(static_cast<int>(PlotTypeGnb::CIR));
  PainterWidgetGnb pwidgetGnbCombo2(&config, &combo2, p);

  mainLayout.addWidget(&combo2, 0, 1);
  mainLayout.addWidget(&pwidgetGnbCombo2, 1, 1);

  KPIListSelectGnb combo3;
  combo3.setCurrentIndex(static_cast<int>(PlotTypeGnb::puschLLR));
  PainterWidgetGnb pwidgetGnbCombo3(&config, &combo3, p);

  mainLayout.addWidget(&combo3, 2, 0);
  mainLayout.addWidget(&pwidgetGnbCombo3, 3, 0);

  KPIListSelectGnb combo4;
  combo4.setCurrentIndex(static_cast<int>(PlotTypeGnb::puschIQ));
  PainterWidgetGnb pwidgetGnbCombo4(&config, &combo4, p);

  mainLayout.addWidget(&combo4, 2, 1);
  mainLayout.addWidget(&pwidgetGnbCombo4, 3, 1);

  KPIListSelectGnb combo5;
  combo5.setCurrentIndex(static_cast<int>(PlotTypeGnb::empty));
  PainterWidgetGnb pwidgetGnbCombo5(&config, &combo5, p);

  mainLayout.addWidget(&combo5, 4, 0);
  mainLayout.addWidget(&pwidgetGnbCombo5, 5, 0);

  KPIListSelectGnb combo6;
  combo6.setCurrentIndex(static_cast<int>(PlotTypeGnb::empty));
  PainterWidgetGnb pwidgetGnbCombo6(&config, &combo6, p);

  mainLayout.addWidget(&combo6, 4, 1);
  mainLayout.addWidget(&pwidgetGnbCombo6, 5, 1);

  // display the main window
  window.show();
  a.exec();

  return nullptr;
}

// main thread of UE
void *nrUEQtscopeThread(void *arg)
{
  PHY_VARS_NR_UE *ue = (PHY_VARS_NR_UE *)arg;

  sleep(1);

  int argc = 1;
  char *argv[] = {(char *)"nrqt_scopeUE"};
  QApplication a(argc, argv);

  // Create a main window (widget)
  QWidget window;
  window.resize(800, 800);
  window.setWindowTitle("UE Scope");

  // Create UE configuration window
  KPIConfigUE config;

  // Main layout
  QGridLayout mainLayout(&window);

  KPIListSelectUE combo1;
  combo1.setCurrentIndex(static_cast<int>(PlotTypeUE::waterFall));
  PainterWidgetUE pwidgetueCombo1(&config, &combo1, ue);

  mainLayout.addWidget(&combo1, 0, 0);
  mainLayout.addWidget(&pwidgetueCombo1, 1, 0);

  KPIListSelectUE combo2;
  combo2.setCurrentIndex(static_cast<int>(PlotTypeUE::CIR));
  PainterWidgetUE pwidgetueCombo2(&config, &combo2, ue);

  mainLayout.addWidget(&combo2, 0, 1);
  mainLayout.addWidget(&pwidgetueCombo2, 1, 1);

  KPIListSelectUE combo3;
  combo3.setCurrentIndex(static_cast<int>(PlotTypeUE::pbchLLR));
  PainterWidgetUE pwidgetueCombo3(&config, &combo3, ue);

  mainLayout.addWidget(&combo3, 2, 0);
  mainLayout.addWidget(&pwidgetueCombo3, 3, 0);

  KPIListSelectUE combo4;
  combo4.setCurrentIndex(static_cast<int>(PlotTypeUE::pbchIQ));
  PainterWidgetUE pwidgetueCombo4(&config, &combo4, ue);

  mainLayout.addWidget(&combo4, 2, 1);
  mainLayout.addWidget(&pwidgetueCombo4, 3, 1);

  KPIListSelectUE combo5;
  combo5.setCurrentIndex(static_cast<int>(PlotTypeUE::pdschLLR));
  PainterWidgetUE pwidgetueCombo5(&config, &combo5, ue);

  mainLayout.addWidget(&combo5, 4, 0);
  mainLayout.addWidget(&pwidgetueCombo5, 5, 0);

  KPIListSelectUE combo6;
  combo6.setCurrentIndex(static_cast<int>(PlotTypeUE::pdschIQ));
  PainterWidgetUE pwidgetueCombo6(&config, &combo6, ue);

  mainLayout.addWidget(&combo6, 4, 1);
  mainLayout.addWidget(&pwidgetueCombo6, 5, 1);

  // display the main window
  window.show();
  a.exec();

  return nullptr;
}

// gNB scope initialization
void nrgNBinitQtScope(scopeParms_t *p)
{
  scopeData_t *scope = (scopeData_t *)calloc(1, sizeof(scopeData_t));

  scope->gNB = p->gNB;
  scope->argc = p->argc;
  scope->argv = p->argv;
  scope->ru = p->ru;
  scope->scopeUpdater = scopeUpdaterGnb;
  scope->copyData = copyData;
  copyDataMutexInit(scope);

  p->gNB->scopeData = scope;

  pthread_t qtscope_thread;
  threadCreate(&qtscope_thread, nrgNBQtscopeThread, scope, (char *)"qtscope", -1, sched_get_priority_min(SCHED_RR));
}

// UE scope initialization
void nrUEinitQtScope(PHY_VARS_NR_UE *ue)
{
  scopeData_t *scope = (scopeData_t *)calloc(1, sizeof(scopeData_t));

  scope->copyData = copyData;
  copyDataMutexInit(scope);

  ue->scopeData = scope;

  pthread_t qtscope_thread;
  threadCreate(&qtscope_thread, nrUEQtscopeThread, ue, (char *)"qtscope", -1, sched_get_priority_min(SCHED_RR));
}

extern "C" void nrqtscope_autoinit(void *dataptr)
{
  AssertFatal((IS_SOFTMODEM_GNB_BIT || IS_SOFTMODEM_5GUE_BIT), "Scope cannot find NRUE or GNB context");

  if (IS_SOFTMODEM_GNB_BIT)
    nrgNBinitQtScope((scopeParms_t *)dataptr);
  else
    nrUEinitQtScope((PHY_VARS_NR_UE *)dataptr);
}
