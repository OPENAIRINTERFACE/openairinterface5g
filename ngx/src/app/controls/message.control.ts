/* eslint-disable no-shadow */
import { FormControl, FormGroup } from '@angular/forms';
import { IMessage, ISubject } from '../api/users.api';

// export class Message implements IMessage {
//   subject: ISubject;
//   body: string;
// }

export class MessageCtrl extends FormGroup {
  subject: ISubject;

  constructor(message: IMessage) {
    super({});

    this.subject = message.subject;

    this.addControl(MessageFCN.body, new FormControl(message.body.replace(/<br\s*[\/]?>/gi, '\n')));
  }

  static newMessageCtrl() {
    const iMessage: IMessage = {
      subject: ISubject.WELCOME,
      body: 'how are you ?',
    };

    return new MessageCtrl(iMessage);
  }

  get bodyFC() {
    return this.get(MessageFCN.body) as FormControl;
  }

  set bodyFC(control: FormControl) {
    this.setControl(MessageFCN.body, control);
  }

  api() {
    const message: IMessage = {
      subject: this.subject,
      body: this.bodyFC.value,
    };

    return message;
  }
}

enum MessageFCN {
  body = 'body',
}
