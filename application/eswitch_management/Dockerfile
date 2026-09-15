ARG DOCA_DEVEL_IMAGE=nvcr.io/nvidia/doca/doca:devel-3.4.0
ARG DOCA_RUNTIME_IMAGE=nvcr.io/nvidia/doca/doca:full-rt-3.4.0
ARG DOCA_PKG_VERSION=3.4.0112

FROM ${DOCA_DEVEL_IMAGE} AS builder
ARG DOCA_PKG_VERSION
ARG VF_SCOPE=0-10

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

WORKDIR /workspace
COPY . /workspace

# The Meson project is below application/, but it also compiles shared source
# modules from the repository. Keep the repository root as the Docker context.
RUN set -eux; \
    for package in doca-common doca-dpdk-bridge doca-flow; do \
      installed_version="$(pkg-config --modversion "${package}")"; \
      test "${installed_version}" = "${DOCA_PKG_VERSION}" || { \
        echo "${package}: expected ${DOCA_PKG_VERSION}, found ${installed_version}" >&2; \
        exit 1; \
      }; \
    done; \
    pkg-config --modversion libdpdk; \
    meson setup /build application/eswitch_management \
      --buildtype=release \
      --prefix=/usr/local \
      -Dvf_scope="${VF_SCOPE}"; \
    meson compile -C /build; \
    meson test -C /build --print-errorlogs; \
    DESTDIR=/stage meson install --strip -C /build

FROM ${DOCA_RUNTIME_IMAGE} AS runtime

ARG DOCA_PKG_VERSION

LABEL org.opencontainers.image.title="BlueField eSwitch Management" \
      com.nvidia.doca.version="${DOCA_PKG_VERSION}" \
      com.nvidia.doca.fast-path="DOCA Flow HWS LPM and CT"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

COPY --from=builder /stage/usr/local/bin/eswitch-management /usr/local/bin/eswitch-management
COPY --from=builder /stage/usr/local/bin/eswitchctl /usr/local/bin/eswitchctl

# Fail the image build if the selected runtime image does not provide every
# shared object required by the application built in the previous stage.
RUN set -eux; \
    dependencies="$(ldd /usr/local/bin/eswitch-management)"; \
    printf '%s\n' "${dependencies}"; \
    ! grep -q "not found" <<<"${dependencies}"; \
    grep -q "libdoca_flow" <<<"${dependencies}"; \
    /usr/local/bin/eswitchctl --help >/dev/null; \
    install -d -m 0755 /run/eswitch-management; \
    install -d -m 0750 /var/lib/eswitch-management

ENV ESWITCH_CONTROL_SOCKET=/run/eswitch-management/control.sock
ENV ESWITCH_STATE_FILE=/var/lib/eswitch-management/eswitch.conf
ENV ESWITCH_SF_IFACE=enp3s0f0s0
ENV ESWITCH_VF_SCOPE=0-10
ENV ESWITCH_HW_ROUTING=1
ENV ESWITCH_HW_ROUTE_CAPACITY=256
ENV ESWITCH_HW_CT=1
ENV ESWITCH_HW_CT_CAPACITY=4096
ENV ESWITCH_UPLINK_ARP_PPS=256
ENV ESWITCH_UPLINK_ARP_BURST=64
ENV ESWITCH_PACKET_DEBUG=0

STOPSIGNAL SIGTERM
HEALTHCHECK --start-period=30s --interval=10s --timeout=3s --retries=3 \
  CMD /usr/local/bin/eswitchctl status >/dev/null || exit 1

ENTRYPOINT ["/usr/local/bin/eswitch-management"]
