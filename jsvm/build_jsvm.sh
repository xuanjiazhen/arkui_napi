#
# Create for build libjsvm.so
#
# Author: DingLei
#
# Date: 2023/12/14
#

SCRIPT_PATCH=$(dirname $(readlink -f "$0"))

options="$(getopt -o h "help,sysroot:,prefix:,target_cpu:,base_path:" -- "$@")" || usage

eval set -- "$options"

usage() {
    echo "Tool $(basename "$0") Usage"
    echo "Options:"
    echo "-h|--help"
    echo "--sysroot <path>            Sysroot path."
    echo "--prefix <perfix>           Cross-compiler prefix."
    echo "--target_cpu <arm/arm64>    Cross-compile CPU types."
    echo "--target_gen_dir <output path>"
    echo "          if target_gen_dir not set, will install only to the default path."
    exit 0
}

die() {
    echo $@
    exit 0
}

do_man_process() {
    do_opt_process $@
    do_env
    do_fetch 2>&1 | tee ${out_dir}/log.do_fetch
    do_patch 2>&1 | tee ${out_dir}/log.do_patch
    do_configure 2>&1 | tee ${out_dir}/log.do_configure
    do_compile 2>&1 | tee ${out_dir}/log.do_compile
    do_install 2>&1 | tee ${out_dir}/log.do_install
    popd
}

do_opt_process() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
        -h|--help)
            usage
            ;;
        --sysroot)
            export sysroot=$2
            shift
            ;;
        --prefix)
            export _prefix=$2
            shift
            ;;
        --target_cpu)
            export target_cpu=$2
            shift
            ;;
        --target_gen_dir)
            export target_gen_dir=$2
            shift
            ;;
        *)
            ;;
        esac
        shift
    done
}

do_fetch() {
    echo "skip."
}

do_patch() {
    echo "skip."
}

do_configure() {
    if [[ ${target_cpu} = "x64" ]]; then
        ./configure --shared; return
    fi
    ./configure \
        --prefix=${workdir} \
        --dest-cpu=${target_cpu} --dest-os=linux \
        --cross-compiling \
        --shared \
        --with-arm-float-abi=hard \
        --without-corepack \
        --without-npm \
        --without-ssl \
        --without-intl
}

do_compile() {
    cpu_num=$(cat /proc/cpuinfo |grep "processor"|wc -l)
    make -j${cpu_num} | tee ${out_dir}
}

do_install () {
    # default install path
    cp -u ${workdir}/out/Release/libjsvm.so ${out_dir}/lib/libjsvm.so
    # target install path
    if [ "x${target_gen_dir}" != "x" ]; then
        cp -u ${out_dir}/lib/* ${target_gen_dir}
    fi
}

do_env() {
    # init workspace
    out_dir=${SCRIPT_PATCH}/out
    workdir=${SCRIPT_PATCH}/sources
    mkdir -p ${out_dir}
    mkdir -p ${out_dir}/include
    mkdir -p ${out_dir}/lib
    mkdir -p ${workdir}
    pushd ${workdir}

    if [[ ${target_cpu} = "arm" ]]; then
        target_name="arm-linux-ohos"
        target_arch="arm"
        target_march="armv7-a"
        host_args="-m32"
    elif [[ ${target_cpu} = "arm64" ]]; then
        target_name="aarch64-linux-ohos"
        target_arch="aarch64"
        target_march="armv8-a"
        host_args="-m64"
    elif [[ ${target_cpu} = "x64" ]]; then
        return # do nothing
    else
        die "Error: target_cpu must be arm or arm64 or x64."
    fi

    cflags_host=${host_args}
    cflags="  --target=${target_name}"
    cflags+=" --sysroot=${sysroot}"
    cflags+=" -mfpu=neon"
    cflags+=" -march=${target_march}"

    # linux host env
    HOST_OS="linux"
    HOST_ARCH="x86_64"
    export LINK_host="${_prefix}/clang++ ${cflags_host}"
    export CXX_host="${_prefix}/clang++ ${cflags_host}"
    export CC_host="${_prefix}/clang ${cflags_host}"
    export AR_host=${_prefix}/llvm-ar

    # target env
    ARCH=${target_arch}
    export CC="${_prefix}/clang ${cflags}"
    export CXX="${_prefix}/clang++ ${cflags}"
    export LD="${_prefix}/ld.lld"
    export AS="${_prefix}/llvm-as"
    export AR="${_prefix}/llvm-ar"
    export STRIP="${_prefix}/llvm-strip"
    export OBJCOPY="${_prefix}/llvm-objcopy"
    export OBJDUMP="${_prefix}/llvm-obidump"
    export RANLIB="${_prefix}/llvm-ranlib"
    export NM="${_prefix}/llvm-nm"
    export STRINGS="${_prefix}/llvm-strings"
    export READELF="${_prefix}/llvm-readelf"
    env | tee ${out_dir}/log.do_env
}

do_man_process $@
