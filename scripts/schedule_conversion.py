#!/usr/bin/env python3

import argparse
import glob
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
    def __init__(self, config_file):
        cfg = self.get_cfg(config_file)
        self.validate_cfg(cfg)
        self.configure(cfg)

    def configure(self, cfg):
        defaults = {
            "test": True,
            "tree_name": "BerkeleyTree.root",
            "save_clusters": False,
            "naod": 10,
            "root_spec": "ROOT/v6-36-04-alice2-2",
            "email": None,
            "recompile": False,
            "verbosity": 1,
        }
        self.dataset = cfg["dataset"]
        convert_cfg = cfg.get("convert", {})
        self.is_test = convert_cfg.get("test", defaults["test"])
        self.tree_name = convert_cfg.get("tree_name", defaults["tree_name"])
        self.save_clusters = convert_cfg.get("save_clusters", defaults["save_clusters"])
        self.naod = convert_cfg.get("naod", defaults["naod"])
        self.root_spec = convert_cfg.get("root_spec", defaults["root_spec"])
        self.email = convert_cfg.get("email", defaults["email"])
        self.recompile = convert_cfg.get("recompile", defaults["recompile"])
        self.verbosity = convert_cfg.get("verbosity", defaults["verbosity"])

        self.is_mc = self.get_origin()
        subdir = "mc_central" if self.is_mc else "data"
        self.input = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/{subdir}/{self.dataset}/AO2D/filelist.txt"
        self.output = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/{subdir}/{self.dataset}/BerkeleyTrees"
        self.detect_mc()

        os.makedirs(self.output, exist_ok = True)
        shutil.copy2(self.config_file, self.output)
        fh = logging.FileHandler(f'{self.output}/scheduler.log', mode = 'w')
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', '%Y-%m-%d %H:%M:%S'))
        log.addHandler(fh)

        log.info( "Starting HyperConverter...")
        log.info(f"Reading HyperConverter configuration from: {self.config_file}")

        self.converter = self.base_path / "bin" / "converter"

        log.info( "HyperConverter configuration:")
        log.info(f"  Converter executable: {self.converter}")
        log.info(f"  AO2D filelist: {self.input}")
        log.info(f"  Output directory: {self.output}")
        log.info(f"  Tree name: {self.tree_name}")
        log.info(f"  Test mode: {self.is_test}")
        log.info(f"  Is MC: {self.is_mc}")
        if not self.is_mc:
            log.info(f"  Save clusters: {self.save_clusters}")
        elif self.is_mc and "save_clusters" in cfg["convert"]:
            log.warning("  Cluster saving setting will be ignored in MC conversion!")
        log.info(f"  Number of AO2Ds per BerkeleyTree: {self.naod}")
        log.info(f"  ROOT package: {self.root_spec}")
        log.info(f"  Email: {self.email}")
        log.info(f"  Recompile converter: {self.recompile}")
        log.info(f"  Verbosity: {self.verbosity}")
        if not self.is_mc:
            log.info( "  Conversion settings:")
            categories = [category for category in ['event_cuts', 'track_cuts', 'cluster_cuts'] if category in cfg['convert']]
            for category in categories:
                log.info(f"    {category}:")
                settings = cfg['convert'][category]
                for param, value in settings.items():
                    log.info(f"      {param}: {value}")
        elif self.is_mc and any(category in cfg["convert"] for category in ['event_cuts', 'track_cuts', 'cluster_cuts']):
            log.warning("Since this dataset is an MC dataset, cuts will be ignored.")

        if not self.converter.is_file():
            log.warning("Converter executable does not exist, compiling now.")
            self.compile_converter()
        elif self.recompile:
            log.warning("Forcing recompilation of converter.")
            self.compile_converter()
        if not os.path.isfile(self.input):
            log.error(f"AO2D filelist at '{self.input}' does not exist!")
            sys.exit(1)

        self.slurm_output = f"{self.output}/slurm_out"
        os.makedirs(self.slurm_output, exist_ok = True)

    def get_cfg(self, config_file):
        if not config_file.endswith(".yaml"):
            config_file = f"{config_file}.yaml"
        self.base_path = Path(__file__).resolve().parent.parent

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

    def validate_cfg(self, cfg):
        if "dataset" not in cfg:
            log.critical("Must define a 'dataset' in the config file!")
            sys.exit(1)
        if "download" not in cfg:
            log.critical("Must define a 'download' section in the config file!")
            sys.exit(1)
        if "hyperdirs" not in cfg["download"]:
            log.critical("Must give the Hyperloop directories in the 'hyperdirs' field in the config file!")
            sys.exit(1)
        if "train" not in cfg["download"]:
            log.critical("Must give the train number in the 'train' field in the config file!")
            sys.exit(1)

    def compile_converter(self):
        cmd = ("shifter --module=cvmfs --image=tch285/o2alma:latest "
              f"/cvmfs/alice.cern.ch/bin/alienv setenv {self.root_spec} -c "
              f"make remake -C {self.base_path}"
        )
        res = subprocess.run(cmd, check = False, capture_output = True, shell = True)
        if res.returncode != 0:
            log.error(f"Compilation failed (exit code {res.returncode}): \n\t{res.stdout}\n\t{res.stderr}")
            sys.exit(res.returncode)

    def get_origin(self):
        log.info("Determining dataset origin (data/MC) via directory search...")
        template = f"/global/cfs/cdirs/alice/alicepro/hiccup/rstorage/alice/run3/{{}}/{self.dataset}/AO2D/filelist.txt"
        is_mc = False
        is_data = False

        # independently check whether there's a corresponding directory in `data` or `mc_central`
        if os.path.isfile(template.format("data")):
            is_data = True
        if os.path.isfile(template.format("mc_central")):
            is_mc = True

        if is_data and is_mc:
            # somehow found directories in both `data` and `mc_central`
            log.critical(f"Found directories for both MC and data for dataset {self.dataset}?")
            sys.exit(1)
        elif not is_data and not is_mc:
            # found no directories in either `data` or `mc_central`
            log.critical(f"Could not find any data or MC directories for dataset {self.dataset}, has the dataset been downloaded?")
            sys.exit(1)

        name = "MC" if is_mc else "data"
        log.info(f"Dataset origin (data/MC) identified: {name}")

        return is_mc

    def detect_mc(self):
        log.info("Cross-checking data/MC origin against AO2D contents...")
        with open(self.input, 'r') as f:
            path = f.readline().strip() 
        cmd = ("shifter -m none --image=rootproject/root:latest "
              f"rootls {path}:*"
        )
        res = subprocess.run(cmd, check = False, shell = True, capture_output = True, encoding = 'utf-8')
        if res.returncode != 0:
            log.error(f"MC detection failed:\n{res.stdout}\n{res.stderr}")
            sys.exit(res.returncode)
        mc_detected = False
        if "O2berkeleytree" in res.stdout:
            mc_detected = True
        else:
            # check TTrees if data
            self.check_trees(res.stdout)

        origin_from_dir = "MC" if self.is_mc else "data"
        if mc_detected == self.is_mc:
            log.info("MC/data origin are consistent between directory search and AO2D contents.")
            log.info(f"Files will be converted as: {origin_from_dir}")
            return
        origin_from_file = "MC" if mc_detected else "data"
        log.critical("Origin via directory search does not match origin via AO2D file contents:")
        log.critical(f"\tDirectory search: {origin_from_dir}")
        log.critical(f"\tAO2D contents : {origin_from_file}")
        sys.exit(1)

    def check_trees(self, output):
        required_trees = ['O2jbc', 'O2jcollision', 'O2jtrack']
        log.info(f"Checking for required TTrees: {", ".join(required_trees)}")
        missing_trees = [tree for tree in required_trees if tree not in output]
        if missing_trees:
            log.critical(f"AO2D does not contain the right TTrees; missing: {missing_trees}")
            sys.exit(1)
        else:
            log.info("Required TTrees verified.")
        if self.save_clusters:
            required_trees_clusters = ['O2jcluster', 'O2jclustertrack', 'O2jemctrack', "O2jemccollisionlb"]
            log.info(f"Cluster info requested, so checking for additional TTrees: {", ".join(required_trees_clusters)}")
            missing_trees_clusters = [tree for tree in required_trees_clusters if tree not in output]
            if missing_trees_clusters:
                log.critical(f"AO2D does not contain the right TTrees; missing: {missing_trees_clusters}")
                sys.exit(1)
            else:
                log.info("Required TTrees for cluster info verified.")

    def setup_input_filelists(self):
        log.info("Setting up input filelists...")

        file_pattern = f"{self.slurm_output}/input_*.txt"
        for file_path in glob.glob(file_pattern):
            try:
                os.remove(file_path)
                log.debug(f"Deleted: {file_path}")
            except OSError as e:
                log.debug(f"Error deleting {file_path}: {e}")

        # each Hyperloop directory corresponds to one run
        hydirs = [str(d) for d in Path(self.input).parent.iterdir() if d.is_dir()]
        paths_mapping = {hydir: [] for hydir in hydirs}
        with open(self.input, 'r') as f:
            filepaths = [filepath.rstrip() for filepath in f]
        for filepath in filepaths:
            hydir_match = ""
            for hydir in hydirs:
                if hydir in filepath:
                    if not hydir_match:
                        hydir_match = hydir
                        paths_mapping[hydir].append(filepath)
                    else:
                        log.critical(f"Somehow two Hyperloop directories ({hydir_match} and {hydir}) matched this file: {filepath}")
                        sys.exit(1)
            if not hydir_match:
                log.critical(f"Did not find any match between file: {filepath} and Hyperloop directories: {hydirs}")
                sys.exit(1)

        ijob = 0
        for hydir, paths in paths_mapping.items():
            paths.sort() # so it's deterministic
            chunks = [paths[i:i + self.naod] for i in range(0, len(paths), self.naod)]
            for chunk in chunks:
                ijob += 1
                with open(f"{self.slurm_output}/input_{ijob}.txt", "w") as f:
                    f.writelines(f"{path}\n" for path in chunk)

        # ijob now equals the total number of jobs
        return ijob

    def schedule(self):
        if self.is_test:
            log.info("Running in local testing mode.")
        else:
            log.info("Running in production mode.")

        notify_opts = f"#SBATCH --mail-type=BEGIN,END\n#SBATCH --mail-user={self.email}" if self.email else ""
        cluster_opt = "--save-clusters" if self.save_clusters else ""
        verbosity = f"-{'v' * self.verbosity}" if self.verbosity else ""
        mc_opt = "--is-mc" if self.is_mc else ""

        njobs = self.setup_input_filelists()

        with open(f"{self.base_path}/templates/convert_nersc.tmpl", 'r') as f:
            contents = f.read()

        contents = contents.replace("{{NJOBS}}", str(njobs))
        contents = contents.replace("{{SLURM_OUT}}", self.slurm_output)
        contents = contents.replace("{{OUTPUT}}", self.output)
        contents = contents.replace("{{NOTIFY_OPTS}}", notify_opts)
        contents = contents.replace("{{CONFIG}}", self.config_file)
        contents = contents.replace("{{TREE_NAME}}", self.tree_name)
        contents = contents.replace("{{CLUSTER_OPT}}", cluster_opt)
        contents = contents.replace("{{CONVERTER_PATH}}", str(self.converter))
        contents = contents.replace("{{VERBOSITY}}", verbosity)
        contents = contents.replace("{{MC_OPT}}", mc_opt)
        contents = contents.replace("{{ROOT_PACK}}", self.root_spec)

        with open(f"{self.output}/convert.sh", 'w') as f:
            f.write(contents)

        if self.is_test:
            log.info("Starting test conversion.")
            result = subprocess.run(f"/usr/bin/bash {self.output}/convert.sh", check=False, shell = True)
            if result.returncode != 0:
                log.error(f"Test conversion crashed, exiting:\n{result.stdout}\n{result.stderr}")
                sys.exit(result.returncode)
            log.info("Test conversion succeeded.")
        else:
            result = subprocess.run(["sbatch", "--parsable", f"{self.output}/convert.sh"], check=False, capture_output=True, encoding = "utf-8")
            if result.returncode != 0:
                log.error(f"Conversion batch submission failed:\n{result.stdout}\n{result.stderr}")
                sys.exit(result.returncode)
            job_id = result.stdout.strip()
            log.info(f"Submitted conversion batch job: ID {job_id}")

        with open(f"{self.base_path}/templates/treelist_nersc.tmpl", 'r') as f:
            contents = f.read()

        contents = contents.replace("{{OUTPUT}}", self.output)
        contents = contents.replace("{{SLURM_OUT}}", self.slurm_output)
        contents = contents.replace("{{TREE_NAME}}", self.tree_name)
        contents = contents.replace("{{ROOT_PACK}}", self.root_spec)
        contents = contents.replace("{{NOTIFY_OPTS}}", notify_opts)

        with open(f"{self.output}/treelist.sh", 'w') as f:
            f.write(contents)

        if self.is_test:
            log.info("Creating treelist.")
            result = subprocess.run(["/usr/bin/bash", f"{self.output}/treelist.sh"], check=False, encoding = "utf-8")
            if result.returncode != 0:
                log.error(f"Treelist creation crashed, exiting:\n{result.stdout}\n{result.stderr}")
                sys.exit(result.returncode)
            log.info("Treelist creation succeeded.")
        else:
            result = subprocess.run(["sbatch", "--parsable", f"--dependency=afterok:{job_id}", f"{self.output}/treelist.sh"], check=False, capture_output=True, encoding = "utf-8")
            if result.returncode != 0:
                log.error(f"Treelist batch submission failed:\n{result.stdout}\n{result.stderr}")
                sys.exit(result.returncode)
            job_id = result.stdout.strip()
            log.info(f"Submitted tree finder batch job: ID {job_id}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert a list of files', formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-c', '--config', help='Path to the config YAML file.')
    args = parser.parse_args()

    scheduler = Converter(args.config)
    scheduler.schedule()