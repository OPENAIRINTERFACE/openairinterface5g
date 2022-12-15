import {HttpEvent, HttpHandler, HttpInterceptor, HttpRequest, HttpResponse} from "@angular/common/http";
import {Injectable} from "@angular/core";
import {Observable} from "rxjs";
import {finalize} from "rxjs/operators";

import {LoadingService} from "../services/loading.service";

@Injectable()
export class SpinnerInterceptor implements HttpInterceptor {
  activeRequests = 0;

  constructor(private loadingService: LoadingService)
  {
  }

  intercept(request: HttpRequest<unknown>, next: HttpHandler)
  {
    if (this.activeRequests === 0) {
      this.loadingService.startLoading();
    }

    this.activeRequests++;

    return next.handle(request).pipe(
        finalize(() => {
          this.activeRequests--;
          if (this.activeRequests === 0) {
            this.loadingService.stopLoading();
          }
        }),
    );
  }
}
