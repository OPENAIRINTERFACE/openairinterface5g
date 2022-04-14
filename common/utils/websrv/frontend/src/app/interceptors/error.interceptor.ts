import { HttpErrorResponse, HttpEvent, HttpHandler, HttpInterceptor, HttpRequest, HttpResponse } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { Observable, throwError } from 'rxjs';
import { catchError, tap } from 'rxjs/operators';
import { DialogService as DialogService } from '../services/dialog.service';

@Injectable()
export class ErrorInterceptor implements HttpInterceptor {
  constructor(public dialogService: DialogService) { }

  intercept(request: HttpRequest<unknown>, next: HttpHandler) {
    return next.handle(request).pipe(
      // tap((event: HttpEvent<any>) => {
      //   if (event instanceof HttpResponse) {
      //     switch (event.status) {
      //       case 200:
      //       case 201:
      //         this.log(GREEN, request.method + ' ' + event.status + ' Success');
      //         this.dialogService.openSnackBar(request.method + ' ' + event.status + ' Success');
      //         break;

      //       default:
      //         break;
      //     }

      //     // return throwError(event.body);
      //   }
      // }),
      catchError((error: HttpErrorResponse) => {
        switch (error.status) {
          case 400:
          case 403:
          case 501:
          case 500:
            this.log(YELLOW, request.method + ' ' + error.status + ' Error: ' + error.error);
            this.dialogService.openDialog({
              title: error.status + ' Error',
              body: error.error,
            });
            break;

          default:
            break;
        }

        return throwError(error.error);
      }),
    );
  }

  private log = (color: string, txt: string) => console.log(color, '[API] ' + txt);
}

const RED = '\x1b[31m%s\x1b[0m';
const GREEN = '\x1b[32m%s\x1b[0m';
const YELLOW = '\x1b[33m%s\x1b[0m';
const MAGENTA = '\x1b[35m%s\x1b[0m';
const CYAN = '\x1b[36m%s%s\x1b[0m';
