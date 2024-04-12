#!/bin/bash
VERBOSE=${VERBOSE:-0}

cd $(dirname $0)
PWD=$(pwd)
if [[ $@ < 1 ]]; then
	echo "run.sh lqs|dag|spreader"
	exit 1
fi
APP=$1
shift
#while [[ $@ > 0 ]]; do
#case $1 in
#*)
#    shift
#;;
#esac
#done
ROQ_CONDA=${ROQ_CONDA:-${CONDA_PREFIX:-"$HOME/roq-conda"}}
echo "ROQ_CONDA=$ROQ_CONDA"
if [ ! -d $ROQ_CONDA ]; then
echo "please install roq first"
fi

ROQ_NAME=${ROQ_NAME:-"mmaker"}
ROQ_MODE=${ROQ_MODE:-"demo"}
ROQ_ROOT=${ROQ_ROOT:-"$(PWD)/../roq-setup"}

USE_TOML_PARAMETERS=${USE_TOML_PARAMETERS:-"false"}

#source $ROQ_CONDA/bin/activate

function cmd {
	CMD="$@"
	echo "cmd: $CMD"
	$CMD
}
GATEWAYS=${GATEWAYS:-"deribit"}
GWS=""
for GW in $GATEWAYS; do
 GWS="$GWS $ROQ_ROOT/run/$GW-$ROQ_MODE.sock"
done

cmd $ROQ_CONDA/bin/roqa --name=$ROQ_NAME --config_file=$PWD/share/config-$APP.toml --strategy=$APP --use_toml_parameters=$USE_TOML_PARAMETERS --log_path=$ROQ_ROOT/var/log/$APP-$ROQ_MODE.log $GWS $@
