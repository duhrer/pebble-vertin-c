# "Vertin", a Peble Watch Face

![Screenshot from the basalt emulator](./screenshots/basalt.png)
![Screenshot from the chalk emulator](./screenshots/chalk.png)
![Screenshot from the diorite emulator](./screenshots/diorite.png)

I wrote this watch face as a learning project. I based the look on [a watch face for Google Wear by ustwo](https://ustwo.com/work/android/wear/). If you want to read about the background and technical challenges, check out [my blog]().


## Requirements

You need a working development environment, and this is not trivial, as the SDK
is ten years old at time of writing.  Initially, I used the [development VM
provided for the March 2025 Hackathon](https://rebble.io/hackathon-002/vm/) to
build and test this project. Currently, I use the VM for testing in emulators,
but use [`rebbletool`](https://github.com/richinfante/rebbletool), an updated
version of the original Pebble command line tools.

There are also people working on [Nix
environments](https://github.com/sorixelle/pebble.nix) and [Docker
containers](https://github.com/pebble-dev/rebble-docker) where you can use the
Pebble SDK.

## Installing

This package uses its `package.json` file to manage its dependencies, so you'll
need to run `npm install` before you can build the package.

Once you've got a working development and installed dependencies, you can build
the app from the root of the repository using a command like:

`pebble build`

After the app is built, you can test it in the emulator using a command like:

`pebble install --emulator basalt`.

You can also install it on your watch.  If you have the pebble app installed on
your phone, enable developer mode, and make a note of the IP adddress.  You can
then install the app using a command like:

`pebble install --phone <YOUR_IP_ADDRESS>`

## Credits

The digits displayed on screen use the the free [Deja Vu Sans
font](https://dejavu-fonts.github.io/).