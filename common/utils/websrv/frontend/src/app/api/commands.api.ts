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

    public readVariables$ = () => this.httpClient.get<IVariable[]>(environment.backend + route + '/variables/');

    public readCommands$ = () => this.httpClient.get<ICommand[]>(environment.backend + route + '/commands/');

    public readModuleVariables$ = (moduleName: string) => this.httpClient.get<IVariable[]>(environment.backend + route + '/' + moduleName + '/variables/');

    public readModuleCommands$ = (moduleName: string) => this.httpClient.get<ICommand[]>(environment.backend + route + '/' + moduleName + '/commands/');

    public runCommand$ = (moduleName: string, variable: IVariable) => this.httpClient.post<string>(environment.backend + route + '/' + moduleName + '/commands/', variable);

}
