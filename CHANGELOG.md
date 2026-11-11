# Changelog

## 0.2.0 - 2026-11-11
- A new icon: the NAS of Material Design Icons, the one Home Assistant shows for `mdi:nas`.
- The question closes by itself when the device stops answering, so "Shut down" never starts a device that is off.
- A tap does nothing while the script still runs (after the device went off it may wait to cut the power), instead of
  running it a second time and showing "Starting".
- A device Home Assistant has no state for shows "Unavailable" and takes no tap, instead of "Off" and a start.
- A power sensor in kW, mW or MW is read as watts, for the tile's number and for "Off below".

## 0.1.0 - 2026-11-10
- First version: a tile that starts a NAS or a server, shuts it down after a question, and shows off, starting,
  running or shutting down with the power it draws. It brings a NAS icon of its own.
