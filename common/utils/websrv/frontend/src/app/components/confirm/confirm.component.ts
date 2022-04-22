/* eslint-disable @typescript-eslint/naming-convention */
import { Component } from '@angular/core';
import { MatDialogRef } from '@angular/material/dialog';

@Component({
  selector: 'app-confirm',
  templateUrl: './confirm.component.html',
  styleUrls: ['./confirm.component.css']
})
export class ConfirmDialogComponent {

  constructor(
    public dialogRef: MatDialogRef<void>
  ) { }

  onNoClick() {
    this.dialogRef.close();
  }
}

