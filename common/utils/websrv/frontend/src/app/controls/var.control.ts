import {UntypedFormControl, UntypedFormGroup} from "@angular/forms";
import {IInfo} from "../api/commands.api";
import {IArgType} from "../api/commands.api";

const enum VariablesFCN {
  name = "name",
  value = "value",
  type = "type",
  modifiable = "modifiable"
}

export class VarCtrl extends UntypedFormGroup {
  type: IArgType;
  constructor(ivar: IInfo)
  {
    super({});
    this.type = ivar.type;
    this.addControl(VariablesFCN.name, new UntypedFormControl(ivar.name));
    this.addControl(VariablesFCN.value, new UntypedFormControl(ivar.value));
    this.addControl(VariablesFCN.type, new UntypedFormControl(ivar.type));
    this.addControl(VariablesFCN.modifiable, new UntypedFormControl(ivar.modifiable));
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
    return this.get(VariablesFCN.name) as UntypedFormControl;
  }

  set nameFC(control: UntypedFormControl)
  {
    this.setControl(VariablesFCN.name, control);
  }

  get valueFC()
  {
    return this.get(VariablesFCN.value) as UntypedFormControl;
  }

  set valueFC(control: UntypedFormControl)
  {
    this.setControl(VariablesFCN.value, control);
  }

  get typeFC()
  {
    return this.get(VariablesFCN.type) as UntypedFormControl;
  }

  set typeFC(control: UntypedFormControl)
  {
    this.setControl(VariablesFCN.type, control);
  }

  get modifiableFC()
  {
    return this.get(VariablesFCN.modifiable) as UntypedFormControl;
  }

  set modifiableFC(control: UntypedFormControl)
  {
    this.setControl(VariablesFCN.modifiable, control);
  }

  get btnTxtFC()
  {
    if (this.type != IArgType.configfile)
      return "set"
      else return "download"
  }
}
