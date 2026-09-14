# mini-order-book

Header only limit order book built in C++  
Benchmarked against data from [LOBSTER](https://lobsterdata.com/)

## Data
The benchmark uses a LOBSTER message file: Intel (INTC), NASDAQ, 21 June 2012,
10 price levels.

1. Request the sample data at https://lobsterdata.com/book-sample
   (LOBSTER approves requests, and may ask for proof of purchase of their book).
2. Accept the [Trial and Sample Data Terms](https://lobsterdata.com/terms/trial-and-sample-data)
   and download the archive before the link expires.
3. Unzip it and copy the file to `data/INTC_2012-06-21_message_10.csv`.

## Sample run
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
