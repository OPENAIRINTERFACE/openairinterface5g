
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
