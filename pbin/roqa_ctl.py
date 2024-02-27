#!/usr/bin/python3
import logging
import json
import requests
import argparse
import requests
import pandas as pd

logging.basicConfig(
    format="%(message)s",
    level=logging.INFO, # logging.DEBUG
)

logging.getLogger("requests").setLevel(logging.WARNING)
logging.getLogger("urllib3").setLevel(logging.WARNING)


parser = argparse.ArgumentParser()
parser.add_argument("module", type=str, help="parameters")
parser.add_argument("action", type=str, help="get|set")
parser.add_argument("--label", type=str, help="label")
parser.add_argument("--exchange", type=str, help="exchange")
parser.add_argument("--symbol", type=str, help="exchange")
parser.add_argument("--account", type=str, help="account")
parser.add_argument("--strategy_id", type=int, help="strategy_id")
parser.add_argument("--value", type=str, help="value")
parser.add_argument("--user", type=str, required=True, help="user")
global args
args = parser.parse_args()

SERVER="localhost:9402"
BASE_URL = "http://{}".format(SERVER)
PARAMETERS_URL = "{}/api/parameters".format(BASE_URL)

if args.module=="parameters":
    if args.action=="get":
        if args.label:
            url = "{}/{}?user={}".format(PARAMETERS_URL,args.label,args.user)
        else:
            url = "{}?user={}".format(PARAMETERS_URL,args.user)
        logging.debug("parameters get label={} user={}".format(args.label, args.user))            
        resp = requests.get(url)
        logging.debug("parameters get result {} <<{}>>".format(resp.status_code, resp.json()))
        df = pd.DataFrame(data=resp.json())
        print(df)
    elif args.action=="put":
        KEYS={"label","symbol","exchange","account","strategy_id","value"}
        data = {k:v for k,v in vars(args).items() if k in KEYS and v is not None}
        url = "{}?user={}".format(PARAMETERS_URL,args.user)
        logging.debug("parameters put user={} data=<<{}>>".format(args.user, data))
        resp = requests.put(url, data=json.dumps(data))
        logging.debug("parameters put result {}".format(resp.status_code))
    else:
        logging.error("invalid action {}".format(args.action))
else:
    logging.error("invalid module {}".format(args.module))