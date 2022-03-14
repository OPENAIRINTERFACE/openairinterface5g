import { HttpClient, HttpParams } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

export enum ISubject {
  ALERT = 'Alert no email',
  GENERAL_INFOS = 'Infos',
  CHECKIN_METHOD = 'Checkin',
  CHECKIN_QUESTION = 'Checkin question',
  WELCOME = 'Welcome',
  CHECKOUT_METHOD = 'Checkout',
  CHECKOUT_QUESTION = 'Checkout question',
  GOODBYE = 'Good Bye',
}

export interface IMessage {
  subject: ISubject;
  body: string;
}

export interface ICalendar {
  summary: string;
  id: string;
}

export interface ICleaner {
  rate: number;
  name: string;
  email: string;
  default: boolean;
}

export interface IUser {
  subjectPrefix: string;
  defaultCheckInHour: number;
  defaultCheckOutHour: number;
  defaultCleaningHours: number;
  busyCal: ICalendar;
  cleaningCal: ICalendar;
  reelCal: ICalendar;
  address: string;
  name: string;
  tel: string;
  messages: IMessage[];
  cleaners: ICleaner[];
  isRegistered: boolean;
  autoSend: boolean;
}

const route = '/users';

@Injectable({
  providedIn: 'root',
})
export class UsersApi {
  constructor(private httpClient: HttpClient) { }

  public read$ = () => this.httpClient.get<IUser>(environment.backend + route);

  public refresh$ = () => this.httpClient.get<IUser>(environment.backend + route + '/refresh');

  public update$ = (user: IUser) => this.httpClient.put<string[]>(environment.backend + route, user);

}
