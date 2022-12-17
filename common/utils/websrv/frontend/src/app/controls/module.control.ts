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
