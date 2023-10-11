#!/bin/bash
ROQ_DIR=$(realpath $(dirname $0)/../roq-rencap)
UMM_DIR=$(realpath $(dirname $0)/../umm)
STRATEGY=${STRATEGY:-MarketMixer@deribit}
cd $ROQ_DIR
ROQ_v=3 ./bin/roq-mmaker-app \
    --name mmaker \
    --config_file $UMM_DIR/src/umm/model/market_mixer/market_mixer.toml \
    --strategy $STRATEGY \
    $ROQ_DIR/run/deribit-demo.sock

