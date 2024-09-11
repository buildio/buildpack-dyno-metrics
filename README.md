# Dyno Metrics Buildpack

Buildpack providing dyno metrics logging.

### Usage

```bash
pack build sample-app --builder heroku/builder:24 --buildpack .
```

THEN

```
docker run sample-app echo hello
```
or
```
docker run -it sample-app bash
```

### Samples

A web process:
```
2024-01-01T09:03:22.000647+00:00 heroku[web.1]: source=web.1 dyno=heroku.222222222.7f9b14bc-1d3b-47f4-91c1-704964b8793f sample#load_avg_1m=0.00 sample#load_avg_5m=0.00
2024-01-01T09:03:22.012194+00:00 heroku[web.1]: source=web.1 dyno=heroku.222222222.7f9b14bc-1d3b-47f4-91c1-704964b8793f sample#memory_total=34.41MB sample#memory_rss=34.33MB sample#memory_cache=0.08MB sample#memory_swap=0.00MB sample#memory_pgpgin=9pages sample#memory_pgpgout=0pages sample#memory_quota=512.00MB
```

A once-off dyno:
```
2024-01-01T09:03:27.292784+00:00 heroku[run.2660]: source=run.2660 dyno=heroku.222222222.cff8130b-c3ef-4c0c-9569-78e988c85342 sample#load_avg_1m=0.48 sample#load_avg_5m=0.41
2024-01-01T09:03:27.303342+00:00 heroku[run.2660]: source=run.2660 dyno=heroku.222222222.cff8130b-c3ef-4c0c-9569-78e988c85342 sample#memory_total=481.50MB sample#memory_rss=0.66MB sample#memory_cache=480.83MB sample#memory_swap=0.00MB sample#memory_pgpgin=3893pages sample#memory_pgpgout=1500pages sample#memory_quota=512.00MB
2024-01-01T09:03:43.796942+00:00 heroku[web.1]: source=web.1 dyno=heroku.222222222.7f9b14bc-1d3b-47f4-91c1-704964b8793f sample#load_avg_1m=0.00 sample#load_avg_5m=0.00
```

An addon:
```
2024-01-01T09:24:50.000000+00:00 app[heroku-redis]: source=REDIS addon=redis-qqqqqqq-95446 sample#active-connections=2 sample#load-avg-1m=4.3 sample#load-avg-5m=4.42 sample#load-avg-15m=4.385 sample#read-iops=0 sample#write-iops=0 sample#memory-total=16070676kB sample#memory-free=8977260kB sample#memory-cached=3648404kB sample#memory-redis=742392bytes sample#hit-rate=0.015762 sample#evicted-keys=0
```

