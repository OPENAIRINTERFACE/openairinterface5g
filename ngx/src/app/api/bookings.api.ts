/* eslint-disable no-shadow */
/* eslint-disable @typescript-eslint/naming-convention */
import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

export enum IState {
  BOOKED = 'booked',
  CHECKIN_HOUR_NOK = 'checkin nok',
  WAITING_CHECKIN_HOUR = 'waiting checkin',
  CHECKIN_HOUR_OK = 'checkin ok',
  INSIDE = 'inside',
  WAITING_CHECKOUT_HOUR = 'waiting checkout',
  CHECKOUT_HOUR_OK = 'checkout ok',
  OUTSIDE = 'outside',
  TELFORGET = 'telforget',
}

export enum IPlatform {
  DIRECT = 'direct',
  AIRBNB = 'airbnb',
  TRIPADVISOR = 'tripadvisor',
  BOOKING = 'booking',
  WIMDU = 'wimdu',
  BEDYCASA = 'bedycasa',
  HOUSETRIP = 'housetrip',
  NINEFLATS = '9flats',
  ONLY = 'only',
  LEBONCOIN = 'leboncoin',
  SEJOURNING = 'sejourning',
  WAY2STAY = 'waytostay'
}
export interface IRange {
  start: string;
  end: string;
}
export interface IGuest {
  given: string;
  email: string;
}
export interface ICleaning {
  cleanerEmail: string;
  hours: number;
}

export interface INewBooking {
  amount: number;
  platform: IPlatform;
  range: IRange;
  guest: IGuest;
}

export interface IBooking extends INewBooking {
  month: string;
  id: string;
  state: IState;
  cleaning?: ICleaning;
}

const route = '/bookings';

@Injectable({
  providedIn: 'root',
})
export class BookingsApi {
  constructor(private httpClient: HttpClient) { }


  public readBookings$ = (month: string) => this.httpClient.get<IBooking[]>(environment.backend + route + '/month/' + month);

  public refreshBookings$ = (month: string) => this.httpClient.get<IBooking[]>(environment.backend + route + '/refresh/' + month);

  public createBooking$ = (booking: INewBooking) => this.httpClient.post<IBooking>(environment.backend + route, booking);

  public updateBooking$ = (booking: IBooking) => this.httpClient.put<string[]>(environment.backend + route, booking);

  public readBookingsList$ = (ids: string[]) => this.httpClient.put<IBooking[]>(environment.backend + route + '/list', ids);

  public deleteBooking$ = (id: string) => this.httpClient.delete(environment.backend + route + '/' + id);
}
