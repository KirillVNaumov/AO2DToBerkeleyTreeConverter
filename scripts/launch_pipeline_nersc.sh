#!/usr/bin/bash

#### PLEASE READ THE README.md FOR MORE DETAILS ####

show_help() {
    cat << EOF
Usage: launch-pipeline-nersc [OPTIONS]

Options:
  -d, --download  Run download
  -c, --convert   Run conversion scheduling
  -t, --test      Run conversion in test mode
  -v, --verbose   Compile converter in verbose mode
  -h, --help      Show this help message; for more details see scripts/README.md

EOF
}

PROJECT_ROOT="$(dirname -- "$(realpath "$0")")/../"
PROJECT_ROOT="$( realpath "$PROJECT_ROOT" )"
STAGING=$CFS/alice/alicepro/hiccup/rstorage/alice/run3/data/staging/$USER

#### -------------- USER PARAMETERS -------------- #####

HYPERLOOP_FILELIST="$PROJECT_ROOT/hylists/example_22o.txt"
# OUTPUT_DIR="$CFS/alice/alicepro/hiccup/rstorage/alice/run3/data/LHC24aj"
OUTPUT_DIR="$STAGING/LHC22o_test"

EMAIL=
NAOD=15

#### ---------- RUNTIME PARAMETERS ---------- #####

NTHREADS=20
CONFIG="$PROJECT_ROOT/tree-cuts.yaml"
AOD_DIR() { echo "$OUTPUT_DIR/AO2D"; }
TREE_DIR() { echo "$OUTPUT_DIR/BerkeleyTrees"; }

SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
# NOTE: alien-token-init stdin prompt broken for pre-release > 43
PACK_SPEC="JAliEn-ROOT/0.7.14-43"

#### ---------- SOURCE CODE ---------- #####


cd "$PROJECT_ROOT/scripts"
. util.sh

PARSEDARGS=$(getopt -o dctvh \
                    --long download,convert,test,verbose,help \
                    -n 'launch-pipeline-nersc' -- "$@")
PARSE_EXIT=$?
if [ $PARSE_EXIT -ne 0 ] ; then exit $PARSE_EXIT ; fi

# NOTE: quotes around PARSEDARGS essential to parse spaces between parsed options
eval set -- "$PARSEDARGS"

download=
convert=
testopt=
buildopt=( "-C" "$PROJECT_ROOT" )

while true; do
    case "$1" in
        -d | --download ) download="true"; shift ;;
        -c | --convert )  convert="true"; shift ;;
        -t | --test )     testopt="--test"; shift ;;
        -v | --verbose )  buildopt+=("BUILD=debug"); shift ;;
        -h | --help )     show_help; exit 0 ;;
        -- ) shift; break ;;
        * ) break ;;
    esac
done

# If no options passed, then assume we do a full test run
if [[ -z "$download" && -z "$convert" ]]; then
    ans=$(prompt "No passed options." "Would you like to do a full test run?")
    if [[ $ans == "y" ]]; then
        download="true"; convert="true"; testopt="--test"
        info "Starting full test run..."
    else
        info "Exiting." && exit 0;
    fi
fi

### Validate output directory in test mode to make sure we're in a staging directory
if [[  -n "$testopt" ]]; then
    if [[ ( "$OUTPUT_DIR" != $STAGING* ) ]]; then
        new_output_dir=$STAGING/$( basename "$OUTPUT_DIR" )
        ans=$(prompt "$OUTPUT_DIR is not a valid staging directory." "Set output directory to:\n\t$new_output_dir")
        if [[ $ans == "y" ]]; then
            OUTPUT_DIR=$new_output_dir
        else
            error "Exiting." && exit 2
        fi
    else info "Output directory validated for test run."; fi
fi

if [ -n "$download" ]; then
    check_cmd shifter

    # Downloads the AO2Ds from Hyperloop using the directory information
    # from HYPERLOOP_FILELIST. The downloaded files are saved to the AOD_DIR.
    "${SHIFTER[@]}" $ALIENV setenv $PACK_SPEC -c \
        python3 "$PROJECT_ROOT"/scripts/download_hyperloop.py \
            --input "$HYPERLOOP_FILELIST" \
            --output "$(AOD_DIR)" \
            --filename AO2D.root \
            --nthreads $NTHREADS
    check_exit $? "Download failed!"
else
    info "Download skipped."
fi

if [ -n "$convert" ]; then
    check_cmd shifter

    # Compile the converter
    pretty "${SHIFTER[@]}" $ALIENV setenv $PACK_SPEC -c \
        make remake "${buildopt[@]}"
    check_exit $? "Compilation failed!"

    # Schedules sbatch jobs to convert a number of AO2Ds found
    # in AOD_DIR into a BerkeleyTree and store the trees in TREE_DIR
    ./schedule_conversion.sh \
        -p "$PROJECT_ROOT/bin/converter" \
        -n "$NAOD" \
        -i "$(AOD_DIR)" \
        -o "$(TREE_DIR)" \
        -c "$CONFIG" \
        -e "$EMAIL" \
        "$testopt"
    check_exit $? "Conversion failed!"
else
    info "Conversion skipped."
fi