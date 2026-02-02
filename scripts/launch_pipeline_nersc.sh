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
SAVECLUSTERS=

#### ---------- RUNTIME PARAMETERS ---------- #####

NTHREADS=20
CONFIG="$PROJECT_ROOT/tree-cuts.yaml"
AOD_DIR() { echo "$OUTPUT_DIR/AO2D"; }
TREE_DIR() { echo "$OUTPUT_DIR/BerkeleyTrees"; }

SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
PYTHON_PACK="xjalienfs/1.6.9-1" # provides rich module
ROOT_PACK="ROOT/v6-36-04-alice2-2"
# ROOT_PACK="JAliEn-ROOT/0.7.14-43"
# ROOT_PACK="ROOT::v6-32-06-alice1-33" # specific root from jalien, works
# ROOT_PACK="ROOT::v6-36-04-alice2-2" # specific root from root, doesn't work
# ROOT_PACK="ROOT::v6-34-06-alice1-1" # specific root from jalien, works

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
clusteropt=$( [ -n "$SAVECLUSTERS" ] && echo "--save-clusters" )
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

    # Check AliEn token
    "${SHIFTER[@]}" $ALIENV setenv xjalienfs/1.6.9-1 -c \
        alien-token-info 2>/dev/null
    ret=$?
    if [[ $ret -eq 0 ]]; then
        info "Valid AliEn token found."
    elif [[ $ret -eq 2 ]]; then
        error "No valid AliEn token found. Run \`source get_token.sh\` to refresh your token."
        exit 2
    else
        error "Unrecognized error $ret, crashing out."
        exit $ret
    fi

    # Downloads the AO2Ds from Hyperloop using the directory information
    # from HYPERLOOP_FILELIST. The downloaded files are saved to the AOD_DIR.
    "${SHIFTER[@]}" $ALIENV setenv $PYTHON_PACK -c \
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
    pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
        make remake "${buildopt[@]}"
        # make cleaner "${buildopt[@]}"
    # pretty "${SHIFTER[@]}" $ALIENV setenv $ROOT_PACK -c \
    #     make "${buildopt[@]}"
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
        -r "$ROOT_PACK" \
        "$clusteropt" \
        "$testopt"
    check_exit $? "Conversion failed!"
else
    info "Conversion skipped."
fi