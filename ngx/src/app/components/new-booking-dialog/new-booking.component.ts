/* eslint-disable @typescript-eslint/naming-convention */
import { Component, Inject } from '@angular/core';
import { MatDialogRef, MAT_DIALOG_DATA } from '@angular/material/dialog';
import { IPlatform, IState } from 'src/app/api/bookings.api';
import { Cleaner } from 'src/app/controls/cleaner.control';
import { NewBookingCtrl } from 'src/app/controls/newbooking.control';
import { UserService } from 'src/app/services/user.service';

@Component({
  selector: 'app-new-booking',
  templateUrl: './new-booking.component.html',
})
export class NewBookingDialogComponent {
  // Platforms
  IPlatform = IPlatform;
  platformValues = Object.values(IPlatform);

  // States
  IState = IState;
  stateValues = Object.values(IState);

  // Cleaners
  cleaners?: Cleaner[];

  constructor(
    public dialogRef: MatDialogRef<NewBookingDialogComponent>,
    @Inject(MAT_DIALOG_DATA) public bookingCtrl: NewBookingCtrl,

    public userService: UserService,
  ) {
    const userCtrl = this.userService.userCtrl;
    if (userCtrl) {
      this.cleaners = userCtrl.api().cleaners;
    } else {
      // TODO
    }
  }

  onNoClick() {
    this.dialogRef.close();
  }
}
