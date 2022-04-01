import { FormArray, FormControl } from '@angular/forms';
import { ISubCommands } from '../api/commands.api';
import { OptionsCtrl } from './options.control';

const enum SubCmdsFCN {
  names = 'names',
}

export class SubCmdCtrl extends OptionsCtrl {

  constructor(isubs: ISubCommands) {
    super(isubs);

    this.addControl(SubCmdsFCN.names, new FormArray(isubs.name.map((name) => new FormControl(name))));
  }

  api() {
    const doc: ISubCommands = {
      type: this.type,
      name: this.namesFA.value
    };

    return doc;
  }

  get namesFA() {
    return this.get(SubCmdsFCN.names) as FormArray;
  }

  set namesFA(fa: FormArray) {
    this.setControl(SubCmdsFCN.names, fa);
  }
}

