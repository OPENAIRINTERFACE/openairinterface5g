import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';


export interface IVariable {
    name: string;
    value: string;
    type: IArgType;
    modifiable: boolean; //set command ?
}

export enum IArgType {
    boolean = "boolean",
    list = "list",
    range = "range",
    string = "string"
}

export interface ICommand {
    name: string;
}

const route = '/oaisoftmodem';

@Injectable({
    providedIn: 'root',
})
export class CommandsApi {
    constructor(private httpClient: HttpClient) { }

    public readVariables$ = (moduleName?: string) => this.httpClient.get<IVariable[]>(environment.backend + route + '/' + (moduleName ? ('/' + moduleName) : "") + '/variables/');

    public readCommands$ = (moduleName?: string) => this.httpClient.get<ICommand[]>(environment.backend + route + '/' + (moduleName ? ('/' + moduleName) : "") + '/commands/');

    public runCommand$ = (moduleName: string, command: ICommand) => this.httpClient.post<string>(environment.backend + route + '/' + moduleName + '/commands/', command);

    public setVariable$ = (variable: IVariable, moduleName?: string) => this.httpClient.post<string>(environment.backend + route + (moduleName ? ('/' + moduleName) : "") + '/variables/', variable);

}
