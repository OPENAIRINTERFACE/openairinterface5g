import { NgModule } from '@angular/core';
import { Routes, RouterModule } from '@angular/router';
import { BookingsComponent } from './components/bookings/bookings.component';
import { CallbackComponent } from './components/callback/callback.component';
import { CleanersComponent } from './components/cleaners/cleaners.component';
import { MessagesComponent } from './components/messages/messages.component';
import { UserComponent } from './components/user/user.component';
import { YearComponent } from './components/year/year.component';
import { AuthGuard } from './guards/auth.guard';

const routes: Routes = [
  { path: '', redirectTo: '/', pathMatch: 'full' },
  { path: 'bookings', component: BookingsComponent, canActivate: [AuthGuard] },
  { path: 'year', component: YearComponent, canActivate: [AuthGuard] },
  { path: 'messages', component: MessagesComponent, canActivate: [AuthGuard] },
  { path: 'cleaners', component: CleanersComponent, canActivate: [AuthGuard] },
  { path: 'user', component: UserComponent, canActivate: [AuthGuard] },
  { path: 'oauth2callback', component: CallbackComponent },
  { path: '**', redirectTo: '' },
];

@NgModule({
  imports: [RouterModule.forRoot(routes, { relativeLinkResolution: 'legacy' })],
  exports: [RouterModule],
})
export class AppRoutingModule { }
