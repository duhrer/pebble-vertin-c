# A Sample Pebble Watchface

![Screenshot from the emulator](./screenshot.png)

This is a sample watch face for a Pebble, written in C.  There's also an earlier
[Javascript version](https://github.com/duhrer/pebble-vertin-js).

You can read about the rationale for this and the technical challenges [on my
blog](https://duhrer.github.io/2025-02-28-pebble-watch-face-configurable-colours/).

## Requirements

You need a working development environment, and this is not trivial, as the SDK
is ten years old at time of writing.  I used the [development VM provided for
the March 2025 Hackathon](https://rebble.io/hackathon-002/vm/).  There are also
people working on [Nix environments](https://github.com/sorixelle/pebble.nix)
and [Docker containers](https://github.com/pebble-dev/rebble-docker) where you
can use the Pebble SDK.

This package specifically uses its `package.json` file to manage its
dependencies, so you'll need to run `npm install` before you can build the
package.

## Installing

Once you've got a working development and installed dependencies, you can build
the app from the root of the repository using a command like:

`pebble build`

After the app is built, you can test it in the emulator using a command like:

`pebble install --emulator basalt`.

You can also install it on your watch.  If you have the pebble app installed on
your phone, enable developer mode, and make a note of the IP adddress.  You can
then install the app using a command like:

`pebble install --phone <YOUR_IP_ADDRESS>`

For more information on the pebble tools refer to [the developer
tutorials](https://developer.rebble.io/developer.pebble.com/tutorials/js-watchface-tutorial/part1/index.html)
are your best resource.
