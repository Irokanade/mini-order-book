# mini-order-book

Benchmarked against data from LOBSTER

## Sample run on my laptop
```
loaded 624040 messages
processed:   618605 (skipped 3559 hidden/halt, rejected 1876 unresolvable)
wall time:   34.049 ms
throughput:  18168126 msgs/sec
latency p50: 41 ns
latency p90: 42 ns
latency p99: 84 ns
latency p999: 167 ns
latency max: 21500 ns
```

## Configure
```shell
cmake -S . -B build
```

## Build
```shell
cmake --build build
```

## Run 
```shell
./build/mini_order_book data/INTC_2012-06-21_message_10.csv
```
