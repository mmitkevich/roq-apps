#!/bin/bash
build=${build:-debug}
VERBOSE=${VERBOSE:-0}

cd $(dirname $0)

echo "$(pwd)/build.sh"
while [[ $@ > 0 ]]; do
case $1 in
--clean)
    echo "rm $(pwd)/build/$build"
    rm -rf $(pwd)/build/$build
    shift
esac
shift
done

if [ ! -d ~/roq-conda ]; then
echo "please install roq first"
fi

. ~/roq-conda/bin/activate

CXX=${CXX:-$(which g++)}
CC=${CC:-$(which gcc)}

rm -rf build/$build
echo "running cmake... CXX=$CXX"
cmake -H. -Bbuild/$build -DCMAKE_INSTALL_PREFIX=~/roq-conda
	-DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DCMAKE_BUILD_TYPE=$build\
	-DCMAKE_EXPORT_COMPILE_COMMANDS=YES\
	-DCMAKE_CXX_FLAGS_DEBUG="-O0 -g"\
    -DUMM_CONFIG_TOML=ON\
	-DUMM_USE_ABSL=ON\
		|| exit 1
echo "running make..."
cd build/$build && make install -s -j6 VERBOSE=$VERBOSE || exit 2
touch compile_commands.json
cat compile_commands.json > ../../../compile_commands.json && sed -i -e ':a;N;$!ba;s/\]\n\n\[/,/g' ../../../compile_commands.json
echo "built roq-mmaker"