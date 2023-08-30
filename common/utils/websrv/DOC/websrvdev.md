# enhancing the web server

###### development platform

Backend devlopment is classical C programming, using [libulfius](https://github.com/babelouest/ulfius/blob/master/API.md) for the web server implementation
and [libjansson](https://jansson.readthedocs.io/en/latest/) for formatting and accessing the JSON http bodies which are used by the angular frontend to exchange data with the server.
The backend processes the http requests coming from the frontend using the ulfius callback mechanism. backend can also send unsollicited data to the frontend using a websocket

Frontend has been developped using the [angular framework](https://angular.io/) which  implies TypeScript and HTML programming with the specificity of the node.js libraries and
angular extensions.

Debugging the frontend side may be more difficult than the backend, some tools may help:
- Some IDE such as [vscode]( https://code.visualstudio.com/) are "angular aware" and can ease debugging your modifications .
-  Setting UTIL log level to informational in the backend  and websrv debug flag to 2 (  `--log_config.util_log_level info --websrv.debug 2` ) will trigger backend traces which may help, including the dump of JSON type http content
- Browser devloper tools such as console may also help

There is a dedicated CMakeLists.txt, located in the websrv directory,  to build both backend and frontend.  Including the websrv option when configuring cmake ( `./build_oai --build-lib websrv` ) is required to be able to include the web server targets in the oai build scripts (either Makefile or ninja).
 `libwebsrv.so` shared library is the backend binary. It is possibly dynamically loaded at runtime, which then triggers the execution of the
`websrv_autoinit` function that initializes the http server. Re-building the backend can be done using either `make websrv` or `ninja websrv` and it also  re-builds the frontend .

The frontend run-time is made of a set of files generated from the TypeScript, HTML, CSS sources via the npm utility. It also includes some directly edited files such as the helpfiles. Frontend run-time is installed in  the `websrv` sub-directory of the build path
(usually `<oai repository>/cmake_targets/ran_build/build`) Re-building frontend can be done via the websrvfront target: `make websrvfront` or `ninja websrvfront`.


###### backend source files

They are all located in the websrv repository
| source file |description |
|---|---|
| websrv.c | main backend file, starts the http server and contains functions for telnet server interface ( softmodem commands tab) |
| websrv.h | the only web server include file, contains utils prototypes, constants definitions, message definitions. Note that it must be kept consistent with frontend, unfortunatly we have not found a way to have common include files between C and javascript |
| websrv_utils.c | utility functions common to all backend sources: dump http request and JSON content. format string response from a file, a C string, a buffer asynchronously loaded. format help from help file |
| websrv_websockets.c | contains functions for the softscope interface (scope tab): initialize, close websocket, dispatch incoming messages, send a websocket message to frontend |
| websrv_scope.c |  softscope specific functions:  callbacks to process softope frontend request and function to send, receive and process softscope websocket messages |
| websrv_noforms.c websrv_noforms.h | stub functions to help using a common softscope interface for xforms softscope and the webserver softscope, could be removed when improving softscope architecture (don't use interface specific function in nr_physcope.c) |


###### main frontend source files

Frontend directory tree comes from the angular framework. The root of this tree is `websrv/frontend/`. Main sub directories or files are:

- `src/app/api` contains TypeScript files with functions to send http requests to the backend. These functions are used from components sources.
- `src/app/components/<component name>` contains the code (TypeScript, HTML and possibly CSS or XCSS)  for a web page, for example the softscope, or popup page used to ask a question or return transaction status.
- `src/app/components/controls` contains TypeScript code used for managing some form fields used in the `softmodem commands` tab.
- `src/app/components/services` contain TypeScript code for utilities such as managing the websocket  interface with the backend or downloading a file.
- `src/app/app-routing-module.ts` defines mapping between urls as entered by user and components
- `src/app/app.component.ts` `src/app/app.component.html` define the first page displayed to user
- `src/environments` contains environment.<build type>.ts file wich defines the `environment` variable depending on the build type. The delivered build scripts are using the `prod` version which  has been written to make frontend and backend to interact properly in a production platform. Other build type is to be used in debug environment where frontend is not downloaded from the backend.
- `src/commondefs.ts`: constant definitions common to several TypeScript sources


[oai web serverinterface  home](websrv.md)
