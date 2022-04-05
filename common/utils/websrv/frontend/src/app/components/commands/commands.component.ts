import { Component } from '@angular/core';
import { Observable } from 'rxjs/internal/Observable';
import { map } from 'rxjs/internal/operators/map';
import { tap } from 'rxjs/internal/operators/tap';
import { CommandsApi, IArgType } from 'src/app/api/commands.api';
import { CmdCtrl } from 'src/app/controls/cmd.control';
import { VarCtrl } from 'src/app/controls/var.control';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  IOptionType = IArgType;

  vars$: Observable<VarCtrl[]>
  cmds$: Observable<CmdCtrl[]>
  selectedCmd?: CmdCtrl
  selectedVar?: VarCtrl

  subvars$?: Observable<VarCtrl[]>
  subcmds$?: Observable<CmdCtrl[]>
  selectedSubCmd?: CmdCtrl
  selectedSubVar?: VarCtrl

  args$?: Observable<VarCtrl[]>
  selectedArg?: VarCtrl

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    this.vars$ = this.commandsApi.readVariables$().pipe(
      map((vars) => vars.map(ivar => new VarCtrl(ivar)))
    );

    this.cmds$ = this.commandsApi.readCommands$().pipe(
      map((cmds) => cmds.map(icmd => new CmdCtrl(icmd))),
      tap(controls => [this.selectedCmd] = controls)
    );
  }

  onCmdSelect() {

    this.subcmds$ = this.commandsApi.readCommands$(`${this.selectedCmd!.nameFC.value}`).pipe(
      map(icmds => icmds.map(icmd => new CmdCtrl(icmd))),
      tap(controls => [this.selectedSubCmd] = controls)
    )

    this.subvars$ = this.commandsApi.readVariables$(`${this.selectedCmd!.nameFC.value}`).pipe(
      map(ivars => ivars.map(ivar => new VarCtrl(ivar))),
      tap(controls => [this.selectedSubVar] = controls)
    )
  }

  onSubCmdSelect() {
    this.args$ = this.commandsApi.readVariables$(`${this.selectedCmd!.nameFC.value}/${this.selectedSubCmd!.nameFC.value}`).pipe(
      map(ivars => ivars.map(ivar => new VarCtrl(ivar))),
      tap(controls => [this.selectedArg] = controls)
    )
  }

  onVarSubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api()).subscribe();
  }

  onSubVarSubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api(), `${this.selectedCmd?.nameFC.value}`).subscribe();
    // this.commandsApi.setVariable$(control.api(), `${this.selectedCmd?.nameFC.value} / ${this.selectedSubCmd?.nameFC.value} / ${control.nameFC.value}`).subscribe();
  }
}
