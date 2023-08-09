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

/*! \file common/utils/websrv/frontend/src/app/controls/row.control.ts
 * \brief: implementation of web interface frontend for oai
 * \implement one row results for commands component
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {FormControl, UntypedFormArray, UntypedFormGroup} from "@angular/forms";
import {IArgType} from "src/commondefs";
import {IParam, IRow, IVariable} from "../api/commands.api";

import {ParamCtrl} from "./param.control";

enum RowFCN {
  paramsFA = "params",
}

export class RowCtrl extends UntypedFormGroup {
  cmdName: string;
  rawIndex: number;
  cmdparam?: IVariable;

  constructor(row: IRow)
  {
    super({})

        this.cmdName = row.cmdName;
    this.rawIndex = row.rawIndex;
    this.addControl(RowFCN.paramsFA, new UntypedFormArray(row.params.map(param => new ParamCtrl(param))));
  }

  get paramsFA()
  {
    return this.get(RowFCN.paramsFA) as UntypedFormArray
  }

  set paramsFA(fa: UntypedFormArray)
  {
    this.setControl(RowFCN.paramsFA, fa);
  }

  set_cmdparam(cmdparam: IVariable)
  {
    this.cmdparam = cmdparam;
  }

  get paramsCtrls(): ParamCtrl[]{return this.paramsFA.controls as ParamCtrl[]}

  api()
  {
    const doc: IRow = {
      rawIndex : this.rawIndex,
      cmdName : this.cmdName,
      params : this.paramsCtrls.map(control => control.api()),
      param : this.cmdparam
    }

    return doc
  }
}
