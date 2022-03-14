
import { ClipboardModule } from '@angular/cdk/clipboard/';
import { HttpClientModule } from '@angular/common/http';
import { NgModule } from '@angular/core';
import { AngularFireModule } from '@angular/fire/compat';
import { FlexLayoutModule } from '@angular/flex-layout';
import { ReactiveFormsModule } from '@angular/forms';
import { MatButtonModule } from '@angular/material/button';
import { MatCardModule } from '@angular/material/card';
import { MatChipsModule } from '@angular/material/chips';
import { MatNativeDateModule } from '@angular/material/core';
import { MatDatepickerModule } from '@angular/material/datepicker';
import { MatDialogModule } from '@angular/material/dialog';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatGridListModule } from '@angular/material/grid-list';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatListModule } from '@angular/material/list';
import { MatMenuModule } from '@angular/material/menu';
import { MatPaginatorModule } from '@angular/material/paginator';
import { MatProgressSpinnerModule } from '@angular/material/progress-spinner';
import { MatSelectModule } from '@angular/material/select';
import { MatSidenavModule } from '@angular/material/sidenav';
import { MatSlideToggleModule } from '@angular/material/slide-toggle';
import { MatSnackBarModule } from '@angular/material/snack-bar';
import { MatTableModule } from '@angular/material/table';
import { MatToolbarModule } from '@angular/material/toolbar';
import { BrowserModule } from '@angular/platform-browser';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { GoogleApiModule, NG_GAPI_CONFIG } from 'ng-gapi';
import { NgxMaterialTimepickerModule } from 'ngx-material-timepicker';
import { BookingsApi } from './api/bookings.api';
import { UsersApi } from './api/users.api';
import { AppRoutingModule } from './app-routing.module';
import { AppComponent } from './app.component';
import { BookingsComponent } from './components/bookings/bookings.component';
import { YearComponent } from './components/year/year.component';
import { CallbackComponent } from './components/callback/callback.component';
import { CleanersComponent } from './components/cleaners/cleaners.component';
import { ErrorDialogComponent } from './components/error-dialog/error-dialog.component';
import { MessagesComponent } from './components/messages/messages.component';
import { NavComponent } from './components/nav/nav.component';
import { NewBookingDialogComponent } from './components/new-booking-dialog/new-booking.component';
import { NewCleanerDialogComponent } from './components/new-cleaner-dialog/new-cleaner.component';
import { UserComponent } from './components/user/user.component';
import { InterceptorProviders } from './interceptors/interceptors';
import { LoadingService } from './services/loading.service';
import { FIREBASE, UserService } from './services/user.service';
import { gapiClientConfig, WorkerService } from './services/worker.service';

@NgModule({
  declarations: [
    AppComponent,
    BookingsComponent,
    YearComponent,
    MessagesComponent,
    UserComponent,
    CallbackComponent,
    NavComponent,
    ErrorDialogComponent,
    NewBookingDialogComponent,
    CleanersComponent,
    NewCleanerDialogComponent,
  ],
  imports: [
    GoogleApiModule.forRoot({ provide: NG_GAPI_CONFIG, useValue: gapiClientConfig }),
    AngularFireModule.initializeApp(FIREBASE),
    BrowserModule,
    AppRoutingModule,
    BrowserAnimationsModule,
    HttpClientModule,
    ClipboardModule,
    MatFormFieldModule,
    MatInputModule,
    ReactiveFormsModule,
    MatChipsModule,
    MatProgressSpinnerModule,
    MatIconModule,
    MatButtonModule,
    FlexLayoutModule,
    MatDatepickerModule,
    MatNativeDateModule,
    MatToolbarModule,
    MatSidenavModule,
    MatListModule,
    MatTableModule,
    MatPaginatorModule,
    MatSelectModule,
    MatDialogModule,
    MatSnackBarModule,
    MatSlideToggleModule,
    MatGridListModule,
    MatCardModule,
    MatMenuModule,
    NgxMaterialTimepickerModule.setLocale('fr-FR'),
  ],
  providers: [
    // services
    LoadingService,
    UserService,
    WorkerService,
    // api
    BookingsApi,
    UsersApi,
    // interceptors
    InterceptorProviders,
  ],
  bootstrap: [AppComponent],
  entryComponents: [ErrorDialogComponent, NewBookingDialogComponent, NewCleanerDialogComponent],
})
export class AppModule { }
