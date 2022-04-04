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

  onCmdSelect() {
    this.subcmds$ = this.commandsApi.readModuleCommands$(`${this.selectedCmd?.nameFC.value}`).pipe(
      map(cmds => cmds.map(cmd => new CmdCtrl(cmd))),
      // tap(cmds => [this.selectedSubCmd] = cmds)
    )

    this.subvars$ = this.commandsApi.readModuleVariables$(`${this.selectedCmd?.nameFC.value}`).pipe(
      map(vars => vars.map(v => new VarCtrl(v))),
      // tap(vars => [this.selectedSubVar] = vars)
    )
  }

  onSubCmdSelect() {
    this.args$ = this.commandsApi.readModuleVariables$(`${this.selectedCmd?.nameFC.value}/${this.selectedSubCmd?.nameFC.value}`).pipe(
      map(vars => vars.map(v => new VarCtrl(v))),
      // tap(vars => [this.selectedSubVar] = vars)
    )
  }

  onVarSubmit(control: VarCtrl) {
    this.commandsApi.runCommand$(`${this.selectedCmd?.nameFC.value}/${control.nameFC.value}`, control.valueFC.value).subscribe();
  }

  onSubVarSubmit(control: VarCtrl) {
    this.commandsApi.runCommand$(`${this.selectedCmd?.nameFC.value}/${this.selectedSubCmd?.nameFC.value}/${control.nameFC.value}`, control.valueFC.value).subscribe();
  }
}
