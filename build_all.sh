#!/bin/bash
# Build script for rebuilding everything
set echo on
echo "Building everything..."


# pushd engine
# source build.sh
# popd
make -f makefile.engine.linux.mak all

ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

# pushd testbed
# source build.sh
# popd

make -f makefile.testbed.linux.mak all
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi
echo "All assemblies built successfully."