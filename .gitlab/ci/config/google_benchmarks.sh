#!/bin/bash

set -xe

readonly version="91ed7eea6856f8785139c58fbcc827e82579243c"
readonly tarball="benchmark-$version.tar.gz"
readonly url="https://github.com/google/benchmark/archive/$version.tar.gz"
readonly sha256sum="d5558cd419c8d46bdc958064cb97f963d1ea793866414c025906ec15033512ed"
readonly install_dir="$HOME/gbench"

if ! [[ "$VTKM_SETTINGS" =~ "benchmarks" ]]; then
  exit 0
fi

cd "$HOME"

echo "$sha256sum  $tarball" > gbenchs.sha256sum
curl --insecure -OL "$url"
sha256sum --check gbenchs.sha256sum
tar xf "$tarball"

mkdir build
mkdir "$install_dir"

cmake -GNinja -S benchmark* -B build -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON
cmake --build build
cmake --install build --prefix "$install_dir"
