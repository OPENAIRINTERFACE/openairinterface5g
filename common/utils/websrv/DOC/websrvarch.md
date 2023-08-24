# web server interface principles

The web server interface is implemented in two parts: a back-end, included in the softmodem process and a front-end which is a browser application.

The oai web server back-end is implemented in a shared library to be loaded by the [oai shared library loader](../../DOC/loader.md) when `--websrv` option is specified on the command line. `libwebsrv.so`  code is common to all oai softmodem executables,
 the current release has been tested with the nr UE and the gNB.

The front-end is an [angular](https://angular.io/docs) application. After being built and installed it is delivered to browsers by the back-end at connection time.

Front-end and back-end communicate via http request - http response transactions, including `json` body. these `json` interfaces are defined in the [frontend/src/app/api/XXXX.api.ts](../frontend/src/app/api/) files.
Back-end uses a callback machanism to map received http requests to dedicated functions. Typically a callback function decapsulates the http requests it receives, looking for the `json`body to determine the requested information or command. Then the back-end builds a
http response, with a `json`body encapsulating the requested information or the requested command result.

When unsolicited communication, from back-end to front-end is necessary, a [websocket](https://www.rfc-editor.org/rfc/rfc6455) link is opened. This is the case for the softscope interface.

# web server interface  source files

web server source files are located in [common/utils/websrv](..)

1. back-end files are directly located in the `websrv` repository
1.  The [frontend/src](../frontend/src) sub-directory contains the angular front-end source tree.
1.  [common/utils/websrv/helpfiles](../helpfiles) contains files delivered to the front-end to respond to help requests.

[oai web server interface home](websrv.md)
