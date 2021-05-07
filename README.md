# DUDE-Star
Software to RX/TX D-STAR, DMR, Fusion YSF/FCS, NXDN, P25, M17, and IAX (AllStar client) over UDP

This software connects to D-STAR, Fusion (DN and VW modes are supported), NXDN, P25, M17 reflectors and AllStar nodes (as an IAX2 client) over UDP.  It is compatible with all of the AMBE3000 based USB devices out there (ThumbDV, DVstick 30, DVSI, etc). It includes software decoding and encoding support, using experimental open source IMBE/AMBE vocoder software.  This software is open source and uses the cross platform C++ library called Qt.  It will build and run on Linux, Windows, and Mac OSX.

This software makes use of software from a number of other open source software projects, including MMDVMHost, MMDVM_CM, XLXD, DSDcc, MBELIB, op25 (GNU Radio), mvoice, and others. Not only is software from these projects being used directly, but learning about the various network protocols and encoding/decoding of the various protocols was only possible thanks to the authors of all of these software projects.

# Optional FLite Text-to-speech build
I added Flite TTS TX capability to DUDE-Star so I didn't have to talk to myself all of the time during development and testing.  To build DUDE-Star with Flite TTS support, uncomment the line #define USE_FLITE from the top of dudestar.pro. You will need the Flite library and development header files installed on your system.  When built with Flite support, 4 TTS check options and a Mic in option will be available at the bottom of the window.  TTS1-TTS4 are 4 voice choices, and Mic in turns off TTS and uses the microphone for input.  The text to be converted to speech and transmitted goes in the text box under the TTS checkboxes.

# M17 support
M17 support is preliminary and is a work in progress, as is the M17 protocol itself. The Codec2 library included in DUDE-Star is a C++ implementation of the original C library taken from the mvoice project.  More info on M17 can be found here: https://m17project.org/

# Usage
Linux users with USB AMBE dongles will need to make sure they have permission to use the USB serial device, and disable the archaic ModeManager service that still exists on many Linux systems. On most systems this means adding your user to the 'dialout' group, and running 'sudo systemctl disable ModemManager.service' and rebooting.  This is a requirement for any serial device to be accessed.

On first launch, DUDE-Star will attempt to download the host files and ID files.  IAX has no host file, node info is manually entered in the settings tab under 'IAX Settings'.

Host/Mod: Select the desired host and module (for D-STAR and M17) from the selections.

Callsign:  Enter your amateur radio callsign.  A valid license is required to use this software.  

DMRID: A valid DMR ID is required to connect to DMR servers.
Latitude/Longitude/Location/Description:  These are DMR config options, sent to the DMR server during connect.   Some servers require specific values here, some do not.  This is specific to the server you are connecting to, so please dont ask what these values should be.

DMR+ IPSC2 hosts:  The format for the DMR+ options string is the complete string including 'Options='.  Create your options string and check 'Send DMR+ options on connect' before connecting.  A description of the DMR+ options string can be found here: https://github.com/g4klx/MMDVMHost/blob/master/DMRplus_startup_options.md .

Talkgroup:  For DMR, enter the talkgroup ID number.  A very active TG for testing functionality on Brandmeister is 91 (Brandmeister Worldwide).  You must TX with a talkgroup entered to link to that talkgroup, just like a real radio.  Any ststics you have defined in BM selfcare will work the same way they do if you were using a hotspot/radio.

MYCALL/URCALL/RPTR1/RPTR2 are for Dstar modes REF/DCS/XRF.  These fields need to be entered correctly before attempting to TX on any DSTAR reflector.  RPTR2 is automatically entered with a suggested value when connected, but can still be modified for advanced users.

# IAX Client for AllStar
Dudestar can connect to an AllStar node as an IAX(2) client.  See the AllStar wiki and other AllStar, Asterisk, and IAX2 protocal related websites for the technical details of IAX2 for AllStar.  This is a basic client and currently only uLaw audio codec is supported.  This is the default codec on most AllStar nodes.

Username: Defined in your nodes iax.conf file, usually iaxclient

Password: Defined as 'secret' in your iax.conf

Node[@Context]: ID and context of your AllStar node. The context is optional and if not specified, defaults to iax-client.

Host: hostname or IP address of node.

Port: UDP port of node, usually 4569.

Add DTMF commands like \*3node, \*1node, \*70, etc in the IAX DTMF box and hit send to send the DTMF string.  The asterisk (*) character is already added on the Droidstar app, so only input the numeric portion of the command (70 instead of *70, etc). Details on various commands can be found at the AllStar wiki and others.

# Compiling
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
My primary development platform is Fedora Linux.  With a proper build environment, the build instructions apply to all other platforms/distributions, including Windows and macOS.

# Builds
64 and 32 bit Windows builds are available from the releases section.  These are statically linked, stand-alone executables, no install required.  *NO support for Windows builds, either building or using.*

