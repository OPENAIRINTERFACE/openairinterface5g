/* eslint-disable no-shadow */

import { FormControl } from '@angular/forms';
import { IBooking, IState } from '../api/bookings.api';
import { Cleaning, CleaningCtrl } from './cleaning.control';
import { NewBooking, NewBookingCtrl } from './newbooking.control';


const enum BookingFCN {
  state = 'state',
  cleaning = 'cleaning',
}

export class Booking extends NewBooking {
  id: string;
  month: string;
  state: IState;
  cleaning?: Cleaning;

  constructor(iBooking: IBooking) {
    super(iBooking)
    this.id = iBooking.id;
    this.month = iBooking.month;
    this.state = iBooking.state;

    if (iBooking.cleaning) {
      this.cleaning = new Cleaning(iBooking.cleaning);
    } else {
      this.cleaning = undefined;
    }
  }
}

export class BookingCtrl extends NewBookingCtrl {
  id: string;
  month: string;

  constructor(ibooking: IBooking) {
    super(ibooking);

    const booking = new Booking(ibooking);

    this.id = booking.id;
    this.month = booking.month;

    if (booking.cleaning) {
      this.addControl(BookingFCN.cleaning, new CleaningCtrl(booking.cleaning));
    }
    this.addControl(BookingFCN.state, new FormControl(booking.state));
  }

  api() {
    const doc: IBooking = {
      id: this.id,
      month: this.month,
      state: this.stateFC.value,
      amount: this.amountFC.value,
      platform: this.platformFC.value,
      range: this.rangeCtrl.api(),
      guest: this.guestCtrl.api(),
      cleaning: this.cleaningCtrl ? this.cleaningCtrl.api() : undefined,
    };

    return doc;
  }

  get cleaningCtrl() {
    return this.get(BookingFCN.cleaning) as CleaningCtrl | undefined;
  }

  set cleaningCtrl(control: CleaningCtrl | undefined) {
    if (this.cleaningCtrl && control) {
      this.setControl(BookingFCN.cleaning, control);
    } else if (control) {
      this.addControl(BookingFCN.cleaning, control);
    } else {
      this.removeControl(BookingFCN.cleaning);
    }
  }

  get stateFC() {
    return this.get(BookingFCN.state) as FormControl;
  }

  set stateFC(control: FormControl) {
    this.setControl(BookingFCN.state, control);
  }
}
