#!/bin/bash
# Apache-2.0. Explicit reset to the CloudStack VS6 baseline on this BF3.
# User must have moved VF0 to the host cloudbr0 trunk before invoking.
set -euo pipefail
deploy_source="$(cd "$(dirname "$0")" && pwd)"
test "${1:-}" = --reset-to-vs6
test "$(id -u)" = 0
test -f /var/lib/eswitch-management/eswitch.conf
test -f /var/lib/eswitch-management/eswitch.conf.router
test -x /etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh
grep -qx 'IMAGE=eswitch-management:3.4.0' /etc/eswitch-management-container.conf
deploy_old_image=$(docker image inspect eswitch-management:3.4.0 --format '{{.Id}}')
deploy_new_image=$(docker image inspect eswitch-management:3.4.0-cloudstack --format '{{.Id}}')
install -d -m 0700 /var/backups/eswitch-management
deploy_backup=$(mktemp -d /var/backups/eswitch-management/cloudstack-20261001-XXXXXX)
printf '%s\n' "${deploy_old_image}" > "${deploy_backup}/old-image-id"
printf '%s\n' "${deploy_new_image}" > "${deploy_backup}/new-image-id"
cp -a /etc/eswitch-management-container.conf "${deploy_backup}/service-environment"
cp -a /etc/systemd/system/eswitch-management-container.service "${deploy_backup}/service-unit"
cp -a /usr/local/bin/eswitchctl "${deploy_backup}/eswitchctl"
cp -a /etc/cloudstack/extensions/dpu-eswitch "${deploy_backup}/installed-extension"
/usr/local/bin/eswitchctl status > "${deploy_backup}/status-before"
/usr/local/bin/eswitchctl vs show > "${deploy_backup}/vs-before"
/usr/local/bin/eswitchctl port show > "${deploy_backup}/ports-before"
printf 'backup=%s\n' "${deploy_backup}"
systemctl stop eswitch-management-container.service
cp -a /var/lib/eswitch-management/eswitch.conf "${deploy_backup}/eswitch.conf"
cp -a /var/lib/eswitch-management/eswitch.conf.router "${deploy_backup}/eswitch.conf.router"
deploy_cache_moved=0
rollback() {
    deploy_exit=$?
    trap - ERR
    set +e
    systemctl stop eswitch-management-container.service
    cp -a "${deploy_backup}/eswitch.conf" /var/lib/eswitch-management/eswitch.conf
    cp -a "${deploy_backup}/eswitch.conf.router" /var/lib/eswitch-management/eswitch.conf.router
    cp -a "${deploy_backup}/eswitchctl" /usr/local/bin/eswitchctl
    cp -a "${deploy_backup}/installed-extension/dpu-eswitch-wrapper.sh" /etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh
    if test -f "${deploy_backup}/installed-extension/dpu_eswitch.py"; then
        cp -a "${deploy_backup}/installed-extension/dpu_eswitch.py" /etc/cloudstack/extensions/dpu-eswitch/dpu_eswitch.py
    elif test -f /etc/cloudstack/extensions/dpu-eswitch/dpu_eswitch.py; then
        mv /etc/cloudstack/extensions/dpu-eswitch/dpu_eswitch.py "${deploy_backup}/failed-helper.py"
    fi
    if test "${deploy_cache_moved}" = 1; then
        mv /var/lib/cloudstack/dpu-eswitch "${deploy_backup}/failed-new-cache"
        mv "${deploy_backup}/wrapper-cache" /var/lib/cloudstack/dpu-eswitch
    fi
    docker tag "${deploy_old_image}" eswitch-management:3.4.0
    systemctl start eswitch-management-container.service
    printf 'Deployment failed; prior configuration/image restored. Backup: %s\n' "${deploy_backup}" >&2
    exit "${deploy_exit}"
}
trap rollback ERR
if test -d /var/lib/cloudstack/dpu-eswitch; then
    mv /var/lib/cloudstack/dpu-eswitch "${deploy_backup}/wrapper-cache"
    deploy_cache_moved=1
fi
install -d -m 0750 /var/lib/cloudstack/dpu-eswitch
install -m 0640 "${deploy_source}/cloudstack-eswitch.conf" /var/lib/eswitch-management/eswitch.conf
install -m 0600 "${deploy_source}/cloudstack-eswitch.conf.router" /var/lib/eswitch-management/eswitch.conf.router
install -m 0755 "${deploy_source}/dpu-eswitch-wrapper.sh" /etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh
install -m 0644 "${deploy_source}/dpu_eswitch.py" /etc/cloudstack/extensions/dpu-eswitch/dpu_eswitch.py
docker tag "${deploy_new_image}" eswitch-management:3.4.0
systemctl start eswitch-management-container.service
deploy_ready=0
for deploy_attempt in $(seq 1 45); do
    if docker exec eswitch-management /usr/local/bin/eswitchctl status > "${deploy_backup}/status-after" 2>"${deploy_backup}/startup-probe-error"; then
        deploy_ready=1
        break
    fi
    sleep 1
done
test "${deploy_ready}" = 1
docker cp eswitch-management:/usr/local/bin/eswitchctl /usr/local/bin/eswitchctl
chmod 0755 /usr/local/bin/eswitchctl
/usr/local/bin/eswitchctl vs show --id 6 > "${deploy_backup}/vs-after"
/usr/local/bin/eswitchctl port show --all > "${deploy_backup}/ports-after"
grep -q 'name=VS6' "${deploy_backup}/vs-after"
test "$(/usr/local/bin/eswitchctl vs show | grep -c '^vs=')" = 1
grep -q 'routers=0 ' "${deploy_backup}/status-after"
/etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh ensure-infrastructure > "${deploy_backup}/wrapper-probe"
test "$(docker inspect eswitch-management --format '{{.Image}}')" = "${deploy_new_image}"
trap - ERR
printf 'deployment=success backup=%s image=%s\n' "${deploy_backup}" "${deploy_new_image}"
cat "${deploy_backup}/vs-after"
cat "${deploy_backup}/status-after"
