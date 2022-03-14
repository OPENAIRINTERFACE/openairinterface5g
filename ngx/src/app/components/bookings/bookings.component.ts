/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable no-shadow */
/* eslint-disable eqeqeq */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component, OnInit } from '@angular/core';
import * as moment from 'moment';
import { Observable, of } from 'rxjs';
import { map, mergeMap, tap } from 'rxjs/operators';
import { AutoSendApi } from 'src/app/api/autosend.api';
import { BookingsApi, IPlatform, IState } from 'src/app/api/bookings.api';
import { IReport, ReportsApi } from 'src/app/api/reports.api';
import { BookingCtrl } from 'src/app/controls/booking.control';
import { Cleaner } from 'src/app/controls/cleaner.control';
import { CleaningCtrl } from 'src/app/controls/cleaning.control';
import { NewBookingCtrl } from 'src/app/controls/newbooking.control';
import { DialogService } from 'src/app/services/dialog.service';
import { LoadingService } from 'src/app/services/loading.service';
import { UserService } from 'src/app/services/user.service';


@Component({
  selector: 'app-bookings',
  templateUrl: './bookings.component.html',
  styleUrls: ['./bookings.component.css'],
})
export class BookingsComponent implements OnInit {
  // table columns
  DISPLAYED_COLUMNS: string[] = ['title', 'nights', 'checkinout', 'platform', 'autosend', 'cleaner'];

  // Platforms
  IPlatform = IPlatform;
  platformValues = Object.values(IPlatform);

  // States
  IState = IState;
  stateValues = Object.values(IState);

  // variables
  bookingsCtrls$: Observable<BookingCtrl[]> = of([]);
  monthly$?: Observable<IReport>;

  // month
  month: string;

  // Cleaners
  cleaners: Cleaner[] = [];

  get isToday() {
    return this.month === moment().format('YYYY-MM');
  }


  constructor(
    public bookingsApi: BookingsApi,
    public autoSendApi: AutoSendApi,
    public loadingService: LoadingService,
    public userService: UserService,
    public reportsApi: ReportsApi,
    public dialogService: DialogService,
  ) {
    const userCtrl = this.userService.userCtrl;
    if (userCtrl) {
      this.cleaners = userCtrl.api().cleaners;
    } else {
      // TODO
    }
    this.month = moment().format('YYYY-MM');
  }

  ngOnInit() {
    this.bookingsCtrls$ = this.bookingsApi.readBookings$(this.month).pipe(
      map((docs) => docs.map((doc) => new BookingCtrl(doc))),
    );

    // this.userService.read$().pipe(
    //   map(userCtrl => {
    //     this.cleaners = userCtrl.api().cleaners;
    //   })
    // ).subscribe()

    this.monthly$ = this.reportsApi.readReport$(this.month)
  }

  onRefresh() {
    this.ngOnInit();
  }

  onSync() {
    this.monthly$ = this.reportsApi.refreshReport$(this.month);

    this.bookingsCtrls$ = this.bookingsApi.refreshBookings$(this.month).pipe(
      map(ibookings => ibookings.map(ibooking => new BookingCtrl(ibooking)))
    );
  }

  onChange(control: BookingCtrl) {
    if (control.cleaningCtrl) {
      control.cleaningCtrl = undefined;
    } else if (this.cleaners.length) {
      control.cleaningCtrl = new CleaningCtrl({
        cleanerEmail: this.cleaners[0].email,
        hours: 3
      });
    }

    control.markAsDirty();
  }

  onToday() {
    this.month = moment().format('YYYY-MM');
    this.ngOnInit();
  }

  onNext() {
    this.month = moment(this.month).add(1, 'months').format('YYYY-MM');
    this.ngOnInit();
  }

  onPrevious() {
    this.month = moment(this.month).subtract(1, 'months').format('YYYY-MM');
    this.ngOnInit();
  }

  onSend() {
    this.bookingsCtrls$ = this.autoSendApi.autoSendMyBookings$(this.month).pipe(
      map((docs) => docs.map((doc) => new BookingCtrl(doc))),
      tap(() => this.onRefresh())
    );
  }

  onNew() {

    const newBookingCtrl: NewBookingCtrl = new NewBookingCtrl({
      range: {
        start: moment(this.month).format(),
        end: moment(this.month).add(3, 'days').format(),
      },
      guest: {
        email: 'john@yahoo.com',
        given: 'john',
      },
      amount: 200,
      platform: IPlatform.AIRBNB
    });

    this.dialogService.openNewBookingDialog$(newBookingCtrl).pipe(
      mergeMap(control => this.bookingsApi.createBooking$(control.api())),
      map(ibooking => new BookingCtrl(ibooking))
    )
      .subscribe(() => this.onRefresh())
  }

  onSubmit(control: BookingCtrl) {
    this.bookingsApi.updateBooking$(control.api()).pipe(
      // take(1),
      tap(() => control.markAsPristine())
    ).subscribe(() => this.onRefresh());
  }

  onDelete(control: BookingCtrl) {
    if (control.id) {
      this.bookingsApi.deleteBooking$(control.id).subscribe(() => this.onRefresh());
    } else {
      // TODO
    }
  }
}
