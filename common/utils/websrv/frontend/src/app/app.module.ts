/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file common/utils/websrv/frontend/src/app/app.module.ts
 * \brief: implementation of web interface frontend for oai
 * \all components, (externals or devlopped for oai) used by the application are imported from here
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {DragDropModule} from "@angular/cdk/drag-drop";
import {HttpClientModule} from "@angular/common/http";
import {NgModule} from "@angular/core";
import {FlexLayoutModule} from "@angular/flex-layout";
import {FormsModule, ReactiveFormsModule} from "@angular/forms";
import {MatButtonModule} from "@angular/material/button";
import {MatCardModule} from "@angular/material/card";
import {MatChipsModule} from "@angular/material/chips";
import {MatDialogModule} from "@angular/material/dialog";
import {MatFormFieldModule} from "@angular/material/form-field";
import {MatGridListModule} from "@angular/material/grid-list";
import {MatInputModule} from "@angular/material/input";
import {MatListModule} from "@angular/material/list";
import {MatProgressSpinnerModule} from "@angular/material/progress-spinner";
import {MatSelectModule} from "@angular/material/select";
import {MatSlideToggleModule} from "@angular/material/slide-toggle";
import {MatSliderModule} from "@angular/material/slider";
import {MatSnackBarModule} from "@angular/material/snack-bar";
import {MatTableModule} from "@angular/material/table";
import {MatTabsModule} from "@angular/material/tabs";
import {MatToolbarModule} from "@angular/material/toolbar";
import {MatTooltipModule} from "@angular/material/tooltip";
import {BrowserModule} from "@angular/platform-browser";
import {BrowserAnimationsModule} from "@angular/platform-browser/animations";
import {NgChartsModule} from "ng2-charts";
import {CommandsApi} from "./api/commands.api";
import {ScopeApi} from "./api/scope.api";
import {InfoApi} from "./api/info.api";
import {AppRoutingModule} from "./app-routing.module";
import {AppComponent} from "./app.component";
import {InfoComponent} from "./components/info/info.component";
import {CommandsComponent} from "./components/commands/commands.component";
import {ConfirmDialogComponent} from "./components/confirm/confirm.component";
import {DialogComponent} from "./components/dialog/dialog.component";
import {QuestionDialogComponent} from "./components/question/question.component";
import {ScopeComponent} from "./components/scope/scope.component";
import {InterceptorProviders} from "./interceptors/interceptors";
import {WebSocketService} from "./services/websocket.service";

@NgModule({
  declarations : [ AppComponent, CommandsComponent, InfoComponent, ConfirmDialogComponent, QuestionDialogComponent, DialogComponent, ScopeComponent ],
  imports : [
    BrowserModule,        AppRoutingModule,   FormsModule,    ReactiveFormsModule, BrowserAnimationsModule,  HttpClientModule, MatButtonModule, FlexLayoutModule, MatDialogModule, DragDropModule,
    MatSliderModule,      MatFormFieldModule, MatInputModule, MatChipsModule,      MatProgressSpinnerModule, MatToolbarModule, MatTableModule,  MatListModule,    MatSelectModule, MatSnackBarModule,
    MatSlideToggleModule, MatGridListModule,  MatCardModule,  MatTabsModule,       MatTooltipModule,         NgChartsModule,
  ],
  providers : [
    // services
    WebSocketService,
    // api
    CommandsApi,
    InfoApi,
    ScopeApi,
    // interceptors
    InterceptorProviders,
  ],
  bootstrap : [ AppComponent ]
})
export class AppModule {
}
