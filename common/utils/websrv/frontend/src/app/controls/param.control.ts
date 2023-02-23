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

/*! \file common/utils/websrv/frontend/src/app/controls/param.control.ts
 * \brief: implementation of web interface frontend for oai
 * \implement one parameter in a result row for commands component
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {UntypedFormControl, UntypedFormGroup} from "@angular/forms";
import {IArgType, IInfo} from "src/commondefs";
import { IParam, IColumn,} from "../api/commands.api";

enum ParamFCN {
  value = "value",
}

export class ParamCtrl extends UntypedFormGroup {
  col: IColumn
  constructor(public param: IParam)
  {
    super({})

        this.col = param.col

    let control: UntypedFormControl
    switch (param.col.type)
    {
      case IArgType.boolean:
        control = new UntypedFormControl((param.value === "true") ? true : false);
        break;

      case IArgType.loglvl:
        control = new UntypedFormControl(param.value);
        break;

      default:
        control = new UntypedFormControl(param.value)
    }

    if (!param.col.modifiable)
      control
          .disable()

              this.addControl(ParamFCN.value, control)
  }

  get valueFC()
  {
    return this.get(ParamFCN.value) as UntypedFormControl
  }

  set valueFC(fc: UntypedFormControl)
  {
    this.setControl(ParamFCN.value, fc);
  }

  api()
  {
    let value: string

    switch (this.col.type)
    {
      case IArgType.boolean:
        value = String(this.valueFC.value);
        break;

      default:
        value = this.valueFC.value
    }

    const doc: IParam = {
      value : value,
      col : this.col
    }

    return doc
  }
}
