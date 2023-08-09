/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file common/utils/websrv/frontend/src/app/interceptors/error.interceptor.ts
 * \brief: implementation of web interface frontend for oai
 * \utility to intercept error response from backend and possibly dispay an error to user 
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
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
          this.dialogService.openErrorDialog(prefix + " " + request.url, "http status: " + error.status + " "  + message);
          return throwError(error);
        }),
    );
  }
}
