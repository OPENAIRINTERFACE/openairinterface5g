import {HTTP_INTERCEPTORS} from "@angular/common/http";
import {ErrorInterceptor} from "./error.interceptor";
import {SpinnerInterceptor} from "./spinner.interceptor";

export const InterceptorProviders = [
  {provide : HTTP_INTERCEPTORS, useClass : SpinnerInterceptor, multi : true},
  {provide : HTTP_INTERCEPTORS, useClass : ErrorInterceptor, multi : true},
];
