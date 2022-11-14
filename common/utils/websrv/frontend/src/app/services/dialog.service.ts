import {HttpErrorResponse} from "@angular/common/http";
import {ElementRef, Injectable, ViewChild} from "@angular/core";
import {NgModule} from "@angular/core";
import {MatDialog} from "@angular/material/dialog";
import {MatSnackBar} from "@angular/material/snack-bar";
import {BrowserModule} from "@angular/platform-browser";
import {Observable} from "rxjs";
import {of} from "rxjs";
import {tap} from "rxjs/operators";
import {CmdCtrl} from "src/app/controls/cmd.control";

import {ICommandOptions, IResp} from "../api/commands.api";
import {CommandsComponent} from "../components/commands/commands.component";
import {ConfirmDialogComponent} from "../components/confirm/confirm.component";
import {DialogComponent} from "../components/dialog/dialog.component";
import {QuestionDialogComponent} from "../components/question/question.component";

@Injectable({
  providedIn : "root",
})
export class DialogService {
  public isDialogOpen = false;
  cmdDialogRef: any;
  constructor(
      private _dialog: MatDialog,
      private _snackBar: MatSnackBar,
  )
  {
  }

  openErrorDialog(title: string, message: string): void
  {
    const ErrDiag = this._dialog.open(DialogComponent, {
      width : "900px",
      data : {
        title : title,
        body : message,
      },
      panelClass : "errRespDialog",
    });
    ErrDiag.afterClosed().subscribe(result => {});
  }

  openCmdDialog(control: CmdCtrl, resp: IResp, title?: string): Observable<IResp>
  {
    this.isDialogOpen = true;
    console.log("Open Cmd dialog");
    var updatable = false;
    if (control.options) {
      for (let opt = 0; opt < control.options.length; opt++) {
        if (control.options[opt] == ICommandOptions.update)
          updatable = true;
      }
    }
    this.cmdDialogRef = this._dialog.open(DialogComponent, {
      height : "80%",
      hasBackdrop : false,

      data : {
        control : control,
        title : title,
        body : resp.display!.join("</p><p>"),
        updatable : updatable,
      },
      panelClass : "cmdRespDialog",
    });

    this.cmdDialogRef.afterClosed().subscribe(() => {
      console.log("The dialog was closed");
      this.isDialogOpen = false;
      control.stopUpdate();
      if (control.ResUpdTimerSubscriber) {
        control.ResUpdTimerSubscriber.unsubscribe();
      }
    });

    return of(resp)
  }

  updateCmdDialog(control: CmdCtrl, resp: IResp, title?: string): Observable<IResp>
  {
    if (this.cmdDialogRef && this.isDialogOpen && resp.display.length) {
      this.cmdDialogRef.componentInstance.data = {control : control, title : title, body : resp.display!.join("</p><p>"), updatable : true};
      return of(resp);
    } else {
      return this.openCmdDialog(control, resp, title);
    }
  }

  openVarRespDialog(resp: IResp): Observable<IResp>
  {
    if (this.isDialogOpen || !resp.display.length) {
      return of(resp);
    }
    console.log("Open Var dialog");
    this.isDialogOpen = true;
    const dialogRef = this._dialog.open(DialogComponent, {
      width : "900px",
      hasBackdrop : true,
      data : {
        title : resp.display![0],
      },
      panelClass : "varRespDialog",
    });

    dialogRef.afterClosed().subscribe(() => {
      console.log("The dialog was closed");
      this.isDialogOpen = false;
    });

    return of(resp)
  }

  openSnackBar(title: string): void
  {
    this._snackBar.open(title, undefined, {
      duration : 500,
      horizontalPosition : "center",
      verticalPosition : "bottom",
    });
  }

  openConfirmDialog(question: string)
  {
    if (this.isDialogOpen) {
      return of(undefined);
    }

    this.isDialogOpen = true;

    return this._dialog.open(ConfirmDialogComponent, {width : "300px", data : {title : question}}).afterClosed().pipe(tap(() => this.isDialogOpen = false));
  }

  openQuestionDialog(title: string, control: CmdCtrl)
  {
    if (this.isDialogOpen) {
      return of(control);
    }

    this.isDialogOpen = true;

    const dialogRef = this._dialog.open(QuestionDialogComponent, {
      width : "300px",
      data : {title : title, control : control},
      panelClass : "questionDialog",
    })

    return dialogRef.afterClosed().pipe(tap(() => this.isDialogOpen = false));
  }
}
