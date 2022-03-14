import { Injectable } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { MatSnackBar } from '@angular/material/snack-bar';
import { of } from 'rxjs';
import { tap } from 'rxjs/operators';
import { ErrorDialogComponent } from '../components/error-dialog/error-dialog.component';
import { NewBookingDialogComponent } from '../components/new-booking-dialog/new-booking.component';
import { NewBookingCtrl } from '../controls/newbooking.control';

@Injectable({
  providedIn: 'root',
})
export class DialogService {
  public isDialogOpen = false;

  constructor(
    private _dialog: MatDialog,
    private _snackBar: MatSnackBar,
  ) { }

  openErrorDialog(data: any): any {
    if (this.isDialogOpen) {
      return false;
    }

    this.isDialogOpen = true;

    const dialogRef = this._dialog.open(ErrorDialogComponent, {
      width: '300px',
      data,
    });

    dialogRef.afterClosed().subscribe((_) => {
      console.log('The dialog was closed');
      this.isDialogOpen = false;
    });
  }

  openNewBookingDialog$(control: NewBookingCtrl) {
    if (this.isDialogOpen) {
      return of(undefined);
    }

    this.isDialogOpen = true;

    const dialogRef = this._dialog.open(NewBookingDialogComponent, {
      width: '800px',
      data: control,
      restoreFocus: false
    });

    return dialogRef.afterClosed().pipe(tap(() => this.isDialogOpen = false));
  }

  openSnackBar(title: string): void {
    this._snackBar.open(title, undefined, {
      duration: 500,
      horizontalPosition: 'center',
      verticalPosition: 'bottom',
    });
  }
}
