import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';


export interface IVariable {
    name: string;
    value: string;
    type: IArgType;
    modifiable: boolean; //set command ?
}

export enum ILogLvl {
    error="error",
    warn="warn",
    analysis="analysis",
    info="info",
    debug="debug",
    trace="trace"
}
export interface ICmdResp {
 component: string;
 level:     ILogLvl;
 enabled:   boolean;
 output:    string;
}

export enum IArgType {
    boolean = "boolean",
    list = "list",
    range = "range",
    number = "number",
    string = "string"
}

export interface ICommand {
    name: string;
    confirm?: string;
}

const route = '/oaisoftmodem';

@Injectable({
    providedIn: 'root',
})
export class CommandsApi {
    constructor(private httpClient: HttpClient) { }

    public readVariables$ = (moduleName?: string) => this.httpClient.get<IVariable[]>(environment.backend + route + '/' + (moduleName ? ('/' + moduleName) : "") + '/variables/');

    public readCommands$ = (moduleName?: string) => this.httpClient.get<ICommand[]>(environment.backend + route + '/' + (moduleName ? ('/' + moduleName) : "") + '/commands/');

    public runCommand$ = (command: ICommand, moduleName: string) => this.httpClient.post<ICmdResp[]>(environment.backend + route + '/' + moduleName + '/commands/', command);

    public setVariable$ = (variable: IVariable, moduleName?: string) => this.httpClient.post<string>(environment.backend + route + (moduleName ? ('/' + moduleName) : "") + '/variables/', variable);

}
