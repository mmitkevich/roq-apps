#!/bin/bash
ROQ_DIR=$(realpath $(dirname $0)/../roq-rencap)
UMM_DIR=$(realpath $(dirname $0)/../umm)
STRATEGY=${STRATEGY:-AlphaMaker@deribt}
cd $ROQ_DIR
ROQ_v=2 ./bin/roq-mmaker-app \
    --name mmaker \
    --config_file $UMM_DIR/src/umm/model/alpha_maker/alpha_maker.toml \
    --strategy $STRATEGY \
    $ROQ_DIR/run/deribit-demo.sock \
    $ROQ_DIR/run/ftx-demo.sock

