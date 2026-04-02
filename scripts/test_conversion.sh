#!/usr/bin/bash

#### TEST CONVERSION ON SINGLE AO2D ####

show_help() {
    cat << EOF
Usage: test-conversion [OPTIONS]

Options:
  -c, --config  Path to config file
  -i, --input   Path to input file
  -o, --output  Path to output file. Set to /global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/$USER/testing/BerkeleyTree.root by default.
  -h, --help    Show this help message

EOF
}

PARSEDARGS=$(getopt -o c:i:o:h \
                    --long compile,config:,input:,output:,help \
                    -n 'test-conversion' -- "$@")
PARSE_EXIT=$?
if [ $PARSE_EXIT -ne 0 ] ; then exit $PARSE_EXIT ; fi

# NOTE: quotes around PARSEDARGS essential to parse spaces between parsed options
eval set -- "$PARSEDARGS"

config=
input=
output=/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/$USER/testing/BerkeleyTree.root

while true; do
    case "$1" in
        -c | --config ) config="$2";    shift 2 ;;
        -i | --input )  input="$2";     shift 2 ;;
        -o | --output ) output="$2";    shift 2 ;;
        -h | --help )   show_help; exit 0 ;;
        -- ) shift; break ;;
        * ) break ;;
    esac
done

project_root="$(dirname -- "$(realpath "$0")")/../"
project_root="$( realpath "$project_root" )"
. "$project_root/scripts/util.sh"

if [ -z "$input"  ]; then error "Specify input file with -i / --input <path/to/file>"; exit 1; fi
if [ -z "$config" ]; then error "Specify config file with -c / --config <path/to/config>"; exit 1; fi

mkdir -p "$( dirname -- "$output" )"
output="$( realpath "$output" )"
output_dir="$( dirname -- "$output" )"
input="$( realpath "$input" )"

save_clusters="$( yq r "$config" 'convert.save_clusters' )"
if [ "${save_clusters,,}" == "true" ]; then
    cluster_opt="--save-clusters"
else
    cluster_opt=""
fi
verbosity="$( yq r "$config" 'convert.verbosity' )"
if [ "$verbosity" == "" ]; then
    # empty string is equal to zero so explicitly check empty string first
    verbosity_opt="-v"
elif [[ "$verbosity" -eq 0 ]]; then
    echo here
    verbosity_opt=""
else
    verbosity_opt="-"
    for _ in $(seq 1 "$verbosity"); do
        verbosity_opt="${verbosity_opt}v"
    done
fi
recompile="$( yq r "$config" 'convert.recompile' )"
if [ "${recompile,,}" == "true" ]; then
    # empty string is equal to zero so explicitly check empty string first
    recompile_opt="true"
else
    recompile_opt=""
fi

info "Config file: $config"
info "Input file: $input"
info "Output directory: $output_dir"
info "Output file: $output"
info "Recompile: $recompile_opt"
info "Cluster option: $cluster_opt"
info "Verbosity: $verbosity_opt"

SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
ROOT_PACK="ROOT/v6-36-04-alice2-2"

input_txt="$output_dir/input.txt"
echo "$input" > "$input_txt"

if [ -n "$recompile_opt" ]; then
    info "Starting compilation."
    pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
        make remake -C "$project_root"
    check_exit $? "Compilation failed!"
    info "Finished compilation."
else
    info "Skipping compilation."
fi

info "Starting conversion."
pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
                "$project_root/bin/converter" \
                -i "$input_txt" -o "$output" -c "$config" \
                "$cluster_opt" "$verbosity_opt"

ecode=$?
info "Finished conversion with code $ecode."