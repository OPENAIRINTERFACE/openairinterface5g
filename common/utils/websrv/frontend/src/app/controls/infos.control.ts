import { FormArray, FormControl, FormGroup } from '@angular/forms';
import { IInfos, IStatus } from '../api/commands.api';


const enum InfosFCN {
    menu_cmds = 'commands',
}

export class InfosCtrl extends FormGroup {

    display_status: IStatus

    constructor(icommand: IInfos) {
        super({});

        this.display_status = icommand.display_status

        this.addControl(InfosFCN.menu_cmds, new FormArray(icommand.menu_cmds.map((cmd) => new FormControl(cmd))));
    }

    api() {
        const doc: IInfos = {
            display_status: this.display_status,
            menu_cmds: this.cmdsFA.value,
        };

        return doc;
    }

    get cmdsFA() {
        return this.get(InfosFCN.menu_cmds) as FormArray;
    }

    set cmdsFA(control: FormArray) {
        this.setControl(InfosFCN.menu_cmds, control);
    }
}
