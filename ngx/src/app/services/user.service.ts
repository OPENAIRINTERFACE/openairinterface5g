import { Injectable } from '@angular/core';
import { AngularFireAuth } from '@angular/fire/compat/auth/';
import { Router } from '@angular/router';
import firebase from 'firebase/compat/app';
import { BehaviorSubject } from 'rxjs';
import { map, tap } from 'rxjs/operators';
import { UsersApi } from '../api/users.api';
import { UserCtrl } from '../controls/user.control';
import { WorkerService } from './worker.service';

const USER_SCOPES = [
  'https://mail.google.com/',
  'https://www.googleapis.com/auth/gmail.modify',
  'https://www.googleapis.com/auth/gmail.compose',
  'https://www.googleapis.com/auth/gmail.send',
  'https://www.googleapis.com/auth/calendar',
  'https://www.googleapis.com/auth/contacts',
  'https://www.googleapis.com/auth/drive',
];

export const FIREBASE = {
  apiKey: 'AIzaSyC5d_2nWqOViYzJltaOKKgCS9RR7aABYZg',
  authDomain: 'serema-2d3df.firebaseapp.com',
  databaseURL: 'https://serema-2d3df.firebaseio.com',
  projectId: 'serema-2d3df',
  storageBucket: 'serema-2d3df.appspot.com',
  messagingSenderId: '126536035414',
  appId: '1:126536035414:web:fa50b91256a64a3d9f9ff7',
  measurementId: 'G-LRC0R7PPJQ',
};

@Injectable({
  providedIn: 'root',
})
export class UserService {
  public isAuthenticated$ = new BehaviorSubject<boolean>(false);
  public userCtrl$ = new BehaviorSubject<UserCtrl | undefined>(undefined);

  constructor(public afa: AngularFireAuth, public usersApi: UsersApi, public router: Router, public workerService: WorkerService) { }

  get isRegistered$() {
    return this.userCtrl$.pipe(map((userCtrl) => (userCtrl ? userCtrl.isRegistered : undefined)));
  }

  get userCtrl() {
    return this.userCtrl$.getValue();
  }

  public register() {
    this.workerService.loginRedirect();
  }

  public unregister() {
    this.workerService.logout();

    const userCtrl = this.userCtrl$.getValue();

    if (userCtrl) {
      userCtrl.isRegistered = false;
      userCtrl.autoSendFC.setValue(false);
      this.update$(userCtrl).subscribe((_) => {
        this.userCtrl$.next(userCtrl);
      });
    } else {
      // TODO
    }
  }

  public onRegisterSuccess() {
    this.workerService.loginRedirectCallback();
    const userCtrl = this.userCtrl;

    if (userCtrl) {
      userCtrl.isRegistered = true;
      this.userCtrl$.next(userCtrl);
    } else {
      // TODO
    }
  }

  public login() {
    const provider = new firebase.auth.GoogleAuthProvider();
    USER_SCOPES.forEach((scope) => provider.addScope(scope));

    // POPUP
    return this.afa
      .signInWithPopup(provider)
      .then((result) => {
        this.isAuthenticated$.next(true);
        localStorage.setItem('authResult', JSON.stringify(result));

        this.usersApi
          .read$()
          .pipe(map((iuser) => new UserCtrl(iuser)))
          .subscribe((userCtrl) => {
            this.userCtrl$ = new BehaviorSubject<UserCtrl | undefined>(userCtrl);
            this.router.navigate(['/bookings']);
          });
      })
      .catch((error) => {
        window.alert(error);
      });
  }

  public logout() {
    return this.afa.signOut().then(() => {
      localStorage.removeItem('authResult');
      this.isAuthenticated$.next(false);
      this.router.navigate(['/']);
    });
  }

  private get authResult() {
    const authResult = localStorage.getItem('authResult');
    if (authResult) {
      return JSON.parse(authResult);
    } else {
      return undefined;
    }
  }

  get email() {
    return this.authResult ? this.authResult.user.email : undefined;
  }

  get accessToken() {
    return this.authResult ? this.authResult.credential.accessToken : undefined;
  }

  get idToken() {
    return this.authResult ? this.authResult.credential.idToken : undefined;
  }

  read$() {
    this.userCtrl$.next(undefined);

    return this.usersApi.read$().pipe(
      map((iuser) => new UserCtrl(iuser)),
      tap((userCtrl) => this.userCtrl$.next(userCtrl)),
    );
  }

  refresh$() {
    this.userCtrl$.next(undefined);

    return this.usersApi.refresh$().pipe(
      map((iuser) => new UserCtrl(iuser)),
      tap((userCtrl) => this.userCtrl$.next(userCtrl)),
    );
  }

  update$(userCtrl: UserCtrl) {
    this.userCtrl$.next(undefined);
    return this.usersApi.update$(userCtrl.api()).pipe(tap((_) => this.userCtrl$.next(userCtrl)));
  }
}
