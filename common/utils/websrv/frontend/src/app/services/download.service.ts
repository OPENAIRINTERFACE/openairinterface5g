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

/*! \file common/utils/websrv/frontend/src/app/services/download.service.ts
 * \brief: implementation of web interface frontend for oai
 * \utility to download a file from backend
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {HttpClient} from "@angular/common/http";
import {HttpHeaders} from "@angular/common/http";
import {HttpParams} from "@angular/common/http";
import {Injectable} from "@angular/core";
import {route} from "src/commondefs";
import {environment} from "src/environments/environment";

@Injectable({
  providedIn : "root",
})

export class DownloadService {
  constructor(private http: HttpClient)
  {
  }

  getFile(url: string)
  {
    const token = "my JWT";
    const headers = new HttpHeaders().set("authorization", "Bearer " + token);
    const postparams = new HttpParams().set("fname", url);
    this.http.post(environment.backend + route + "/file", postparams, {headers, responseType : "blob" as "json"}).subscribe((response: any) => {
      let dataType = response.type;
      let binaryData = [];
      binaryData.push(response);
      let downloadLink = document.createElement("a");
      downloadLink.href = window.URL.createObjectURL(new Blob(binaryData, {type : dataType}));
      downloadLink.setAttribute("download", url);
      document.body.appendChild(downloadLink);
      downloadLink.click();
    })
  }
};
