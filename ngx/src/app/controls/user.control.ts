import { FormArray, FormControl, FormGroup } from '@angular/forms';
import { IUser } from '../api/users.api';
import { CalendarCtrl } from './calendar.control';
import { CleanerCtrl } from './cleaner.control';
import { MessageCtrl } from './message.control';

// export class User implements IUser {
//   subjectPrefix: string;
//   busyCal: ICalendar;
//   cleaningCal: ICalendar;
//   reelCal: ICalendar;
//   address: string
//   messages : Message[]
//   cleaners : Cleaner[]
//   isRegistered : boolean
//   autoSend : boolean
//   name: string
//   tel: string

// }

export class UserCtrl extends FormGroup {
  isRegistered: boolean;

  constructor(iuser: IUser) {
    super({});

    this.isRegistered = iuser.isRegistered;

    this.addControl(UserFCN.busyCal, new CalendarCtrl(iuser.busyCal));
    this.addControl(UserFCN.cleaningCal, new CalendarCtrl(iuser.cleaningCal));
    this.addControl(UserFCN.reelCal, new CalendarCtrl(iuser.reelCal));

    this.addControl(UserFCN.subjectPrefix, new FormControl(iuser.subjectPrefix));
    this.addControl(UserFCN.address, new FormControl(iuser.address));
    this.addControl(UserFCN.name, new FormControl(iuser.name));
    this.addControl(UserFCN.tel, new FormControl(iuser.tel));

    this.addControl(UserFCN.defaultCheckInHour, new FormControl(iuser.defaultCheckInHour));
    this.addControl(UserFCN.defaultCheckOutHour, new FormControl(iuser.defaultCheckOutHour));
    this.addControl(UserFCN.defaultCleaningHours, new FormControl(iuser.defaultCleaningHours));

    this.addControl(UserFCN.messages, new FormArray(iuser.messages.map((imessage) => new MessageCtrl(imessage))));
    this.addControl(UserFCN.cleaners, new FormArray(iuser.cleaners.map((icleaner) => new CleanerCtrl(icleaner))));
    this.addControl(UserFCN.autoSend, new FormControl(iuser.autoSend));
  }

  get subjectPrefixFC() {
    return this.get(UserFCN.subjectPrefix) as FormControl;
  }

  set subjectPrefixFC(control: FormControl) {
    this.setControl(UserFCN.subjectPrefix, control);
  }

  get defaultCheckInHourFC() {
    return this.get(UserFCN.defaultCheckInHour) as FormControl;
  }

  set defaultCheckInHourFC(control: FormControl) {
    this.setControl(UserFCN.defaultCheckInHour, control);
  }

  get defaultCleaningHoursFC() {
    return this.get(UserFCN.defaultCleaningHours) as FormControl;
  }

  set defaultCleaningHoursFC(control: FormControl) {
    this.setControl(UserFCN.defaultCleaningHours, control);
  }

  get defaultCheckOutHourFC() {
    return this.get(UserFCN.defaultCheckOutHour) as FormControl;
  }

  set defaultCheckOutHourFC(control: FormControl) {
    this.setControl(UserFCN.defaultCheckOutHour, control);
  }

  get busyCalCtrl() {
    return this.get(UserFCN.busyCal) as CalendarCtrl;
  }

  set busyCalCtrl(control: CalendarCtrl) {
    this.setControl(UserFCN.busyCal, control);
  }

  get cleaningCalCtrl() {
    return this.get(UserFCN.cleaningCal) as CalendarCtrl;
  }

  set cleaningCalCtrl(control: CalendarCtrl) {
    this.setControl(UserFCN.cleaningCal, control);
  }

  get reelCalCtrl() {
    return this.get(UserFCN.reelCal) as CalendarCtrl;
  }

  set reelCalCtrl(control: CalendarCtrl) {
    this.setControl(UserFCN.reelCal, control);
  }

  get nameFC() {
    return this.get(UserFCN.name) as FormControl;
  }

  set nameFC(control: FormControl) {
    this.setControl(UserFCN.name, control);
  }

  get telFC() {
    return this.get(UserFCN.tel) as FormControl;
  }

  set telFC(control: FormControl) {
    this.setControl(UserFCN.tel, control);
  }

  get addressFC() {
    return this.get(UserFCN.address) as FormControl;
  }

  set addressFC(control: FormControl) {
    this.setControl(UserFCN.address, control);
  }

  get messagesFA() {
    return this.get(UserFCN.messages) as FormArray;
  }

  set messagesFA(fa: FormArray) {
    this.setControl(UserFCN.messages, fa);
  }

  get cleanersFA() {
    return this.get(UserFCN.cleaners) as FormArray;
  }

  set cleanersFA(fa: FormArray) {
    this.setControl(UserFCN.cleaners, fa);
  }

  get autoSendFC() {
    return this.get(UserFCN.autoSend) as FormControl;
  }

  set autoSendFC(control: FormControl) {
    this.setControl(UserFCN.autoSend, control);
  }

  api() {
    const config: IUser = {
      subjectPrefix: this.subjectPrefixFC.value,
      defaultCheckInHour: this.defaultCheckInHourFC.value,
      defaultCleaningHours: this.defaultCleaningHoursFC.value,
      defaultCheckOutHour: this.defaultCheckOutHourFC.value,
      busyCal: this.busyCalCtrl.value,
      cleaningCal: this.cleaningCalCtrl.value,
      reelCal: this.reelCalCtrl.value,
      address: this.addressFC.value,
      name: this.nameFC.value,
      tel: this.telFC.value,
      messages: this.messagesFA.controls.map((control) => (control as MessageCtrl).api()),
      cleaners: this.cleanersFA.controls.map((control) => (control as CleanerCtrl).api()),
      isRegistered: this.isRegistered,
      autoSend: this.autoSendFC.value,
    };

    return config;
  }
}

enum UserFCN {
  subjectPrefix = 'subjectPrefix',
  defaultCheckOutHour = 'defaultCheckOutHour',
  defaultCleaningHours = 'defaultCleaningHours',
  defaultCheckInHour = 'defaultCheckInHour',
  busyCal = 'busyCal',
  cleaningCal = 'cleaningCal',
  reelCal = 'reelCal',
  address = 'address',
  name = 'name',
  tel = 'tel',
  messages = 'messages',
  cleaners = 'cleaners',
  autoSend = 'autoSend',
}
