(c) copyright 2023 Mikhail Mitkevich
see [LICENSE](./LICENSE)

# Prerequisites
## Install ROQ and ensure gateways are running
You may use https://github.com/mtxpt/roq-setup

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

```
$ cd ~/roq-apps && ./run.sh lqs
```
