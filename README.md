(c) copyright 2023 Mikhail Mitkevich
see [LICENSE](./LICENSE)

# Prerequisites
## Install ROQ and ensure gateways are running
You may use https://github.com/mtxpt/roq-setup

The following is assuming `~/roq-setup` installed and roq conda is in `~/roq-conda`

You may need to change your API keys in `~/roq-setup/demo/deribit.toml` and `~/roq-setup/demo/secrets.toml`

The format of ~/roq-setup/demo/secrets.toml

```
[deribit]
REN_DERIBIT="YOUR-DERIBIT-API-SECRET" 
```

also change YOUR-DERIBIT-API-KEY in  ~/roq-setup/demo/deribit.toml in `login=` line

```
  [accounts.REN_DERIBIT] 
  master = true
  symbols = ".*"
  login="YOUR-DERIBIT-API-KEY"
#  secret="secrets.toml"
```

# LQS - LiQuidation Strategy

## edit config
```
$ vi ~/roq-apps/share/config-lqs.toml
```

## build

### OSX
you need to set ROQ_ARCH to build with clang++ on Mac M1

```
export ROQ_ARCH="MacOSX-arm64"
```

to build:

```
$ git clone git@github.com:mtxpt/roq-apps.git
$ source ~/roq-conda/bin/activate
$ cd ~/roq-apps && ./build.sh
```

## run

### please run `roq-deribit` gateway first

You may use ~/roq-setup as follows:

```
~/roq-setup/scripts/roq-gateway run deribit
```

### check processes:

check if `roq-deribit` gateway process is running

```
$ ps -ef|grep roq-deribit
mike      213933  213927 16 18:17 ?        00:08:56 /home/mike/code/roq-setup/bin/roq-deribit --name deribit --exchange deribit --cache_dir /home/mike/code/roq-setup/demo/auth --config_file /home/mike/code/roq-setup/demo/deribit.toml --client_listen_address /home/mike/code/roq-setup/run/deribit-demo.sock --event_log_iso_week=false --secrets_file /home/mike/code/roq-setup/demo/secrets.toml --event_log_utimes_on_sync=true --event_log_sync_freq=5m --auth_keys_file /home/mike/code/roq-setup/real/roq-keys.json --auth_is_uat=true --fix_uri tcp://test.deribit.com:9881 --ws_uri wss://test.deribit.com/ws/api/v2 --ws_market_data_max_subscriptions_per_stream=999 --fix_cancel_on_disconnect=true --service_listen_address=9402 --udp_snapshot_port=20230 --udp_incremental_port=20231 --udp_snapshot_address=127.0.0.1 --udp_incremental_address=127.0.0.1 --loop_sleep=500ns --loop_timer_freq=2500ns --log_path /home/mike/code/roq-setup/var/log/deribit-demo.log --log_rotate_on_open=true --log_max_files=3
```

### Now run the strategy

```
$ cd ~/roq-apps && USE_TOML_PARAMETERS=true ./run.sh lqs
```

NOTE this script uses ROQ_ROOT -> ../roq-setup/ to find place for logs and deribit gateway run file

### check if `roqa` binary is running 
```
$ ps -ef|grep roqa
mike      221839  221829 32 19:11 ?        00:00:01 /home/mike/code/roq-apps/build/debug/src/roq/core/roqa --name mmaker --config_file /home/mike/code/roq-apps/share/config-lqs.toml --strategy lqs /home/mike/code/roq-setup/run/deribit-demo.sock --log_path /home/mike/code/roq-setup/var/log/lqs.log --log_flush_freq 1ms
```

NOTE `--name mmaker` - it is the name of roqa process please ensure you have user in your deribit.toml

## logs

for strategy
```
$ tail -f ~/roq-setup/var/log/lqs-demo.log
```

for gateway 
```
$ tail -f ~/roq-setup/var/log/deribit-demo.log
```

## using lqs_ctl.sh

`roqa` could operate 
1) with toml-based configuration setup (--use_toml_parameters=true or USE_TOML_PARAMETERS=true), when snapshot of parameters is read from toml file. This could be useful for debugging without touching gateway parameters sqlite3  database.

2) with sqllite3 databse used as parameters storage (--use_toml_parameters=false), which is default.

Initially, to import snapshot of parameters from toml file into sqlite3 database please use the following 

```
$ cd roq-apps
$ ./pbin/roqa_ctl.py parameters import --config_file ./share/config-lqs.toml  --user mmaker
```

To verify snapshot of parameters from sqlite3 database in the gateway use the following

```
cd roq-apps
$ ./pbin/roqa_ctl.py parameters get --user mmaker

                      label  strategy_id      account exchange         symbol             value
0     oms:post_fill_timeout          100  REN_DERIBIT  deribit  BTC-PERPETUAL              2000
1     oms:post_fill_timeout          100  REN_DERIBIT  deribit    BTC-29MAR24              4000
0   oms:post_cancel_timeout          100  REN_DERIBIT  deribit  BTC-PERPETUAL               200
```


To remove all parameters from sqlite3 database please use sqlite3 tool directly (with stopped gateway)

```
$ sudo systemctl stop roq-deribit
$ sqlite3  ~/roq-setup/demo/auth/deribit/db.sqlite3 
sqlite> delete from parameters;
Ctrl+D [ENTER]

$ sudo systemctl start roq-deribit
```

To modify parameters in sqlite db using running gateway,
from command line use lqs_ctl.sh script as follows

```
$ SYMBOL=BTC-PERPETUAL EXCHANGE=deribit STRATEGY_ID=100 ./lqs_ctl.sh lqs:buy_volume 20
```

Above sets parameter 'lqs:buy_volume' to 20 for specified symbol,exchange,strategy_id
