#!/bin/bash

limit=1
while true
do
  limit=`echo 1+$limit|bc`
  a=' '$a
  echo $limit
done
