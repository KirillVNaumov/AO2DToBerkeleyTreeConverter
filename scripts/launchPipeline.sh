export HYPERLOOP_TXT="../hyperloop_list.txt"
export HYPERLOOP_DATA_DIR="../AO2Ds"
export OUTPUT_DIR="../AODs"

export CONVERTER_PATH="$(dirname -- $(realpath $0))/../"

# Compile the converter
make -C $CONVERTER_PATH

if [ ! -f $CONVERTER_PATH/bin/converter ]; then
    echo "The converter binary doesn't exist."
		exit 1
fi

if ! which alien_find >/dev/null; then
		echo "alien_find is not available"
		exit 1
fi

# Requires alien_find
# Downloads the AO2Ds from the Hypeloop using the directory information
# from HYPERLOOP_TXT. The downloaded files are saved to the HYPERLOOP_DATA_DIR.
python3 downloadHyperloop.py \
--inputfilelist=$HYPERLOOP_TXT \
--outputfolder=$HYPERLOOP_DATA_DIR \
--filename=AO2D.root \
--nThreads=15

if ! which sbatch >/dev/null; then
    echo "sbatch is not available"
    exit 1
fi

# Requires sbatch
# Schedules sbatch jobs to run the AO2D to AOD converter for each of the
# AO2D files found recursively in HYPERLOOP_DATA_DIR and store the
# resulted AOD files into the OUTPUT_DIR
python3 scheduleConversion.py \
--nFilesPerJob=10 \
--input=$HYPERLOOP_DATA_DIR \
--output=$OUTPUT_DIR \
--converterPath=$CONVERTER_PATH/bin/ \
--config=$CONVERTER_PATH/treeCuts.yaml
