# alsa-midi-latency-test

alsa-midi-latency-test measures the roundtrip time of a MIDI message in the alsa subsystem of the Linux kernel using a high precision timer. It calculates the worst case roundtrip time of all sent MIDI messages and displays a histogram of the roundtrip time jitter:

![alsa-midi-latency-test](https://raw.github.com/koppi/alsa-midi-latency-test/master/alsa-midi-latency-test.gif "alsa midi latency test")

## Install from source code
```bash
git clone https://github.com/koppi/alsa-midi-latency-test.git
cd alsa-midi-latency-test/
```
The following packages are required to build alsa-midi-latency-test:
```bash
sudo apt -y install debhelper autotools-dev automake libasound2-dev
```
Compile alsa-midi-latency-test as follows:
```bash
sh autogen.sh
./configure
make
```
Install alsa-midi-latency-test as follows:
```bash
sudo make install
```
or build and install an Ubuntu / Debian package:
```bash
sudo apt -y install devscripts
debuild -uc -us
sudo dpkg -i ../alsa-midi-latency-test*.deb
sudo apt -f install # for linux-sound-base package
```

– Tested on Ubuntu 22.04 [![.github/workflows/ubuntu.yml](https://github.com/koppi/alsa-midi-latency-test/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/koppi/alsa-midi-latency-test/actions/workflows/ubuntu.yml)

## Run alsa-midi-latency-test
 * ``` $ alsa-midi-latency-test -l ```

   Lists available MIDI input and output ports.

 * ``` $ alsa-midi-latency-test -i [input port] -o [output port] ```

   This runs the benchmark with the given input and output port. Note, that the
   input and output ports have to be connected using a MIDI cable in the real
   hardware to loop the MIDI message back.

 * ``` $ man alsa-midi-latency-test ```

   The man page contains documentation for all available command line switches.

### Benchmark Results

   Please share your results in the [Wiki](../../wiki/).

### Definitions

 * [Wikipedia EN - Latency in engineering](http://tinyurl.com/wikipedia-latency-engineering)
 * [Wikipedia EN - Latency with audio](http://tinyurl.com/wikipedia-latency-audio)

## See also

 * http://lists.linuxaudio.org/listinfo/linux-audio-tuning

   The linux-audio tuning (LAT) mailing list is to help GNU/Linux distribution
   maintainers  and  other interested users to share information on system
   performance tuning matters, especially with regard to real-time Linux
   kernels.

 * http://www.alsa-project.org/

   The Advanced Linux Sound Architecture.

 * [http://www.evc-soft.nl/evc/products/miditest/](http://web.archive.org/web/20061128213256/http://www.evc-soft.nl/evc/products/miditest/)

   The MidiTest software for Microsoft Windows.

 * https://github.com/raboof/realtimeconfigquickscan

   Linux configuration checker for systems to be used for real-time audio.

 * https://codeberg.org/rtcqs/rtcqs

   rtcqs, pronounced arteeseeks, is a Python port of the realtimeconfigquickscan project.

## Thanks

 * to **Arnout Engelen** - [raboof](https://github.com/raboof) for initial testing and giving feedback.
 * to **Clemens Ladisch** - [cladisch](https://github.com/cladisch) for a number of fixes with the high precision timer and alsa midi event handling.
 * to **Giulio Moro** - [giuliomoro](https://github.com/giuliomoro) for code cleanup and various fixes and support for UART I/O.

## BUGS and AUTHORS

Please report bugs to the authors.

 * **Jakob Flierl** - [koppi](https://github.com/koppi)

-- November, 2009, last updated Jan 2023.
