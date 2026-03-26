#!/usr/bin/bash

#### PLEASE READ scripts/README.md FOR MORE DETAILS ####

show_help() {
    cat << EOF
Usage: run-conversion [OPTIONS]

Options:
  -c, --config  Path to configuration file
  -h, --help    Show this help message; for more details see scripts/README.md

EOF
}

PARSEDARGS=$(getopt -o c:h \
                    --long config:,help \
                    -n 'run-conversion' -- "$@")
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

# provides rich and PyYAML modules, while preserving access to sbatch
module load python/3.12-25.3.0

python3 "$PROJECT_ROOT"/scripts/schedule_conversion.py -c "$config"

module unload python/3.12-25.3.0