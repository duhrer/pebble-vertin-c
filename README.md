# A Sample Pebble Watchface

This is a sample watch face for a Pebble, written in C.  There's also an earlier [Javascript version](https://github.com/duhrer/pebble-vertin-js).

## Requirements

You need a working development environment, and this is not trivial, as the SDK
is ten years old at time of writing.  I used the [development VM provided for
the March 2025 Hackathon](https://rebble.io/hackathon-002/vm/).  There are also
people working on [Nix environments](https://github.com/sorixelle/pebble.nix)
and [Docker containers](https://github.com/pebble-dev/rebble-docker) where you
can use the SDK.

Once you have a working environment, [the developer
tutorials](https://developer.rebble.io/developer.pebble.com/tutorials/js-watchface-tutorial/part1/index.html)
are your best resource for the basic commands needed to build and install this
watch face.

## Installing

Assuming a working environment, you should be able to run `pebble build` and
`pebble install` from the root of the repository.

![Screenshot from the emulator](./screenshot.png)

The C graphics libraries offer many more options for drawing a watch face in layers, but sadly don't offer drawing within a clipping path.  So, the numbers don't invert properly, but instead change colour when the second hand is more than halfway through them.
