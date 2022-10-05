#!/bin/bash

# install script for Linux

echo "|~P I M A X INSTALLER~|"

echo "The default VST3 directory is ~/.vst3  and /lib/vst3 if running as root"
echo "You may supply a different install directory as a second argument"
read -p "Would you like to install VST3 version? [Y/n] " arg1 arg2

if [[ $arg1 == "y" || $arg1 == "Y" || $arg1 == "" ]]; then
    if [[ $EUID == 0 ]]; then
        vst3_dir="/lib/vst3"
        if [[ $arg2 != "/lib/vst3" && $arg2 != "" ]]; then
            vst3_dir="$arg2"
        fi

        [ ! -d $vst3_dir ] && sudo mkdir $vst3_dir

        sudo cp -r PiMax.vst3 $vst3_dir/PiMax.vst3
        echo "Installed PiMax to $vst3_dir/PiMax.vst3"
    else
        vst3_dir="$HOME/.vst3"
        if [[ $arg2 != "$HOME/.vst3" && $arg2 != "" ]];
        then
            vst3_dir="$arg2"
            echo "setting vst3 dir to: $arg2"
        fi
        cp -r PiMax.vst3 $vst3_dir/PiMax.vst3
        # Need to check for errors here!
        echo "Installed PiMax to $vst3_dir/PiMax.vst3"
    fi

    echo "Installed PiMax VST3"
fi

echo "The default LV2 directory is ~/.lv2  and /lib/lv2 if running as root"
echo "You may supply a different install directory as a second argument"
read -p "Would you like to install LV2 version? [Y/n] " arg1 arg2

if [[ $arg1 == "y" || $arg1 == "Y" || $arg1 == "" ]]; then
    if [[ $EUID == 0 ]]; then
        lv2_dir="/lib/lv2"
        if [[ $arg2 != "/lib/lv2" && $arg2 != "" ]]; then
            lv2_dir="$arg2"
        fi
        sudo cp -r PiMax.lv2 $lv2_dir/PiMax.lv2
        echo "Installed PiMax to $lv2_dir/PiMax.lv2"
    else
        lv2_dir="$HOME/.lv2"
        if [[ $arg2 != "$HOME/.lv2" && $arg2 != "" ]]; then
            lv2_dir="$arg2"
            echo "setting lv2 dir to: $arg2"
        fi
        cp -r PiMax.lv2 $lv2_dir/PiMax.lv2
        echo "Installed PiMax to $lv2_dir/PiMax.lv2"
    fi

    echo "Installed PiMax LV2"
fi

echo "The default CLAP directory is ~/.clap  and /lib/clap if running as root"
echo "You may supply a different install directory as a second argument"
read -p "Would you like to install CLAP version? [Y/n] " arg1 arg2

if [[ $arg1 == "y" || $arg1 == "Y" || $arg1 == "" ]]; then
    if [[ $EUID == 0 ]]; then
        clap_dir="/lib/clap"
        if [[ $arg2 != "/lib/clap" && $arg2 != "" ]]; then
            clap_dir="$arg2"
        fi
        sudo cp -r PiMax.clap $clap_dir/PiMax.clap
        echo "Installed PiMax to $clap_dir/PiMax.clap"
    else
        clap_dir="$HOME/.clap"
        if [[ $arg2 != "$HOME/.clap" && $arg2 != "" ]]; then
            clap_dir="$arg2"
            echo "setting clap dir to: $arg2"
        else
            clap_dir="$HOME/.clap"
        fi

        cp -r PiMax.clap $clap_dir/PiMax.clap
        echo "Installed PiMax to $clap_dir/PiMax.clap"
    fi

    echo "Installed PiMax CLAP"
fi

echo "Finished installation. Enjoy using PiMax!"
