#!/bin/sh

# 该脚本至少需要一个参数，用于选择编译的目标文件所适配的单板
# 这里对脚本适配的单板进行了精简，只保留了 SCP_1GDDR 单板
if [ -z $1 ]
then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo "Please use correct make config."
    echo "For example make SCP_1GDDR for SCP 1G DDR CoreBoard linux,android OS"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    exit 1
fi

# 判断参数，仅支持 SCP_1GDDR 单板，另外增加了 clean 参数
if   [ "$1" = "SCP_1GDDR" ]
then
    # 设置第二目录，该目录下有 bl1，bl2 tzsw 文件，用于生成最终的 uboot使用
    sec_path="../CodeSign4SecureBoot_SCP/"
    CoreBoard_type="SCP"
elif [ "$1" = "clean" ]
then
    echo "Delete most generated files."
else
    echo "make config error,please use correct params......"
    exit 1
fi


# 检测编译主机 CPU 数量，加快编译速度
CPU_JOB_NUM=$(grep processor /proc/cpuinfo | awk '{field=$NF};END{print field+1}')

# 记录当前目录，后面使用
ROOT_DIR=$(pwd)

#clean
make distclean

# 删除特定单板的链接文件，该链接文件在 make itop_4412_android_config_scp_1GDDR 时
# 由 mkconfig 脚本创建，链接到了 SCP 单板特定的 汇编文件
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
            # 由 mkconfig 脚本处理，在 Makefile 中被调用
            # 主要是处理与特定处理器，单板相关的配置
            make itop_4412_android_config_scp_1GDDR
        fi

        # 进行编译，最大核心编译
        make -j$CPU_JOB_NUM

        # 检查是否生成 bl2 文件，未生成则表示编译失败
        if [ ! -f checksum_bl2_14k.bin ]
        then
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            echo "There are some error(s) while building uboot,"
            echo "please use command make to check."
            echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            exit 2
        fi

        # 拷贝 bl2 u-boot.bin 到第二目录，用于生成最终 u-boot 文件
        cp -rf checksum_bl2_14k.bin $sec_path
        cp -rf u-boot.bin $sec_path
        rm checksum_bl2_14k.bin

        cd $sec_path

        if  [ "$CoreBoard_type" = "SCP" ]
        then
            # 合并 bl1 bl2 u-boot tzse ，生成最终的 u-boot 镜像
            # 此处 u-boot 镜像组成，参考 其他文档
            cat E4412_N.bl1.SCP2G.bin bl2.bin all00_padding.bin u-boot.bin tzsw_SMDK4412_SCP_2GB.bin > u-boot-iTOP-4412.bin
        fi

        # u-boot 镜像移动到uboot目录，删除中间文件
        mv u-boot-iTOP-4412.bin $ROOT_DIR

        rm checksum_bl2_14k.bin
        #rm BL2.bin.signed.4412
        rm u-boot.bin

        echo
        echo
        ;;

    esac
