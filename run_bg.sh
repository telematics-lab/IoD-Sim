#!/bin/bash

nohup ./ns3/ns3 run "iodsim --config=../scenario/$1.json" &