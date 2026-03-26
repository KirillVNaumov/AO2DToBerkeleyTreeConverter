#!/usr/bin/bash

#### PLEASE READ scripts/README.md FOR MORE DETAILS ####

show_help() {
    cat << EOF
Usage: run-download [OPTIONS]

Options:
  -c, --config  Path to configuration file
  -h, --help    Show this help message; for more details see scripts/README.md

EOF
}

PARSEDARGS=$(getopt -o c:h \
                    --long config:,help \
                    -n 'run-download' -- "$@")
PARSE_EXIT=$?
if [ $PARSE_EXIT -ne 0 ] ; then exit $PARSE_EXIT ; fi

# NOTE: quotes around PARSEDARGS essential to parse spaces between parsed options
eval set -- "$PARSEDARGS"

while true; do
    case "$1" in
        -c | --config ) config="$2"; shift 2 ;;
        -h | --help )   show_help; exit 0 ;;
        -- ) shift; break ;;
        * ) break ;;
    esac
done

PROJECT_ROOT="$(dirname -- "$(realpath "$0")")/../"
PROJECT_ROOT="$( realpath "$PROJECT_ROOT" )"
. "$PROJECT_ROOT/scripts/util.sh"

if [ -z "$config" ]; then
    error "Config file must be passed with '-c <path/to/config>'" && exit 1
fi

SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
PYTHON_PACK="xjalienfs/1.6.9-1" # provides rich module and AliEn tools

check_cmd shifter

# Check AliEn token
"${SHIFTER[@]}" $ALIENV setenv $PYTHON_PACK -c \
    alien-token-info 2>/dev/null
ret=$?
if [[ $ret -eq 0 ]]; then
    info "Valid AliEn token found."
elif [[ $ret -eq 2 ]]; then
    error "No valid AliEn token found. Run \`source scripts/get_token.sh\` to refresh your token."
    exit 2
else
    error "Unrecognized error $ret while checking AliEn token, crashing out."
    exit $ret
fi

"${SHIFTER[@]}" $ALIENV setenv $PYTHON_PACK -c \
    python3 "$PROJECT_ROOT"/scripts/download_hyperloop.py -c "$config"