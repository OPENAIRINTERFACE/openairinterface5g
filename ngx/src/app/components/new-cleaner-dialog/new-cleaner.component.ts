import { Component, Inject } from '@angular/core';
import { MatDialogRef, MAT_DIALOG_DATA } from '@angular/material/dialog';
import { CleanerCtrl } from '../../controls/cleaner.control';

@Component({
  selector: 'app-new-cleaner',
  templateUrl: './new-cleaner.component.html',
})
export class NewCleanerDialogComponent {
  constructor(public dialogRef: MatDialogRef<NewCleanerDialogComponent>, @Inject(MAT_DIALOG_DATA) public cleanerCtrl: CleanerCtrl) { }

  onNoClick() {
    this.dialogRef.close();
  }
}
