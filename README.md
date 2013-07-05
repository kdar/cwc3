cwc3
====

Gives more power and control to the host of a Warcraft III game. The main purpose of this program is to kick people from Warcraft III games since it is not implemented in the game itself. It also has several ways to automatically kick people based on certain parameters (If they're from the wrong country, they ping too high, etc..).

# Discontinued

This project is no longer developed. The reason being I was no longer interested in the game and there is a better method
for achieving what this program does, and that's with proxying.

# User interface

ncurses using libcdk

# Features

* Can get anyone's latency whether their router is blocking ICMP or not
* Ability to kick anyone on command
* Auto join kick on: High ping, Wrong country, Kicked previously, Multiple IP
* Contains a list of the currently connected users and their info
* Event history list
* Can execute commands via the interface or inside Wc3 itself
* A title bar containing the game time, map name, and game name

# Notes

This program assumes it is run on a router or a machine that can sniff the traffic of the computer running Warcraft III.
