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

/*! \file common/utils/websrv/frontend/src/app/api/help.api.ts
 * \brief: implementation of web interface frontend for oai
 * \api's definitions for the help module, which can be used to provides help text via tooltips
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
import {Observable} from "rxjs";
import {map} from "rxjs/operators";
import {environment} from "src/environments/environment";
import {route} from "src/commondefs";

export interface HelpRequest {
  module: string;
  command: string;
  object: string;
}

export interface HelpResp {
  text: string;
}
const hlproute = route + "/helpfiles/";

@Injectable({
  providedIn : "root",
})

export class HelpApi {
  constructor(private httpClient: HttpClient)
  {
  }

  public getHelp$ = (req: HelpRequest) => this.httpClient.get<HelpResp>(environment.backend + hlproute + req.module + "_" + req.command + "_" + req.object + ".html", {observe : "response"});

  public getHelpText(module: string, command: string, object: string): Observable<string>
  {
	  
    return this.getHelp$({module : module, command : command.replace(" ", "_"), object : object.replace(" ", "_")})
        .pipe(map(
            (response => { return (response.status == 201) ? response.body!.text.replace(/<!--(?:.|\n)*?-->/gm, '') : ""; }),
            )); // pipe
  }
}
