import { FormControl, FormGroup } from '@angular/forms';
import { IGuest } from '../api/bookings.api';


export class Guest {
  given: string;
  email: string;

  constructor(iGuest: IGuest) {
    this.given = iGuest.given;
    this.email = iGuest.email;
  }
}

export class GuestCtrl extends FormGroup {
  constructor(guest: Guest) {
    super({});

    this.addControl(GuestFCN.given, new FormControl(guest.given));
    this.addControl(GuestFCN.email, new FormControl(guest.email));
  }

  get givenFC() {
    return this.get(GuestFCN.given) as FormControl;
  }

  set givenFC(control: FormControl) {
    this.setControl(GuestFCN.given, control);
  }

  get emailFC() {
    return this.get(GuestFCN.email) as FormControl;
  }

  set emailFC(control: FormControl) {
    this.setControl(GuestFCN.email, control);
  }

  api() {
    const guest: IGuest = {
      given: this.givenFC.value,
      email: this.emailFC.value,
    };

    return guest;
  }
}

enum GuestFCN {
  given = 'given',
  email = 'email',
}
