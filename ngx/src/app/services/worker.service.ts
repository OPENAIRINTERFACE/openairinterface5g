/* eslint-disable @typescript-eslint/naming-convention */
import { Injectable } from '@angular/core';
import { GoogleAuthService, NgGapiClientConfig } from 'ng-gapi';
import { environment } from 'src/environments/environment';
import { AutoSendApi } from '../api/autosend.api';

import GoogleUser = gapi.auth2.GoogleUser;

const WORKER_SCOPES = [
  'https://mail.google.com/',
  'https://www.googleapis.com/auth/gmail.modify',
  'https://www.googleapis.com/auth/gmail.compose',
  'https://www.googleapis.com/auth/gmail.send',

  'https://www.googleapis.com/auth/calendar',

  'https://www.googleapis.com/auth/contacts',

  'https://www.googleapis.com/auth/drive',
];


export const gapiClientConfig: NgGapiClientConfig = {
  discoveryDocs: ['https://analyticsreporting.googleapis.com/$discovery/rest?version=v4'],
  /**
   * The app's client ID, found and created in the Google Developers Console.
   */
  // client_id?: string;
  client_id: environment.WORKER_CLIENT_ID,

  /**
   * The domains for which to create sign-in cookies. Either a URI, single_host_origin, or none.
   * Defaults to single_host_origin if unspecified.
   */
  // cookie_policy: ,

  /**
   * The scopes to request, as a space-delimited string. Optional if fetch_basic_profile is not set to false.
   */
  // scope?: string;
  scope: WORKER_SCOPES.join(' '),

  /**
   * Fetch users' basic profile information when they sign in. Adds 'profile' and 'email' to the requested scopes. True if unspecified.
   */
  // fetch_basic_profile?: boolean;

  /**
   * The Google Apps domain to which users must belong to sign in. This is susceptible to modification by clients,
   * so be sure to verify the hosted domain property of the returned user. Use GoogleUser.getHostedDomain() on the client,
   * and the hd claim in the ID Token on the server to verify the domain is what you expected.
   */
  // hosted_domain?: string;

  /**
   * Used only for OpenID 2.0 client migration. Set to the value of the realm that you are currently using for OpenID 2.0,
   * as described in <a href="https://developers.google.com/accounts/docs/OpenID#openid-connect">OpenID 2.0 (Migration)</a>.
   */
  // openid_realm?: string;

  /**
   * The UX mode to use for the sign-in flow.
   * By default, it will open the consent flow in a popup.
   */
  // ux_mode?: "popup" | "redirect";
  ux_mode: 'redirect',

  /**
   * If using ux_mode='redirect', this parameter allows you to override the
   * default redirect_uri that will be used at the end of the consent flow.
   * The default redirect_uri is the current URL stripped of query parameters and hash fragment.
   */
  redirect_uri: window.location.origin + '/oauth2callback',
};

@Injectable({
  providedIn: 'root',
})
export class WorkerService {
  private user?: GoogleUser;

  constructor(private autosendApi: AutoSendApi, private googleAuthService: GoogleAuthService) { }

  get email() {
    return this.user ? this.user.getBasicProfile().getEmail() : undefined;
  }

  // popup
  public loginPopup() {
    this.googleAuthService.getAuth().subscribe(async (auth) => {
      this.user = await auth.signIn();
    });
  }

  // redirect
  public loginRedirect() {
    this.googleAuthService.getAuth().subscribe(async (auth) => {
      const { code } = await auth.grantOfflineAccess();
      this.autosendApi.register$(code).subscribe();
    });
  }

  public loginRedirectCallback() {
    this.googleAuthService.getAuth().subscribe((auth) => {
      this.user = auth.currentUser.get();
    });
  }

  public logout() {
    this.googleAuthService.getAuth().subscribe((auth) => {
      auth.signOut();
    });
  }
}
