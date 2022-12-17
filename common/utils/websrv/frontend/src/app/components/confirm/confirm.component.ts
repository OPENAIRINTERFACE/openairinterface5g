/* eslint-disable @typescript-eslint/naming-convention */
import {Component, Inject} from "@angular/core";
import {MAT_DIALOG_DATA, MatDialogRef} from "@angular/material/dialog";

@Component({selector : "app-confirm", templateUrl : "./confirm.component.html", styleUrls : [ "./confirm.component.css" ]})
export class ConfirmDialogComponent {
  constructor(public dialogRef: MatDialogRef<void>, @Inject(MAT_DIALOG_DATA) public data: any)
  {
  }

  onNoClick()
  {
    this.dialogRef.close();
  }
}
