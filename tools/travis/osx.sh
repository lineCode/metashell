#!/bin/bash

set -ex

brew update >/dev/null

# Oclint installs something under /usr/local/include/c++ which
# conflicts with files installed by gcc
brew cask uninstall oclint

brew install p7zip

if [ "$CXX" = "g++" ]; then
  brew install gcc@6
  export CXX="g++-6"
fi

# Test that the download version links are correct

git fetch --tags
git tag
egrep $(tools/latest_release --no_dots --prefix=version-) README.md
egrep $(tools/latest_release --no_dots --prefix=version-) docs/index.md

# Get the templight binary

cd 3rd/templight
  ARCHIVE_NAME=templight_osx10.13.2_x86_64.tar.bz2
  wget https://github.com/metashell/templight_binary/releases/download/templight_d2be28/${ARCHIVE_NAME}
  tar -xvjf ${ARCHIVE_NAME}
cd ../..

# Test the code

BUILD_THREADS=2 NO_TEMPLIGHT=1 METASHELL_NO_DOC_GENERATION=1 ./build.sh
