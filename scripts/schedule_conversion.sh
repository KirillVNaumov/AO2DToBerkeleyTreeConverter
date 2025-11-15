#!/usr/bin/bash

show_help() {
    cat << EOF
Usage: conversion-scheduler [OPTIONS]

Options:
  -p, --path PATH       Set path to converter binary (required)
  -i, --input INPUT     Set input AO2D directory (required)
  -o, --output OUTPUT   Set output tree directory (required)
  --aod-name FILENAME   Set filename for AO2Ds (default: AO2D.root)
  --tree-name FILENAME  Set filename for trees (default: BerkeleyTree.root)
  -n, --nfiles NFILES   Set number of AO2Ds per converted tree (default: 10)
  -c, --config CONFIG   Set tree configuration YAML file (default: tree-cuts.yaml in project root)
  -e, --email EMAIL     Request slurm to send email notifications to EMAIL at start and end (default: false)
  -t, --test            Run conversion in test mode (default: false)
  -h, --help            Show this help message

EOF
}

project_root=$( realpath "$(dirname -- "$(realpath "$0")")/../" )
cd "$project_root/scripts"
. util.sh

# using GNU getopt in place of Python argparse
PARSEDARGS=$(getopt -o p:i:o:n:c:r:e:th \
                    --long path:,input:,output:,aod-name:,tree-name:,nfiles:,config:,root-pack:,email:,test,help \
                    -n 'conversion-scheduler' -- "$@")
PARSE_EXIT=$?
if [ $PARSE_EXIT -ne 0 ] ; then exit $PARSE_EXIT ; fi

# NOTE: quotes around PARSEDARGS essential to parse spaces between parsed options
eval set -- "$PARSEDARGS"

converter_path=
input=
output=
aod_name=
tree_name=
nfiles_per_tree=
config=
root_pack=
email=
test=false
while true; do
    case "$1" in
        -p | --path )      converter_path="$2"; shift 2 ;;
        -i | --input )     input="$2"; shift 2 ;;
        -o | --output )    output="$2"; shift 2 ;;
        --aod-name )       aod_name="$2"; shift 2 ;;
        --tree-name )      tree_name="$2"; shift 2 ;;
        -n | --nfiles )    nfiles_per_tree="$2"; shift 2 ;;
        -c | --config )    config="$2"; shift 2 ;;
        -r | --root-pack ) root_pack="$2"; shift 2 ;;
        -e | --email )     email="$2"; shift 2 ;;
        -t | --test )      test="true"; shift ;;
        -h | --help )      show_help; exit 0 ;;
        -- ) shift; break ;;
        * ) break ;;
    esac
done

[ -z "$converter_path" ] && error "Path to converter binary required, crashing out." && exit 1
[ -z "$input" ] && error "Input AO2D directory required, crashing out." && exit 1
[ -z "$output" ] && error "Output tree directory required, crashing out." && exit 1

[ ! -f "$converter_path" ] && error "Converter path '$converter_path' not valid." && exit 1
[ ! -d "$input" ] && error "Input directory '$input' not found." && exit 1
[ ! -d "$output" ] && warn "Output directory '$output' not found, creating." && mkdir -p "$output"

[ -z "$aod_name" ] && warn "AO2D filename not specified, defaulting to AO2D.root." && aod_name=AO2D.root
[ -z "$tree_name" ] && warn "Tree filename not specified, defaulting to BerkeleyTree.root." && tree_name=BerkeleyTree.root
[ -z "$nfiles_per_tree" ] && warn "Number of AO2D files per tree not specified, defaulting to 10." && nfiles_per_tree=10
[ -z "$config" ] && warn "Configuration not specified, defaulting to tree-cuts.yaml in root directory." && \
    config=$project_root/tree-cuts.yaml
[ ! -f "$config" ] && error "Configuration file '$config' not found." && exit 1
[ -z "$root_pack" ] && error "ROOT package not set." && exit 1

settings=$(cat << EOF
Beginning conversion. Your conversion settings:
    Converter path: $converter_path
    Input AOD directory: $input
    Output tree directory: $output
    AOD name: $aod_name
    Tree name: $tree_name
    Number of files per tree: $nfiles_per_tree
    Config file: $config
    ROOT package: $root_pack
    Email: $email
    Test mode: $test
EOF
)
info "$settings"

if [ $test == "true" ]; then
    info "Running in local testing mode."
    convcmd=/usr/bin/bash
else
    info "Running in production mode."
    check_cmd sbatch
    # convcmd=/usr/bin/bash
    convcmd="sbatch --parsable"
fi

if [[ -d "$output" && "$(ls -A "$output" )" ]]; then
    # Tree directory exists but is non empty
    warn "Non-empty tree directory found, clearing." && rm -rf "${output:?}/"* && info "Tree directory cleared."
elif [ ! -d "$output" ]; then
    # Tree directory doesn't exist
    mkdir -p "$output" && info "Tree directory created."
else
    # Tree directory exists but is empty
    info "Tree directory found."
fi

aod_list=$output/AOD_list.txt
tot_nfiles=$(find "$input/" -type f -name "$aod_name" | tee "$aod_list" | wc -l)
info "Total files: $tot_nfiles"

njobs=$(( (tot_nfiles + nfiles_per_tree - 1) / nfiles_per_tree ))

slurm_out=$output/slurm
mkdir -p "$slurm_out"

notify_opts=
if [ -n "$email" ]; then
    notify_opts="#SBATCH --mail-type=BEGIN,END\n#SBATCH --mail-user=$email"
fi

sed -e "s|{{NJOBS}}|$njobs|g" \
    -e "s|{{SLURM_OUT}}|$slurm_out|g" \
    -e "s|{{OUTPUT}}|$output|g" \
    -e "s|{{NOTIFY_OPTS}}|$notify_opts|g" \
    -e "s|{{CONFIG}}|$config|g" \
    -e "s|{{INPUT_FILELIST}}|$aod_list|g" \
    -e "s|{{NFILES_PER_TREE}}|$nfiles_per_tree|g" \
    -e "s|{{TREE_NAME}}|$tree_name|g" \
    -e "s|{{CONVERTER_PATH}}|$converter_path|g" \
    -e "s|{{ROOT_PACK}}|$root_pack|g" \
    "$project_root/templates/convert_nersc.tmpl" > "$output/convert.sh"

if [ $test == "true" ]; then
    $convcmd "$output/convert.sh"
    treecmd=/usr/bin/bash
else
    conv_id=$( $convcmd "$output/convert.sh" )
    # treecmd=/usr/bin/bash
    info "Submitted conversion batch job (ID $conv_id)"
    treecmd="sbatch --dependency=afterok:$conv_id"
fi

tree_list=$output/tree_list.txt

sed -e "s|{{OUTPUT}}|$output|g" \
    -e "s|{{SLURM_OUT}}|$slurm_out|g" \
    -e "s|{{TREE_NAME}}|$tree_name|g" \
    -e "s|{{TREE_LIST}}|$tree_list|g" \
    "$project_root/templates/treelist_nersc.tmpl" > "$output/treelist.sh"

$treecmd "$output/treelist.sh"

