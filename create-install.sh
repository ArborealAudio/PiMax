#!/bin/sh

[ ! -d "installer/PiMax-linux" ] && mkdir -p installer/PiMax-linux

cp -r ./build/PiMax_artefacts/CLAP/PiMax.clap ./installer/PiMax-linux/PiMax.clap
cp -r ./build/PiMax_artefacts/LV2/PiMax.lv2 ./installer/PiMax-linux/PiMax.lv2
cp -r ./build/PiMax_artefacts/VST3/PiMax.vst3 ./installer/PiMax-linux/PiMax.vst3
cp -r ./build/PiMax_artefacts/VST/libPiMax.so ./installer/PiMax-linux/libPiMax.so
cp ./install.sh ./installer/PiMax-linux/install.sh

cd installer
tar -czvf PiMax-linux.tar.gz PiMax-linux