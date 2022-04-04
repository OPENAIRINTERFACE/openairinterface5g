import { FormControl, FormGroup } from '@angular/forms';
import { ICommand } from '../api/commands.api';

const enum CmdFCN {
  name = 'name',
}

export class CmdCtrl extends FormGroup {

  constructor(cmd: ICommand) {
    super({});

    this.addControl(CmdFCN.name, new FormControl(cmd.name));
  }

  api() {
    const doc: ICommand = {
      name: this.nameFC.value
    };

    return doc;
  }

  get nameFC() {
    return this.get(CmdFCN.name) as FormControl;
  }

  set nameFC(fc: FormControl) {
    this.setControl(CmdFCN.name, fc);
  }
}

