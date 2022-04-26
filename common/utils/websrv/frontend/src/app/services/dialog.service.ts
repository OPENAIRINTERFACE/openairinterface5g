import { HttpErrorResponse } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
import { MatSnackBar } from '@angular/material/snack-bar';
import { Observable } from 'rxjs/internal/Observable';
import { of } from 'rxjs/internal/observable/of';
import { tap } from 'rxjs/internal/operators/tap';
import { IResp } from '../api/commands.api';
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

  openErrorDialog(error: HttpErrorResponse): Observable<any> {
    if (this.isDialogOpen) {
      return of(undefined);
    }

    this.isDialogOpen = true;

    return this._dialog.open(ErrorDialogComponent, {
      width: '900px',
      data: {
        title: error.status + ' Error',
        body: error.error,
      },
    }).afterClosed()
      .pipe(tap(() => this.isDialogOpen = false));
  }

  openRespDialog(resp: IResp, title?: string): Observable<IResp> {
    if (this.isDialogOpen || !resp.display.length) {
      return of(resp);
    }

    this.isDialogOpen = true;

    const dialogRef = this._dialog.open(ErrorDialogComponent, {
      width: '900px',
      data: {
        title: title,
        body: resp.display!.join("</p><p>")
      },
    });

    dialogRef.afterClosed().subscribe((_) => {
      console.log('The dialog was closed');
      this.isDialogOpen = false;
    });

    return of(resp)
  }

  openSnackBar(title: string): void {
    this._snackBar.open(title, undefined, {
      duration: 500,
      horizontalPosition: 'center',
      verticalPosition: 'bottom',
    });
  }

  openConfirmDialog(question: string) {
    if (this.isDialogOpen) {
      return of(undefined);
    }

    this.isDialogOpen = true;

    return this._dialog.open(ConfirmDialogComponent, {
      width: '300px',
      data: { title: question }
    })
      .afterClosed()
      .pipe(tap(() => this.isDialogOpen = false));
  }
}
