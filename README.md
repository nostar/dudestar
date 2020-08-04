# DUDE-Star
Software to RX/TX D-STAR, DMR, Fusion YSF, NXDN, and P25 reflectors and repeaters/gateways over UDP

This software connects to D-STAR, Fusion, NXDN, and P25 reflectors and gateways/repeaters over UDP.  It is similar in functionality to BlueDV (except not as pretty), and is compatible with all of the AMBE3000 based USB devices out there (ThumbDV, DVstick 30, etc). If a compatible DV dongle is detected, TX is enabled.  If no DV dongle is detected, or is in use by another instance of DUDE-Star, then it become a software AMBE decoding application, formerly known as DUDE-Star RX. This software is open source and uses the cross platform C++ library called Qt.  It will build and run on Linux, Windows, and Mac OSX.

This software makes use of software from a number of other open source software projects, including:

MMDVM_CM: https://github.com/nostar/MMDVM_CM

XLXD: https://github.com/LX3JL/xlxd

DSDcc: https://github.com/f4exb/dsdcc

DSD: https://github.com/szechyjs/dsd

MBELIB: https://github.com/szechyjs/mbelib

Not only is software from these projects being used directly, but learning about the various network protocols and encoding/decoding of the various protocols was only possible thanks to the authors of all of these software projects.

# Optional FLite Text-to-speech build
I added Flite TTS TX capability to DUDE-Star so I didn't have to talk to myself all of the time during development and testing.  To build DUDE-Star with Flite TTS support, uncomment the line #define USE_FLITE from the top of dudestar.h. You will need the Flite library and development header files installed on your system.  When built with Flite support, 4 TTS check options and a Mic in option will be available at the bottom of the window.  TTS1-TTS4 are 4 voice choices, and Mic in turns off TTS and uses the microphone for input.  The text to be converted to speech and transmitted goes in the text box under the TTS checkboxes.

# Usage
On first launch, DUDE-Star will attempt to download the DMR ID list and the DPlus host file.  The remaining host files will be downloaded as each one is selected.

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

# Builds
There are no longer any builds available.  The ability to build this software on the platform you choose will assure that you are capable of testing this very early, very beta software.
There is also an Android build called DROID-Star at the Play Store as a beta release.

