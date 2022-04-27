import { Component } from '@angular/core';
import { Observable } from 'rxjs/internal/Observable';
import { of } from 'rxjs/internal/observable/of';
import { map } from 'rxjs/internal/operators/map';
import { mergeMap } from 'rxjs/internal/operators/mergeMap';
import { filter } from 'rxjs/operators';
import { CommandsApi, IArgType, IColumn, ICommand } from 'src/app/api/commands.api';
import { CmdCtrl } from 'src/app/controls/cmd.control';
import { Param, ParamFC } from 'src/app/controls/param.control';
import { VarCtrl } from 'src/app/controls/var.control';
import { DialogService } from 'src/app/services/dialog.service';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  IOptionType = IArgType;

  vars$: Observable<VarCtrl[]>
  modules$: Observable<CmdCtrl[]>
  selectedModule?: CmdCtrl
  selectedCmd?: ICommand

  modulevars$?: Observable<VarCtrl[]>
  modulecmds$?: Observable<CmdCtrl[]>

  //table columns
  displayedColumns: string[] = []
  rows$: Observable<ParamFC[][]> = new Observable<ParamFC[][]>()
  columns: IColumn[] = []

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
    public dialogService: DialogService
  ) {
    this.vars$ = this.commandsApi.readVariables$().pipe(
      map((vars) => vars.map(ivar => new VarCtrl(ivar)))
    );

    this.modules$ = this.commandsApi.readCommands$().pipe(
      map((cmds) => cmds.map(icmd => new CmdCtrl(icmd)))
    );
  }


  onModuleSelect(control: CmdCtrl) {

    this.selectedModule = control

    this.modulecmds$ = this.commandsApi.readCommands$(`${control.nameFC.value}`).pipe(
      map(icmds => icmds.map(icmd => new CmdCtrl(icmd)))
    )

    this.modulevars$ = this.commandsApi.readVariables$(`${control.nameFC.value}`).pipe(
      map(ivars => ivars.map(ivar => new VarCtrl(ivar))),
    )
  }

  onVarSubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api()).subscribe();
  }

  onModuleVarsubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api(), `${this.selectedModule!.nameFC.value}`)
      .pipe(
        map(resp => this.dialogService.openRespDialog(resp))
      ).subscribe();
  }

  onCmdSubmit(control: CmdCtrl) {
    this.selectedCmd = control.api()

    const obs = control.confirm
      ? this.dialogService.openConfirmDialog(control.confirm).pipe(
        filter(confirmed => confirmed)
      )
      : of(null)

    this.rows$ = obs.pipe(
      mergeMap(() => this.commandsApi.runCommand$(control.api(), `${this.selectedModule!.nameFC.value}`)),
      mergeMap(resp => {
        if (resp.display[0]) return this.dialogService.openRespDialog(resp, 'cmd ' + control.nameFC.value + ' response:')
        else return of(resp)
      }),
      map(resp => {
        this.columns = resp.table!.columns
        this.displayedColumns = this.columns.map(col => col.name)

        let rows = [];

        for (let i = 0; i < resp.table!.rows.length; i++) {
          let row: Param[] = []

          for (let j = 0; j < this.columns.length; j++) {
            row[j] = new Param(
              resp.table!.rows[i][j],
              this.columns[j],
              j,
              this.selectedModule!.nameFC.value,
              this.selectedCmd!.name
            )
          }
          rows[i] = row.map(param => new ParamFC(param))
        }
        return rows
      })
    );
  }

  onParamSubmit(row: ParamFC[]) {
    row.map(paramFC => this.commandsApi.setParam$(paramFC.api()).subscribe());
  }


}