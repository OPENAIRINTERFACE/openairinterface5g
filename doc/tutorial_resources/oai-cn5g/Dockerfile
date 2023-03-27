ARG BASE_IMAGE=ubuntu:jammy

FROM $BASE_IMAGE as ims

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
      psmisc \
      git \
      asterisk \
  && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["asterisk", "-fp"]
