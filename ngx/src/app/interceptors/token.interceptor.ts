/* eslint-disable @typescript-eslint/naming-convention */
import { HttpEvent, HttpHandler, HttpHeaders, HttpInterceptor, HttpRequest } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { UserService } from '../services/user.service';

@Injectable()
export class TokenInterceptor implements HttpInterceptor {
  constructor(public userService: UserService) { }

  intercept(request: HttpRequest<any>, next: HttpHandler) {

    const accessToken = this.userService.accessToken!;
    const idToken = this.userService.idToken!;

    // Clone the request and set the new header in one step.
    const authReq = request.clone({
      setHeaders: {
        'Content-Type': 'application/json',
        'access-token': accessToken,
        'id-token': idToken,
      },
    });


    console.error(request.method + ' ' + request.url);

    // send cloned request with header to the next handler.
    return next.handle(authReq);
  }
}
