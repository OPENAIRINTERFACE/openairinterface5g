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

/*! \file common/utils/websrv/frontend/src/app/api/commands.api.ts
 * \brief: implementation of web interface frontend for oai
 * \api's definitions for the commands module, which provides web interface to telnet server commands
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
import {route, IArgType, IInfo} from "src/commondefs";
export interface IModule {
  name: string;
}

export enum ILogLvl {
  error = "error",
  warn = "warn",
  analysis = "analysis",
  info = "info",
  debug = "debug",
  trace = "trace"
}

export enum ILogOutput {
  stdout = "stdout",
  telnet = "telnet",
  web = "web",
  file = "/tmp/<component>.log",
}

export enum ICommandOptions {
  update = "update", // result can be updated, triggers update button on result page when set
  help = "help" // help tooltip available on command buttons
}

export interface IVariable {
  name: string;
  value: string;
  type: IArgType;
  modifiable: boolean; // set command ?
}

export interface ICommand {
  name: string;
  confirm?: string;
  question?: IQuestion[];
  param?: IVariable[];
  options?: ICommandOptions[];
}
export interface ITable {
  columns: IColumn[];
  rows: string[];
}
export interface IQuestion {
  display: string;
  pname: string;
  type: IArgType;
}
export interface IResp {
  display: string[], table?: ITable
}

export interface IParam {
  value: string, col: IColumn
}

export interface IColumn { // should use IVariable ?
  name: string;
  type: IArgType;
  modifiable: boolean; // set command ?
  help: boolean; // is help available
}
export interface IRow {
  params: IParam[], rawIndex: number, cmdName: string,
      param?: IVariable // to transmit the initial command parameter, ex: the channel model index when modify a channel model
}


@Injectable({
  providedIn : "root",
})
export class CommandsApi {
  constructor(private httpClient: HttpClient)
  {
  }

  public readModules$ = () => this.httpClient.get<IModule[]>(environment.backend + route + "commands/");

  public readVariables$ = (moduleName: string) => this.httpClient.get<IInfo[]>(environment.backend + route + moduleName + "/variables/");

  public readCommands$ = (moduleName: string) => this.httpClient.get<ICommand[]>(environment.backend + route + moduleName + "/commands/");

  public runCommand$ = (command: ICommand, moduleName: string) => this.httpClient.post<IResp>(environment.backend + route + moduleName + "/commands/", command);

  public setCmdVariable$ = (variable: IInfo, moduleName: string) => this.httpClient.post<IResp>(environment.backend + route + moduleName + "/variables/", variable);

  public setCmdParams$ = (row: IRow, moduleName: string) => this.httpClient.post<IResp>(environment.backend + route + moduleName + "/set/", row);
}
