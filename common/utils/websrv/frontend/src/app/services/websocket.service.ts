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

/*! \file common/utils/websrv/frontend/src/app/services/websocket.service.ts
 * \brief: implementation of web interface frontend for oai
 * \utility implementing a web interface with the backend
 * \author:  Yacine  El Mghazli, Francois TABURET
 * \date 2022
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: yacine.el_mghazli@nokia-bell-labs.com  francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
import {Injectable} from "@angular/core";
import {webSocket, WebSocketSubject} from "rxjs/webSocket";
import {environment} from "src/environments/environment";

export enum webSockSrc {
  softscope = "s".charCodeAt(0),
  logview = "l".charCodeAt(0),
}

export interface Message {
  source: webSockSrc;
  fullbuff: ArrayBuffer;
}
export const arraybuf_data_offset = 8; // 64 bits (8 bytes) header

const deserialize = (fullbuff: ArrayBuffer):
    Message => {
      const header = new DataView(fullbuff, 0, arraybuf_data_offset); // header
      return {source : header.getUint8(0), fullbuff : fullbuff};
    }

const serialize = (msg: Message):
    ArrayBuffer => {
      let buffview = new DataView(msg.fullbuff);
      buffview.setUint8(0, msg.source); // header
      return buffview.buffer;
    }

@Injectable() export class WebSocketService {
  public subject$: WebSocketSubject<Message>;

  constructor()
  {
    this.subject$ = webSocket<Message>({
      url : environment.backend.replace("http", "ws") + "softscope",
      openObserver : {next : () => { console.log("WS connection established") }},
      closeObserver : {next : () => { console.log("WS connextion closed") }},
      serializer : msg => serialize(msg),
      deserializer : msg => deserialize(msg.data),
      binaryType : "arraybuffer"
    });
  }

  // public get scopeSubject$() {
  //   return this.subject$.multiplex(
  //     () => { console.log('WS scope2 connection established') },
  //     () => { console.log('WS scope2 connection closed') },
  //     msg => msg.source === webSockSrc.softscope,
  //   );
  // }

  // public get loggerSubject$() {
  //   return this.subject$.multiplex(
  //     () => { console.log('WS logger connection established') },
  //     () => { console.log('WS logger connection closed') },
  //     msg => msg.source === webSockSrc.logview,
  //   );
  // }

  public send(msg: Message)
  {
    console.log("Message sent to websocket: ", msg.fullbuff);
    this.subject$.next(msg);
  }

  // public close() {
  //   this.subject$.complete();
  // }

  // public error() {
  //   this.subject$.error({ code: 4000, reason: 'I think our app just broke!' });
  // }
}
