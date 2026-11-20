# UniFi hotspot

> Needs the UniFi Hotspot Manager integration (ufozone/ha-unifi-voucher) in Home Assistant, with its QR code entity
> enabled (it is disabled by default).

Your guest Wi-Fi on one tile: the QR code to join the network, the network's name, the current voucher code in large
digits, how long it is valid, and two keys that refresh the voucher or create a new one.

## Set up

1. In Home Assistant, install UniFi Hotspot Manager and set the name of your guest network in its options, so it makes
   a QR code. Enable the entity `image.<id>_qr_code` (Settings, Devices, the integration's device, the hidden entities).
2. Add the plugin to a screen in Tessera. The screen builds once.
3. In Layout, place the **Guest Wi-Fi** tile from the library's Plugins group, at any size from 1x1 to 6x6.
4. In the inspector, choose the QR code entity (`image.<id>_qr_code`) as the tile's entity.
5. Choose the **Voucher** (`sensor.<id>_voucher`), and for the keys the **Refresh key** (`button.<id>_update`) and the
   **Create key** (`button.<id>_create`). Leave a key empty to leave it out.
6. The screen must be allowed to perform actions (the screen's settings in Tessera), for the keys.

## Good to know

- The tile takes any size from 1x1 to 6x6. On a wide tile the QR code stands on the left, on a tall one above the
  code; a 1x1 tile shows the QR code alone. What does not fit goes: the validity first, then the network's name, then
  the keys. Keys too narrow for their words show an icon. The option **QR code only** shows the QR code alone at any
  size.
- Boards that show no pictures (the CYD, the Waveshare 3.5-inch, the Hosyond 4-inch) show the code and the keys only.
- A key says **Sent** for a few seconds after a press, until the new code is there.
- The plugin talks to nothing but Home Assistant: no internet service.

## A hobby project

This plugin is a private, non-commercial hobby project. I build and maintain it in my spare time and share it
free of charge as open source under the [MIT License](LICENSE). It is free and will stay free: there is no paid
version, no paid feature and no paid support.

Bug reports and ideas are welcome, but please keep in mind:

- There is no support and no guaranteed response time. I answer when I find the time, which can take days or weeks.
- Not every request gets built. I add what fits the plugin and what I can keep working over time.
- Pull requests are welcome; for a larger change, open an issue first so we can agree on the way.

### No warranty, no liability

The plugin is provided "as is", without any warranty and without guaranteed support, updates or fixes. You use it at
your own risk. To the extent permitted by law, I accept no liability for any damage or data loss resulting from its
use. The full terms are in the [MIT License](LICENSE).

### Buy me a coffee

If the plugin is useful to you and you would like to say thanks, you are welcome to buy me a coffee. A donation is
entirely voluntary: it is a thank-you, not a payment. It does not buy support, features, a response time or any other
service, and the plugin stays exactly the same for everyone, whether they donate or not.

[![Buy me a coffee](https://img.buymeacoffee.com/button-api/?text=Buy%20me%20a%20coffee&emoji=&slug=hflocki&button_colour=FFDD00&font_colour=000000&font_family=Cookie&outline_colour=000000&coffee_colour=ffffff)](https://www.buymeacoffee.com/hflocki)

☕ [buymeacoffee.com/hflocki](https://www.buymeacoffee.com/hflocki)
