# alsa-midi-latency-test [![travis](https://travis-ci.org/koppi/alsa-midi-latency-test.png?branch=master)](https://travis-ci.org/koppi/alsa-midi-latency-test)

alsa-midi-latency-test measures the roundtrip time of a MIDI message in the alsa subsystem of the linux kernel using a high precision timer. It calculates the worst case roundtrip time of all sent MIDI messages and displays a histogram of the rountrip time jitter:

![alsa-midi-latency-test](https://raw.github.com/koppi/alsa-midi-latency-test/master/alsa-midi-latency-test.gif "alsa midi latency test")

## Install from source code
```bash
git clone https://github.com/koppi/alsa-midi-latency-test.git
cd alsa-midi-latency-test/
```
The following packages are required to build alsa-midi-latency-test:
```bash
sudo apt-get install debhelper autotools-dev automake libasound2-dev
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
debuild
sudo dpkg -i ../alsa-midi-latency-test*.deb
```

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

Benchmark results for various MIDI adapters can be found in [benchmarks/](benchmarks). The setup of the MIDI loop back chains is as follows:

 * [PC - USB - Elektron TM-1 OUT - MIDI loop - Elektron TM-1 IN - USB - PC](benchmarks/elektron-tm1.txt)
 * [PC - USB - Polytec GM5 midibox OUT - MIDI loop - Polytec GM5 midibox IN - USB - PC](benchmarks/gm5x5x5.txt)
 * [PC - Edirol UA-25EX - PC](benchmarks/um2ex.txt)
 * [PC - M-Audio 2496 - PC](benchmarks/m-audio-2496.txt)
 * [PC - USB - MidiSport 2x2 'Anniversary Edition' port B OUT - MidiSport 2x2 'Anniversary Edition' port B IN - USB - PC](benchmarks/midisport2x2ann.txt)
 * [PC - MPU-401-Port on motherboard - PC](benchmarks/mpu401.txt)
 * [PC - Edirol UM-2EX - PC](benchmarks/um2ex.txt)
 * [PC - Yamaha UX256 (6-Port) - PC](benchmarks/yamaha-ux256.txt)

### User Experiences - Archive

#### 2015

 * More to be written. Please share your results in the [Wiki](../../wiki/).

#### 2014

 * [morsanuto - MIDI Through virtual device loopback test](https://github.com/raboof/alsa-midi-latency-test/wiki/MIDI-Through-virtual-device-loopback-test)
 * [morsanuto - Precision Measuring Instruments : reference MIDI hardware](https://github.com/raboof/alsa-midi-latency-test/wiki/Precision-Measuring-Instruments-:-reference-MIDI-hardware)
 * [morsanuto - Wireless MIDI performance : CME WIDI X8](https://github.com/raboof/alsa-midi-latency-test/wiki/Wireless-MIDI-performance-:-CME-WIDI-X8)

#### 2013

 * [Robin Gareus - Utility to measure jackaudio MIDI latency](https://github.com/x42/jack_midi_latency)
   see http://rg42.org/wiki/midilatency for more details
 * [ Andre Majorel - LAU: ESI Romio II and alsa-midi-latency-test failure](http://linuxaudio.org/mailarchive/lau/2014/1/26/203927)

#### 2012

 * [Silicon Stuff - Serial Port MIDI on the Raspberry Pi](http://www.siliconstuff.com/2012/08/serial-port-midi-on-raspberry-pi.html)
 * [Clemens Ladisch - comp.audio.jackit: MIDI support for OpenBSD?](http://en.it-usenet.org/thread/11091/12748/)
 
   (The primary purpose is not so much measuring the hardware but the kernel's maximum scheduling delay. The UM-2EX results were done on a very optimized kernel.)

#### 2011

 * [Silicon Stuff - Raspberry Pi Synthesizer](http://www.raspberrypi.org/phpBB3/viewtopic.php?f=38&t=15422&start=25)
 * [WARPWOOD Wiki](http://www.warpwood.com/wiki/linux-audio/#index9h2)
 * [Mentoj Dija - cheap usb-audio-interface]()

#### 2010

 * [drummerforum.de](http://www.drummerforum.de/forum/)
     * [siktuned - Asio 4 all mit addictive drums problem mit latenz](http://www.drummerforum.de/forum/48500-asio-4-all-mit-addictive-drums-problem-mit-latenz.html#post787463)
 * [ffado.org](http://subversion.ffado.org/wiki/)
     * [IRQ Priorities How-To](http://subversion.ffado.org/wiki/IrqPriorities)
 * [64studio-users](http://www.mail-archive.com/64studio-users@lists.64studio.com)
     * [Ralf Mardorf - PCI MIDI jitter - comparison Ubuntu (bad) and Suse	(might be ok)](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02099.html)
     * [Ralf Mardorf - 3.0b and 3.3a amd64 MIDI latency test for several	kernels](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02103.html)
     * [Ralf Mardorf - USB Device 0x170b:0x11, TerraTec EWX24/96](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02047.html)
     * [Ralf Mardorf - Correlation of alsa -p value and hw MIDI jitter](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02109.html)
     * [Ralf Mardorf - MIDI jitter - Today for 4 of 4 tests my USB device did pass all tests](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02089.html)
     * [Ralf Mardorf - ALSA MIDI latency test results are far away from	reality](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg02104.html)
 * [ubuntu-studio-users](https://lists.ubuntu.com/archives/ubuntu-studio-users/)
     * [Ralf Mardorf - Real time for Ubuntu Studio 10.04 amd64](https://lists.ubuntu.com/archives/ubuntu-studio-users/2010-July/006392.html)
 * Blog Posts
     * [Matt Wheeler's Blog - Putting -rt kernels first in grub2](http://funkyhat.org/2010/01/19/putting-rt-kernels-first-in-grub2/)

#### 2009

 * [rt.wiki.kernel.org](https://rt.wiki.kernel.org/)
     * [Geunsik Lim - Worstcase Latency Test Scenario](https://rt.wiki.kernel.org/articles/w/o/r/Worstcase_Latency_Test_Scenario_72eb.html)
     * [various - Systems based on Real time preempt Linux](https://rt.wiki.kernel.org/articles/s/y/s/Systems_based_on_Real_time_preempt_Linux_29a7.html)
 * [64studio-users](http://www.mail-archive.com/64studio-users@lists.64studio.com)
     * [geoff - MIDI timing](http://www.mail-archive.com/64studio-users@lists.64studio.com/msg01635.html)

#### 2000

 * [Benno Senoner - Linux v2.4 - scheduling latency tests / high performance low latency audio](http://www.gardena.net/benno/linux/audio/)

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

## Thanks

 * to Arnout Engelen for initial testing and giving feedback.
 * to Clemens Ladisch for a number of fixes with the high precision timer and
   alsa midi event handling.

## BUGS and AUTHORS

Please report bugs to the authors.

 * Jakob Flierl <jakob.flierl@gmail.com>

-- November, 2009, last updated July 2015.
