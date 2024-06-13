#!/bin/bash

# post-install script to set up files in the appropriate folder for packaging
# run in root of repo

set -e

name=$PLUGIN_NAME
version=v${PLUGIN_VERSION}
version_num=$PLUGIN_VERSION
clap=false
vst3_no_recurse=false
manual=true
presets=false

script_dir=$(dirname $0)

if [[ $# -eq 0 ]]; then
    echo "--clap | include CLAP [default = off]
    --vst3-no-bundle | to maintain backwards compatiblity with plugins like PiMax that didn't ship Windows VST3 as a bundle but a single file
    --no-manual | set no manual present at root dir [default = on]
    --presets | copy factory presets into installer [default = off]"
    exit 0
fi

for arg in "$@"; do
    case $arg in
    --clap) clap=true
    shift
    ;;
    --vst3-no-bundle) vst3_no_recurse=true
    shift
    ;;
    --no-manual) manual=false
    shift
    ;;
    --presets) presets=true
    shift
    ;;
    esac
done

[ -d pkg ] && rm -r pkg
mkdir pkg

echo "Copying binaries..."

cp build/${name}_artefacts/Release/VST/${name}.dll pkg
if [[ $vst3_no_recurse == false ]]; then ## workaround for legacy VST3 install format
    cp -r build/${name}_artefacts/Release/VST3/${name}.vst3 pkg
else
    cp -r build/${name}_artefacts/Release/VST3/${name}.vst3/Contents/x86_64-win/${name}.vst3 pkg
fi
if [[ $clap == true ]]; then
    echo "Including CLAP..."
    cp -r build/${name}_artefacts/Release/CLAP/${name}.clap pkg
fi

[ $manual == true ] cp manual/${name}.pdf pkg/${name}_manual.pdf
if [[ $presets == true ]]; then
    [ -d pkg/Factory] && rm -r pkg/Factory
    cp -r presets pkg/Factory
fi
cp LICENSE pkg/LICENSE

echo "Finishing installer package..."
cd pkg
cp $script_dir/win-extra/install_script.iss .

# remove CLAP lines if not packaging it
if [[ $clap == false ]]; then
    sed -i '/CLAP/d' install_script.iss
    sed -i '/Directory3/d' install_script.iss
fi

[[ $presets == false ]] && sed -i '/Presets/d' install_script.iss

cp $script_dir/win-extra/icon.ico .
iscc -Dname=$name -Dversion=$version_num install_script.iss

echo "Installer built in $PWD/Output"
echo "Exited with code $?"
