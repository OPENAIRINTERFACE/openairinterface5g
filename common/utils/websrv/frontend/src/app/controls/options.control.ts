import { FormGroup } from "@angular/forms";
import { IOption, IOptionType } from "../api/commands.api";

export class OptionsCtrl extends FormGroup {

  type: IOptionType

  constructor(ioption: IOption) {
    super({});

    this.type = ioption.type
  }
}