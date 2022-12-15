import {UntypedFormControl, UntypedFormGroup} from "@angular/forms";
import {IArgType, IColumn, IParam} from "../api/commands.api";

enum ParamFCN {
  value = "value",
}

export class ParamCtrl extends UntypedFormGroup {
  col: IColumn
  constructor(public param: IParam)
  {
    super({})

        this.col = param.col

    let control: UntypedFormControl
    switch (param.col.type)
    {
      case IArgType.boolean:
        control = new UntypedFormControl((param.value === "true") ? true : false);
        break;

      case IArgType.loglvl:
        control = new UntypedFormControl(param.value);
        break;

      default:
        control = new UntypedFormControl(param.value)
    }

    if (!param.col.modifiable)
      control
          .disable()

              this.addControl(ParamFCN.value, control)
  }

  get valueFC()
  {
    return this.get(ParamFCN.value) as UntypedFormControl
  }

  set valueFC(fc: UntypedFormControl)
  {
    this.setControl(ParamFCN.value, fc);
  }

  api()
  {
    let value: string

    switch (this.col.type)
    {
      case IArgType.boolean:
        value = String(this.valueFC.value);
        break;

      default:
        value = this.valueFC.value
    }

    const doc: IParam = {
      value : value,
      col : this.col
    }

    return doc
  }
}
