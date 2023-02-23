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

/*! \file common/utils/websrv/frontend/src/app/components/info/info.component.ts
 * \brief: implementation of web interface frontend for oai
 * \info component web interface implementation (works with info.component.html)
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {Component} from "@angular/core";
import {IArgType, IInfo} from "src/commondefs";
import {ViewEncapsulation} from "@angular/core";
import {UntypedFormArray} from "@angular/forms";
import {Observable} from "rxjs";
import {filter, map, switchMap, tap} from "rxjs/operators";
import {InfoApi} from "src/app/api/info.api";
import {InfoCtrl} from "src/app/controls/info.control";
import {ModuleCtrl} from "src/app/controls/module.control";
import {VarCtrl} from "src/app/controls/var.control";
import {DialogService} from "src/app/services/dialog.service";
import {DownloadService} from "src/app/services/download.service";

    @Component({
      selector : "app-info",
      templateUrl : "./info.component.html",
      styleUrls : [ "./info.component.scss" ],
      encapsulation : ViewEncapsulation.None,
    }) export class InfoComponent {

  infos$: Observable<VarCtrl[]>;



  constructor(
      public infoApi: InfoApi,
      public downloadService: DownloadService,

  )
  {
    this.infos$ = this.infoApi.readInfos$().pipe(map((infos) => infos.map(info => new InfoCtrl(info))));
  }


  onInfoSubmit(control: InfoCtrl)
  {
    let info: IInfo = control.api();

    if (info.type === IArgType.configfile) {
      this.downloadService.getFile(info.value)
    }
  }

}
