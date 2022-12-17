import {HttpErrorResponse, HttpEvent, HttpHandler, HttpInterceptor, HttpRequest, HttpResponse} from "@angular/common/http";
import {Injectable} from "@angular/core";
import {Observable, throwError} from "rxjs";
import {catchError, tap} from "rxjs/operators";
import {DialogService as DialogService} from "../services/dialog.service";

@Injectable()
export class ErrorInterceptor implements HttpInterceptor {
  constructor(public dialogService: DialogService)
  {
  }

  intercept(request: HttpRequest<unknown>, next: HttpHandler)
  {
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
          let prefix: string = "oai web interface [API]: ";
          let message: string = " ";
          switch (error.status) {
            case 400:
            case 403:
              console.error(prefix + request.method + " " + error.status + " Error: " + error.error.toString());
              break;
            case 501:
            case 500:
              console.warn(prefix + request.method + " " + error.status + " Error: " + error.error.toString());
              break;
            default:
              console.log(prefix + request.method + " " + error.status + " Error: " + error.error.toString());
              break;
          }
          if (error.error instanceof ErrorEvent) {
            message = error.error.message;
          } else {
            // The backend returned an unsuccessful response code.
            // The response body may contain clues as to what went wrong
            message = JSON.stringify(error.error);
          }
          this.dialogService.openErrorDialog(prefix + " " + error.status, message);
          return throwError(error);
        }),
    );
  }
}
