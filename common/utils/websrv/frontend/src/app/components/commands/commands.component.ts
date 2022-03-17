/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable no-shadow */
/* eslint-disable eqeqeq */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component } from '@angular/core';
import { FormControl } from '@angular/forms';
import { Observable } from 'rxjs';
import { map, tap } from 'rxjs/operators';
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
  selectedCmd?: FormControl

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    this.infos$ = this.commandsApi.readInfos$().pipe(
      map((doc) => new InfosCtrl(doc)),
    );
  }

}
