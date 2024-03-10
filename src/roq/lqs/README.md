# lqs

Very basic multi-leg liquidation strategy

each futures-like leg leg has underlying (either some existing spot instrument or virtual one), and position

underlying.delta is aggregated from the positions

quoting is done either passively or aggressively (execution_mode=JOIN,CROSS,JOIN_PLUS,CROSS_MINUS)

quoting volumes are calculated with two constraints
a) to liquidate position (so the absolute position is never increasing)
b) to keep delta reasonable (so the delta should not exceed thresholds)

