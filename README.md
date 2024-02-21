(c) copyright 2023 Mikhail Mitkevich
see [LICENSE](./LICENSE)

# Prerequisites
## Install ROQ and ensure gateways are running
You may use https://github.com/mtxpt/roq-setup

The following is assuming `~/roq-setup` installed and roq conda is in `~/roq-conda`

You may need to change your API keys in `~/roq-setup/demo/deribit.toml` and `~/roq-setup/demo/secrets.toml`

# LQS - LiQuidation Strategy

## edit config
```
$ vi ~/roq-apps/share/config-lqs.toml
```

## build
```
$ git clone git@github.com:mtxpt/roq-apps.git
$ source ~/roq-conda/bin/activate
$ cd ~/roq-apps && ./build.sh
```

## run

### please run `roq-deribit` gateway first

```
$ cd ~/roq-apps && ./run.sh lqs
```

### check processes:

check if `roq-deribit` gateway process is running

```
$ ps -ef|grep roq-deribit
mike      213933  213927 16 18:17 ?        00:08:56 /home/mike/code/roq-setup/bin/roq-deribit --name deribit --exchange deribit --cache_dir /home/mike/code/roq-setup/demo/auth --config_file /home/mike/code/roq-setup/demo/deribit.toml --client_listen_address /home/mike/code/roq-setup/run/deribit-demo.sock --event_log_iso_week=false --secrets_file /home/mike/code/roq-setup/demo/secrets.toml --event_log_utimes_on_sync=true --event_log_sync_freq=5m --auth_keys_file /home/mike/code/roq-setup/real/roq-keys.json --auth_is_uat=true --fix_uri tcp://test.deribit.com:9881 --ws_uri wss://test.deribit.com/ws/api/v2 --ws_market_data_max_subscriptions_per_stream=999 --fix_cancel_on_disconnect=true --service_listen_address=9402 --udp_snapshot_port=20230 --udp_incremental_port=20231 --udp_snapshot_address=127.0.0.1 --udp_incremental_address=127.0.0.1 --loop_sleep=500ns --loop_timer_freq=2500ns --log_path /home/mike/code/roq-setup/var/log/deribit-demo.log --log_rotate_on_open=true --log_max_files=3
```

check if `roqa` binary is running 
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
