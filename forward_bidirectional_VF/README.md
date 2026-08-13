# VF eSwitch Forward — DOCA Flow 3.4

โปรเจกต์นี้สร้าง DOCA application ใหม่สำหรับ BlueField-3 ซึ่งรับ VF
representor สองตัว แล้วติดตั้ง hardware flow สองทิศทางลงใน eSwitch:

```text
VF-A (logical port 1) ─────► VF-B (logical port 2)
VF-A (logical port 1) ◄───── VF-B (logical port 2)
```

แอปใช้ DOCA Flow จริง (`libdoca_flow`) ไม่ใช่ `tc`, OVS หรือ
`rte_flow` โดยตรง โค้ดอ้างอิง lifecycle จาก sample
`/opt/mellanox/doca/samples/doca_flow/flow_switch_single` ของ image ที่ใช้ build

> คำเตือน: pipe นี้เป็น root pipe และกำหนด miss เป็น DROP จึงตั้งใจแยก VF
> สองตัวที่เลือกออกจาก traffic อื่น ทดสอบบน VF/เครือข่ายที่แยกจาก management
> network ก่อนเสมอ การหยุด process/container จะ teardown pipe และคืน datapath เดิม

## 1. Library ที่ใช้

| Library | หน้าที่ในโปรเจกต์ |
| --- | --- |
| `doca-common` | error types, logging และ device objects พื้นฐาน |
| `doca-flow` | สร้าง switch ports, pipe, entries, counters และ inject rules ลง hardware |
| `doca-argp` | parse `-r pci/<PF>,pf0vf<N>` และเปิด `doca_dev`/`doca_dev_rep` |
| `doca-dpdk-bridge` | เชื่อม DOCA device กับ DPDK PF proxy port |
| `libdpdk` | EAL, PF proxy queue, hugepage-backed mbuf และ mlx5 PMD |

`doca_rdma_bridge_get_dev_pd()` ถูกใช้ตาม official DOCA 3.4 switch sample เพื่อ
ส่ง protection-domain handle ให้ mlx5 ตอน probe PF proxy โดย dependency นี้ถูกดึง
ผ่านชุด dependency เดียวกับ official sample

DOCA Flow ไม่ได้ใช้ `doca_ctx`/`doca_pe` แบบ task-based libraries ทั่วไป
completion ของการเพิ่ม entry ถูก drain ด้วย `doca_flow_entries_process()` และ
callback ที่อยู่ใน `flow_common.c`

## 2. ไฟล์ที่จำเป็น

```text
forward_bidirectional_VF/
├── include/vf_eswitch.h       # contract ระหว่าง main กับ Flow logic
├── src/main.c                 # log, ARGP, VF validation, DPDK/PF probing
├── src/vf_eswitch.c           # DOCA Flow init, ports, pipe, entries, counters
├── meson.build                # dependency และ compilation manifest
├── Dockerfile                # multi-stage build ด้วย DOCA 3.4 image
├── scripts/
│   ├── check-environment.sh   # read-only preflight
│   └── vendor-doca-helpers.sh # ดึง helper source ที่ตรงกับ DOCA install
└── vendor/doca/               # generated; ไม่แก้ด้วยมือ
```

เหตุผลที่ import helper จาก DOCA install แทนการเขียนใหม่ทั้งหมดคือ device probing
และ port lifecycle เปลี่ยนได้ตาม DOCA release ตัว image/installation ที่กำลัง build
จึงเป็น source of truth

ลำดับอ่านโค้ดแบบปูพื้น:

1. `include/vf_eswitch.h` — กำหนดว่า topology มี 3 logical ports และ 2 entries
2. `src/vf_eswitch.c:create_vm_switching_pipe()` — สร้าง template ของ root pipe
3. `src/vf_eswitch.c:add_forward_entry()` — เติมค่าจริงของแต่ละ direction
4. `src/vf_eswitch.c:vf_eswitch_run()` — init Flow, start ports, commit entries และ teardown
5. `src/main.c` — รับ `-r` สองค่า, probe PF proxy และจัด DPDK lifecycle
6. `meson.build` — รวม source ของเราเข้ากับ helper ที่ตรงกับ DOCA 3.4
7. `Dockerfile` — ทำขั้นตอนเดียวกับ native build ภายใน image ที่กำหนด

## 3. Mapping จาก Flow CLI เป็น C API

Flow CLI เดิม:

```text
create pipe_match port_meta=0xffffffff
create fwd type=port,port_id=0xffff
create pipe port_id=0,name=vm_switching,root_enable=1,fwd=1

create entry_match port_meta=1
create fwd type=port,port_id=2
add entry ...

create entry_match port_meta=2
create fwd type=port,port_id=1
add entry ...
```

C API 3.4 ที่ตรงกัน:

| CLI | C API |
| --- | --- |
| `port_meta=0xffffffff` ที่ pipe | `match.parser_meta.port_id = UINT16_MAX` |
| `type=port,port_id=0xffff` ที่ pipe | `fwd.type = DOCA_FLOW_FWD_PORT; fwd.port_id = UINT16_MAX` |
| `root_enable=1` | `doca_flow_pipe_cfg_set_is_root(cfg, true)` |
| entry `port_meta=1` | `match.parser_meta.port_id = 1` |
| entry forward `port_id=2` | `fwd.port_id = 2` |
| `add entry` | `doca_flow_pipe_basic_add_entry(...)` |

ค่า pipe ID ที่ Flow CLI แสดงเป็น runtime handle จึงไม่เก็บหรือ hard-code ใน C
application ตัวแปร `struct doca_flow_pipe *pipe` คือ handle ที่ใช้จริง

## 4. Lifecycle ของโปรแกรม

```text
ARGP เปิด PF + VF representors
        │
        ▼
DPDK EAL เริ่มโดยไม่ auto-probe mlx5
        │
        ▼
probe เฉพาะ PF proxy ด้วย dv_flow_en=2
        │
        ▼
doca_flow_init("switch,hws")
        │
        ▼
start logical ports: PF=0, VF-A=1, VF-B=2
        │
        ▼
create root BASIC pipe (constructor validation)
        │
        ▼
batch add 1→2 และ 2→1 + process callbacks
        │
        ▼
อ่าน counters จนได้รับ SIGINT/SIGTERM
        │
        ▼
destroy pipe → stop ports → doca_flow_destroy
```

## 5. เตรียม BlueField

รันบน BlueField Arm ไม่ใช่ x86 host:

```bash
uname -m
# aarch64
```

เครื่องนี้ใช้ hugepage 512 MiB ดังนั้นจอง 4 หน้า = 2 GiB:

```bash
echo 4 | sudo tee \
  /sys/kernel/mm/hugepages/hugepages-524288kB/nr_hugepages

sudo mkdir -p /dev/hugepages
mountpoint -q /dev/hugepages || sudo mount -t hugetlbfs \
  -o pagesize=512M nodev /dev/hugepages

grep -E 'HugePages_Total|HugePages_Free|Hugepagesize|Hugetlb' /proc/meminfo
```

ตรวจ PF และ representors:

```bash
lspci -Dnn -d 15b3:
devlink port show
```

นำ repository ไปยัง BlueField หลังจาก commit และ push แล้ว:

```bash
git clone https://github.com/PpoH2233/Sample-DOCA-Application.git
cd Sample-DOCA-Application
cd forward_bidirectional_VF
```

หรือถ้ายังไม่ต้องการ push สามารถคัดลอกจากเครื่องพัฒนาโดยใช้ `scp`/`rsync`
ไปยัง BlueField แล้วเข้า directory `Sample-DOCA-Application/forward_bidirectional_VF`
ก่อน build

## 6. Build แบบ native ใน development container

จาก directory นี้บน BlueField:

```bash
chmod +x scripts/*.sh
./scripts/vendor-doca-helpers.sh
./scripts/check-environment.sh

meson setup build --buildtype=debugoptimized
meson compile -C build
```

ตรวจว่า binary link DOCA Flow จริง:

```bash
ldd build/vf-eswitch-forward | grep -E 'doca_flow|doca_common'
```

## 7. Run แบบ native

VF ต่อเนื่องกัน:

```bash
sudo ./build/vf-eswitch-forward -- \
  -l 60 \
  -r 'pci/03:00.0,pf0vf[0-1]'
```

VF ไม่ต่อเนื่องกัน:

```bash
sudo ./build/vf-eswitch-forward -- \
  -l 60 \
  -r pci/03:00.0,pf0vf0 \
  -r pci/03:00.0,pf0vf7
```

ลำดับ `-r` สำคัญ: ตัวแรกเป็น logical port 1 (VF-A), ตัวที่สองเป็น logical
port 2 (VF-B) แต่ rules ถูกสร้างทั้งสองทิศทางจึง forward ได้เหมือนกัน

## 8. Build container

Docker build context ต้องเป็น directory นี้:

```bash
sudo docker build \
  --no-cache \
  --progress=plain \
  -t vf-eswitch-forward:doca-3.4.0 \
  .
```

ใช้ `--no-cache` หลังแก้ `meson.build` เพื่อไม่ให้ Docker นำ build layer
ที่ล้มเหลวหรือ configuration เก่ากลับมาใช้

Dockerfile ใช้ image เดียวกันทั้ง builder และ runtime เพื่อป้องกัน DOCA/DPDK
library version skew:

```text
nvcr.io/nvidia/doca/doca:devel-cuda13.0.0-3.4.0-devel
```

## 9. Run container

```bash
sudo docker run --rm \
  --name vf-eswitch-forward \
  --privileged \
  --network host \
  --ulimit memlock=-1:-1 \
  --stop-timeout 15 \
  --mount type=bind,src=/dev/hugepages,dst=/dev/hugepages \
  vf-eswitch-forward:doca-3.4.0 \
  -- \
  -l 60 \
  -r 'pci/03:00.0,pf0vf[0-1]'
```

`--privileged` ให้ container เข้าถึง mlx5/RDMA/devlink resources,
`--network host` ทำให้เห็น interfaces/representors ของ DPU และ hugepage mount
ทำให้ DPDK ใช้ pool ของ host ได้

## 10. พิสูจน์ว่า hardware flow ทำงาน

1. รอ log `Installed rule: logical port 1 -> logical port 2`
2. รอ log `Installed rule: logical port 2 -> logical port 1`
3. ส่ง ARP/ping หรือ traffic ที่ควบคุมได้ระหว่าง VF ทั้งสอง
4. ดู counter `VF-A -> VF-B` และ `VF-B -> VF-A` เพิ่มขึ้น
5. ตรวจ linkage ใน container:

```bash
sudo docker exec vf-eswitch-forward \
  ldd /usr/local/bin/vf-eswitch-forward | grep libdoca_flow
```

หยุดด้วย `Ctrl-C` หรือ:

```bash
sudo docker stop vf-eswitch-forward
```

โปรแกรมจะทำ rollback ตามลำดับ: destroy pipe, flush/stop ports และ
`doca_flow_destroy()` หาก counter ไม่เพิ่ม ห้ามเพิ่ม rule อื่นทันที ให้ตรวจลำดับ
VF input, representor ที่เลือก, entry callback และ DOCA/DPDK logs ก่อน
