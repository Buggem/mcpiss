![The Minecraft Pretty Info Scanning Software (MCPISS) Logo, yellow water block](logo.png)

<!-- The Minecraft Pretty Info Scanning Software (MCPISS) -->
**Scans Minecraft servers, in pretty fashion.**

## Building
The software only supports Linux-based operating systems at this time, however this may change in the future.

You'll need the yyjson devel packages and the library itself.
Static or dynamic linking are both fine, however this tutorial will only show dynamic linking for simplicity.

There is also an option to disable experimental and likely unstable text component parsing.
Append `-DNO_MCTEXT_COMPONENTS` to CFLAGS (`make CFLAGS="-DNO_MCTEXT_COMPONENTS"`) or define `NO_MCTEXT_COMPONENTS` in a header.

```
make
```

Use `make clean` to remove the executable and .o files.

## Installation
Installation is not necessary for the program to function, but is an optional method to get it into your `PATH`.

```
sudo make install
```
## Usage
The tool has several command line options, which can be listed using `--help`.

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
See LICENSE for details.

## Reporting bugs

Bug reports should be made using the GitHub issue tracker.<br>
If it is a security related issue, I recommend you to create a pull request so it can be resolved immediately.
