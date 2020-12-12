# DUDE-Star
Software to RX/TX D-STAR, DMR, Fusion YSF/FCS, NXDN, P25, and M17 reflectors and repeaters/gateways over UDP

This software connects to D-STAR, Fusion, NXDN, P25, and M17 reflectors and gateways/repeaters over UDP.  It is compatible with all of the AMBE3000 based USB devices out there (ThumbDV, DVstick 30, DVSI, etc). It includes software decoding and encoding support, using experimental open source IMBE/AMBE vocoder software.  This software is open source and uses the cross platform C++ library called Qt.  It will build and run on Linux, Windows, and Mac OSX.

This software makes use of software from a number of other open source software projects, including MMDVMHost, MMDVM_CM, XLXD, DSDcc, MBELIB, op25 (GNU Radio), mvoice, and others. Not only is software from these projects being used directly, but learning about the various network protocols and encoding/decoding of the various protocols was only possible thanks to the authors of all of these software projects.

# Optional FLite Text-to-speech build
I added Flite TTS TX capability to DUDE-Star so I didn't have to talk to myself all of the time during development and testing.  To build DUDE-Star with Flite TTS support, uncomment the line #define USE_FLITE from the top of dudestar.h. You will need the Flite library and development header files installed on your system.  When built with Flite support, 4 TTS check options and a Mic in option will be available at the bottom of the window.  TTS1-TTS4 are 4 voice choices, and Mic in turns off TTS and uses the microphone for input.  The text to be converted to speech and transmitted goes in the text box under the TTS checkboxes.

# M17 support
M17 support is preliminary and is a work in progress, as is the M17 protocol itself. The Codec2 library included in DUDE-Star is a C++ implementation of the original C library taken from the mvoice project.  More info on M17 can be found here: https://m17project.org/

# Usage
On first launch, DUDE-Star will attempt to download the DMR ID list and host files for all modes.

Host/Mod: Select the desired host and module (for D-STAR) from the selections.

Callsign:  Enter your amateur radio callsign.  A valid license is required to use this software.  A valid DMR ID is required to connect to DMR servers.

Talkgroup:  For DMR, enter the talkgroup ID number.  A very active TG for testing functionality on Brandmeister is 91 (Brandmeister Worldwide)

MYCALL/URCALL/RPTR1/RPTR2 are always visible, but are only relevent to Dstar modes REF/DCS/XRF.  These fields need to be entered correctly before attempting to TX on any DSTAR reflector.  RPTR2 is automatically entered with a suggested value when connected, but can still be modified for advanced users.

# Compiling on Linux
This software is written in C++ on Linux and requires mbelib and QT5, and natually the devel packages to build.  With these requirements met, run the following:
```
qmake
make
```
qmake may have a different name on your distribution i.e. on Fedora it's called qmake-qt5

Notes for building/running Debian/Raspbian:  In addition to the Linux build requirements, there are some additional requirements for running this QT application in order for the audio devices to be correctly detected:
```
sudo apt-get install libqt5multimedia5-plugins libqt5serialport5-dev qtmultimedia5-dev libqt5multimediawidgets5 libqt5multimedia5-plugins libqt5multimedia5
```
And if pulseaudio is not currently installed:
```
sudo apt-get install pulseaudio
```
And finally mbelib from https://github.com/szechyjs/mbelib

# Builds
A 32 bit Windows executable is available from the releases section.  This is a statically linked, stand-alone executable. It does not need to be installed.  Just run it and go.

