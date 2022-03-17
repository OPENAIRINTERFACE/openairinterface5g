import { Component } from '@angular/core';
import { FormControl } from '@angular/forms';
import { Observable } from 'rxjs/internal/Observable';
import { map } from 'rxjs/internal/operators/map';
import { CommandsApi } from 'src/app/api/commands.api';
import { InfosCtrl } from 'src/app/controls/infos.control';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  infos$: Observable<InfosCtrl>
  // infos: InfosCtrl

  selected = new FormControl()

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    this.infos$ = this.commandsApi.readInfos$().pipe(
      map((doc) => new InfosCtrl(doc)),
    );

    // this.infos = new InfosCtrl({
    //   display_status: {
    //     config_file: '../../../ci-scripts/conf_files/gnb.band78.sa.fr1.106PRB.usrpn310.conf',
    //     executable_function: "gnb"
    //   },
    //   menu_cmds: [
    //     "telnet",
    //     "softmodem",
    //     "loader",
    //     "measur",
    //     "rfsimu"
    //   ]
    // })

    // const BODY_JSON = {
    //   "display_status": {
    //     "config_file": '../../../ci-scripts/conf_files/gnb.band78.sa.fr1.106PRB.usrpn310.conf',
    //     "executable_function": "gnb"
    //   },
    //   "menu_cmds": [
    //     "telnet",
    //     "softmodem",
    //     "loader",
    //     "measur",
    //     "rfsimu"
    //   ]
    // }

  }

  onSubmit() {
    this.commandsApi.runCommand$(this.selected.value).subscribe();
  }
}
