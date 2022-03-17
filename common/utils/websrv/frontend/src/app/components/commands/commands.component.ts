/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable no-shadow */
/* eslint-disable eqeqeq */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component, OnInit } from '@angular/core';
import { FormControl } from '@angular/forms';
import { CommandsApi } from 'src/app/api/commands.api';
import { InfosCtrl } from 'src/app/controls/infos.control';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  // infos$: Observable<InfosCtrl>
  infos: InfosCtrl

  selectedCmd?: FormControl

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    // this.infos$ = this.commandsApi.readInfos$().pipe(
    //   map((doc) => new InfosCtrl(doc)),
    // );

    this.infos = new InfosCtrl({
      display_status: {
        config_file: '../../../ci-scripts/conf_files/gnb.band78.sa.fr1.106PRB.usrpn310.conf',
        executable_function: "gnb"
      },
      menu_cmds: [
        "telnet",
        "softmodem",
        "loader",
        "measur",
        "rfsimu"
      ]
    })

    // console.log(JSON.stringify(this.infos, null, 2))
  }
}
