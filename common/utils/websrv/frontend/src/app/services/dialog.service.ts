import { Injectable } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { MatSnackBar } from '@angular/material/snack-bar';
import { of } from 'rxjs/internal/observable/of';
import { tap } from 'rxjs/internal/operators/tap';
import { ConfirmDialogComponent } from '../components/confirm/confirm.component';
import { ErrorDialogComponent } from '../components/error-dialog/error-dialog.component';

@Injectable({
  providedIn: 'root',
})
export class DialogService {
  public isDialogOpen = false;

  constructor(
    private _dialog: MatDialog,
    private _snackBar: MatSnackBar,
  ) { }

  openDialog(data: any): any {
    if (this.isDialogOpen) {
      return false;
    }

    this.isDialogOpen = true;

    const dialogRef = this._dialog.open(ErrorDialogComponent, {
      width: '900px',
      data,
    });

    dialogRef.afterClosed().subscribe((_) => {
      console.log('The dialog was closed');
      this.isDialogOpen = false;
    });
  }

  openSnackBar(title: string): void {
    this._snackBar.open(title, undefined, {
      duration: 500,
      horizontalPosition: 'center',
      verticalPosition: 'bottom',
    });
  }

  openConfirmDialog() {
    if (this.isDialogOpen) {
      return of(undefined);
    }

    this.isDialogOpen = true;

    return this._dialog.open(ConfirmDialogComponent, {
      width: '300px'
    })
      .afterClosed()
      .pipe(tap(() => this.isDialogOpen = false));
  }
}
