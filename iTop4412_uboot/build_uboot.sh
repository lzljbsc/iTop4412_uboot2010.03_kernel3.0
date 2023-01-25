#!/bin/sh

if [ -z $1 ]
then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "Please use correct make config."
    echo "For example make SCP_1GDDR for SCP 1G DDR CoreBoard linux,android OS"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit 1
fi

if   [ "$1" = "SCP_1GDDR" ]
then
    sec_path="../CodeSign4SecureBoot_SCP/"
    CoreBoard_type="SCP"
elif [ "$1" = "clean" ]
then
    echo "Delete most generated files."
else
    echo "make config error,please use correct params......"
    exit 1
fi


CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+1}')
ROOT_DIR=$(pwd)

#clean
make distclean

#rm link file
rm ${ROOT_DIR}/board/samsung/smdkc210/lowlevel_init.S
rm ${ROOT_DIR}/cpu/arm_cortexa9/s5pc210/cpu_init.S

case "$1" in
    clean)
        echo make clean
        make mrproper
        ;;
    *)
        if [ ! -d $sec_path ]
        then
            echo "**********************************************"
            echo "[ERR]please get the CodeSign4SecureBoot first"
            echo "**********************************************"
            return
        fi

        if [ "$1" = "SCP_1GDDR" ]
        then
            make itop_4412_android_config_scp_1GDDR
        fi

        make -j$CPU_JOB_NUM

        if [ ! -f checksum_bl2_14k.bin ]
        then
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "There are some error(s) while building uboot,"
            echo "please use command make to check."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            exit 2
        fi

        cp -rf checksum_bl2_14k.bin $sec_path
        cp -rf u-boot.bin $sec_path
        rm checksum_bl2_14k.bin

        cd $sec_path

        if  [ "$CoreBoard_type" = "SCP" ]
        then
            cat E4412_N.bl1.SCP2G.bin bl2.bin all00_padding.bin u-boot.bin tzsw_SMDK4412_SCP_2GB.bin > u-boot-iTOP-4412.bin
        fi

        mv u-boot-iTOP-4412.bin $ROOT_DIR

        rm checksum_bl2_14k.bin
        #rm BL2.bin.signed.4412
        rm u-boot.bin

        echo
        echo
        ;;

    esac
