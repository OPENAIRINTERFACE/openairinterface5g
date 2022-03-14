/* eslint-disable no-shadow */

import { FormControl, FormGroup } from '@angular/forms';
import { INewBooking, IPlatform } from '../api/bookings.api';
import { Guest, GuestCtrl } from './guest.control';
import { Range, RangeCtrl } from './range.control';


const enum NewBookingFCN {
  guest = 'guest',
  amount = 'amount',
  platform = 'platform',
  range = 'range'
}

export class NewBooking {
  amount: number;
  platform: IPlatform;
  guest: Guest;
  range: Range;

  constructor(iNewBooking: INewBooking) {
    this.amount = iNewBooking.amount!;
    this.platform = iNewBooking.platform!;
    this.range = new Range(iNewBooking.range!);
    this.guest = new Guest(iNewBooking.guest!);
  }
}

export class NewBookingCtrl extends FormGroup {

  constructor(inewbooking: INewBooking) {
    super({});

    const newbooking = new NewBooking(inewbooking);

    this.addControl(NewBookingFCN.amount, new FormControl(newbooking.amount));
    this.addControl(NewBookingFCN.platform, new FormControl(newbooking.platform));
    this.addControl(NewBookingFCN.range, new RangeCtrl(newbooking.range));
    this.addControl(NewBookingFCN.guest, new GuestCtrl(newbooking.guest));
  }

  api() {
    const doc: INewBooking = {
      amount: this.amountFC.value,
      platform: this.platformFC.value,
      range: this.rangeCtrl.api(),
      guest: this.guestCtrl.api(),
    };

    return doc;
  }

  get guestCtrl() {
    return this.get(NewBookingFCN.guest) as GuestCtrl;
  }

  set guestCtrl(control: GuestCtrl) {
    this.setControl(NewBookingFCN.guest, control);
  }

  get rangeCtrl() {
    return this.get(NewBookingFCN.range) as RangeCtrl;
  }

  set rangeCtrl(control: RangeCtrl) {
    this.setControl(NewBookingFCN.range, control);
  }

  get amountFC() {
    return this.get(NewBookingFCN.amount) as FormControl;
  }

  set amountFC(control: FormControl) {
    this.setControl(NewBookingFCN.amount, control);
  }

  get platformFC() {
    return this.get(NewBookingFCN.platform) as FormControl;
  }

  set platformFC(control: FormControl) {
    this.setControl(NewBookingFCN.platform, control);
  }

  get shortName() {
    return this.guestCtrl.givenFC.value + ' (' + this.platformFC.value + ')';
  }
}
