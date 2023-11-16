(c) copyright 2023 Mikhail Mitkevich
see [LICENSE](./LICENSE)


# Design

Please see [./share/config.toml](./share/config.toml) for configuration example

Configuration is done per virtual instrument e.g. `quote:deribit:BTC.P`
It is modeled as `pricer::Node` in the pricer's code [src/roq/pricer/Node.hpp](src/roq/pricer/Node.hpp)

the following snippet shows configuration of target spread parameter

```
# set spread:target=10bp for 'quote:deribit:BTC.P' node (spread is a compute item defined above )
[[parameter]]
    label           = "spread.target"
    symbol          = "BTC.P"
    exchange        = "quote.deribit"
    value           = "bp:10"
```

`exchange` here is just a namespace for `symbol` in pricer and does not correspond to any real exchange (FIXME). Reason - paramters are per-exchange in roq-gateway's `Parameter` sqlite storage.

`label` is used to define parameter, in the form of `compute_name.parameter_name`, where `compute_name` is explained below.

Each node has several `compute` algorithms, forming transformation pipeline

```
# set compute list for 'quote.deribit:BTC.P' node (in order)
[[parameter]]
    label           = "compute"
    symbol          = "BTC.P"
    exchange        = "quote.deribit"
    value           = "product ema shift spread"
```

`product` tells that this node is a multiplication of prices of set of reference nodes (defined separately)
`ema` tells that result of above multiplication should be smoothed with EMA algorithm (with `ema.omega` smoothing coefficient)
`shift` tells that quotes should be shifted according to inventory exposure
`spread` tells that target spread should be applied

Indirectly this `compute` section defines ordering of parameter structures in the Node's inplace `pricer::Node::storage` byte array (TODO)

This requires processing of 'compute' label before processing any of parameter updates, and requires smart reordering of the parameter update message (TODO)


19000 100 20000 200
18000 150 18500 230

                                                A1->M->N
mdata.deribit:BTC.P                   exposure.deribit:BTC.P
   market deribit:BTC-PERPETUAL         portfolio A1
                                        market deribit:BTC-PERPETUAL
                        \               / (bid,ask=(100@19000, 200@20000)) => exposure = -200+100 = -100 @ 20000
      type=mdata          \            / type = exposure
                    quote.deribit:BTC.P
                        product.mdata shift.exposure
                                        
