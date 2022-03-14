/* eslint-disable no-shadow */
import { FormControl, FormGroup } from '@angular/forms';
import { ICalendar } from '../api/users.api';

// export class Calendar implements ICalendar {
//   summary: string = '';
//   id: string = '';
// }

export class CalendarCtrl extends FormGroup {
  constructor(calendar: ICalendar) {
    super({});

    this.addControl(CalendarFCN.summary, new FormControl(calendar.summary));
    this.addControl(CalendarFCN.id, new FormControl(calendar.id));
  }

  get idFC() {
    return this.get(CalendarFCN.id) as FormControl;
  }

  set idFC(control: FormControl) {
    this.setControl(CalendarFCN.id, control);
  }

  get summaryFC() {
    return this.get(CalendarFCN.summary) as FormControl;
  }

  set summaryFC(control: FormControl) {
    this.setControl(CalendarFCN.summary, control);
  }

  api() {
    const calendar: ICalendar = {
      summary: this.summaryFC.value,
      id: this.idFC.value,
    };

    return calendar;
  }
}

enum CalendarFCN {
  id = 'id',
  summary = 'summary',
}
