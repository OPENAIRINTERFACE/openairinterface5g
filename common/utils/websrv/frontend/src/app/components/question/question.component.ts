/* eslint-disable @typescript-eslint/naming-convention */
import {Component, Inject} from "@angular/core";
import {MAT_DIALOG_DATA, MatDialogRef} from "@angular/material/dialog";
import {CmdCtrl} from "src/app/controls/cmd.control";

export interface QuestionDialogData {
  title: string;
  control: CmdCtrl;
}
@Component({selector : "app-question", templateUrl : "./question.component.html", styleUrls : [ "./question.component.css" ]})
export class QuestionDialogComponent {
  constructor(
      public dialogRef: MatDialogRef<QuestionDialogComponent>,
      @Inject(MAT_DIALOG_DATA) public data: QuestionDialogData,
  )
  {
  }
  onNoClick()
  {
    this.dialogRef.close();
  }
}
