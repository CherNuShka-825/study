@echo off
start "Xming+PuTTY" /b .\Xming.exe :0 -dpi 85 -clipboard -trayicon -c -multiwindow -reset -terminate -unixkill -logfile Xming.log
