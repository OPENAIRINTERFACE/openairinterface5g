import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

export interface IStatus {
    config_file: string;
    executable_function: string;
}

export interface IInfos {
    display_status: IStatus;
    menu_cmds: string[];
}

const route = '/oaisoftmodem';

@Injectable({
    providedIn: 'root',
})
export class CommandsApi {
    constructor(private httpClient: HttpClient) { }

    public readInfos$ = () => this.httpClient.get<IInfos>(environment.backend + route);
}


// FRANCOIS_BODY = {
//     "main_oai softmodem": [
//         {
//             "display_status": [
//                 { "Config file": "../../../ci-scripts/conf_files/gnb.band78.sa.fr1.106PRB.usrpn310.conf" },
//                 { "Executable function": "gnb" }
//             ]
//         },
//         {
//             "menu_cmds": [
//                 "telnet",
//                 "softmodem",
//                 "loader",
//                 "measur",
//                 "rfsimu"
//             ]
//         }
//     ]
// }

// YASS_BODY = {
//     display_status: {
//         config_file: '../../../ ci - scripts / conf_files / gnb.band78.sa.fr1.106PRB.usrpn310.conf',
//         executable_function: "gnb"

//     },
//     menu_cmds: [
//         "telnet",
//         "softmodem",
//         "loader",
//         "measur",
//         "rfsimu"
//     ]
// }
