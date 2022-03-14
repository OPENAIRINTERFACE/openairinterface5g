import { FormControl, FormGroup } from '@angular/forms';
import { ICleaner } from '../api/users.api';

export const DEFAULT_CLEANER_NAME = 'greenchap94@gmail.com';

export const DEFAULT_CLEANER: ICleaner = {
  email: DEFAULT_CLEANER_NAME,
  rate: 10,
  name: 'test cleaner',
  default: true
};

export class Cleaner implements ICleaner {
  rate: number;
  name: string;
  email: string;
  default: boolean;

  constructor(iCleaner: ICleaner) {
    this.rate = iCleaner.rate;
    this.email = iCleaner.email;
    this.name = iCleaner.name;
    this.default = iCleaner.default;
  }
}

export class CleanerCtrl extends FormGroup {
  constructor(cleaner: Cleaner) {
    super({});

    this.addControl(CleanerFCN.rate, new FormControl(cleaner.rate));
    this.addControl(CleanerFCN.email, new FormControl(cleaner.email));
    this.addControl(CleanerFCN.name, new FormControl(cleaner.name));
    this.addControl(CleanerFCN.default, new FormControl(cleaner.default));
  }

  static newCleanerCtrl() {
    return new CleanerCtrl(DEFAULT_CLEANER);
  }

  get rateFC() {
    return this.get(CleanerFCN.rate) as FormControl;
  }

  set rateFC(control: FormControl) {
    this.setControl(CleanerFCN.rate, control);
  }

  get emailFC() {
    return this.get(CleanerFCN.email) as FormControl;
  }

  set emailFC(control: FormControl) {
    this.setControl(CleanerFCN.email, control);
  }

  get nameFC() {
    return this.get(CleanerFCN.name) as FormControl;
  }

  set nameFC(control: FormControl) {
    this.setControl(CleanerFCN.name, control);
  }

  get defaultFC() {
    return this.get(CleanerFCN.default) as FormControl;
  }

  set defaultFC(control: FormControl) {
    this.setControl(CleanerFCN.default, control);
  }

  api() {
    const cleaner: ICleaner = {
      rate: this.rateFC.value,
      email: this.emailFC.value,
      name: this.nameFC.value,
      default: this.defaultFC.value
    };

    return cleaner;
  }
}

enum CleanerFCN {
  rate = 'rate',
  email = 'email',
  name = 'name',
  default = 'default',
}
