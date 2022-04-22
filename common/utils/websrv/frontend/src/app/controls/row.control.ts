import { FormControl, FormGroup } from '@angular/forms';
import { IArgType, IRow } from '../api/commands.api';



export class RowCtrl extends FormGroup {

  type: IArgType

  constructor(irow: IRow, type: IArgType) {
    super({});
    this.type = type
    this.addControl(type, new FormControl(irow));
  }

  api() {

    switch (this.type) {
      case IArgType.boolean:
        return this.boolFC.value ? "true" : "false";
      // case IArgType.list:
      //   return this.listFC.value as string;
      case IArgType.number:
        return this.numberFC.value as string;
      // case IArgType.range:
      //   return this.rangeFC.value as string;
      case IArgType.string:
        return this.stringFC.value
    }
  }

  get boolFC() {
    return this.get(IArgType.boolean) as FormControl;
  }

  set boolFC(control: FormControl) {
    this.setControl(IArgType.boolean, control);
  }

  get listFC() {
    return this.get(IArgType.list) as FormControl;
  }

  set listFC(control: FormControl) {
    this.setControl(IArgType.list, control);
  }

  get numberFC() {
    return this.get(IArgType.number) as FormControl;
  }

  set numberFC(control: FormControl) {
    this.setControl(IArgType.number, control);
  }

  get rangeFC() {
    return this.get(IArgType.range) as FormControl;
  }

  set rangeFC(control: FormControl) {
    this.setControl(IArgType.range, control);
  }

  get stringFC() {
    return this.get(IArgType.string) as FormControl;
  }

  set stringFC(control: FormControl) {
    this.setControl(IArgType.string, control);
  }
}

