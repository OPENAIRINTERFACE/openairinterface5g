/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable no-shadow */
/* eslint-disable eqeqeq */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component, OnInit } from '@angular/core';
import * as moment from 'moment';
import { BehaviorSubject, Observable, of } from 'rxjs';
import { map, mergeMap, tap } from 'rxjs/operators';
import { AutoSendApi } from 'src/app/api/autosend.api';
import { BookingsApi, IPlatform } from 'src/app/api/bookings.api';
import { IReport, ReportsApi } from 'src/app/api/reports.api';
import { LoadingService } from 'src/app/services/loading.service';
import { UserService } from 'src/app/services/user.service';
import { BookingCtrl } from '../../controls/booking.control';


@Component({
  selector: 'app-year',
  templateUrl: './year.component.html',
  styleUrls: ['./year.component.css'],
})
export class YearComponent implements OnInit {

  DISPLAYED_COLUMNS: string[] = ['dates', 'guest', 'nights', 'platform', 'rate', 'brut', 'cleanings', 'net'];
  year: string;
  yearly$?: Observable<IReport>;
  bookingsCtrls$?: Observable<BookingCtrl[]>;
  rates = new Map<string, number>();

  constructor(
    public bookingsApi: BookingsApi,
    public reportsApi: ReportsApi,
    public autoSendApi: AutoSendApi,
    public loadingService: LoadingService,
    public userService: UserService,
  ) {
    this.year = moment().format('YYYY');

    const userCtrl = this.userService.userCtrl;
    if (userCtrl) {
      userCtrl.api().cleaners.forEach((cleaner) => this.rates.set(cleaner.email, cleaner.rate));
    } else {
      // TODO
    }
  }


  onNext() {
    this.year = moment(this.year).add(1, 'years').format('YYYY');
    this.ngOnInit();
  }

  nights(control: BookingCtrl) {
    return moment(control.rangeCtrl.endDayFC.value).diff(moment(control.rangeCtrl.startDayFC.value), 'days') + 1
  }

  dates(control: BookingCtrl) {
    return moment(control.rangeCtrl.startDayFC.value).format('DD/MM') + ' to ' + moment(control.rangeCtrl.endDayFC.value).format('DD/MM')
  }

  rate(control: BookingCtrl) {
    return Math.round(this.brut(control) / this.nights(control))
  }

  brut(control: BookingCtrl) {
    let comission = 0
    if (control.platformFC.value === IPlatform.BOOKING) comission = 0.2
    return (1 - comission) * control.amountFC.value;
  }

  cleaning(control: BookingCtrl) {
    return control.cleaningCtrl
      ? this.rates.get(control.cleaningCtrl.cleanerEmailFC.value)! * control.cleaningCtrl.hours
      : 0
  }

  onPrevious() {
    this.year = moment(this.year).subtract(1, 'years').format('YYYY');
    this.ngOnInit();
  }

  onRefresh() {
    this.ngOnInit();
  }

  onSync() {
    this.yearly$ = this.reportsApi.refreshReports$(this.year).pipe(
      map(this.processReports)
    )

    this.bookingsCtrls$ = this.yearly$.pipe(
      mergeMap(yearly => this.bookingsApi.readBookingsList$(yearly.bookings)),
      map(ibookings => ibookings.map(ibooking => new BookingCtrl(ibooking)))
    );
  }

  ngOnInit() {
    this.yearly$ = this.reportsApi.readReports$(this.year).pipe(
      map(this.processReports)
    )

    this.bookingsCtrls$ = this.yearly$.pipe(
      mergeMap(yearly => this.bookingsApi.readBookingsList$(yearly.bookings)),
      map(ibookings => ibookings.map(ibooking => new BookingCtrl(ibooking)))
    );
  }

  private processReports(ireports: IReport[]) {
    let yearly: IReport = {
      brut: 0,
      cleanings: 0,
      month: this.year, //FIXME
      nights: 0,
      occupation: 0,
      net: 0,
      bookings: []
    };

    ireports.forEach((ireport) => {
      yearly.brut = yearly.brut + ireport.brut;
      yearly.cleanings = yearly.cleanings + ireport.cleanings;
      yearly.nights = yearly.nights + ireport.nights;
      yearly.bookings = yearly.bookings.concat(ireport.bookings);
    })

    yearly.net = yearly.brut - yearly.cleanings;
    yearly.netRate = Math.round(yearly.net / yearly.nights);
    yearly.nightRate = Math.round(yearly.brut / yearly.nights);
    yearly.occupation = Math.round(100 * yearly.nights / 365);

    return yearly
  }

  onSubmit(control: BookingCtrl) {
    this.bookingsApi.updateBooking$(control.api()).pipe(
      // take(1),
      tap(() => control.markAsPristine())
    ).subscribe();
  }
}
