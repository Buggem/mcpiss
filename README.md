![The Minecraft Pretty Info Scanning Software (MCPISS) Logo, yellow water block](logo.png)

<!-- The Minecraft Pretty Info Scanning Software (MCPISS) -->
**Scans Minecraft servers, in pretty fashion.**

## Building
The software only supports Linux-based operating systems at this time, however this may change in the future.

You'll need the yyjson devel packages and the library itself.
Static or dynamic linking are both fine, however this tutorial will only show dynamic linking for simplicity.

There is also an option to disable experimental and likely unstable text component parsing. Append `-DNO_MCTEXT_COMPONENTS` to your compiler arguments or add define `NO_MCTEXT_COMPONENTS` in a header.

```
gcc *.c -I./ -lyyjson -o mcpiss
```

## Usage
The tool has several command line options, however none are documented by an in-executable usage prompt at this time.

This will definitely change in the future.

`--ip` can be used to define what IP you want to ping. The default is 127.0.0.1.<br>
Port syntax (X.X.X.X:port) is not supported, but simillar effects can be achieved using the `--port` flag, described below.<br>
Domain name resolution is not supported at this time.<br>

**An example of this flag's usage is `mcpiss --ip 192.0.2.1`.**

`--port` can be used to define what port of the IP you want to ping. The default is 25565.<br>

**An example of this flag's usage is `mcpiss --port 12345`.**

`--dns` can be used to define what domain name/IP you want to specify in the handshake. The default is the IP you are connecting to.<br>
This solves the problem of multiple servers residing on one IP with different DNS records.<br>
One example of this is Hypixel, where without specifying `--dns mc.hypixel.net` the server closes the connection after the handshake.<br>

**An example of this flag's usage is `mcpiss --ip 192.0.2.1 --dns mc.example.com`.**

`--verbose` can be used to log to a higher extent.<br>
An alias of `--verbose` is `-v`.<br>
**An example of this flag's usage is `mcpiss --verbose`.**

## Licensing

This software is licensed under the GNU General Public License version 3.0.


## Reporting bugs

Bug reports should be made using the GitHub issue tracker.<br>
If it is a security related issue, I recommend you to create a pull request so it can be resolved immediately.
