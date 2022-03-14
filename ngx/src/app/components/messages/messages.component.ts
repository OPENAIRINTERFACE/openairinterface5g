import { Component, OnInit } from '@angular/core';
import { UserCtrl } from 'src/app/controls/user.control';
import { MessageCtrl } from 'src/app/controls/message.control';
import { LoadingService } from 'src/app/services/loading.service';
import { UserService } from 'src/app/services/user.service';

@Component({
  selector: 'app-messages',
  templateUrl: './messages.component.html',
  styleUrls: ['./messages.component.css'],
})
export class MessagesComponent implements OnInit {
  constructor(public loadingService: LoadingService, public userService: UserService) { }

  ngOnInit() {
    this.userService.read$().subscribe();
  }

  onRefresh() {
    this.ngOnInit();
  }

  messagesCtrls(userCtrl: UserCtrl) {
    return userCtrl.messagesFA.controls as MessageCtrl[];
  }

  onSubmit(userCtrl: UserCtrl) {
    this.userService.update$(userCtrl).subscribe(() => this.onRefresh());
  }
}
