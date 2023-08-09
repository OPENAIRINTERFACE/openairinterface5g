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

/*! \file common/utils/websrv/frontend/src/app/components/scope/scope.component.ts
 * \brief: implementation of web interface frontend for oai
 * \scope component web interface implementation (works with scope.component.html)
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {Component, EventEmitter, OnDestroy, OnInit, Output, QueryList, ViewChildren} from "@angular/core";
import {Chart, ChartConfiguration} from "chart.js";
import {BaseChartDirective} from "ng2-charts";
import {Subscription} from "rxjs";
import {HelpApi} from "src/app/api/help.api";
import {IGraphDesc, IScopeDesc, IScopeGraphType, ISigDesc, ScopeApi} from "src/app/api/scope.api";
import {arraybuf_data_offset, Message, WebSocketService, webSockSrc} from "src/app/services/websocket.service";

export interface RxScopeMessage {
  msgtype: number;
  chartid: number;
  dataid: number;
  segnum: number;
  update: boolean;
  content: ArrayBuffer;
}

const deserialize = (fullbuff: ArrayBuffer):
    RxScopeMessage => {
      const header = new DataView(fullbuff, 0, arraybuf_data_offset);
      return {
        // source: src.getUint8(0),  //header
        msgtype : header.getUint8(1), // header
        chartid : header.getUint8(3), // header
        dataid : header.getUint8(4), // header
        segnum : header.getUint8(2), // header
        update : (header.getUint8(5) == 1) ? true : false, // header
        content : fullbuff.slice(arraybuf_data_offset) // data
      };
    }

const serialize = (msg: TxScopeMessage):
    ArrayBuffer => {
      const byteArray = new TextEncoder().encode(msg.content);

      let arr = new Uint8Array(byteArray.byteLength + arraybuf_data_offset);
      arr.set(byteArray, arraybuf_data_offset) // data
      let buffview = new DataView(arr.buffer);
      buffview.setUint8(1, msg.msgtype); // header

      return buffview.buffer;
    }

export interface TxScopeMessage {
  msgtype: number;
  content: string;
}

/*------------------------------------*/
/* constants that must match backend (websrv.h or phy_scope.h) */
const SCOPEMSG_TYPE_TIME = 3;
const SCOPEMSG_TYPE_DATA = 10;
const SCOPEMSG_TYPE_DATAACK = 11;
const SCOPEMSG_TYPE_DATAFLOW = 12;

const SCOPEMSG_DATA_IQ = 1;
const SCOPEMSG_DATA_LLR = 2;
const SCOPEMSG_DATA_WF = 3;
const SCOPEMSG_DATA_TRESP = 4;
/*---------------------------------------*/
@Component({
  selector : "app-scope",
  templateUrl : "./scope.component.html",
  styleUrls : [ "./scope.component.css" ],
})

export class ScopeComponent implements OnInit, OnDestroy {
  // data for scope status area
  scopetitle = "";
  scopesubtitle = "";
  scopetime = "";
  scopestatus = "stopped";
  skippedmsg = "0";
  bufferedmsg = "0";
  // data for scope control area
  startstop = "start";
  startstop_color = "warn";
  rfrate = 2;
  data_ACK = false;
  // data for Time Response chart
  TRespgraph: IGraphDesc = {title : "", type: IScopeGraphType.TRESP, id: -1, srvidx: -1};
  enable_TResp = false;
  // data for scope iq constellation area
  iqgraph_list: IGraphDesc[] = [];
  selected_channels = [ "" ];
  iqmax = 32767;
  iqmin = -32767;
  iqxmin = this.iqmin;
  iqymin = this.iqmin;
  iqxmax = this.iqmax;
  iqymax = this.iqmax;
  // data for scope LLR area
  llrgraph_list: IGraphDesc[] = [];
  selected_llrchannels = [ "" ];
  sig_list: ISigDesc[] = [
    {target_id : 0, antenna_id: 0},
    {target_id : 1, antenna_id: 0},
    {target_id : 2, antenna_id: 0},
    {target_id : 3, antenna_id: 0},
  ];
  selected_sig: ISigDesc = {target_id : 0, antenna_id: 0};
  llrythresh = 5;
  llrmin = 0;
  llrmax = 200000;
  llrxmin = this.llrmin;
  llrxmax = this.llrmax;
  // data for scope WatterFall area
  WFgraph_list: IGraphDesc[] = [];
  selected_WF = "";
  llrchart?: Chart;
  iqcchart?: Chart;
  wfchart?: Chart;
  trespchart?: Chart;
  nwf: number[] = [ 0, 0, 0, 0 ];

  // websocket service object and related subscription for message reception
  wsSubscription?: Subscription;

  @Output() ScopeEnabled = new EventEmitter<boolean>();

  @ViewChildren(BaseChartDirective) charts?: QueryList<BaseChartDirective>;

  public TRespDatasets: ChartConfiguration<"line">[ "data" ]["datasets"] = [
    {
      data : [],
      label: "",
      pointRadius: 1,
      showLine: true,
      animation: false,
      fill: false,
      pointStyle: "circle",
      backgroundColor: "rgba(255,0,0,0)",
      borderColor: "red",
      pointBorderColor: "red",
    },
  ];
  public IQDatasets: ChartConfiguration<"scatter">[ "data" ]["datasets"] = [
    {
      data : [],
      label: "C1",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      fill: false,
      pointStyle: "circle",
      //      pointBackgroundColor: 'yellow',
      backgroundColor: "yellow",
      borderWidth: 0,
      pointBorderColor: "yellow",
      //    parsing: false,
    },
    {
      data : [],
      label: "C2",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "cyan",
      backgroundColor: "cyan",
      borderWidth: 0,
      pointBorderColor: "cyan",
      //      parsing: false,
    },
    {
      data : [],
      label: "C3",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "red",
      backgroundColor: "red",
      borderWidth: 0,
      pointBorderColor: "red",
      //      parsing: false,
    }
  ];

  public LLRDatasets: ChartConfiguration<"scatter">[ "data" ]["datasets"] = [
    {
      data : [],
      label: "LLR1",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      fill: false,
      pointStyle: "circle",
      pointBackgroundColor: "yellow",
      backgroundColor: "yellow",
      borderWidth: 0,
      pointBorderColor: "yellow",
      //     parsing: false,
    },
    {
      data : [],
      label: "LL2",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "cyan",
      backgroundColor: "cyan",
      borderWidth: 0,
      pointBorderColor: "cyan",
      parsing: false,
    },
    {
      data : [],
      label: "LLR3",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "red",
      backgroundColor: "red",
      borderWidth: 0,
      pointBorderColor: "red",
      parsing: false,
    }
  ];

  public WFDatasets: ChartConfiguration<"scatter">[ "data" ]["datasets"] = [
    {
      data : [],
      label: "WFblue",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      fill: false,
      pointStyle: "circle",
      pointBackgroundColor: "blue",
      backgroundColor: "blue",
      borderWidth: 0,
      pointBorderColor: "blue",
      //     parsing: false,
    },
    {
      data : [],
      label: "WFgreen",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "green",
      backgroundColor: "green",
      borderWidth: 0,
      pointBorderColor: "green",
      //      parsing: false,
    },
    {
      data : [],
      label: "WFyellow",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "yellow",
      backgroundColor: "yellow",
      borderWidth: 0,
      pointBorderColor: "yellow",
      //      parsing: false,
    },
    {
      data : [],
      label: "WFred",
      pointRadius: 0.5,
      showLine: false,
      animation: false,
      pointStyle: "circle",
      pointBackgroundColor: "red",
      backgroundColor: "red",
      borderWidth: 0,
      pointBorderColor: "red",
      //     parsing: false,
    }
  ];
  //  help text from backend
  help_ack: string = "";

  public TRespOptions: ChartConfiguration<"line">[ "options" ] = {
    responsive : true,
    //    aspectRatio: 2,
    plugins: {
      legend: {display: true, labels: {boxWidth: 10, boxHeight: 10}},
      tooltip: {
        enabled: false,
      },
    },
  };
  TRespLabels: number[] = [ 0 ];
  public IQOptions: ChartConfiguration<"scatter">[ "options" ] = {
    responsive : true,
    aspectRatio: 1,
    //    scales: {
    //      xAxes: {
    //		  min: -5,
    //		  max:5,
    //      }
    //    },
    plugins: {
      legend: {display: true, labels: {boxWidth: 10, boxHeight: 10}},
      tooltip: {
        enabled: false,
      },
    },
  };

  public LLROptions: ChartConfiguration<"scatter">[ "options" ] = {
    responsive : true,
    aspectRatio: 3,
    scales: {
      xAxes: {
        min: 0,
      },
      //      yAxes: {
      //		  min: -100,
      //		  max: 100
      //      }
    },
    plugins: {
      legend: {display: true, labels: {boxWidth: 10, boxHeight: 10}},
      tooltip: {
        enabled: false,
      },
    },
  };

  public WFOptions: ChartConfiguration<"scatter">[ "options" ] = {
    responsive : true,
    aspectRatio: 5,
    scales: {
      xAxes: {
        min: 0,
        //		  max:800,
      },
      yAxes: {
        min: 0,
        max: 80,
        reverse: true,
      }
    },
    plugins: {
      legend: {display: true, labels: {boxWidth: 10, boxHeight: 10}},
      tooltip: {
        enabled: false,
      },
    },
  }

  constructor(private scopeApi: ScopeApi, private wsService: WebSocketService, public helpApi: HelpApi)
  {
    console.log("Scope constructor ");
  }

  ngOnInit()
  {
    console.log("Scope ngOnInit ");
    this.scopeApi.getScopeInfos$().subscribe(resp => { this.configScope(resp); });

    this.helpApi.getHelpText("scope", "control", "dataack").subscribe(resp => { this.help_ack = resp; }, err => { this.help_ack = ""; });
  }

  ngOnDestroy()
  {
    console.log("Scope ngOnDestroy ");
    this.wsSubscription?.unsubscribe();
  }

  DecodScopeBinmsgToString(message: ArrayBuffer): string
  {
    const enc = new TextDecoder("utf-8");
    return enc.decode(message);
  }

  private configScope(resp: IScopeDesc)
  {
    if (resp.title === "none") {
      this.ScopeEnabled.emit(false);
    } else {
      this.ScopeEnabled.emit(true);
      this.scopetitle = resp.title;
      this.iqgraph_list.length = 0;
      this.llrgraph_list.length = 0;
      for (let graphIndex = 0; graphIndex < resp.graphs.length; graphIndex++) {
        if (resp.graphs[graphIndex].type == IScopeGraphType.IQs) {
          this.iqgraph_list.push(resp.graphs[graphIndex]);
          this.IQDatasets[this.iqgraph_list.length - 1].label = resp.graphs[graphIndex].title;
        }
        if (resp.graphs[graphIndex].type == IScopeGraphType.LLR) {
          this.llrgraph_list.push(resp.graphs[graphIndex]);
          this.LLRDatasets[this.llrgraph_list.length - 1].label = resp.graphs[graphIndex].title;
        }
        if (resp.graphs[graphIndex].type == IScopeGraphType.WF) {
          this.WFgraph_list.push(resp.graphs[graphIndex]);
        }
        if (resp.graphs[graphIndex].type == IScopeGraphType.TRESP) {
          this.TRespgraph = resp.graphs[graphIndex];
          this.TRespDatasets[0].label = resp.graphs[graphIndex].title;
        }
      }
      this.charts?.forEach((child) => {child.chart?.update()});
    }
  }

  private ProcessScopeMsg(message: RxScopeMessage)
  {
    if (this.scopestatus === "starting") {
      this.scopestatus = "started";
      this.startstop = "stop";
      this.startstop_color = "started";
    }
    let d = 0;
    let x = 0;
    let y = 0;
    switch (message.msgtype) {
      case SCOPEMSG_TYPE_TIME:
        this.scopetime = this.DecodScopeBinmsgToString(message.content);
        break;
      case SCOPEMSG_TYPE_DATAFLOW:
        let infobuff = this.DecodScopeBinmsgToString(message.content).split("|");
        if (infobuff.length >= 2) {
          this.skippedmsg = infobuff[0]!;
          this.bufferedmsg = infobuff[1]!;
        }
        break;
      case SCOPEMSG_TYPE_DATA:
        const bufferview = new DataView(message.content);
        if (message.update) {
          console.log("Starting scope update chart " + message.chartid.toString() + ", dataset " + message.dataid.toString() + " data length: " + bufferview.byteLength);
        }
        switch (message.chartid) {
          case SCOPEMSG_DATA_TRESP:
            d = 0;
            for (let i = 0; i < bufferview.byteLength; i = i + 4) {
              this.TRespDatasets[0].data[d] = {x : d, y : bufferview.getFloat32(i, true)};
              this.TRespLabels[d] = d;
              d++;
            }
            if (message.update) {
              this.trespchart!.update();
              console.log(" TRESP view update completed " + d.toString() + " points ");
            }
            break;
          case SCOPEMSG_DATA_IQ:
            this.IQDatasets[message.dataid].data.length = 0;
            for (let i = 0; i < bufferview.byteLength; i = i + 4) {
              this.IQDatasets[message.dataid].data[i / 4] = {x : bufferview.getInt16(i, true), y : bufferview.getInt16(i + 2, true)};
            }

            if (message.update) {
              this.iqcchart!.update();
            }
            break;
          case SCOPEMSG_DATA_LLR:
            this.LLRDatasets[message.dataid].data.length = 0;
            let xoffset = 0;
            d = 0;
            for (let i = 4; i < (bufferview.byteLength - 2); i = i + 4) {
              xoffset = xoffset + bufferview.getInt16(i + 2, true);
              this.LLRDatasets[message.dataid].data[d] = {x : xoffset, y : bufferview.getInt16(i, true)};
              d++;
            }
            this.LLRDatasets[message.dataid].data[d] = {x : bufferview.getInt32(0, true), y : 0};
            if (message.update) {
              this.llrchart!.update();
            }

            break;
          case SCOPEMSG_DATA_WF:
            if (message.update) {
              if (message.segnum == 0) {
                for (let i = 0; i < this.WFDatasets.length; i++) {
                  this.nwf[i] = 0;
                  this.WFDatasets[i].data.length = 0;
                }
              }
            }
            for (let i = 2; i < bufferview.byteLength - 2; i = i + 4) { // first 16 bits in buffer contains the number of points in the message
              x = bufferview.getInt16(i, true);
              y = bufferview.getInt16(i + 2, true);
              this.wfchart!.scales.xAxes.max = bufferview.getInt16(bufferview.byteLength - 4, true);
              this.WFDatasets[message.dataid].data[this.nwf[message.dataid]] = {x : x, y : y};
              this.nwf[message.dataid]++;
            }
            if (message.update) {
              this.wfchart!.update();
              console.log(" WF view update completed " + d.toString() + "points, ");
            }
            break;
          default:
            break;
        }

        this.sendMsg(SCOPEMSG_TYPE_DATAACK, "Chart " + message.chartid.toString() + ", dataset " + message.dataid.toString());
        break;
      default:
        break;
    }
  }

  private sendMsg(type: number, strmessage: string)
  {
    this.wsService.send({source : webSockSrc.softscope, fullbuff : serialize({msgtype : type, content : strmessage})});
    console.log("Scope sent msg type " + type.toString() + " " + strmessage);
  }

  private SendScopeParams(name: string, value: string, graphid: number): number
  {
    let status = 0;
    this.scopeApi.setScopeParams$({name : name, value : value, graphid : graphid})
        .subscribe(response => { console.log(response.status); },
                   err => {
                     console.log("scope SendScopeParams: error received: " + err);
                     this.StopScope();
                   },
                   () => console.log("scope SendScopeParams OK"));
    return status;
  }

  private StopScope()
  {
    if (this.wsSubscription)
      this.wsSubscription.unsubscribe();

    this.scopestatus = "stopped";
    this.startstop = "start";
    this.startstop_color = "warn";
    this.skippedmsg = "0";
    this.bufferedmsg = "0";
  }

  startorstop()
  {
    if (this.scopestatus === "stopped") {
      let status = this.SendScopeParams("startstop", "start", 0);
      if (status == 0) {
        this.llrchart = Chart.getChart("llr");
        this.iqcchart = Chart.getChart("iqc");
        this.wfchart = Chart.getChart("wf");
        this.trespchart = Chart.getChart("tresp");
        this.IQDatasets.forEach((dataset) => {dataset.data.length = 0});
        this.LLRDatasets.forEach((dataset) => {dataset.data.length = 0});
        this.WFDatasets.forEach((dataset) => {dataset.data.length = 0});
        this.TRespDatasets.forEach((dataset) => {dataset.data.length = 0});
        this.charts?.forEach((child, index) => {child.chart?.update()});
        this.scopestatus = "starting";
        this.SigChanged(this.selected_sig.target_id);
        this.OnRefrateChange();
        this.OnIQxminChange();
        this.OnIQxmaxChange();
        this.OnIQyminChange();
        this.OnIQymaxChange();
        this.OnYthreshChange();
        this.OnLLRxminChange();
        this.OnLLRxmaxChange();
        this.channelsChanged(this.selected_channels);
        this.llrchannelsChanged(this.selected_llrchannels);
        this.WFChanged(this.selected_WF);
        this.onEnableTResp();
        for (let i = 0; i < this.WFDatasets.length; i++) {
          this.nwf[i] = 0;
          this.WFDatasets[i].data.length = 0;
        }
        this.wsService = new (WebSocketService);

        this.wsSubscription = this.wsService.subject$.subscribe(msg => this.ProcessScopeMsg(deserialize(msg.fullbuff)),
                                                                err => {
                                                                  console.error("WebSocket Observer got an error: " + err);
                                                                  this.StopScope()
                                                                },
                                                                () => { console.error("WebSocket Observer completed: "); })
      }
    } else {
      this.SendScopeParams("startstop", "stop", 0);
      this.StopScope();
    }
  }

  OnRefrateChange()
  {
    this.SendScopeParams("refrate", (this.rfrate * 10).toString(), 0);
  }

  OnLLRxminChange()
  {
    this.SendScopeParams("llrxmin", (this.llrxmin).toString(), 0);
  }
  OnLLRxmaxChange()
  {
    this.SendScopeParams("llrxmax", (this.llrxmax).toString(), 0);
  }

  OnIQxminChange()
  {
    this.SendScopeParams("xmin", (this.iqxmin).toString(), 0);
  }
  OnIQxmaxChange()
  {
    this.SendScopeParams("xmax", (this.iqxmax).toString(), 0);
  }
  OnIQyminChange()
  {
    this.SendScopeParams("ymin", (this.iqymin).toString(), 0);
  }
  OnIQymaxChange()
  {
    this.SendScopeParams("ymax", (this.iqymax).toString(), 0);
  }

  channelsChanged(titles: string[])
  {
    this.selected_channels = titles;
    this.iqgraph_list.forEach(graph => {
      const [enabled] = titles.filter(value => graph.title === value);
      this.SendScopeParams("enabled", enabled ? "true" : "false", graph.srvidx);
    })
  }

  llrchannelsChanged(titles: string[])
  {
    this.selected_llrchannels = titles;
    this.llrgraph_list.forEach(graph => {
      const [enabled] = titles.filter(value => graph.title === value);
      this.SendScopeParams("enabled", enabled ? "true" : "false", graph.srvidx);
    })
  }

  SigChanged(value: number)
  {
    this.selected_sig.target_id = value;
    if (this.scopetitle === "gNB")
      this.scopesubtitle = " - sig from UE" + value.toString() + " antenna" + this.selected_sig.antenna_id;
    else
      this.scopesubtitle = " - sig from gNB" + value.toString() + " antenna" + this.selected_sig.antenna_id;
    ;
    this.SendScopeParams("TargetSelect", value.toString(), 0);
  }

  WFChanged(value: string)
  {
    this.selected_WF = value;

    for (let i = 0; i < this.WFgraph_list.length; i++) {
      if (this.WFgraph_list[i].title === value) {
        this.SendScopeParams("enabled", "true", this.WFgraph_list[i].srvidx);
        this.WFDatasets[0].label = value;
        this.WFDatasets[1].label = "(>avg*2)";
        this.WFDatasets[2].label = "(>avg*10)";
        this.WFDatasets[3].label = "(>avg*100)";
      } else {
        this.SendScopeParams("enabled", "false", this.WFgraph_list[i].srvidx);
      }
    }
    for (let i = 0; i < this.WFDatasets.length; i++) {
      this.nwf[i] = 0;
      this.WFDatasets[i].data.length = 0;
    }
  }
  onEnableTResp()
  {
    this.SendScopeParams("enabled", this.enable_TResp.toString(), this.TRespgraph.srvidx);
  }

  onDataACKchange()
  {
    this.SendScopeParams("DataAck", this.data_ACK.toString(), 0);
  }

  OnYthreshChange()
  {
    this.SendScopeParams("llrythresh", this.llrythresh.toString(), 0);
  }

  RefreshTResp()
  {
    this.TRespDatasets[0].data.length = 0;
    this.trespchart!.update();
  }

  RefreshIQV()
  {
    this.IQDatasets.forEach((dataset) => {dataset.data.length = 0});
    this.iqxmin = this.iqmin;
    this.iqymin = this.iqmin;
    this.iqxmax = this.iqmax;
    this.iqymax = this.iqmax;
    this.iqcchart!.update();
  }

  RefreshLLR()
  {
    this.LLRDatasets.forEach((dataset) => {dataset.data.length = 0});
    this.llrxmin = this.llrmin;
    this.llrxmax = this.llrmax;
    this.llrchart!.update();
  }

  RefreshWF()
  {
    this.WFDatasets.forEach((dataset) => {dataset.data.length = 0});
    this.nwf = [ 0, 0, 0, 0 ];
    this.wfchart!.update();
  }
}
