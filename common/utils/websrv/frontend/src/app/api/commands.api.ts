import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { environment } from 'src/environments/environment';

export interface IOption {
    key: string;
    value: number;
}
export interface ICommand {
    name: string;
    options: IOption[];
}

export interface IStatus {
    infos: string;
}

export interface ILog {
    text: string;
}

const route = '/commands';

@Injectable({
    providedIn: 'root',
})
export class CommandsApi {
    constructor(private httpClient: HttpClient) { }

    public readCommands$ = () => this.httpClient.get<ICommand[]>(environment.backend + route + '/list');

    public readStatus$ = () => this.httpClient.get<IStatus>(environment.backend + route + '/status');

    public runCommand$ = (command: ICommand) => this.httpClient.post<ICommand>(environment.backend + route, command);

    public readLogs$ = () => this.httpClient.get<IStatus>(environment.backend + route + '/logs');
}