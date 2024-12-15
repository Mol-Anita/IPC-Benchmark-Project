#!/bin/bash

RAW_RESULTS_FILE="raw_results_server_client.txt"
RUNS_COUNT=1000
RESULTS_JSON="results.json"
ROUNDTRIP_NUMBER=100

get_cnt_avg_max_min_dev() {
    COUNT=0
    SUM=0
    SUMSQR=0
    MAX=0
    MIN=10000000

    while read -r N; do
        (( COUNT++ ))
        SUM=$(bc <<<"$SUM + $N")
        SUMSQR=$(bc <<<"$SUMSQR + ($N * $N)")
        MAX=$(bc <<<"if ($MAX > $N) $MAX else $N")
        MIN=$(bc <<<"if ($MIN < $N) $MIN else $N")
    done

    if [ "$COUNT" -eq 0 ]; then
        echo "0, 0, 0, 0, 0"
        return
    fi

    AVG=$(bc <<<"scale=9; $SUM / $COUNT")
    VAR=$(bc <<<"scale=9; ($SUMSQR - $COUNT * $AVG * $AVG) / $COUNT")
    STDDEV=$(bc <<<"scale=9; sqrt($VAR)")
    echo "$COUNT 0$AVG 0$MAX 0$MIN 0$STDDEV"
}

> $RAW_RESULTS_FILE
> $RESULTS_JSON

echo "{" > $RESULTS_JSON

FIRST_SERVER=true

for SERVER_TYPE in tcp_server fifo_server usocket_server msgqueue_server; do
    CLIENT_TYPE="${SERVER_TYPE/_server/_client}"
    $FIRST_SERVER || echo ',' >> $RESULTS_JSON
    FIRST_SERVER=false

    echo "  \"$SERVER_TYPE\": {" >> $RESULTS_JSON
    FIRST_FILE=true

    # Iterate over each data file and its size
    for DATA_FILE in data/1kb.bin data/8kb.bin data/16kb.bin data/32kb.bin; do
        $FIRST_FILE || echo ',' >> $RESULTS_JSON
        FIRST_FILE=false

        case "$DATA_FILE" in
            data/1kb.bin) DATA_SIZE=1024;;
            data/8kb.bin) DATA_SIZE=8192;;
            data/16kb.bin) DATA_SIZE=16384;;
            data/32kb.bin) DATA_SIZE=32768;;
        esac
        echo "\"$SERVER_TYPE\" \"$CLIENT_TYPE\" \"$DATA_SIZE\""
        # Run the server in the background
        ./$SERVER_TYPE $DATA_SIZE &
        SERVER_PID=$!
        sleep 1

        for ((i=0; i<RUNS_COUNT; i++)); do
            ./$CLIENT_TYPE $DATA_FILE $DATA_SIZE $ROUNDTRIP_NUMBER >> $RAW_RESULTS_FILE
        done

        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null

        VALUES=$(tail -n $RUNS_COUNT $RAW_RESULTS_FILE | get_cnt_avg_max_min_dev | sed -e 's/ /, /g')

        if [ -z "$VALUES" ]; then
            VALUES="0, 0, 0, 0, 0"
        fi

        if [ -f "$RESULTS_JSON" ] && [ -s "$RESULTS_JSON" ]; then
            EXISTING_VALUES=$(grep -oP "\"$SERVER_TYPE\":\s*{[^}]*\"$DATA_SIZE\":\s*\[[^]]*\]" $RESULTS_JSON | sed 's/^[^[]*\[\([^]]*\)\].*/\1/')

            if [ -n "$EXISTING_VALUES" ]; then
                read -r OLD_COUNT OLD_AVG OLD_MAX OLD_MIN OLD_STDDEV <<<$(echo "$EXISTING_VALUES" | sed 's/,/ /g')

                read -r NEW_COUNT NEW_AVG NEW_MAX NEW_MIN NEW_STDDEV <<<$(echo "$VALUES" | sed 's/,/ /g')

                AVG=$(bc <<<"scale=9; ($OLD_AVG + $NEW_AVG) / 2")
                MAX=$(bc <<<"if ($OLD_MAX > $NEW_MAX) $OLD_MAX else $NEW_MAX")
                MIN=$(bc <<<"if ($OLD_MIN < $NEW_MIN) $OLD_MIN else $NEW_MIN")
                STDDEV=$(bc <<<"scale=9; ($OLD_STDDEV + $NEW_STDDEV) / 2")

                VALUES="$NEW_COUNT, $AVG, $MAX, $MIN, $STDDEV"
            fi
        fi

        echo -n "    \"$DATA_SIZE\": [ $VALUES ]" >> $RESULTS_JSON
    done

    echo "  }" >> $RESULTS_JSON
done

echo "}" >> $RESULTS_JSON
