## CrossAuth is a metamod-r module that allows Steam authentication between mismatched client and server mod.

That means you can connect cs1.6 clients into condition games and vice versa while using Steam authentication.

Without this module you would get "STEAM validation rejected" message.
This is because the server checks the AppId stored in the player's ticket.
What this module does is quite simple -- it sets the server's AppId to the same value as the connecting client.

## Requirements

In order to run this module you need:
1. [ReHLDS](https://github.com/dreamstalker/rehlds)
2. [Metamod-r](https://github.com/theAsmodai/metamod-r)

## Installation

Simply grab a binary from [the latest release](https://github.com/jonatan1024/CrossAuth/releases/latest) and register it in your `plugins.ini`.

### Windows

Download `win32.zip` and extract `crossauth.dll` into `addons/crossauth`. Then append into `plugins.ini` this line:
```ini
win32 addons\crossauth\crossauth.dll
```

### Linux

Download `linux.zip` and extract `crossauth_mm_i386.so` into `addons/crossauth`. Then append into `plugins.ini` this line:
```ini
linux addons/crossauth/crossauth_mm_i386.so
```
