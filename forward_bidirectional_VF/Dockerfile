ARG DOCA_IMAGE=nvcr.io/nvidia/doca/doca:devel-cuda13.0.0-3.4.0-devel

FROM ${DOCA_IMAGE} AS builder

WORKDIR /src
COPY . /src

RUN chmod +x scripts/vendor-doca-helpers.sh \
 && scripts/vendor-doca-helpers.sh \
 && pkg-config --modversion doca-common \
 && pkg-config --modversion doca-flow \
 && meson setup build --buildtype=release \
 && meson compile -C build \
 && DESTDIR=/out meson install -C build

FROM ${DOCA_IMAGE}

COPY --from=builder /out/usr/local/bin/vf-eswitch-forward /usr/local/bin/vf-eswitch-forward

STOPSIGNAL SIGTERM
ENTRYPOINT ["/usr/local/bin/vf-eswitch-forward"]
