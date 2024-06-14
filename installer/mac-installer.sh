#!/bin/bash

# codesign & set up files in the appropriate folder for packaging
# run from root of repo
# Depends on pre-existing .pkgproj to build installer

set -e

name=$PLUGIN_NAME
version=v${PLUGIN_VERSION}
clap=false
manual=true
compile_manual=false
presets=false

if [[ $# -eq 0 ]]; then
    echo
    "--clap | build a CLAP
    --no-manual | set no manual present at root dir
    --compile-manual | compile manual using Typst
    --presets | include factory presets folder"
    exit 0
fi

for arg in "$@"; do
    case $arg in
    --clap) clap=true
    shift
    ;;
    --no-manual) manual=false
    shift
    ;;
    --compile-manual) compile_manual=false
    shift
    ;;
    --presets) presets=true
    shift
    ;;
    esac
done

[ -d pkg ] && rm -r pkg
mkdir pkg

echo "Copying binaries from build dir..."

plugins=()

cp -r build/${name}_artefacts/Release/AU/ pkg/Components/
plugins+=( Components/${name}.component )
cp -r build/${name}_artefacts/Release/VST3/ pkg/VST3/
plugins+=( VST3/${name}.vst3 )
cp -r build/${name}_artefacts/Release/VST/ pkg/VST/
plugins+=( VST/${name}.vst )
if [[ $clap == true ]]; then
    echo "Including CLAP..."
    cp -r build/${name}_artefacts/Release/CLAP/ pkg/CLAP/
    plugins+=( CLAP/${name}.clap )
fi
if [[ $manual == true ]]; then
    [ $compile_manual == true ] && typst compile manual/${name}.typ
    cp manual/${name}.pdf pkg/${name}_manual.pdf
fi
if [[ $presets == true ]]; then
    echo "Including presets"
	[ -d "pkg/Factory" ] && rm -r pkg/Factory
	cp -r presets/ "pkg/Factory"
fi
cp LICENSE pkg
cp installer/${name}.pkgproj pkg

cd pkg

echo "Signing binaries..."

codesign -f -o runtime --timestamp -s "$APP_CERT" ${plugins[@]}

echo "Finishing installer package..."
sed -i "" "s/#PLUGIN_VERSION#/${version}/g" ${name}.pkgproj
packagesbuild --package-version $version ${name}.pkgproj 
cd build
[ ! -d ${name}-mac ] && mkdir ${name}-mac
productsign --timestamp --sign "$INSTALL_CERT" $name.pkg ${name}-mac/$name.pkg
cp ../${name}_manual.pdf ${name}-mac
cp ../LICENSE ${name}-mac
hdiutil create -volname ${name}-mac -srcfolder ${name}-mac -ov -format UDBZ ${name}-mac.dmg
codesign -s $APP_CERT --timestamp ${name}-mac.dmg
xcrun notarytool submit ${name}-mac.dmg --apple-id $APPLE_USER --password $NOTARY_PW --team-id $TEAM_ID --wait
xcrun stapler staple ${name}-mac.dmg

echo "Exited with code $?"

