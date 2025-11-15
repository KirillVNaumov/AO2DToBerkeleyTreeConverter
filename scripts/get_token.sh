#!/usr/bin/bash

#### This script MUST BE SOURCED (not run with ./get_token.sh) within your shell to work! ####

__get_token__() {
    if [ "${0##*/}" = "get_token.sh" ]; then
        echo "This script must be sourced!" >&2; exit 1
    fi

    local SHIFTER=("shifter" "--module=cvmfs" "--image=tch285/o2alma:latest")
    local ALIENV="/cvmfs/alice.cern.ch/bin/alienv"
    local XJALIEN_VER="xjalienfs/1.6.9-1"
    local THISD
    THISD="$(dirname -- "$(readlink --canonicalize --no-newline "${BASH_SOURCE[0]}")")"
    . "$THISD/util.sh"

    "${SHIFTER[@]}" $ALIENV setenv "$XJALIEN_VER" -c \
        alien-token-init 2>/dev/null
    local ret=$?
    if [[ $ret -eq 0 ]]; then
        info "AliEn token built."
    elif [[ $ret -eq 107 ]]; then
        error "Incorrect password, try again."
    else
        error "Unrecognized error $ret, crashing out."
    fi
    return $ret
}

__get_token__
unset -f __get_token__