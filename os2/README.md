PDCurses for OS/2
=================

This directory contains PDCurses source code files specific to OS/2.


Building
--------

- Choose the appropriate makefile for your compiler:

        Makefile.bcc - Borland C++ 2.0
        Makefile     - EMX 0.9b+
        iccos2.mak   - C Set/2
        Makefile.wcc - Open Watcom 1.8+

- Optionally, you can build in a different directory than the platform
  directory by setting PDCURSES_SRCDIR to point to the directory where
  you unpacked PDCurses, and changing to your target directory:

        set PDCURSES_SRCDIR=c:\pdcurses

- Build it:

        make -f makefilename

  (For Watcom, use "wmake" instead of "make"; for MSVC or C Set/2,
  "nmake".) You'll get the libraries (pdcurses.lib or .a, depending on
  your compiler; and panel.lib or .a), the demos (*.exe), and a lot of
  object files. Note that the panel library is just a copy of the main
  library, provided for convenience; both panel and curses functions are
  in the main library.

  You can also use the optional parameter "DLL=Y" with EMX, to build the
  library as a DLL:

        make -f Makefile DLL=Y


Distribution Status
-------------------

The files in this directory are released to the Public Domain.
