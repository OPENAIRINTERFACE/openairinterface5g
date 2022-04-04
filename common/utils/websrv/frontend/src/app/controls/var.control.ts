import { FormControl, FormGroup } from '@angular/forms';
import { IVariable } from '../api/commands.api';


const enum VariablesFCN {
  name = 'name',
  value = "value",
  type = "type",
  modifiable = "modifiable"
}


export class VariableCtrl extends FormGroup {

  constructor(ivar: IVariable) {
    super({});

    this.addControl(VariablesFCN.name, new FormControl(ivar.name));
    this.addControl(VariablesFCN.value, new FormControl(ivar.value));
    this.addControl(VariablesFCN.type, new FormControl(ivar.type));
    this.addControl(VariablesFCN.modifiable, new FormControl(ivar.modifiable));
  }

  api() {
    const doc: IVariable = {
      name: this.nameFC.value,
      value: this.valueFC.value,
      type: this.typeFC.value,
      modifiable: this.modifiableFC.value
    };

    return doc;
  }

  get nameFC() {
    return this.get(VariablesFCN.name) as FormControl;
  }

  set nameFC(control: FormControl) {
    this.setControl(VariablesFCN.name, control);
  }

  get valueFC() {
    return this.get(VariablesFCN.value) as FormControl;
  }

  set valueFC(control: FormControl) {
    this.setControl(VariablesFCN.value, control);
  }

  get typeFC() {
    return this.get(VariablesFCN.value) as FormControl;
  }

  set typeFC(control: FormControl) {
    this.setControl(VariablesFCN.value, control);
  }

  get modifiableFC() {
    return this.get(VariablesFCN.modifiable) as FormControl;
  }

  set modifiableFC(control: FormControl) {
    this.setControl(VariablesFCN.modifiable, control);
  }
}

