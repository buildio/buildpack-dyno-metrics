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
