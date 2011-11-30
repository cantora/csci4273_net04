make start_node && for (( i=1; i <= $1; i++)); do CMD="bash ./start_node_and_block.sh $i $2 $3"; echo $CMD; screen $CMD  ; done
