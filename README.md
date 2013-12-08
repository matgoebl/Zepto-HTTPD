Zepto-HTTPD
===========

*A demonstration of a minimalistic http server implementation.*  
*Zepto- (symbol z) is a prefix in the metric system denoting *10^-21.*

This is a very minimalistic `HTTP/1.0` working implementation in just
100 lines of code (100 characters wide) written in C for any POSIX OS.
Is results in a less than 10kB executable that only depends on libc.

Features
--------

- It serves web pages from the file system. Known content types are `.html`, `.txt` and `.jpg`,
  all other files are served as binary (`application/octet-stream`).
- It supports simple CGI-scripts:
    - All files in `/cgi/` are executed as CGI scripts, the content-type is derived from the file extension.  
      Examples: `/cgi/env.txt /cgi/index.html /cgi/download.nph`
    - It sets `REMOTE_ADDR`, `REQUEST_URI`, `PATH_INFO` and `QUERY_STRING`.
    - It decodes all query parameters (`QUERY_STRING`) to environment variables named `CGIPARAM_*`.  
      Example: `?a=1&B=2` results in `CGIPARAM_A=1` and  `CGIPARAM_B=2`. Also decodes %XX and +.
    - It writes all header fileds to `ZHTTP_*` environment variables, e.g. `ZHTTP_USER_AGENT`.  
      See `/cgi/env.txt` to get a list.
    - `NPH` (No Parsed Headers) scripts are supported with the extension `.nhp`:
      Those scripts must send their own headers. See `/cgi/download.nph` for an example that calculates
      the missing `Content-Length` header by itself.
- It supports very simplistic authentication. If the `ZAUTH` environment is set, the Authorization
  header from the client must match (try `make authtest`).  
  Example: `ZAUTH='Basic dXNlcjpwYXNz' ./zhttpd`  
  (`dXNlcjpwYXNz` is base64 encoded `user:pass`)
- It allows `PUT`, `POST` and `DELETE` methods to the `/data`/ subdirectory. `DELETE` can be emulated
  by e.g. `GET /data/somefiles??method=delete`.
- Short logging output.
- Debug output when the environment `DEBUG` is set.

Tricks and limitations
----------------------

- It only supports only `HTTP/1.0`, because  `HTTP/1.0` can be kept very simple.
  `HTTP/1.1` adds many mandatory features and hence complexity.
- It uses the environment as an associative array for header variables and query parameters.
- It provides a very limited set of known file types.
- It has limited error handling: It responds with status `404`, `401` or `503` and returns the message `HTTP Error` in all cases.

Usage
-----

`zhttpd [ PORT [ HTMLROOT [ INDEXPAGE ] ] ]`  
Default: `zhttpd 8888 . /index.html`

`PORT` is the tcp port to listen on, `HTMLROOT` is the root directory for web pages,
`INDEXPAGE` is the main index when no page is requested (e.g. just `http://127.0.0.1/`).
`INDEXPAGE` can be a cgi script, e.g. `/cgi/files.html`.

History
-------

Version 0.4 (included for historic reasons) contained a small ajax application,
demonstrating with how little effort an embedded ajax interface can be implemented.
Since version 0.5 the ajax demo is removed and a PUT/DELETE function has been added.
Version 0.4 is more readable, the current versions suffers from feature overload - this happens to almost every
piece of software that matures.. ;-)


License
-------
    Copyright 2010-2013 by Matthias Goebl <matthias.goebl*goebl.net>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

See the LICENSE file for a complete copy of the license.
