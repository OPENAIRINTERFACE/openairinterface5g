import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';


export enum IOptionType {
    subcommand = "subcommand",
    variable = "variable"
}
export interface IOption {
    type: IOptionType;
}

export interface IVariable extends IOption {
    name: string;
    value: string;
    modifiable: boolean;
}
export interface ISubCommands extends IOption {
    name: string[];
}

const route = '/oaisoftmodem';

@Injectable({
    providedIn: 'root',
})
export class CommandsApi {
    constructor(private httpClient: HttpClient) { }

    public readStatus$ = () => this.httpClient.get<IVariable[]>(environment.backend + route + '/status/');

    public readModules$ = () => this.httpClient.get<string[]>(environment.backend + route + '/modules/');

    public getOptions$ = (cmdName: string) => this.httpClient.get<IOption[]>(environment.backend + route + '/module/' + cmdName);

    public runCommand$ = (cmdName: string) => this.httpClient.post<string>(environment.backend + route + '/command/' + cmdName, {});

    public setVariable$ = (variable: IVariable) => this.httpClient.post<string>(environment.backend + route + '/variable/' + variable.name, variable.value);

}
