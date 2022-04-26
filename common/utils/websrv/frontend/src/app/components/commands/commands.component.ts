import { Component } from '@angular/core';
import { Observable } from 'rxjs/internal/Observable';
import { of } from 'rxjs/internal/observable/of';
import { map } from 'rxjs/internal/operators/map';
import { mergeMap } from 'rxjs/internal/operators/mergeMap';
import { filter } from 'rxjs/operators';
import { CommandsApi, IArgType, IColumn } from 'src/app/api/commands.api';
import { CmdCtrl } from 'src/app/controls/cmd.control';
import { EntryFC } from 'src/app/controls/entry.control';
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
  // selectedVar?: VarCtrl

  subvars$?: Observable<VarCtrl[]>
  cmds$?: Observable<CmdCtrl[]>
  // selectedSubCmd?: CmdCtrl
  // selectedSubVar?: VarCtrl

  args$?: Observable<VarCtrl[]>
  // selectedArg?: VarCtrl

  //table columns
  displayedColumns: string[] = []
  rows$: Observable<EntryFC[][]> = new Observable<EntryFC[][]>()
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
      map((cmds) => cmds.map(icmd => new CmdCtrl(icmd))),
      // tap(controls => [this.selectedCmd] = controls)
    );
  }


  onModuleSelect(control: CmdCtrl) {

    this.selectedModule = control

    this.cmds$ = this.commandsApi.readCommands$(`${control.nameFC.value}`).pipe(
      map(icmds => icmds.map(icmd => new CmdCtrl(icmd)))
    )

    this.subvars$ = this.commandsApi.readVariables$(`${control.nameFC.value}`).pipe(
      map(ivars => ivars.map(ivar => new VarCtrl(ivar))),
      // tap(controls => [this.selectedSubVar] = controls)
    )
  }

  onCmdSelect(control: CmdCtrl) {

    // this.selectedSubCmd = control

    this.args$ = this.commandsApi.readVariables$(`${this.selectedModule!.nameFC.value}/${control.nameFC.value}`).pipe(
      map(ivars => ivars.map(ivar => new VarCtrl(ivar)))
    )
  }

  onVarSubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api()).subscribe();
  }

  onSubVarSubmit(control: VarCtrl) {
    this.commandsApi.setVariable$(control.api(), `${this.selectedModule!.nameFC.value}`)
      .pipe(
        map(resp => this.dialogService.openRespDialog(resp))
      ).subscribe();
  }

  onCmdSubmit(control: CmdCtrl) {

    const obs = control.confirm
      ? this.dialogService.openConfirmDialog(control.confirm).pipe(filter(confirmed => confirmed))
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
          let row: EntryFC[] = []

          for (let j = 0; j < this.columns.length; j++) {
            row[j] = new EntryFC(resp.table!.rows[i][j], this.columns[j].type)
          }
          rows[i] = row
        }
        return rows
      })
    );
  }


}