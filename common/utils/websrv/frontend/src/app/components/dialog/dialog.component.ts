import {Component, ElementRef, Inject, OnInit, ViewChild} from "@angular/core";
import {MAT_DIALOG_DATA} from "@angular/material/dialog";
import {CmdCtrl} from "src/app/controls/cmd.control";
import {DialogService} from "src/app/services/dialog.service";

@Component({
  selector : "app-dialog",
  templateUrl : "./dialog.component.html",
  styleUrls : [ "./dialog.component.css" ],
})
export class DialogComponent {
  constructor(@Inject(MAT_DIALOG_DATA) public data: any)
  {
  }
  onUpdate(control: CmdCtrl)
  {
    if (control.updbtnname === "Stop update") {
      control.stopUpdate();
    } else {
      control.startUpdate();
    }
  }
}
