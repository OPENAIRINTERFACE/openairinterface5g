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

/*! \file common/utils/websrv/frontend/src/app/controls/info.control.ts
 * \brief: implementation of web interface frontend for oai
 * \implement one info item for info component
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

const enum InfosFCN {
  name = "name",
  value = "value",
  type = "type",
  modifiable = "modifiable"
}

export class InfoCtrl extends UntypedFormGroup {
  type: IArgType;
  constructor(ivar: IInfo)
  {
    super({});
    this.type = ivar.type;
    this.addControl(InfosFCN.name, new UntypedFormControl(ivar.name));
    this.addControl(InfosFCN.value, new UntypedFormControl(ivar.value));
    this.addControl(InfosFCN.type, new UntypedFormControl(ivar.type));
    this.addControl(InfosFCN.modifiable, new UntypedFormControl(ivar.modifiable));
  }

  api()
  {
    const doc: IInfo = {
      name : this.nameFC.value,
      value : String(this.valueFC.value), // FIXME
      type : this.typeFC.value,
      modifiable : this.modifiableFC.value
    };

    return doc;
  }

  get nameFC()
  {
    return this.get(InfosFCN.name) as UntypedFormControl;
  }

  set nameFC(control: UntypedFormControl)
  {
    this.setControl(InfosFCN.name, control);
  }

  get valueFC()
  {
    return this.get(InfosFCN.value) as UntypedFormControl;
  }

  set valueFC(control: UntypedFormControl)
  {
    this.setControl(InfosFCN.value, control);
  }

  get typeFC()
  {
    return this.get(InfosFCN.type) as UntypedFormControl;
  }

  set typeFC(control: UntypedFormControl)
  {
    this.setControl(InfosFCN.type, control);
  }

  get modifiableFC()
  {
    return this.get(InfosFCN.modifiable) as UntypedFormControl;
  }

  set modifiableFC(control: UntypedFormControl)
  {
    this.setControl(InfosFCN.modifiable, control);
  }

  get btnTxtFC()
  {
    if (this.type != IArgType.configfile)
      return "set"
      else return "download"
  }
}
