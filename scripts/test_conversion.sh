#!/usr/bin/bash

#### TEST CONVERSION ON SINGLE AO2D ####

show_help() {
  cat << EOF
Usage: test-conversion [OPTIONS]

Options:
  -c, --compile   Recompile converter
  -h, --help      Show this help message

EOF
}

PARSEDARGS=$(getopt -o ch \
          --long compile,help \
          -n 'test-conversion' -- "$@")
PARSE_EXIT=$?
if [ $PARSE_EXIT -ne 0 ] ; then exit $PARSE_EXIT ; fi

# NOTE: quotes around PARSEDARGS essential to parse spaces between parsed options
eval set -- "$PARSEDARGS"

compile=

while true; do
  case "$1" in
    -c | --convert )  compile="true"; shift ;;
    -h | --help )     show_help; exit 0 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

project_root="$(dirname -- "$(realpath "$0")")/../"
project_root="$( realpath "$project_root" )"
output=/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/mhwang/convert_test
config_file=$project_root/tree-cuts.yaml
filelist=/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/mhwang/LHC22o_test/BerkeleyTrees/AOD_list.txt
# filelist=/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/mhwang/LHC24aj/AO2D/filelist_AO2D_root.txt

start=1
stop=1

SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
# PYTHON_PACK="xjalienfs/1.6.9-1" # provides rich module
ROOT_PACK="ROOT/v6-36-04-alice2-2"
buildopt=( "-C" "$project_root" "BUILD=debug" )
. "$project_root/scripts/util.sh"

input_txt=$output/input.txt
rm -f "$input_txt"
mkdir -p "$output"
output_file="$output/BerkeleyTree.root"

info "File(s) to be converted: "
for (( file_idx = start; file_idx <= stop; file_idx++ )); do
  input_file=$(sed -n "${file_idx}p" $filelist)
  info "  $input_file"
  echo "$input_file" >> "$input_txt"
done

if [[ -n "$compile" ]]; then
  # pretty shifter --image=tch285/converter:latest make remake -C /global/common/software/alice/mhwang/converter
  info "Starting compilation."
  pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
    make remake "${buildopt[@]}"
  check_exit $? "Compilation failed!"
  info "Finished compilation."
else
  info "Skipping compilation."
fi

# cmd="shifter --image=tch285/converter:latest /global/common/software/alice/mhwang/converter/bin/converter \
#       -i $input_txt -o $output_file -c $config_file"
info "Starting conversion."
pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
        "$project_root/bin/converter" \
        -i $input_txt -o $output_file -c "$config_file"
# $cmd
ecode=$?
info "Finished conversion with code $ecode."