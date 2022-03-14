import { HttpClient, HttpParams } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';
import { IBooking } from './bookings.api';
import { IUser } from './users.api';

const route = '/autosend';

@Injectable({
  providedIn: 'root',
})
export class AutoSendApi {
  constructor(private httpClient: HttpClient) { }

  public autoSendMyBookings$ = (month: string) => this.httpClient.get<IBooking[]>(environment.backend + route + '/my/month/' + month);

  public autoSendUsersBookings$ = (month: string) => this.httpClient.get<IBooking[]>(environment.backend + route + '/users/month/' + month);

  public register$ = (code: string) => {
    const params = new HttpParams().append('code', code);
    return this.httpClient.post<IUser>(environment.backend + route, null, { params });
  }
}