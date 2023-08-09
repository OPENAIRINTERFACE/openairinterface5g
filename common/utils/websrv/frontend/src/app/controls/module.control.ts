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

/*! \file common/utils/websrv/frontend/src/app/controls/module.control.ts
 * \brief: implementation of web interface frontend for oai
 * \implement one module item for commands component
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {UntypedFormArray, UntypedFormGroup} from "@angular/forms";
import {IModule} from "../api/commands.api";

const enum ModuleFCN {
  vars = "variables",
  cmds = "commands"
}

export class ModuleCtrl extends UntypedFormGroup {
  name: string

  constructor(imodule: IModule)
  {
    super({});
    this.name = imodule.name;
    this.addControl(ModuleFCN.vars, new UntypedFormArray([]));
    this.addControl(ModuleFCN.cmds, new UntypedFormArray([]));
  }

  get varsFA()
  {
    return this.get(ModuleFCN.vars) as UntypedFormArray;
  }

  set varsFA(fa: UntypedFormArray)
  {
    this.setControl(ModuleFCN.vars, fa);
  }

  get cmdsFA()
  {
    return this.get(ModuleFCN.cmds) as UntypedFormArray;
  }

  set cmdsFA(fa: UntypedFormArray)
  {
    this.setControl(ModuleFCN.cmds, fa);
  }
}
