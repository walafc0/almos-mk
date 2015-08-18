#! /bin/bash

#--------------------------------------------------------------------
# File      : tsar-sim.sh
# Author    : Ghassan Almaless
# Contact   : ghassan.almaless@lip6.fr
# Copyright : UPMC/LIP6
# Version   : 1.2
# Date      : 2010/11/07  (YYYY/MM/DD)
#--------------------------------------------------------------------
# This script is released under the GNU public license version 2
#--------------------------------------------------------------------
# Description : 
# - This script will generate TSAR hardware description, 
#   compile it to BIB binary format and than it will start 
#   TSAR simulator.
#
# - The following arguments can be passed to this script (order is not relevant)
#     -xmax: number of clusters in a row
#     -ymax: number of clusters in a column
#     -nproc: number of CPUs per Cluster
#     -xfb: frameBuffer's X-length
#     -yfb: frameBuffer's Y-length
#     -bscpu: BootStrap CPU (0 to xmax*ymax*nproc-1)
#     -memsz: per cluster memory size in bytes.
#     -o: output file name
#     -g: generate TSAR hardware description, dont call the simulator.
#   Default values are (in order) 2 2 4 XX 0x800000 "arch-info.bin"
#--------------------------------------------------------------------

if [ -z "$ALMOS_TOP" ]
then
    echo "Error: ALMOS_TOP environment variable is missing"
    exit 1
fi

xmax=2
ymax=2
ncpu=4
xfb=512
yfb=512
memsz=0xC00000
bscpu= # let gen-arch-info choose for us #
output="arch-info.bin"
noSim="false"

usage()
{
    echo "The following arguments can be passed to this script (order is not relevant)"
    echo "   -xmax: number of clusters in a row"
    echo "   -ymax: number of clusters in a column"
    echo "   -nproc: number of CPUs per Cluster"
    echo "   -xfb: frameBuffer's X-length"
    echo "   -yfb: frameBuffer's Y-length"
    echo "   -bscpu: BootStrap CPU (0 to xmax*ymax*nproc-1)"
    echo "   -memsz: per cluster memory size in bytes"
    echo "   -o: output file name"
    echo "   -g: generate TSAR hardware description, dont call the simulator"
    echo ""
    echo "Default values are (in order) $xmax $ymax $ncpu $xfb $yfb $bscpu $memsz $output"
}

while [ $# -gt 0 ]
do
    case "$1" in
	-xmax)  xmax=$2;      shift;;
	-ymax)  ymax=$2;      shift;;
	-nproc) ncpu=$2;      shift;;
	-xfb)   xfb=$2;       shift;;
	-yfb)   yfb=$2;       shift;;
	-o)     output="$2";  shift;;
	-bscpu) bscpu=$2;     shift;;
	-memsz) memsz=$2;     shift;;
	-g)     noSim="true";;
	-*) echo "$0: error - unrecognized option $1" 1>&2; usage 1>&2; exit 1;;
	*) echo "unexpected option/argument $1" 1>&2; usage 1>$2; exit 2;;
    esac
    shift
done

memsz=$(printf "%d" $memsz)
GEN_ARCH_INFO="$ALMOS_TOP/scripts/arch_info_gen.sh"
INFO2BIB="$ALMOS_TOP/tools/bin/info2bib"
SIM="$ALMOS_TOP/platform/bin/tsar-sim.x"
INFO_FILE="/tmp/tsar-${xmax}${ymax}${ncpu}_${xfb}x${yfb}.info"

error_arch_info()
{
    echo "Cannot generate platform description, command faild or $GEN_ARCH_INFO is not in your PATH" 
    exit 2
}

error_info2bib()
{
    echo "Cannot compile platform description to BIB binary format, command faild or $INFO2BIB is not in your PATH"
    exit 3
}

error_sim()
{
    echo "Cannot launch the simulator, command faild or $SIM is not in your PATH"
    exit 4
}

$GEN_ARCH_INFO $memsz $xmax $ymax $ncpu $bscpu > $INFO_FILE             || error_arch_info
$INFO2BIB -i $INFO_FILE -o $output                               || error_info2bib

if [ $noSim = "false" ]; then
    $SIM -XMAX $xmax -YMAX $ymax -NPROCS $ncpu -XFB $xfb -YFB $yfb -MEMSZ $memsz || error_sim
fi

#-------------------------------------------------------------------------------#
#                                End of script                                  # 
#-------------------------------------------------------------------------------#