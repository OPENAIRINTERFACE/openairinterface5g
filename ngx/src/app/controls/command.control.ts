/* eslint-disable no-shadow */

import { FormArray, FormControl, FormGroup } from '@angular/forms';
import { ICommand } from '../api/commands.api';
import { OptionCtrl } from './option.control';


const enum CommandFCN {
  name = 'name',
  options = 'options',
}

export class CommandCtrl extends FormGroup {

  constructor(icommand: ICommand) {
    super({});

    this.addControl(CommandFCN.name, new FormControl(icommand.name));
    this.addControl(CommandFCN.options, new FormArray(icommand.options.map((ioption) => new OptionCtrl(ioption))));
  }

  api() {
    const doc: ICommand = {
      name: this.nameFC.value,
      options: this.optionsFA.controls.map(control => (control as OptionCtrl).api()),
    };

    return doc;
  }

  get nameFC() {
    return this.get(CommandFCN.name) as FormControl;
  }

  set nameFC(control: FormControl) {
    this.setControl(CommandFCN.name, control);
  }

  get optionsFA() {
    return this.get(CommandFCN.options) as FormArray;
  }

  set optionsFA(control: FormArray) {
    this.setControl(CommandFCN.options, control);
  }
}
