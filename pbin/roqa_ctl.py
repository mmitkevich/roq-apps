#!/bin/env python3
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
parser.add_argument("--config_file", type=str, help="config_file")
global args
args = parser.parse_args()

SERVER="localhost:9402"
BASE_URL = "http://{}".format(SERVER)
PARAMETERS_URL = "{}/api/parameters".format(BASE_URL)

def unpack_parameters(p, key="symbol", value="value"):
    #logging.debug("p={}".format(p))
    if key in p and isinstance(p[key],list):
        for i, k in enumerate(p[key]):
            p_2 = p.copy()
            p_2[key] = k
            if isinstance(p[value], list):
                p_2[value] = p[value][min(i,len(p[value])-1)]
            else:
                p_2[value] = p[value]
            yield  p_2
    else:
        yield p

def get_parameters(args):
    if args.label:
        url = "{}/{}?user={}".format(PARAMETERS_URL,args.label,args.user)
    else:
        url = "{}?user={}".format(PARAMETERS_URL,args.user)
    logging.debug("parameters get label={} user={}".format(args.label, args.user))            
    resp = requests.get(url)
    logging.debug("parameters get result {} <<{}>>".format(resp.status_code, resp.json()))
    if not args.label:
        dfs = []
        for label in resp.json():
            args_2 = vars(args)
            args_2["label"] = label
            df = get_parameters(argparse.Namespace(**args_2))
            dfs.append(df)
        return pd.concat(dfs,axis=0) if len(dfs)>0 else None
    else:
        df = pd.DataFrame(data=resp.json())
        return df

def put_parameters(args):
    KEYS={"label","symbol","exchange","account","strategy_id","value"}
    data = {k:v for k,v in vars(args).items() if k in KEYS and v is not None}        
    url = "{}?user={}".format(PARAMETERS_URL,args.user)
    logging.info("parameters put user={} data=<<{}>>".format(args.user, data))
    resp = requests.put(url, data=json.dumps(data))
    logging.debug("parameters put result {}".format(resp.status_code))

def import_parameters(args):
    import tomli
    plist=[]
    with open(args.config_file, "rb") as f:
        data = tomli.load(f)
    for ps in data["parameter"]:
        for p in unpack_parameters(ps):
            logging.info("p={}".format(p))
            args_2 = vars(args)
            if 'strategy' in p:
                p['strategy_id']=int(p['strategy'])
            args_2.update(p)
            put_parameters(argparse.Namespace(**args_2))

if args.module=="parameters":
    if args.action=="get":
        df = get_parameters(args)
        print(df)
    elif args.action=="put":
        put_parameters(args)
    elif args.action=="import":
       import_parameters(args)
    else:
        logging.error("invalid action {}".format(args.action))
else:
    logging.error("invalid module {}".format(args.module))