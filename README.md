# NAS power

A tile for [Tessera](https://github.com/MaxGramser/tessera-plugins) that starts a NAS or a server from the screen and
shuts it down after a question. It shows what the device does now (off, starting, running, shutting down) and the
power it draws.

## What it shows

- **Off:** the device does not answer and draws less than the idle power.
- **Starting:** it draws power but does not answer yet, or you just tapped the tile. The icon blinks.
- **Running:** it answers. The icon is lit.
- **Shutting down:** its script runs while the device is still there. The icon blinks.
- **Unavailable:** Home Assistant has no state for the device. A tap does nothing until it has one.

A tap starts the device when it is off. While it runs, a tap opens a card that asks first. While it starts or shuts
down, a tap does nothing.

## Set up

1. Add the plugin to a screen in Tessera. The screen builds once.
2. In Layout, place the **NAS power** tile from the library's Plugins group and choose its entity: something that is on
   while the device answers, such as a ping `binary_sensor`.
3. In the inspector, fill in:

   | Option | What |
   |---|---|
   | **Script or switch** | What starts the device and shuts it down. A script is run for both, so it must do the right one itself; a switch is turned on and off. |
   | **Power sensor** | Optional. The power the device draws. It shows on the tile, and tells starting from off before the device answers. |
   | **Off below** | Under this power the device counts as off. 5 W unless you change it. |

## Good to know

- With a script, "Shutting down" shows for as long as the script runs. A script that returns at once shows it only
  for that moment.
- Without a power sensor the tile knows "Starting" only from a tap on itself, for at most five minutes.
- While the script runs, a tap does nothing: a script may still wait to cut the power after the device went off.
- The question closes by itself when the device stops answering while it stands.
- The power shows in watts, whatever the sensor's unit (mW, W, kW or MW).
- The tile is 1x1 or 2x1.
- The NAS icon is the plugin's own, since Tessera's icon set has none: `tools/make_icon.py` draws the shape of "nas"
  of [Material Design Icons](https://pictogrammers.com/library/mdi/icon/nas/) (Apache License 2.0), the icon Home
  Assistant shows for `mdi:nas`.
- It uses nothing outside the home: the screen talks to Home Assistant only.
