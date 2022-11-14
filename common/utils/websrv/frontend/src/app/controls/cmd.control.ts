import {UntypedFormArray, UntypedFormControl, UntypedFormGroup} from "@angular/forms";
import {Subscription} from "rxjs";
import {Observable} from "rxjs/internal/Observable";
import {ICommand, ICommandOptions, IQuestion} from "src/app/api/commands.api";
import {HelpApi, HelpRequest, HelpResp} from "src/app/api/help.api";

const enum CmdFCN {
  name = "name",
  vars = "variables",
  confirm = "confirm",
  answer = "answer"
}

export class CmdCtrl extends UntypedFormGroup {
  confirm?: string;
  question?: IQuestion;
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
    this.addControl(CmdFCN.vars, new UntypedFormArray([]));

    this.confirm = cmd.confirm;
    this.question = cmd.question;
    this.cmdname = cmd.name;
    this.options = cmd.options;
    this.updbtnname = "Start update"
  }

  api()
  {
    const doc: ICommand = {
      name : this.nameFC.value,
      param : this.question ? {name : this.question!.pname, value : this.answerFC.value, type : this.question!.type, modifiable : false} : undefined,
      options : this.options
    };

    return doc;
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
