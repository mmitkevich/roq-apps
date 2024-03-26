DIR=$(dirname $0)
if [[ $# < 2 ]]; then
	echo usage:
	echo SYMBOL=BTC-PERPETUAL EXCHANGE=deribit STRATEGY_ID=100 $0 lqs:buy_volume 20
	exit 1
fi
STRATEGY_ID=${STRATEGY_ID:-100}
EXCHANGE=${EXCHANGE:-deribit}
SYMBOL=${SYMBOL:-"BTC-PERPETUAL"}
ACCOUNT=${ACCOUNT:-"REN_DERIBIT"}
$DIR/pbin/roqa_ctl.py parameters put  --label "$1" --value "$2" --symbol "$SYMBOL" --exchange "$EXCHANGE"  --strategy_id $STRATEGY_ID --account "$ACCOUNT" --user mmaker

