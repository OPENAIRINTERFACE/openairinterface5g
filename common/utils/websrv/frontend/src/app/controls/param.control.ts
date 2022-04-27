import { FormControl } from '@angular/forms';
import { IArgType, IColumn } from '../api/commands.api';

export class Param {

  constructor(
    public value: string,
    public col: IColumn,
    public rawIndex: number,
    public moduleName: string,
    public cmdName: string) { }
}

export class ParamFC extends FormControl {

  constructor(public param: Param) {
    super(param.value);
  }

  api() {
    this.param.value = this.value
    return this.param
  }
}

