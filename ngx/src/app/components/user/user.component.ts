/* eslint-disable @typescript-eslint/member-ordering */
import { Component, OnInit } from '@angular/core';
import { UserCtrl } from 'src/app/controls/user.control';
import { LoadingService } from 'src/app/services/loading.service';
import { UserService } from './../../services/user.service';

@Component({
  selector: 'app-user',
  templateUrl: './user.component.html',
  styleUrls: ['./user.component.css'],
})
export class UserComponent implements OnInit {
  onRefresh = this.ngOnInit;

  constructor(public userService: UserService, public loadingService: LoadingService) { }

  ngOnInit() {
    this.userService.read$().subscribe();
  }

  onSync() {
    this.userService.refresh$().subscribe();
  }

  onSubmit(userCtrl: UserCtrl) {
    this.userService.update$(userCtrl).subscribe(() => this.onRefresh());
  }

  ical(id: string) {
    return 'https://calendar.google.com/calendar/ical/' + encodeURIComponent(id) + '/public/basic.ics';
  }
}
