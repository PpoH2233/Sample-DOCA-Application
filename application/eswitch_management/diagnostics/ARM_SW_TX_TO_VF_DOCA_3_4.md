# ARM Software TX to a VF with DOCA Flow 3.4

เอกสารนี้เป็น handoff สำหรับ Codex ที่รันอยู่บน BlueField-3 เพื่อสร้าง packet
บน Arm แล้วส่งออกไปยัง VF ผ่าน eSwitch โดยอ้างอิงพฤติกรรมของ DOCA Flow 3.4
เท่านั้น

เป้าหมายแรกคือพิสูจน์เส้นทาง Software TX แบบเล็กที่สุดก่อนนำกลับไปรวมกับ
ARP responder ใน `eswitch-management`:

```text
Arm application
    -> fresh rte_mbuf
    -> rte_eth_tx_burst(proxy/parent DPDK port)
    -> eSwitch dispatch
    -> target VF representor
    -> VM interface
```

## NVIDIA documentation evidence

ข้อสรุปว่า application บน BlueField Arm สามารถ inject packet กลับเข้า eSwitch
ไม่ได้มาจากการอนุมานเรื่อง ownership เพียงอย่างเดียว แต่ประกอบจาก contract ที่
NVIDIA ระบุไว้สองส่วนในเอกสาร archive 3.4:

### 1. Arm/ECPF เป็นผู้ควบคุม eSwitch ใน DPU mode

[BlueField Modes of Operation — DPU Mode](https://networking-docs.nvidia.com/doca/archive/3-4-0/bluefield-modes-of-operation#dpu-mode)
ระบุว่า:

- NIC resources และ data path ถูกควบคุมโดย embedded Arm subsystem
- ECPF controls the NIC embedded switch
- traffic ระหว่าง host interface กับ network ผ่าน representors บน Arm ใน
  slow path
- virtual switch บน Arm สามารถติดตั้ง rules ลง eSwitch เพื่อสร้าง fast path

ดังนั้นคำว่า “BlueField เป็นเจ้าของ eSwitch” ในบริบทนี้ต้องหมายถึง BlueField
กำลังทำงานใน **DPU mode / ECPF ownership mode** ไม่ใช่ NIC mode

### 2. Software บน Arm สามารถส่ง packet กลับเข้า hardware/eSwitch ได้

[DOCA Flow 3.4 — Architecture and Steering Domains](https://networking-docs.nvidia.com/doca/archive/3-4-0/doca-flow#steering-domains)
ระบุโดยตรงว่า packet ที่ถูกส่งไปยัง Arm เพื่อ exception handling สามารถ
reinjected กลับเข้า hardware ได้ และใน switch mode:

- Software TX เข้า EGRESS root pipe อัตโนมัติ
- packet ที่ส่งด้วย `rte_eth_tx_burst()` คือ Software TX ที่เข้า EGRESS root
- slow-path model `SP2` ถูกนิยามเป็น
  `DEFAULT -> RSS -> SW -> Tx -> EGRESS(root) -> fwd_port -> VF`
- `parser_meta.port_id == UINT16_MAX` ใช้แยก software-originated traffic ที่
  ส่งโดย application ออกจาก hardware-forwarded traffic

นี่คือหลักฐานที่ตรงกับ use case การสร้าง ARP reply บน Arm:

```text
BlueField DPU mode / ECPF owns eSwitch
    -> DOCA application runs on Arm
    -> application allocates and fills an rte_mbuf
    -> rte_eth_tx_burst() performs Software TX
    -> packet re-enters the eSwitch at EGRESS root
    -> DOCA_FLOW_FWD_PORT selects the VF representor
    -> packet reaches the host VF/VM
```

Ownership เป็นเงื่อนไขที่ทำให้ Arm ควบคุม NIC/eSwitch แต่ API contract ที่
ยืนยันการ inject packet จริงคือ Software-TX behavior ของ DOCA Flow ไม่ควรใช้
ownership เพียงอย่างเดียวเป็นหลักฐานว่า TX implementation ถูกต้อง

### NVIDIA implementation references shipped with DOCA

นอกจากข้อความใน Programming Guide แล้ว NVIDIA ยังมี source ที่แสดงกลไก
reinjection จริง ให้ตรวจไฟล์จาก DOCA 3.4 installation บน BlueField:

```bash
rg -n 'handle_arp|response_pkt|rte_eth_tx_burst|DYNFIELD|TX_METADATA' \
  /opt/mellanox/doca/applications/psp_gateway \
  /opt/mellanox/doca/samples/doca_flow/flow_switch_to_wire
```

Reference แรกคือ `psp_gateway/psp_gw_pkt_rss.cpp` ฟังก์ชัน `handle_arp()`:

1. allocate `response_pkt` จาก DPDK mempool
2. ใส่ TX metadata flag ลง dynamic mbuf field
3. กำหนด `data_len` และ `pkt_len`
4. สร้าง Ethernet + ARP reply header ใน mbuf
5. เรียก `rte_eth_tx_burst(port_id, queue_id, &response_pkt, 1)`

นี่เป็นตัวอย่าง NVIDIA ที่ตรงกับ use case gateway ARP reply มากที่สุด

Reference ที่สองคือ `flow_switch_to_wire_sample.c` ฟังก์ชัน
`handle_rx_tx_pkts()`:

1. รับ packet จาก proxy port ด้วย `rte_eth_rx_burst()`
2. เลือก destination logical port
3. เรียก `rte_flow_dynf_metadata_set(mbuf, dst_port)`
4. เปิด `RTE_MBUF_DYNFLAG_TX_METADATA`
5. reinject ด้วย `rte_eth_tx_burst()` บน proxy port

ต้องตรวจ source ที่ติดตั้งอยู่จริงก่อน copy เนื่องจากชื่อ dynamic flag และ
signature อาจเปลี่ยนตาม DPDK build ที่ bundle มากับ DOCA 3.4

### Verify the ownership condition

ตรวจบน BlueField ก่อนทดสอบ dataplane:

```bash
sudo mlxconfig -d 03:00.0 q | \
  rg 'INTERNAL_CPU_(MODEL|ESWITCH_MANAGER|OFFLOAD_ENGINE)'
```

ผลต้องสอดคล้องกับ DPU/ECPF ownership ตาม firmware รุ่นที่ติดตั้งอยู่ หากระบบ
อยู่ใน NIC mode จะใช้เหตุผลและเส้นทาง Arm-owned eSwitch ตามเอกสารส่วนนี้ไม่ได้

สำหรับเครื่องปัจจุบัน ค่า ECPF ownership ถูกตั้งเป็น Next Boot แล้ว แต่ยังไม่
ถือว่า prerequisite ผ่านจนกว่าจะ full power cycle และคอลัมน์ Current แสดง:

```text
INTERNAL_CPU_PAGE_SUPPLIER   ECPF
INTERNAL_CPU_ESWITCH_MANAGER ECPF
INTERNAL_CPU_IB_VPORT0       ECPF
INTERNAL_CPU_OFFLOAD_ENGINE  ENABLED
```

## ข้อสรุปจากเอกสาร DOCA Flow 3.4

เอกสาร [DOCA Flow 3.4](https://networking-docs.nvidia.com/doca/archive/3-4-0/doca-flow)
ระบุพฤติกรรมที่เกี่ยวข้องดังนี้:

1. ใน switch mode การจัดการ pipe และ entry ต้องทำผ่าน **switch manager
   port** ซึ่งหาได้จาก `doca_flow_port_switch_get()`
2. ห้ามเรียก `rte_eth_dev_configure()`, `rte_eth_rx_queue_setup()` หรือ
   `rte_eth_dev_start()` กับ VF/SF representor; DPDK RX/TX queue ของ slow
   path ต้องอยู่บน proxy/parent DPDK port
3. packet ที่ส่งด้วย `rte_eth_tx_burst()` ถือเป็น Software TX
4. Software TX จะเข้า egress direction ของ eSwitch
5. `parser_meta.port_id == UINT16_MAX` ใช้ระบุ packet ที่มาจาก Software TX
   และมีประโยชน์สำหรับ match ใน EGRESS root pipe
6. `doca_flow_fwd.port_id` คือ logical port ID ของปลายทางที่ต้องการ forward
7. switch mode แบบ **ไม่ใส่ `expert`** รองรับการใส่ destination port ID ลงใน
   mbuf metadata แล้วส่งตรงไปยัง port นั้น
8. switch mode แบบ **ใส่ `expert`** ไม่ทำ port-ID handling ให้อัตโนมัติ;
   application ต้องสร้าง EGRESS pipeline สำหรับ dispatch เอง
9. มี EGRESS root ได้เพียงหนึ่ง pipe ใน egress root slot

ดังนั้นต้องทดสอบสองโมเดลแยกกัน ห้ามผสม semantics ของ non-expert กับ expert
ในรอบทดสอบเดียว

## Source of truth บน BlueField

ก่อนเขียนโค้ด ให้ตรวจ version, installed headers และ sample ที่ติดตั้งอยู่จริง:

```bash
pkg-config --modversion doca-flow
ldd /build/eswitch-management/eswitch-management | rg 'libdoca_flow|librte_ethdev|librte_mbuf'

rg -n 'rte_flow_dynf_metadata_set|RTE_MBUF_DYNFLAG_TX_METADATA|rte_eth_tx_burst' \
  /opt/mellanox/doca/samples/doca_flow/flow_switch_to_wire

rg -n 'DOCA_FLOW_PIPE_DOMAIN_EGRESS|parser_meta.port_id|doca_flow_port_switch_get' \
  /opt/mellanox/doca/samples/doca_flow/flow_switch_to_wire

rg -n 'rte_flow_dynf_metadata_register|rte_flow_dynf_metadata_set|RTE_MBUF_DYNFLAG_TX_METADATA' \
  /opt/mellanox/dpdk/include /opt/mellanox/doca/include
```

Expected DOCA version for this test is `3.4.0112`. The installed header and
installed `flow_switch_to_wire` sample are authoritative. If an API signature
differs from the snippets below, copy the signature and call sequence from the
installed sample instead of guessing.

## Port model

Keep these three IDs separate:

| Name | Meaning | Used by |
| --- | --- | --- |
| parent/proxy DPDK port ID | DPDK ethdev that owns the Arm RX/TX queues | `rte_eth_rx_burst()` and `rte_eth_tx_burst()` |
| switch manager Flow port | unified eSwitch Flow object | pipe creation, entry addition and `doca_flow_entries_process()` |
| target representor logical port ID | destination VF representor | mbuf TX metadata in non-expert mode, or `doca_flow_fwd.port_id` in expert mode |

Never assume that VF10 is DPDK port 1, 2, or 4. Discover and log the mapping at
runtime, for example:

```text
PORT MAP: parent=<id> target=<id> host=1 pf=0 vf=10
```

Send the packet with:

```c
rte_eth_tx_burst(parent_dpdk_port_id, tx_queue_id, &mbuf, 1);
```

Do **not** send it with `rte_eth_tx_burst(target_representor_port_id, ...)`.

## Common packet construction

Use a fresh mbuf from the mempool associated with the parent/proxy TX queue.
For an Ethernet ARP reply, create a 60-byte frame excluding FCS:

```c
struct rte_mbuf *m = rte_pktmbuf_alloc(parent_mbuf_pool);
if (m == NULL)
    return DOCA_ERROR_NO_MEMORY;

uint8_t *frame = rte_pktmbuf_append(m, 60);
if (frame == NULL) {
    rte_pktmbuf_free(m);
    return DOCA_ERROR_NO_MEMORY;
}

memset(frame, 0, 60);

/* Ethernet */
memcpy(frame + 0, vm_mac, 6);       /* destination */
memcpy(frame + 6, gateway_mac, 6);  /* source */
frame[12] = 0x08;
frame[13] = 0x06;

/* ARP: Ethernet + IPv4, reply */
frame[14] = 0x00; frame[15] = 0x01; /* htype Ethernet */
frame[16] = 0x08; frame[17] = 0x00; /* ptype IPv4 */
frame[18] = 6;
frame[19] = 4;
frame[20] = 0x00; frame[21] = 0x02; /* opcode reply */
memcpy(frame + 22, gateway_mac, 6);
memcpy(frame + 28, gateway_ip_be_bytes, 4);
memcpy(frame + 32, vm_mac, 6);
memcpy(frame + 38, vm_ip_be_bytes, 4);

m->ol_flags = 0;
```

The expected frame for the current test tuple is:

```text
dst MAC       7e:83:a5:77:11:06
src MAC       02:00:00:65:00:01
EtherType     0x0806
ARP opcode    2
sender MAC    02:00:00:65:00:01
sender IP     192.168.0.1
target MAC    7e:83:a5:77:11:06
target IP     192.168.0.10
frame length  60 bytes without FCS
```

After `rte_eth_tx_burst()` returns `1`, ownership of the mbuf has transferred
to the PMD. Do not access or free it. Free it only when the return value is `0`.

## Experiment A: non-expert direct destination metadata

Run this experiment first because it follows the software reinjection sequence
shown by the installed `flow_switch_to_wire` sample and does not depend on a
custom EGRESS dispatch rule.

### Flow initialization

Use:

```text
switch,hws,hairpinq_num=4
```

Do not include `expert`.

Register DPDK dynamic Flow metadata before using the mbuf metadata field:

```c
int rc = rte_flow_dynf_metadata_register();
if (rc < 0)
    return DOCA_ERROR_DRIVER;
```

Only configure/start DPDK queues on the parent/proxy ethdev. Start representors
through `doca_flow_port_start()` with their `doca_dev_rep`; do not initialize
their DPDK RX/TX queues.

### Software TX

After building the packet, set the destination representor logical port ID in
the mbuf exactly as the installed sample does:

```c
rte_flow_dynf_metadata_set(m, target_representor_port_id);
m->ol_flags |= RTE_MBUF_DYNFLAG_TX_METADATA;

uint16_t accepted = rte_eth_tx_burst(parent_dpdk_port_id, 0, &m, 1);
if (accepted == 0)
    rte_pktmbuf_free(m);
```

For this experiment, the acceptance criterion is packet visibility on the VM,
not a custom EGRESS entry counter. The documented non-expert mechanism may use
the destination metadata to dispatch the packet through internal switch rules.

### Expected path

```text
fresh mbuf on Arm
    -> TX destination metadata = target representor logical port ID
    -> rte_eth_tx_burst(parent/proxy port)
    -> internal non-expert destination-port handling
    -> VF
    -> VM
```

## Experiment B: expert mode through an EGRESS root pipe

Run this only after Experiment A has a recorded result. This experiment tests
the custom pipeline model needed by the current `eswitch-management` design.

### Flow initialization

Use:

```text
switch,hws,hairpinq_num=4,expert
```

Do not put destination-port metadata on the TX mbuf in this experiment:

```c
m->ol_flags = 0;
```

### Minimal EGRESS pipeline

Create the pipe on the switch manager Flow port returned by:

```c
struct doca_flow_port *sw_port = doca_flow_port_switch_get(started_parent_flow_port);
```

Use one root EGRESS BASIC pipe with one entry:

```text
pipe name:       SW_TX_EGRESS_ROOT
root:            true
domain:          DOCA_FLOW_PIPE_DOMAIN_EGRESS
match template:  parser_meta.port_id is changeable
forward type:    DOCA_FLOW_FWD_PORT
forward port:    changeable per entry
miss:            DROP

entry match:     parser_meta.port_id = UINT16_MAX
entry forward:   target representor logical port ID
entry monitor:   non-shared counter
```

The significant value is `UINT16_MAX`: DOCA Flow 3.4 documents this
`parser_meta.port_id` value as software-originated traffic sent by
`rte_eth_tx_burst()`.

Copy the exact pipe/entry call signatures from the installed 3.4 sample or
installed headers. The intended structure is:

```c
struct doca_flow_match pipe_match = {0};
struct doca_flow_fwd pipe_fwd = {0};
struct doca_flow_fwd pipe_miss = {.type = DOCA_FLOW_FWD_DROP};

pipe_match.parser_meta.port_id = UINT16_MAX; /* changeable in template */
pipe_fwd.type = DOCA_FLOW_FWD_PORT;
pipe_fwd.port_id = UINT16_MAX;               /* changeable in template */

/* cfg is created on sw_port, marked root, and assigned EGRESS domain. */
```

Then add the concrete entry:

```c
struct doca_flow_match entry_match = {0};
struct doca_flow_fwd entry_fwd = {0};

entry_match.parser_meta.port_id = UINT16_MAX; /* SW Tx origin */
entry_fwd.type = DOCA_FLOW_FWD_PORT;
entry_fwd.port_id = target_representor_port_id;

/* Add a non-shared counter and commit using the installed 3.4
 * doca_flow_pipe_basic_add_entry() signature. */
```

Call `doca_flow_entries_process(sw_port, ...)` and require the entry callback to
report `DOCA_FLOW_ENTRY_STATUS_SUCCESS` before transmitting. A non-negative
return from `doca_flow_entries_process()` is a processed-entry count; zero is
not automatically an error.

### Expected path

```text
fresh mbuf on Arm, no destination metadata
    -> rte_eth_tx_burst(parent/proxy port)
    -> EGRESS root
    -> match parser_meta.port_id == UINT16_MAX
    -> DOCA_FLOW_FWD_PORT(target representor)
    -> VF
    -> VM
```

Both of the following must become true:

```text
SW_TX_EGRESS_ROOT entry counter increments
VM tcpdump receives the generated ARP reply
```

## Test procedure

The PF can be owned by only one DOCA/DPDK primary process. Stop the production
daemon before starting a standalone reproducer. A different Unix socket or
`--file-prefix` does not isolate ownership of PCI function `03:00.0`.

### On the VM

No script is required. Start capture before launching Software TX:

```bash
sudo tcpdump -eni ens6 -nn -vvv \
  'arp and ether src 02:00:00:65:00:01 and ether dst 7e:83:a5:77:11:06'
```

An unsolicited ARP reply is sufficient for the delivery test. The VM does not
need to run `ping` or `arping` if the reproducer sends packets on a timer.

### On the BlueField Arm/container

Run only one experiment per process invocation. Log at least:

```text
DOCA version and Flow mode
parent/proxy DPDK port ID
target VF identity and target logical port ID
switch manager pointer/availability
metadata registration result
EGRESS entry callback status
mbuf length, data length, segment count and ol_flags
complete first packet hex dump
rte_eth_tx_burst accepted count
parent ethdev opackets/oerrors delta
EGRESS entry packet/byte counter delta (Experiment B)
```

Use a unique DPDK file prefix for the standalone process:

```bash
./sw-tx-to-vf-test \
  -l 0 --file-prefix=sw-tx-to-vf-test -- \
  --device 03:00.0 \
  --rep 'pci/03:00.0,c1pf0vf10' \
  --target-vf 10 \
  --vm-mac 7e:83:a5:77:11:06 \
  --gateway-mac 02:00:00:65:00:01 \
  --vm-ip 192.168.0.10 \
  --gateway-ip 192.168.0.1 \
  --tx-count 10 \
  > /tmp/sw-tx-to-vf-test.log 2>&1
```

The exact application options above are a proposed interface for the test
program, not DOCA-provided arguments. If adapting the existing reproducer, map
them to its current environment variables instead.

## Result matrix

| Result | Meaning | Next action |
| --- | --- | --- |
| Experiment A reaches VM | Parent TX queue and non-expert destination metadata are correct | use this model for immediate ARP delivery, or continue B if custom egress processing is required |
| A accepted by DPDK but VM sees nothing | problem is below/custom-independent of EGRESS root | verify proxy TX port, metadata flag, target logical port mapping and installed sample parity |
| A works, B has EGRESS counter 0 | expert EGRESS root does not recognize the SW Tx tuple | inspect `parser_meta.port_id`, root-slot ownership, mode string and actual programmed rules |
| B counter increments but VM sees nothing | SW Tx entered EGRESS; failure is after the entry | verify `doca_flow_fwd.port_id`, representor identity and VM/VF mapping |
| B counter and VM capture both succeed | expert software-reinjection path is proven | port the minimal sequence into `eswitch-management` |

`rte_eth_tx_burst() == 1` and increasing parent `opackets` only prove that the
PMD accepted the packet. They do not prove entry into the EGRESS root or receipt
by the VM.

## Instructions for the BlueField Codex

1. Do not modify the production ARP responder first. Extend the standalone TX
   reproducer or create a minimal isolated reproducer.
2. Inspect `/opt/mellanox/doca/samples/doca_flow/flow_switch_to_wire` and the
   installed headers before adding any API call.
3. Implement Experiment A and Experiment B as separate runtime modes.
4. Use the discovered runtime mapping for VF10; do not hard-code its DPDK port.
5. In both modes, transmit only through the parent/proxy DPDK TX queue.
6. Use a fresh 60-byte mbuf for every packet and do not free accepted mbufs.
7. Attach a counter to the Experiment B EGRESS entry and query it after TX.
8. Report both raw logs and a compact summary containing:

```text
mode=<non-expert-meta|expert-egress>
parent=<id>
target=<id>
accepted=<n>
parent_opackets_delta=<n>
egress_hit=<n|not-applicable>
vm_received=<n>
```

9. Do not merge the implementation into `eswitch-management` until at least
   one mode has `vm_received > 0` and the successful mechanism is unambiguous.

## Current evidence and open question

The existing hardware-generated comparator has already shown:

```text
DEFAULT ingress selected = 10
EGRESS root hit          = 10
```

This proves that hardware traffic can cross `DEFAULT -> EGRESS -> VF`. It does
not prove Software TX reinjection.

The current daemon has separately shown:

```text
rte_eth_tx_burst accepted packets
parent opackets increased
EGRESS counters remained zero
VM received no ARP reply
```

The experiments above isolate whether the missing piece is the non-expert
destination metadata mechanism or the expert-mode Software-TX match/EGRESS
attachment.
