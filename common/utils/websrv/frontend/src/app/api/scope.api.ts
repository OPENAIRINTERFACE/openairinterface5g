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

/*! \file common/utils/websrv/frontend/src/app/scope/scope.api.ts
 * \brief: implementation of web interface frontend for oai
 * \api's definitions for the scope module, which provides a web interface to the oai soft scope
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {HttpClient} from "@angular/common/http";
import {Injectable} from "@angular/core";
import {environment} from "src/environments/environment";
import {route} from "src/commondefs";

export enum IScopeGraphType {
  IQs = "IQs",
  LLR = "LLR",
  WF = "WF",
  TRESP = "TRESP",
  UNSUP = "UNSUP"
}

export interface IGraphDesc {
  title: string;
  type: IScopeGraphType;
  id: number;
  srvidx: number;
}

export interface IScopeDesc {
  title: string;
  graphs: IGraphDesc[];
}

export interface IScopeCmd {
  name: string;
  graphid?: number; // the graph srvidx
  value: string;
}

export interface ISigDesc {
  target_id: number;
  antenna_id: number;
}

const scoperoute = route + "/scopectrl/";

@Injectable({
  providedIn : "root",
})
export class ScopeApi {
  constructor(private httpClient: HttpClient)
  {
  }

  public getScopeInfos$ = () => this.httpClient.get<IScopeDesc>(environment.backend + scoperoute);

  public setScopeParams$ = (cmd: IScopeCmd) => this.httpClient.post(environment.backend + scoperoute, cmd, {observe : "response"});
}
