#!/bin/bash
echo "Making sure no previous replicas are up..."
killall skvbc_replica

echo "Running replica 1..."
../TesterReplica/skvbc_replica -k setA_replica -f 1 -c 0 -i 0 >& /dev/null &
echo "Running replica 2..."
../TesterReplica/skvbc_replica -k setA_replica -f 1 -c 0 -i 1 >& /dev/null &
echo "Running replica 3..."
../TesterReplica/skvbc_replica -k setA_replica -f 1 -c 0 -i 2 >& /dev/null &
echo "Running replica 4..."
../TesterReplica/skvbc_replica -k setA_replica -f 1 -c 0 -i 3 >& /dev/null &
