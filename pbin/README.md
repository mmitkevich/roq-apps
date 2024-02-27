### get parameters list from gateway

```
$ ./pbin/roqa_ctl.py parameters get --user mmaker
                0
0  lqs:underlying
```

### get parameter value by label

```
$ ./pbin/roqa_ctl.py parameters get --user mmaker --label 'lqs:underlying'
            label  strategy_id account exchange         symbol    value
0  lqs:underlying          100    None  deribit  BTC-PERPETUAL  BTC:USD
1  lqs:underlying          100    None  deribit    BTC-29MAR24  BTC:USD
```

### put parameter into gateway

```
$ ./pbin/roqa_ctl.py parameters put --symbol 'BTC-29MAR24' --exchange 'deribit' --strategy_id 100 --label 'lqs:underlying' --value 'BTC:USD' --user mmaker
```