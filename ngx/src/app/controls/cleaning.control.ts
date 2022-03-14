import { FormControl, FormGroup } from '@angular/forms';
import { ICleaning } from '../api/bookings.api';


export class Cleaning implements ICleaning {
  cleanerEmail: string;
  hours: number;

  constructor(iCleaning: ICleaning) {
    this.cleanerEmail = iCleaning.cleanerEmail;
    this.hours = iCleaning.hours;
  }
}

export class CleaningCtrl extends FormGroup {
  hours: number;

  constructor(cleaning: ICleaning) {
    super({});

    this.hours = cleaning.hours;

    this.addControl(CleaningFCN.cleaner, new FormControl(cleaning.cleanerEmail));
  }

  get cleanerEmailFC() {
    return this.get(CleaningFCN.cleaner) as FormControl;
  }

  set cleanerEmailFC(control: FormControl) {
    this.setControl(CleaningFCN.cleaner, control);
  }

  api() {
    const cleaning: ICleaning = {
      hours: this.hours,
      cleanerEmail: this.cleanerEmailFC.value,
    };

    return cleaning;
  }
}

enum CleaningFCN {
  cleaner = 'cleaner',
}
