ARG DOCA_DEVEL_IMAGE=nvcr.io/nvidia/doca/doca:devel-3.4.0
ARG DOCA_RUNTIME_IMAGE=nvcr.io/nvidia/doca/doca:full-rt-3.4.0

FROM ${DOCA_DEVEL_IMAGE} AS builder
ARG VF_SCOPE=all

WORKDIR /workspace
COPY . /workspace

# The Meson project is below application/, but it also compiles shared source
# modules from the repository. Keep the repository root as the Docker context.
RUN pkg-config --modversion doca-common \
 && pkg-config --modversion doca-dpdk-bridge \
 && pkg-config --modversion doca-flow \
 && pkg-config --modversion libdpdk \
 && meson setup /build application/eswitch_management \
      --buildtype=release \
      --prefix=/usr/local \
      -Dvf_scope="${VF_SCOPE}" \
 && meson compile -C /build \
 && DESTDIR=/stage meson install --strip -C /build

FROM ${DOCA_RUNTIME_IMAGE} AS runtime

COPY --from=builder /stage/usr/local/bin/eswitch-management /usr/local/bin/eswitch-management
COPY --from=builder /stage/usr/local/bin/eswitchctl /usr/local/bin/eswitchctl

# Fail the image build if the selected runtime image does not provide every
# shared object required by the application built in the previous stage.
RUN ldd /usr/local/bin/eswitch-management \
 && ! ldd /usr/local/bin/eswitch-management | grep -q "not found" \
 && /usr/local/bin/eswitchctl --help >/dev/null \
 && install -d -m 0755 /run/eswitch-management \
 && install -d -m 0750 /var/lib/eswitch-management

ENV ESWITCH_CONTROL_SOCKET=/run/eswitch-management/control.sock
ENV ESWITCH_STATE_FILE=/var/lib/eswitch-management/eswitch.conf

STOPSIGNAL SIGTERM
HEALTHCHECK --start-period=30s --interval=10s --timeout=3s --retries=3 \
  CMD /usr/local/bin/eswitchctl status >/dev/null || exit 1

ENTRYPOINT ["/usr/local/bin/eswitch-management"]
