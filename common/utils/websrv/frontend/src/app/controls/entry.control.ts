import { FormControl } from '@angular/forms';
import { IArgType } from '../api/commands.api';


export class EntryFC extends FormControl {

  type: IArgType

  constructor(entry: string, type: IArgType) {
    super(entry);
    this.type = type
  }
}

