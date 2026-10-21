# s60-telegram

A native Telegram client for Nokia S60 3rd Edition FP2 phones, written in Symbian C++ with its own MTProto implementation. It's built and tested on a Nokia E72 (320×240, QWERTY).

## Features

- **Login:** QR code login (scan it with Telegram on another device), plus two-step verification passwords (SRP). Phone-number login is there too, but codes may not arrive for new API ids.
- **Chat list:**
  - Your Telegram folders as tabs, plus All chats and Archive.
  - Unread badges, mute and pin icons, and read ticks.
  - Round avatars, and previews of the last message's photo.
- **Messages:**
  - Read, reply, edit, delete (for you or for everyone), forward, copy, react (up to 5 reactions shown per message), and see the pinned message.
  - Load older history, and view albums as one entry.
- **Media:**
  - Photo viewer with zoom, pan and save.
  - Files open in the phone's player, or download with one key.
  - Send photos (several at once as an album), files and camera shots.
- **Links:** open in the phone's browser.
- **Notifications:** checks for new messages every 10 s while open and every 30 s in the background. Shows a pop-up and vibrates when you're away, or the keypad is locked.
- **Keyboard shortcuts:** QWERTY shortcuts for most actions, listed in Options → Shortcuts.
- **Languages:** English, Russian and Ukrainian, following the phone's language by default.

## Building

The app builds inside the [SymbianApps](https://github.com/RuhanSA079/SymbianApps) toolchain: a Docker image with GCC 14, GnuPoc and the Nokia SDKs.

1. Set up the toolchain and the S60 3rd Edition SDK by following that repository's README. Then build its shared libraries:
   ```sh
   env/fetch-s60v31.sh
   ./sym sh env/install-sdk-s60v31.sh
   SYM_SDK=s60v31 ./sym sh env/build.sh apps/common
   ```
2. Clone this repository into `apps/tgnative`. The folder name matters.
   ```sh
   git clone https://github.com/denisilin-developer/s60-telegram apps/tgnative
   ```
3. Register an app at [my.telegram.org](https://my.telegram.org) and put your `api_id` and `api_hash` into a config file:
   ```sh
   cp apps/tgnative/src/tg_config.h.example apps/tgnative/src/tg_config.h
   ```
4. Build the package:
   ```sh
   SYM_SDK=s60v31 ./sym sh env/build.sh apps/tgnative    # -> out/tgnative_s60v31.sisx
   ```

The package is self-signed and includes Nokia's Open C runtime (P.I.P.S.).

## Tests

`sh tests/run-tests.sh selftest` builds the protocol core for macOS and checks the TL parser, crypto, SRP, the QR encoder and the text helpers. It doesn't need a phone or a Telegram account.

## License

MIT. The reaction emoji are from [Noto Emoji](https://github.com/googlefonts/noto-emoji) (Apache 2.0, see `data/emoji/LICENSE`).
