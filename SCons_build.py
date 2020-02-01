# .. "Copyright (c) 2008 Robert B. Reese, Bryan A. Jones, J. W. Bruce ("AUTHORS")"
#    All rights reserved.
#    (R. Reese, reese_AT_ece.msstate.edu, Mississippi State University)
#    (B. A. Jones, bjones_AT_ece.msstate.edu, Mississippi State University)
#    (J. W. Bruce, jwbruce_AT_ece.msstate.edu, Mississippi State University)
#
#    Permission to use, copy, modify, and distribute this software and its
#    documentation for any purpose, without fee, and without written agreement is
#    hereby granted, provided that the above copyright notice, the following
#    two paragraphs and the authors appear in all copies of this software.
#
#    IN NO EVENT SHALL THE "AUTHORS" BE LIABLE TO ANY PARTY FOR
#    DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
#    OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE "AUTHORS"
#    HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#    THE "AUTHORS" SPECIFICALLY DISCLAIMS ANY WARRANTIES,
#    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
#    AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
#    ON AN "AS IS" BASIS, AND THE "AUTHORS" HAS NO OBLIGATION TO
#    PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
#
#    Please maintain this header in its entirety when copying/modifying
#    these files.
#
# *************************************************
# SCons_build.py - Build all libraries and examples
# *************************************************
# Given a build environment, this file performs specific builds
# using the PIC24 library. It should be called from an SCons script
# that exports:
#
# - env: the build environment to use
# - buildTargets: a list of strings specifying what to build
#   (see if statements toward the end of this file).
#
# This file delegates to :doc:`templates/SConscript.py` to build all
# templates.

import os
Import('buildTargets env bin2hex linker_side_effect')

## Inform SCons about the dependencies in the template-based files
SConscript('templates/SConscript.py', 'env')

# Common sources used for the PIC24 support library
# =================================================
PIC24SupportLibSources = [
  'lib/src/dataXfer.c',
  'lib/src/dataXferImpl.c',
  'lib/src/pic24_adc.c',
  'lib/src/pic24_clockfreq.c',
  'lib/src/pic24_configbits.c',
  'lib/src/pic24_ecan.c',
  'lib/src/pic24_flash.c',
  'lib/src/pic24_i2c.c',
  'lib/src/pic24_serial.c',
  'lib/src/pic24_spi.c',
  'lib/src/pic24_timer.c',
  'lib/src/pic24_uart.c',
  'lib/src/pic24_util.c' ]

# Functions used to build the library
# ===================================
# This function builds a program which includes the PIC24 library.
def buildProgramWithCommonSources(
  # A list of source files to be built into one program.
  sourceFileList,
  # A list of source files upon which all sources
  # in the sourceFileList depend. Wildcards are not
  # supported.
  commonSources,
  # An Environment in which to build these sources.
  buildEnvironment,
  # A string to serve as an alias for this build.  commonSources,
  aliasString):

  be = buildEnvironment
  p = be.Program(sourceFileList + commonSources)
  linker_side_effect(be, p)
  # Pick the name of the target to be the first c file in the list
  bin2hex(sourceFileList[0], be, aliasString)

# This function takes a list of source files (including wildcards),
# adds the PIC24 common sources to each item, then uses Program to build each item.
def buildWithCommonSources(
  # A list of source files (which may include
  # wildcards) to be built.
  sourceFileList,
  # A list of source files upon which all sources
  # in the sourceFileList depend. Wildcards are not
  # supported.
  commonSources,
  # An Environment in which to build these sources.
  buildEnvironment,
  # A exceptionFileName={key=value pairs used to create the
  # custom environment correpsonding to the
  # exceptionFile} dict
  exceptionDict,
  # A string to serve as an alias for this set of
  # builds.
  aliasString):

  for sourceFileGlob in sourceFileList:
    for sourceFile in Glob(sourceFileGlob, True, True, True):
        # See if this is an exception
        if sourceFile in exceptionDict:
          # Yes, so modify environment with dict of changes.
          be = buildEnvironment.Clone()
          flags = exceptionDict[sourceFile]
##          print flags
##          be.MergeFlags(flags) # produces weird
          # errors, so hand-code a simple alternative
          # TODO: This overwrites flags, which is a BAD thing
          for a in flags:
            be[a] = flags[a]
        else:
          # No, just use the existing environment
          be = buildEnvironment
        buildProgramWithCommonSources([sourceFile], commonSources, be,
          aliasString)


# Compile the support library into objects for the default
# environment.
PIC24SupportLibObjects = env.Object(PIC24SupportLibSources)

# Definition of targets
# =====================

if 'BUILD_DIR' in buildTargets:
  buildWithCommonSources(['BUILD_DIR/*.c'],
    PIC24SupportLibObjects, env,
    {'BUILD_DIR\\main.c'  : { 'CPPDEFINES': ['HARDWARE_PLATFORM=EMBEDDED_F14', 'UART1_RX_INTERRUPT', 'UART1_TX_INTERRUPT', 'BUILT_ON_ESOS']}},
    'BUILD_DIR')
