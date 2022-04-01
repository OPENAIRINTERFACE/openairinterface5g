import { FormControl } from '@angular/forms';
import { IOptionType, IVariable } from '../api/commands.api';
import { OptionsCtrl } from './options.control';



const enum VariablesFCN {
  name = 'name',
  value = "value",
  modifiable = "modifiable"
}


export class VariableCtrl extends OptionsCtrl {

  constructor(ivar: IVariable) {
    super(ivar);

    this.addControl(VariablesFCN.name, new FormControl(ivar.name));
    this.addControl(VariablesFCN.value, new FormControl(ivar.value));
    this.addControl(VariablesFCN.modifiable, new FormControl(ivar.modifiable));
  }

  api() {
    const doc: IVariable = {
      type: this.type,
      name: this.nameFC.value,
      value: this.valueFC.value,
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

  get modifiableFC() {
    return this.get(VariablesFCN.modifiable) as FormControl;
  }

  set modifiableFC(control: FormControl) {
    this.setControl(VariablesFCN.modifiable, control);
  }
}

