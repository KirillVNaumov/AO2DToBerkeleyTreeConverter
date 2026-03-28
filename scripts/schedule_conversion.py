#!/usr/bin/env python3

import argparse
import logging
import os
import shutil
import subprocess
import sys
from pathlib import Path

import yaml
from rich.logging import RichHandler

log = logging.getLogger('scheduler')
log.setLevel(logging.DEBUG)
log.addHandler(RichHandler(level = logging.INFO, log_time_format = "[%X]"))

class Converter:
    _defaults = {
        "test": True,
        "tree_name": "BerkeleyTree.root",
        "save_clusters": False,
        "naod": 10,
        "root_spec": "ROOT/v6-36-04-alice2-2",
        "email": None,
        "recompile": False,
        "verbosity": 1,
    }

    def __init__(self, config_file):
        self.configure(config_file)

    def configure(self, config_file):
        self.base_path = Path(__file__).resolve().parent.parent
        cfg = self.get_cfg(config_file)
        self.dataset = cfg["dataset"]
        self.input = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/{self.dataset}/AO2D/filelist.txt"
        self.output = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/data/{self.dataset}/BerkeleyTrees"

        os.makedirs(self.output, exist_ok = True)
        shutil.copy2(self.config_file, self.output)
        fh = logging.FileHandler(f'{self.output}/scheduler.log', mode = 'w')
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', '%Y-%m-%d %H:%M:%S'))
        log.addHandler(fh)

        log.info(f"Reading Converter configuration from: {self.config_file}")

        self.is_test = cfg["convert"].get("test", self._defaults["test"])
        self.tree_name = cfg["convert"].get("tree_name", self._defaults["tree_name"])
        self.save_clusters = cfg["convert"].get("save_clusters", self._defaults["save_clusters"])
        self.naod = cfg["convert"].get("naod", self._defaults["naod"])
        self.root_spec = cfg["convert"].get("root_spec", self._defaults["root_spec"])
        self.email = cfg["convert"].get("email", self._defaults["email"])
        self.recompile = cfg["convert"].get("recompile", self._defaults["recompile"])
        self.verbosity = cfg["convert"].get("verbosity", self._defaults["verbosity"])

        self.converter = self.base_path / "bin" / "converter"

        log.info( "Converter configuration:")
        log.info(f"  Converter executable: {self.converter}")
        log.info(f"  AO2D filelist: {self.input}")
        log.info(f"  Output directory: {self.output}")
        log.info(f"  Tree name: {self.tree_name}")
        log.info(f"  Test mode: {self.is_test}")
        log.info(f"  Save clusters: {self.save_clusters}")
        log.info(f"  Number of AO2Ds per tree: {self.naod}")
        log.info(f"  ROOT package: {self.root_spec}")
        log.info(f"  Email: {self.email}")
        log.info(f"  Recompile converter: {self.recompile}")
        log.info(f"  Verbosity: {self.verbosity}")
        log.info( "  Conversion settings:")
        for category in ['event_cuts', 'track_cuts', 'cluster_cuts']:
            log.info(f"    {category}:")
            settings = cfg['convert'][category]
            for param, value in settings.items():
                log.info(f"      {param}: {value}")

        if not self.converter.is_file():
            log.warning("Converter executable does not exist, compiling now.")
            self.compile_converter()
        elif self.recompile:
            log.warning("Forcing recompilation of converter.")
            self.compile_converter()
        if not os.path.isfile(self.input):
            log.error(f"AO2D filelist at '{self.input}' does not exist!")
            sys.exit(0)

        self.slurm_output = f"{self.output}/slurm_out"
        os.makedirs(self.slurm_output, exist_ok = True)

    def get_cfg(self, config_file):
        if not config_file.endswith(".yaml"):
            config_file = f"{config_file}.yaml"

        if config_file.startswith("/"):
            self.config_file = config_file
        elif os.path.isfile(f"{self.base_path}/config/{config_file}"):
            self.config_file = f"{self.base_path}/config/{config_file}"
        elif os.path.isfile(f"{os.getcwd()}/{config_file}"):
            self.config_file = f"{os.getcwd()}/{config_file}"
        else:
            raise FileNotFoundError("Could not find a valid configuration file!")

        with open(self.config_file) as stream:
            cfg = yaml.safe_load(stream)
        return cfg

    def compile_converter(self):
        cmd = ("shifter --module=cvmfs --image=tch285/o2alma:latest "
              f"/cvmfs/alice.cern.ch/bin/alienv setenv {self.root_spec} -c "
              f"make remake -C {self.base_path}"
        )
        res = subprocess.run(cmd, shell = True)#, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        if res.returncode != 0:
            log.error("Compilation failed!")
            sys.exit(res.returncode)

    def schedule(self):
        if self.is_test:
            log.info("Running in local testing mode.")
        else:
            log.info("Running in production mode.")

        with open(self.input, 'r') as f:
            tot_nfiles = sum(1 for line in f)
        njobs = (tot_nfiles + self.naod - 1) // self.naod

        notify = f"#SBATCH --mail-type=BEGIN,END\n#SBATCH --mail-user={self.email}" if self.email else ""
        cluster = "--save-clusters" if self.save_clusters else ""

        verbosity = ""
        if self.verbosity:
            verbosity = f"-{'v' * self.verbosity}"

        with open(f"{self.base_path}/templates/convert_nersc.tmpl", 'r') as f:
            contents = f.read()

        contents = contents.replace("{{NJOBS}}", str(njobs))
        contents = contents.replace("{{SLURM_OUT}}", self.slurm_output)
        contents = contents.replace("{{OUTPUT}}", self.output)
        contents = contents.replace("{{NOTIFY_OPTS}}", notify)
        contents = contents.replace("{{CONFIG}}", self.config_file)
        contents = contents.replace("{{INPUT_FILELIST}}", self.input)
        contents = contents.replace("{{NFILES_PER_TREE}}", str(self.naod))
        contents = contents.replace("{{TREE_NAME}}", self.tree_name)
        contents = contents.replace("{{CLUSTER_OPT}}", cluster)
        contents = contents.replace("{{CONVERTER_PATH}}", str(self.converter))
        contents = contents.replace("{{VERBOSITY}}", verbosity)
        contents = contents.replace("{{ROOT_PACK}}", self.root_spec)

        with open(f"{self.output}/convert.sh", 'w') as f:
            f.write(contents)

        if self.is_test:
            result = subprocess.run(f"/usr/bin/bash {self.output}/convert.sh", shell = True)
            log.info("Starting test conversion.")
            if result.returncode != 0:
                log.error("Test conversion crashed, exiting.")
                sys.exit(result.returncode)
            log.info("Test conversion succeeded.")
        else:
            job_id = subprocess.run(["sbatch", "--parsable", f"{self.output}/convert.sh"], stdout = subprocess.PIPE, stderr = subprocess.PIPE, encoding = "utf-8").stdout.strip()
            log.info(f"Submitted conversion batch job: ID {job_id}")

        with open(f"{self.base_path}/templates/treelist_nersc.tmpl", 'r') as f:
            contents = f.read()

        contents = contents.replace("{{OUTPUT}}", self.output)
        contents = contents.replace("{{SLURM_OUT}}", self.slurm_output)
        contents = contents.replace("{{TREE_NAME}}", self.tree_name)
        contents = contents.replace("{{ROOT_PACK}}", self.root_spec)
        contents = contents.replace("{{NOTIFY_OPTS}}", notify)

        with open(f"{self.output}/treelist.sh", 'w') as f:
            f.write(contents)

        if self.is_test:
            log.info("Creating treelist.")
            result = subprocess.run(["/usr/bin/bash", f"{self.output}/treelist.sh"], encoding = "utf-8")
            if result.returncode != 0:
                log.error("Treelist creation crashed, exiting.")
                sys.exit(result.returncode)
            log.info("Treelist creation succeeded.")
        else:
            job_id = subprocess.run(["sbatch", "--parsable", f"--dependency=afterok:{job_id}", f"{self.output}/treelist.sh"], stdout = subprocess.PIPE, stderr = subprocess.PIPE, encoding = "utf-8").stdout.strip()
            log.info(f"Submitted tree finder batch job: ID {job_id}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert a list of files', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', '--config', help='Path to the config YAML file.')
    args = parser.parse_args()

    scheduler = Converter(args.config)
    scheduler.schedule()