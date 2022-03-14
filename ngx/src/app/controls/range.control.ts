import { FormControl, FormGroup } from '@angular/forms';
import * as moment from 'moment';
import { IRange } from '../api/bookings.api';

export class Range {
  startDay: Date;
  endDay: Date;
  checkin: string;
  checkout: string;

  constructor(iRange: IRange) {
    this.startDay = moment(iRange.start).local().toDate();
    this.endDay = moment(iRange.end).local().toDate();

    this.checkin = moment(iRange.start).local().format('HH:mm');
    this.checkout = moment(iRange.end).local().format('HH:mm');
  }
}

export class RangeCtrl extends FormGroup {
  constructor(range: Range) {
    super({});

    this.addControl(RangeFCN.startDay, new FormControl(range.startDay));
    this.addControl(RangeFCN.endDay, new FormControl(range.endDay));
    this.addControl(RangeFCN.checkin, new FormControl(range.checkin));
    this.addControl(RangeFCN.checkout, new FormControl(range.checkout));
  }

  get startDayFC() {
    return this.get(RangeFCN.startDay) as FormControl;
  }

  set startDayFC(control: FormControl) {
    this.setControl(RangeFCN.startDay, control);
  }

  get endDayFC() {
    return this.get(RangeFCN.endDay) as FormControl;
  }

  set endDayFC(control: FormControl) {
    this.setControl(RangeFCN.endDay, control);
  }

  get checkinFC() {
    return this.get(RangeFCN.checkin) as FormControl;
  }

  set checkinFC(control: FormControl) {
    this.setControl(RangeFCN.checkin, control); // TODO update booking state when specifying checkin hour
  }

  get checkoutFC() {
    return this.get(RangeFCN.checkout) as FormControl;
  }

  set checkoutFC(control: FormControl) {
    this.setControl(RangeFCN.checkout, control); // TODO update booking state when specifying checkin hour
  }

  api() {
    const checkin = moment(this.checkinFC.value, 'LTS');
    const start = moment(this.startDayFC.value).set({
      hour: checkin.get('hour'),
      minute: checkin.get('minute'),
    });

    const checkout = moment(this.checkoutFC.value, 'LTS');
    const end = moment(this.endDayFC.value).set({
      hour: checkout.get('hour'),
      minute: checkout.get('minute'),
    });

    const range: IRange = {
      start: start.format(),
      end: end.format(),
    };

    return range;
  }
}

enum RangeFCN {
  startDay = 'start',
  endDay = 'end',
  checkin = 'checkin',
  checkout = 'checkout',
}
