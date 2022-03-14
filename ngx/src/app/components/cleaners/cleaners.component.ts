import { Component, OnInit } from '@angular/core';
import { MatDialog } from '@angular/material/dialog';
// import { UsersApi } from 'src/app/api/users.api';
import { CleanerCtrl } from 'src/app/controls/cleaner.control';
import { UserCtrl } from 'src/app/controls/user.control';
import { LoadingService } from 'src/app/services/loading.service';
import { UserService } from 'src/app/services/user.service';
import { NewCleanerDialogComponent } from '../new-cleaner-dialog/new-cleaner.component';

@Component({
  selector: 'app-cleaners',
  templateUrl: './cleaners.component.html',
  styleUrls: ['./cleaners.component.css'],
})
export class CleanersComponent implements OnInit {
  constructor(
    // private usersApi: UsersApi,
    public loadingService: LoadingService,
    public userService: UserService,
    public dialog: MatDialog,
  ) { }

  ngOnInit() {
    this.userService.read$().subscribe();
  }

  onRefresh() {
    this.ngOnInit();
  }

  cleanersCtrls(userCtrl: UserCtrl) {
    return userCtrl.cleanersFA.controls as CleanerCtrl[];
  }

  onSubmit(userCtrl: UserCtrl) {
    this.userService.update$(userCtrl).subscribe(() => this.onRefresh());
  }

  onDelete(control: CleanerCtrl) {
    const userCtrl = this.userService.userCtrl;

    if (userCtrl) {
      const index = userCtrl.cleanersFA.controls.indexOf(control);
      if (index > -1) {
        userCtrl.cleanersFA.controls.splice(index, 1);
      }

      this.onSubmit(userCtrl);
    } else {
      // TODO
    }
  }

  onNew() {
    const dialogRef = this.dialog.open(NewCleanerDialogComponent, {
      width: '400px',
      data: CleanerCtrl.newCleanerCtrl(),
    });

    dialogRef.afterClosed().subscribe((control) => {
      const userCtrl = this.userService.userCtrl;
      if (userCtrl) {
        userCtrl.cleanersFA.push(control);
        this.onSubmit(userCtrl);
      } else {
        // TODO
      }
    });
  }
}
