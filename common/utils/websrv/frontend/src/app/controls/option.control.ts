/* eslint-disable no-shadow */

import { FormControl, FormGroup } from '@angular/forms';
import { IOption } from '../api/commands.api';


const enum OptionFCN {
    key = 'key',
    value = 'value',
}

export class Option {
    key: string;
    value: number;

    constructor(ioption: IOption) {
        this.key = ioption.key;
        this.value = ioption.value;
    }
}

export class OptionCtrl extends FormGroup {

    constructor(ioption: IOption) {
        super({});

        const option = new Option(ioption);

        this.addControl(OptionFCN.key, new FormControl(option.key));
        this.addControl(OptionFCN.value, new FormControl(option.value));
    }

    api() {
        const doc: IOption = {
            key: this.keyFC.value,
            value: this.valueFC.value,
        };

        return doc;
    }

    get keyFC() {
        return this.get(OptionFCN.key) as FormControl;
    }

    set keyFC(control: FormControl) {
        this.setControl(OptionFCN.key, control);
    }

    get valueFC() {
        return this.get(OptionFCN.value) as FormControl;
    }

    setvalueFC(control: FormControl) {
        this.setControl(OptionFCN.value, control);
    }
}
