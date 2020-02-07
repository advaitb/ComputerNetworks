### Estimating data and bandwidth independent delay

Small data size gives a better estimate of the functional and data independent overhead whereas big data size gives a more accurate representation of the bandwidth of the network. So, our strategy is to sample different data sizes from small to large and see the trend. Ideally calculate the latencies of small and large datasets (average of some range or where we see an inflection point) and then estimate the independent delay from that.
