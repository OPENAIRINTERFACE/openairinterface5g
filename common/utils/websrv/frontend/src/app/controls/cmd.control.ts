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

/*! \file common/utils/websrv/frontend/src/app/controls/cmd.control.ts
 * \brief: implementation of web interface frontend for oai
 * \implement a command for commands component
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

import {UntypedFormArray, UntypedFormControl, UntypedFormGroup} from "@angular/forms";
import {Subscription} from "rxjs";
import {Observable} from "rxjs/internal/Observable";
import {ICommand, ICommandOptions, IQuestion, IVariable} from "src/app/api/commands.api";
import {HelpApi, HelpRequest, HelpResp} from "src/app/api/help.api";

const enum CmdFCN {
  name = "name",
  vars = "variables",
  confirm = "confirm",
  answer = "answer",
  answerb = "answerb"
}

export class CmdCtrl extends UntypedFormGroup {
  confirm?: string;
  question?: IQuestion[];
  cmdname: string;
  options?: ICommandOptions[];
  public ResUpdTimer?: Observable<number>;
  public ResUpdTimerSubscriber?: Subscription;
  updbtnname: string;
  hlp_cmd: string = "";

  constructor(cmd: ICommand)
  {
    super({});

    this.addControl(CmdFCN.name, new UntypedFormControl(cmd.name));
    this.addControl(CmdFCN.answer, new UntypedFormControl(""));
    this.addControl(CmdFCN.answerb, new UntypedFormControl(""));
    this.addControl(CmdFCN.vars, new UntypedFormArray([]));

    this.confirm = cmd.confirm;
    this.question = cmd.question;
    this.cmdname = cmd.name;
    this.options = cmd.options;
    this.updbtnname = "Start update";
  }

  api()
  {
    const doc: ICommand = {
      name : this.nameFC.value,
      param : this.question ? this.setParams() : undefined,
//      param : this.question ? {name : this.question!.pname, value : this.answerFC.value, type : this.question!.type, modifiable : false} : undefined,
      options : this.options
    };

    return doc;
  }
  
  setParams ()
  {
  var vars : IVariable[]=new Array();
	 for (let i = 0; i < this.question!.length; i++) {
		  vars.push({name:this.question![i].pname,
		             value:(i==0)?this.answerFC.value:this.answerbFC.value,
		             type:this.question![i].type,
		             modifiable:false })
	 } 
	 return vars;
  }
  
  isResUpdatable(): boolean
  {
    if (this.options) {
      for (let opt = 0; opt < this.options.length; opt++) {
        if (this.options[opt] == ICommandOptions.update)
          return true;
      }
    } else {
      return false;
    }
    return false;
  }

  stopUpdate()
  {
    if (this.ResUpdTimerSubscriber) {
      this.updbtnname = "Start update"
    }
  }

  startUpdate()
  {
    if (this.ResUpdTimerSubscriber && this.ResUpdTimer) {
      this.updbtnname = "Stop update"
    }
  }

  get nameFC()
  {
    return this.get(CmdFCN.name) as UntypedFormControl;
  }

  set nameFC(fc: UntypedFormControl)
  {
    this.setControl(CmdFCN.name, fc);
  }

  get answerFC()
  {
    return this.get(CmdFCN.answer) as UntypedFormControl;
  }
  
  get answerbFC()
  {
    return this.get(CmdFCN.answerb) as UntypedFormControl;
  }

  get varsFA()
  {
    return this.get(CmdFCN.vars) as UntypedFormArray;
  }

  set varsFA(fa: UntypedFormArray)
  {
    this.setControl(CmdFCN.vars, fa);
  }

  public get_cmd_help(helpApi: HelpApi, module: string)
  {
    if (this.options) {
      for (let j = 0; j < this.options!.length; j++) {
        if (this.options![j] == ICommandOptions.help) {
          helpApi.getHelpText("cmd", module, this.cmdname).subscribe(resp => { this.hlp_cmd = resp; }, err => { this.hlp_cmd = ""; });
        }
      }
    }
  }
}
