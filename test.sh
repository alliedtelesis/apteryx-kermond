#!/bin/bash
TESTS=$1

if [ "$( docker container inspect -f '{{.State.Status}}' apteryx-kermond )" = "running" ]; then
    if [ ! -z $TESTS ]; then
        docker exec apteryx-kermond pytest-3 -vv -k $TESTS
    else
        docker exec apteryx-kermond pytest-3 -vv
    fi
else
    echo "ERROR: apteryx-kermond not running"
fi
