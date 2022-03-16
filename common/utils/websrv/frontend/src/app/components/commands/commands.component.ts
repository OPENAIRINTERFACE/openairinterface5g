/* eslint-disable @typescript-eslint/member-ordering */
/* eslint-disable no-shadow */
/* eslint-disable eqeqeq */
/* eslint-disable @typescript-eslint/naming-convention */
import { Component } from '@angular/core';
import { Observable } from 'rxjs';
import { map, tap } from 'rxjs/operators';
import { CommandsApi } from 'src/app/api/commands.api';
import { CommandCtrl } from 'src/app/controls/command.control';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  commandsCtrls$: Observable<CommandCtrl[]>
  selectedCtrl?: CommandCtrl

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    this.commandsCtrls$ = this.commandsApi.readCommands$().pipe(
      map((docs) => docs.map((doc) => new CommandCtrl(doc))),
    );
  }

  onSubmit(control: CommandCtrl) {
    this.commandsApi.runCommand$(control.api()).pipe(
      // take(1),
      tap(() => control.markAsPristine())
    ).subscribe();
  }

}
