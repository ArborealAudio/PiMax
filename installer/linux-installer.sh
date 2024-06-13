#!/bin/bash

# create installer archive for Linux

set -e

name=$PLUGIN_NAME
version=v${PLUGIN_VERSION}
clap=false
lv2=false
copy_manual=true
presets=false

script_dir=$(dirname $0)

if [[ $# -eq 0 ]]; then
    echo "--clap"$'\t'"CLAP needs to be packaged
    --no-manual"$'\t'"No manual in repo
    --compile-manual"$'\t'"Compile manual from typst file
    --presets"$'\t'"Include factory presets folder"
    exit 0
fi

for arg in "$@"; do
    case $arg in
    --clap) clap=true
    shift
    ;;
    --no-manual) copy_manual=false
    shift
    ;;
	--compile-manual) compile_manual=true
	shift
	;;
    --presets) presets=true
    shift
    ;;
    esac
done

dest=pkg/${name}-linux
[ -d $dest ] && rm -r $dest
mkdir -p $dest

echo "Copying binaries..."

cp -r build/${name}_artefacts/Release/VST3 ${dest}
cp -r build/${name}_artefacts/Release/VST ${dest}
[ $lv2 == true ] && cp -r build/${name}_artefacts/Release/LV2 ${dest}
if [[ $clap == true ]]; then
    echo "Including CLAP..."
    cp -r build/${name}_artefacts/Release/CLAP ${dest}
fi
if [[ $copy_manual == true ]]; then
	[ $compile_manual == true ] && typst compile manual/${name}.typ manual/${name}.pdf
    cp manual/${name}.pdf ${dest}/${name}_manual.pdf
fi
if [ $presets == true ]; then
    [ -d ${dest}/Factory ] && rm -r ${dest}/Factory
    cp -r presets/ ${dest}/Factory
fi

cp LICENSE ${dest}/LICENSE
cp $script_dir/linux-readme.md ${dest}/README.md

cd $dest
sed -i "s/#PLUGIN#/${name}/g" ${name}-linux/README.md
tar cJvf ${name}-linux.tar.xz ${name}-linux

echo "Exited with code $?"
