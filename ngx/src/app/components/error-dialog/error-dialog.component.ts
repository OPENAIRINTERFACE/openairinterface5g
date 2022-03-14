import { Component, Inject, OnInit } from '@angular/core';
import { MAT_DIALOG_DATA } from '@angular/material/dialog';

@Component({
  selector: 'app-error-dialog',
  templateUrl: './error-dialog.component.html',
  styleUrls: ['./error-dialog.component.css'],
})
export class ErrorDialogComponent {
  JSON = JSON
  title = 'Angular-Interceptor';
  constructor(@Inject(MAT_DIALOG_DATA) public data: any) {
  }
}
