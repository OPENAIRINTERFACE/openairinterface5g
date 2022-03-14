import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

export interface IReport {
  month: string;
  bookings: string[];
  brut: number;
  cleanings: number;
  nights: number;
  occupation: number;
  net: number;
  nightRate?: number;
  netRate?: number;
}

const route = '/reports';

@Injectable({
  providedIn: 'root',
})
export class ReportsApi {
  constructor(private httpClient: HttpClient) { }

  public readReport$ = (month: string) => this.httpClient.get<IReport>(environment.backend + route + '/month/' + month);

  public refreshReport$ = (month: string) => this.httpClient.put<IReport>(environment.backend + route + '/month/' + month, null);

  public readReports$ = (year: string) => this.httpClient.get<IReport[]>(environment.backend + route + '/year/' + year);

  public refreshReports$ = (year: string) => this.httpClient.put<IReport[]>(environment.backend + route + '/year/' + year, null);
}
