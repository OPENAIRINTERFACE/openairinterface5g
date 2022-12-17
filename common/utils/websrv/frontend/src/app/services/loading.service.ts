import {Injectable} from "@angular/core";
import {Subject} from "rxjs";

@Injectable({
  providedIn : "root",
})
export class LoadingService {
  isLoading$ = new Subject<boolean>();

  constructor()
  {
  }

  startLoading()
  {
    this.isLoading$.next(true);
  }

  stopLoading()
  {
    this.isLoading$.next(false);
  }
}
