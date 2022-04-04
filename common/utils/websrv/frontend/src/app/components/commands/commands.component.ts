import { Component } from '@angular/core';
import { FormControl } from '@angular/forms';
import { Observable } from 'rxjs/internal/Observable';
import { map } from 'rxjs/internal/operators/map';
import { CommandsApi, IArgType } from 'src/app/api/commands.api';
import { CmdCtrl } from 'src/app/controls/cmd.control';
import { VariableCtrl as VarCtrl } from 'src/app/controls/var.control';
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
      map((ivars) => ivars.map(ivar => new VarCtrl(ivar)))
    );

    this.cmds$ = this.commandsApi.readCommands$().pipe(
      map((ivars) => ivars.map(ivar => new CmdCtrl(ivar)))
    );
  }

  onCmdSubmit(control: CmdCtrl) {
    this.commandsApi.runCommand$(control.api()).subscribe();
  }

  onCmdSelect(control: CmdCtrl) {
    this.subcmds$ = this.commandsApi.readModuleCommands$(control.nameFC.value).pipe(
      map(cmds => cmds.map(cmd => new CmdCtrl(cmd))),
      // tap(cmds => [this.selectedSubCmd] = cmds)
    )

    this.subvars$ = this.commandsApi.readModuleVariables$(control.nameFC.value).pipe(
      map(vars => vars.map(v => new VarCtrl(v))),
      // tap(vars => [this.selectedSubVar] = vars)
    )
  }

  onSubCmdSelect(control: CmdCtrl) {
    this.args$ = this.commandsApi.readModuleVariables$(control.nameFC.value).pipe(
      map(vars => vars.map(v => new VarCtrl(v))),
      // tap(vars => [this.selectedSubVar] = vars)
    )
  }

  onVarSubmit(control: VarCtrl) {
    control.nameFC = new FormControl(`${this.selectedCmd?.nameFC.value} ${this.selectedSubCmd?.nameFC.value} ${control.nameFC.value}`.trim())
    this.commandsApi.setVariable$(control.api()).subscribe();
  }
}
