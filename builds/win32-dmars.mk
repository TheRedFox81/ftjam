# Makefile to build jam with Digital Mars C/C++ on Win32 systems
#
# To use it, you must be in the top Jam source directory,
# have the compiler in your path, and call:
#
#  set JAM_TOOLSET=DIGITALMARS
#  make -f builds\win32-dmars.mk
#
# the program "jam.exe" will be created in a new directory
# named "bin.ntx86"
#
CC        = dmc
CFLAGS    = -DNT
TARGET    = -o jam0.exe
LINKLIBS  = kernel32.lib user32.lib advapi32.lib

all: jam0
	attrib -r jambase.c
	jam0

include common.mk
