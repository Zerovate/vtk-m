#!/bin/bash

set -xe

readonly version="refs/heads/main"
readonly tarball="$version.tar.gz"
readonly url="https://github.com/google/benchmark/archive/$tarball"
readonly sha256sum="dc458fac3c7a40612f085dac56671f3f6d7316083b09b9c9f9fca33e09f71772"
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
