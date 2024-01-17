Plan
1) to write full example of above hedging communcation (to proove myself)
2) continue with c++  coding of the logic
3) unit tests of the logic

Milestone 1 (here it could be reviewed at least)

1) to implement single-host variant roq::PositionUpdate from gateway [mikhail , hans, daniel] <-- POC
2) UDP PortfolioUpdate message (from hans) or CustomMetrics message (with ExposureUpdate)
3) WebSocket interface with roq-risk-proxy (communicates liquidation groups into  roq-mmaker (lqs))

4) testing in deribit testnet / binance testnet

5) OMS modify order testing with binance (a lot of testing with binance)






Motivating Example
~~~~~~~~~~~~~~~~~~

T       DELTA           LEG1.BUY           LEG1.SELL           LEG2.BUY        LEG2.SELL            Comment
T0      1               1001@102           0@NAN               0@NAN           1000@$107            we start from zero delta p

        LEG1 <- LEG2            BUY 100 @ $102             SELL 105 @ $107

T1      -4              1001@102           0@NAN               100@102         105@107              sold 5 at $10

           BUY 4 @102

