
# MiSTer Main App FORK

This is a fork of the main app that contorls the MiSTer.

Original documanetation can be found on the [upstream
repository](https://github.com/MiSTer-devel/Main_MiSTer) and in the [MiSTer
github project](https://github.com/MiSTer-devel).

It follows a description of the improvements contained in this patch.

# New commands

All the new features are exposed through the /dev/MiSTer_cmd interface. Text
command can be written in this file with standard unix tool (echo, fopen, etc).

If none of the new commands are used, the MiSTer Main App will just work as
before: no new processes are run, no new functions are triggered, and so on.

# Loading a core

This scenario is already handled by the upstream app. You can load a new core
with the command

```
load_core /path/to/core/file
```

Please note that after loading the core, the Main App will restart itself. So
if you need to write other command after a `load_core` you have to wait some
time (1 second will be enough).

# Slave menu application / launcher

A slave application can be launched with the command

```
slave_enable 1
```

This command will also switch to the linux Virtual Terminal so that the slave
app can show some text or graphics.

The application, during its execution, can interact with
the MiSTer app through /dev/MiSTer_cmd, e.g. to load a core a rom.

The command

```
slave_enable 0
```

switches back to the standard MiSTer Main App visualization.

Note that subsequent `slave_enable 0` or `1` will not start again the slave
app, but instead will send `SIGSTOP` and `SIGTERM` to the already running one.
If the old one exited, a new one will be launched with `slave_enable 1`.

The slave app must take into account the image-resetting behaviour of the
MiSTer Main App. So, if it sends a `load_core` command, it can send some other
command with the caveat exposed in the previous section about the `load_core`
command. Moreover, it should terminate itself few time after, since the MiSTer
Main App does not know anymore of the slave process.

NOTE: for now the slave application to call is hardcoded in the source. It is

```
/media/fat/fbmenu/lua /media/fat/fbmenu/fbmenu.lua
```

