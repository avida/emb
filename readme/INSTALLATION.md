# Installation

1) Install CMake:

```sh
sudo apt update
sudo apt install -y cmake
```

2) Install Arduino CLI into your home bin directory:

```sh
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/local/bin sh
```

If needed, add `~/local/bin` to your `PATH`.

3) Install Arduino AVR core:

```sh
arduino-cli core update-index
arduino-cli core install arduino:avr
```

4) Install all required Arduino CLI libraries listed in `arduino-libs.txt`:

```sh
awk '!/^\s*#/ && NF { print }' arduino-libs.txt | xargs -r -n1 arduino-cli lib install
```

5) Install Raspberry Pi Rust cross-build prerequisites:

```sh
rustup target add arm-unknown-linux-musleabihf
```
