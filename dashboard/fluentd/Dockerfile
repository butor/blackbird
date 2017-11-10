FROM fluent/fluentd

# for fluentd v0.12 or later
RUN fluent-gem install fluent-plugin-influxdb -v "~> 0.3"  --no-document
# for fluentd v0.14 or later
# RUN fluent-gem install fluent-plugin-influxdb -v 1.0.0.rc1 --no-document # for fluentd v0.14 or later

RUN fluent-gem install fluent-plugin-flatten-hash -v "< 0.5.0" --no-document
