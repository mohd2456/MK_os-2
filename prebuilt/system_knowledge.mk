# MK System Knowledge Base
# Format: source|relation|target|weight
# 100+ facts about computers, operating systems, networking, and hardware.
# Enables MK to reason about the system it runs on.

# === HARDWARE ===
cpu|is_a|processor|1.0
cpu|executes|instructions|1.0
cpu|has|cores|1.0
cpu|has|cache|1.0
cpu|has|clock speed|1.0
cpu|measured_in|gigahertz|1.0
ram|is_a|memory|1.0
ram|is_a|volatile storage|1.0
ram|stores|running programs|1.0
ram|measured_in|gigabytes|1.0
ssd|is_a|storage device|1.0
ssd|faster_than|hdd|1.0
ssd|has|no moving parts|1.0
ssd|uses|flash memory|1.0
gpu|is_a|processor|1.0
gpu|used_for|graphics rendering|1.0
gpu|used_for|parallel computation|1.0
gpu|used_for|machine learning|1.0
motherboard|is_a|circuit board|1.0
motherboard|connects|all components|1.0
motherboard|has|chipset|1.0
cache|is_a|fast memory|1.0
cache|levels|L1 L2 L3|1.0
cache|closer_to|cpu than ram|1.0
bus|is_a|data pathway|1.0
bus|connects|cpu to memory|1.0
bus|has|bandwidth|1.0
pcie|is_a|bus standard|1.0
pcie|used_for|gpu connection|1.0
nvme|is_a|storage protocol|1.0
nvme|uses|pcie bus|1.0
power supply|provides|electricity|1.0
heatsink|used_for|cooling cpu|1.0
fan|used_for|airflow cooling|1.0
thermal paste|used_for|heat transfer|1.0

# === OS CONCEPTS ===
process|is_a|running program|1.0
process|has|memory space|1.0
process|has|process id|1.0
process|can|spawn child processes|1.0
thread|is_a|execution unit|1.0
thread|shares|process memory|1.0
thread|lighter_than|process|1.0
kernel|is_a|os core|1.0
kernel|manages|hardware resources|1.0
kernel|handles|system calls|1.0
kernel|runs_in|privileged mode|1.0
syscall|is_a|kernel interface|1.0
syscall|used_for|requesting os services|1.0
interrupt|is_a|signal to cpu|1.0
interrupt|causes|context switch|1.0
interrupt|types|hardware software|1.0
scheduler|is_a|os component|1.0
scheduler|manages|process execution|1.0
scheduler|uses|time slicing|1.0
context switch|is_a|os operation|1.0
context switch|saves|process state|1.0
virtual memory|is_a|memory abstraction|1.0
virtual memory|uses|page tables|1.0
page fault|is_a|memory event|1.0
deadlock|is_a|concurrency problem|1.0
deadlock|caused_by|circular waiting|1.0
mutex|is_a|synchronization primitive|1.0
mutex|prevents|race conditions|1.0
semaphore|is_a|synchronization primitive|1.0
ipc|stands_for|inter-process communication|1.0
ipc|methods|pipe shared_memory message_queue|1.0
daemon|is_a|background process|1.0
daemon|runs|without terminal|1.0
shell|is_a|command interpreter|1.0
shell|provides|user interface to os|1.0

# === NETWORKING ===
tcp|is_a|transport protocol|1.0
tcp|provides|reliable delivery|1.0
tcp|uses|three-way handshake|1.0
tcp|has|flow control|1.0
udp|is_a|transport protocol|1.0
udp|provides|fast unreliable delivery|1.0
udp|used_for|streaming gaming dns|1.0
http|is_a|application protocol|1.0
http|runs_over|tcp|1.0
http|uses|request response model|1.0
https|is_a|secure http|1.0
https|uses|tls encryption|1.0
dns|is_a|naming system|1.0
dns|resolves|domain to ip address|1.0
dns|port|53|1.0
ip address|is_a|network identifier|1.0
ip address|versions|ipv4 ipv6|1.0
port|is_a|endpoint number|1.0
port|range|0 to 65535|1.0
socket|is_a|communication endpoint|1.0
socket|combines|ip address and port|1.0
packet|is_a|data unit|1.0
packet|has|header and payload|1.0
router|is_a|network device|1.0
router|forwards|packets between networks|1.0
firewall|is_a|security device|1.0
firewall|filters|network traffic|1.0
nat|stands_for|network address translation|1.0
bandwidth|measured_in|bits per second|1.0
latency|measured_in|milliseconds|1.0
localhost|ip_address|127.0.0.1|1.0

# === FILE SYSTEMS ===
inode|is_a|file metadata|1.0
inode|stores|permissions size timestamps|1.0
block|is_a|storage unit|1.0
block|typical_size|4096 bytes|1.0
partition|is_a|disk division|1.0
partition|has|filesystem|1.0
mount|is_a|operation|1.0
mount|attaches|filesystem to directory tree|1.0
permissions|is_a|access control|1.0
permissions|format|rwx for owner group others|1.0
chmod|changes|file permissions|1.0
chown|changes|file ownership|1.0
symlink|is_a|file pointer|1.0
hardlink|is_a|additional name for file|1.0
apfs|is_a|filesystem|1.0
apfs|used_by|macos|1.0
ext4|is_a|filesystem|1.0
ext4|used_by|linux|1.0
ntfs|is_a|filesystem|1.0
ntfs|used_by|windows|1.0

# === SECURITY ===
encryption|is_a|security technique|1.0
encryption|protects|data confidentiality|1.0
aes|is_a|encryption algorithm|1.0
aes|type|symmetric|1.0
rsa|is_a|encryption algorithm|1.0
rsa|type|asymmetric|1.0
hash|is_a|one-way function|1.0
hash|used_for|integrity verification|1.0
sha256|is_a|hash algorithm|1.0
md5|is_a|hash algorithm|1.0
md5|considered|insecure|0.9
firewall|is_a|security measure|1.0
firewall|blocks|unauthorized access|1.0
sandbox|is_a|isolation technique|1.0
sandbox|restricts|process permissions|1.0
token|is_a|authentication credential|1.0
token|types|jwt oauth api_key|1.0
tls|is_a|security protocol|1.0
tls|provides|encrypted communication|1.0
ssh|is_a|secure protocol|1.0
ssh|used_for|remote access|1.0
ssh|port|22|1.0

# === MACOS SPECIFIC ===
macos|is_a|operating system|1.0
macos|built_on|darwin|1.0
darwin|is_a|unix-based kernel|1.0
darwin|derived_from|bsd|1.0
homebrew|is_a|package manager|1.0
homebrew|used_on|macos|1.0
homebrew|command|brew install|1.0
launchd|is_a|service manager|1.0
launchd|replaces|init system|1.0
launchd|uses|plist files|1.0
spotlight|is_a|search engine|1.0
spotlight|indexes|file system|1.0
xcode|is_a|ide|1.0
xcode|used_for|macos ios development|1.0
terminal.app|is_a|terminal emulator|1.0
activity monitor|is_a|process viewer|1.0
disk utility|is_a|disk manager|1.0
keychain|is_a|credential store|1.0
gatekeeper|is_a|security feature|1.0
sip|stands_for|system integrity protection|1.0
macos|filesystem|apfs|1.0
macos|shell|zsh|1.0

# === LINUX ===
bash|is_a|shell|1.0
bash|stands_for|bourne again shell|1.0
systemd|is_a|init system|1.0
systemd|manages|services|1.0
apt|is_a|package manager|1.0
apt|used_on|debian ubuntu|1.0
grep|is_a|search tool|1.0
grep|searches|text patterns|1.0
sed|is_a|stream editor|1.0
sed|used_for|text transformation|1.0
awk|is_a|text processor|1.0
awk|used_for|column extraction|1.0
pipe|is_a|ipc mechanism|1.0
pipe|symbol|||1.0
pipe|connects|stdout to stdin|1.0
cron|is_a|scheduler|1.0
cron|used_for|periodic tasks|1.0
top|is_a|process monitor|1.0
ps|is_a|process lister|1.0
kill|is_a|signal sender|1.0
sudo|provides|elevated privileges|1.0
root|is_a|superuser|1.0
/etc|contains|configuration files|1.0
/var|contains|variable data|1.0
/tmp|contains|temporary files|1.0
/home|contains|user directories|1.0
stdin|is_a|standard input|1.0
stdout|is_a|standard output|1.0
stderr|is_a|standard error|1.0

# === ADVANCED CPU ARCHITECTURE ===
branch prediction|is_a|cpu optimization|1.0
branch prediction|predicts|conditional branch outcome|1.0
branch prediction|types|static dynamic|1.0
branch predictor|accuracy|over 95 percent in modern cpus|0.95
speculative execution|is_a|cpu optimization|1.0
speculative execution|executes|instructions before branch resolved|1.0
speculative execution|vulnerability|spectre meltdown|1.0
simd|stands_for|single instruction multiple data|1.0
simd|processes|multiple data with one instruction|1.0
avx|is_a|simd instruction set|1.0
avx|stands_for|advanced vector extensions|1.0
avx-512|processes|512 bits per instruction|1.0
sse|is_a|simd instruction set|1.0
sse|stands_for|streaming simd extensions|1.0
pipelining|is_a|cpu technique|1.0
pipelining|stages|fetch decode execute memory writeback|1.0
pipelining|increases|instruction throughput|1.0
pipeline hazard|types|data structural control|1.0
out-of-order execution|is_a|cpu technique|1.0
out-of-order execution|executes|instructions when operands ready|1.0
out-of-order execution|uses|reorder buffer|1.0
superscalar|is_a|cpu architecture|1.0
superscalar|executes|multiple instructions per cycle|1.0
instruction level parallelism|is_a|cpu concept|1.0
register file|stores|cpu working data|1.0
register|is_a|fastest storage in cpu|1.0
program counter|tracks|next instruction address|1.0
alu|stands_for|arithmetic logic unit|1.0
alu|performs|arithmetic and logical operations|1.0
fpu|stands_for|floating point unit|1.0
clock cycle|is_a|basic cpu timing unit|1.0
cpi|stands_for|cycles per instruction|1.0
ipc|stands_for|instructions per cycle|1.0
microarchitecture|is_a|cpu implementation|1.0
isa|stands_for|instruction set architecture|1.0
isa|examples|x86_64 arm risc-v|1.0
risc|stands_for|reduced instruction set computer|1.0
cisc|stands_for|complex instruction set computer|1.0
arm|is_a|risc architecture|1.0
arm|used_in|mobile devices apple silicon|1.0
x86_64|is_a|cisc architecture|1.0
x86_64|used_in|desktop and server cpus|1.0
apple m1|is_a|arm processor|1.0
apple m1|has|unified memory architecture|1.0

# === MEMORY SYSTEMS ===
numa|stands_for|non-uniform memory access|1.0
numa|has|local and remote memory|1.0
numa|local_access|faster than remote|1.0
cache coherence|ensures|consistent data across caches|1.0
mesi protocol|is_a|cache coherence protocol|1.0
mesi|states|modified exclusive shared invalid|1.0
moesi protocol|is_a|cache coherence protocol|1.0
moesi|adds|owned state to mesi|1.0
tlb|stands_for|translation lookaside buffer|1.0
tlb|caches|page table entries|1.0
tlb miss|causes|page table walk|1.0
huge pages|are|larger than standard 4KB pages|1.0
huge pages|sizes|2MB or 1GB typically|1.0
huge pages|reduce|tlb misses|1.0
memory hierarchy|levels|register cache ram disk|1.0
l1 cache|typical_size|32-64 KB|1.0
l1 cache|latency|approximately 1-4 cycles|1.0
l2 cache|typical_size|256 KB to 1 MB|1.0
l2 cache|latency|approximately 10-20 cycles|1.0
l3 cache|typical_size|4-64 MB shared|1.0
l3 cache|latency|approximately 30-50 cycles|1.0
ram|latency|approximately 100-300 cycles|1.0
cache line|typical_size|64 bytes|1.0
cache associativity|types|direct-mapped set-associative fully-associative|1.0
write-back cache|writes|to memory only on eviction|1.0
write-through cache|writes|to memory immediately|1.0
prefetching|is_a|cache optimization|1.0
prefetching|loads|data before needed|1.0
memory controller|manages|ram access|1.0
ddr|stands_for|double data rate|1.0
ddr5|is_a|latest ddr generation|1.0
ecc memory|detects_and_corrects|single bit errors|1.0
memory bandwidth|measured_in|GB per second|1.0
false sharing|is_a|performance problem|1.0
false sharing|occurs_when|threads access same cache line|1.0

# === GPU COMPUTING ===
cuda|is_a|gpu programming platform|1.0
cuda|created_by|nvidia|1.0
cuda|stands_for|compute unified device architecture|1.0
opencl|is_a|gpu programming standard|1.0
opencl|is_a|cross-platform|1.0
shader|is_a|gpu program|1.0
shader|types|vertex fragment geometry compute|1.0
vertex shader|processes|vertex data|1.0
fragment shader|processes|pixel colors|1.0
compute shader|used_for|general purpose gpu computing|1.0
warp|is_a|nvidia execution unit|1.0
warp|contains|32 threads|1.0
wavefront|is_a|amd execution unit|1.0
wavefront|contains|64 threads|1.0
memory coalescing|is_a|gpu optimization|1.0
memory coalescing|combines|memory accesses into one transaction|1.0
gpu|has|thousands of cores|1.0
gpu|optimized_for|parallel workloads|1.0
gpu memory|types|global shared local registers|1.0
shared memory|is_a|fast on-chip gpu memory|1.0
tensor core|is_a|specialized gpu unit|1.0
tensor core|accelerates|matrix multiplication|1.0
ray tracing core|is_a|specialized gpu unit|1.0
ray tracing core|accelerates|ray-triangle intersection|1.0
vulkan|is_a|graphics api|1.0
vulkan|successor_to|opengl|1.0
metal|is_a|graphics api|1.0
metal|created_by|apple|1.0
directx|is_a|graphics api|1.0
directx|created_by|microsoft|1.0
gpgpu|stands_for|general purpose gpu computing|1.0
gpu bandwidth|much_higher_than|cpu bandwidth|1.0

# === ADVANCED OS CONCEPTS ===
page replacement|is_a|memory management algorithm|1.0
lru|stands_for|least recently used|1.0
lru|is_a|page replacement algorithm|1.0
clock algorithm|is_a|page replacement algorithm|1.0
clock algorithm|approximates|lru|1.0
working set|is_a|memory management concept|1.0
working set|contains|pages in active use|1.0
thrashing|occurs_when|too much paging|1.0
thrashing|severely_degrades|performance|1.0
disk scheduling|algorithms|fcfs sstf scan c-scan look c-look|1.0
scan|also_called|elevator algorithm|1.0
c-scan|provides|more uniform wait times than scan|1.0
journaling filesystem|logs|changes before applying|1.0
journaling filesystem|types|write-ahead log|1.0
ext4|has|journaling|1.0
xfs|is_a|journaling filesystem|1.0
xfs|optimized_for|large files|1.0
btrfs|is_a|copy-on-write filesystem|1.0
btrfs|has|snapshots and checksums|1.0
zfs|is_a|filesystem and volume manager|1.0
zfs|has|data integrity verification|1.0
zfs|has|copy-on-write|1.0
zfs|has|snapshots clones compression dedup|1.0
copy on write|is_a|optimization technique|1.0
copy on write|delays|copying until modification|1.0
priority inversion|is_a|scheduling problem|1.0
priority inversion|occurs_when|low priority holds resource needed by high priority|1.0
priority inheritance|solves|priority inversion|1.0
real-time os|guarantees|response within deadline|1.0
real-time os|types|hard real-time soft real-time|1.0
preemptive scheduling|can_interrupt|running process|1.0
cooperative scheduling|requires|process to yield|1.0
completely fair scheduler|is_a|linux scheduler|1.0
completely fair scheduler|uses|red-black tree|1.0

# === VIRTUALIZATION ===
hypervisor|is_a|virtualization software|1.0
hypervisor|types|type 1 bare metal and type 2 hosted|1.0
type 1 hypervisor|runs_on|hardware directly|1.0
type 1 hypervisor|examples|vmware esxi kvm xen|1.0
type 2 hypervisor|runs_on|host operating system|1.0
type 2 hypervisor|examples|virtualbox vmware workstation|1.0
kvm|stands_for|kernel-based virtual machine|1.0
kvm|is_a|linux kernel module|1.0
kvm|turns_linux_into|type 1 hypervisor|1.0
virtio|is_a|virtualization standard|1.0
virtio|provides|paravirtualized device drivers|1.0
paravirtualization|modifies|guest os for performance|1.0
full virtualization|emulates|complete hardware|1.0
memory ballooning|is_a|memory management technique|1.0
memory ballooning|reclaims|memory from guests|1.0
live migration|moves|running vm between hosts|1.0
live migration|has|minimal downtime|1.0
vfio|is_a|device passthrough framework|1.0
vfio|gives|vm direct hardware access|1.0
sr-iov|is_a|hardware virtualization|1.0
sr-iov|allows|sharing pcie device|1.0
nested virtualization|runs|hypervisor inside vm|1.0
qemu|is_a|machine emulator|1.0
qemu|often_used_with|kvm|1.0
vmware vsphere|is_a|enterprise virtualization platform|1.0

# === CONTAINERS ===
linux namespace|is_a|isolation mechanism|1.0
linux namespace|types|pid net mnt uts ipc user cgroup|1.0
pid namespace|isolates|process id space|1.0
network namespace|isolates|network stack|1.0
mount namespace|isolates|filesystem mounts|1.0
cgroups|stands_for|control groups|1.0
cgroups|limit|resource usage|1.0
cgroups|controls|cpu memory io network|1.0
cgroups v2|is_a|unified hierarchy|1.0
overlayfs|is_a|union filesystem|1.0
overlayfs|used_by|docker for layers|1.0
overlayfs|has|lower upper merged directories|1.0
seccomp|is_a|linux security feature|1.0
seccomp|restricts|system calls available|1.0
capabilities|is_a|linux security feature|1.0
capabilities|breaks|root privileges into units|1.0
container runtime|examples|runc containerd cri-o|1.0
runc|is_a|oci-compliant container runtime|1.0
oci|stands_for|open container initiative|1.0
oci|defines|container image and runtime specs|1.0
container image|is_a|layered filesystem|1.0
container image|includes|app and dependencies|1.0
buildkit|is_a|docker build engine|1.0
buildkit|supports|parallel builds and caching|1.0
podman|is_a|daemonless container engine|1.0
podman|compatible_with|docker cli|1.0

# === DISTRIBUTED SYSTEMS ===
cap theorem|states|cannot have consistency availability and partition tolerance|1.0
cap theorem|proposed_by|eric brewer|1.0
cap theorem|choose|two of three|1.0
paxos|is_a|consensus algorithm|1.0
paxos|guarantees|agreement in distributed systems|1.0
paxos|proposed_by|leslie lamport|1.0
raft|is_a|consensus algorithm|1.0
raft|designed_to_be|understandable|1.0
raft|used_by|etcd consul|1.0
raft|has|leader election and log replication|1.0
consistent hashing|is_a|distribution technique|1.0
consistent hashing|minimizes|remapping on node changes|1.0
consistent hashing|used_by|dynamo cassandra|1.0
vector clock|is_a|logical clock|1.0
vector clock|tracks|causality in distributed systems|1.0
lamport timestamp|is_a|logical clock|1.0
lamport timestamp|establishes|partial ordering|1.0
crdt|stands_for|conflict-free replicated data type|1.0
crdt|enables|automatic conflict resolution|1.0
crdt|types|g-counter pn-counter g-set lww-register|1.0
two phase commit|is_a|distributed transaction protocol|1.0
two phase commit|phases|prepare and commit|1.0
saga pattern|is_a|distributed transaction pattern|1.0
saga pattern|uses|compensating transactions|1.0
gossip protocol|is_a|dissemination protocol|1.0
gossip protocol|spreads|information epidemically|1.0
split brain|is_a|partition problem|1.0
split brain|occurs_when|cluster divides|1.0
quorum|is_a|majority agreement|1.0
quorum|formula|n/2 + 1 nodes|1.0
leader election|is_a|distributed algorithm|1.0
heartbeat|is_a|failure detection mechanism|1.0

# === ADVANCED NETWORKING ===
bgp|stands_for|border gateway protocol|1.0
bgp|is_a|internet routing protocol|1.0
bgp|connects|autonomous systems|1.0
bgp|is_a|path vector protocol|1.0
ospf|stands_for|open shortest path first|1.0
ospf|is_a|interior routing protocol|1.0
ospf|uses|dijkstra algorithm|1.0
ospf|is_a|link-state protocol|1.0
mpls|stands_for|multiprotocol label switching|1.0
mpls|uses|labels for forwarding|1.0
mpls|faster_than|ip routing|1.0
sdn|stands_for|software defined networking|1.0
sdn|separates|control plane and data plane|1.0
openflow|is_a|sdn protocol|1.0
openflow|programs|network switches|1.0
dpdk|stands_for|data plane development kit|1.0
dpdk|bypasses|kernel network stack|1.0
dpdk|achieves|high packet throughput|1.0
ebpf|stands_for|extended berkeley packet filter|1.0
ebpf|runs|programs in linux kernel|1.0
ebpf|used_for|networking tracing security|1.0
vxlan|stands_for|virtual extensible lan|1.0
vxlan|is_a|overlay network protocol|1.0
tcp congestion control|algorithms|reno cubic bbr|1.0
bbr|stands_for|bottleneck bandwidth and round-trip time|1.0
bbr|created_by|google|1.0
arp|stands_for|address resolution protocol|1.0
arp|resolves|ip address to mac address|1.0
icmp|stands_for|internet control message protocol|1.0
icmp|used_by|ping traceroute|1.0
mtu|stands_for|maximum transmission unit|1.0
mtu|ethernet_default|1500 bytes|1.0
tcp window scaling|allows|windows larger than 64KB|1.0
nagle algorithm|reduces|small packet overhead|1.0
keepalive|detects|dead connections|1.0

# === STORAGE SYSTEMS ===
raid 0|is_a|striping|1.0
raid 0|has|no redundancy|1.0
raid 0|improves|performance|1.0
raid 1|is_a|mirroring|1.0
raid 1|has|full redundancy|1.0
raid 5|is_a|striping with parity|1.0
raid 5|tolerates|one disk failure|1.0
raid 6|is_a|striping with double parity|1.0
raid 6|tolerates|two disk failures|1.0
raid 10|combines|mirroring and striping|1.0
erasure coding|is_a|data protection method|1.0
erasure coding|more_efficient_than|replication|1.0
erasure coding|used_by|cloud storage systems|1.0
nvme|stands_for|non-volatile memory express|1.0
nvme|designed_for|flash storage|1.0
nvme|latency|microseconds|1.0
nvme|uses|pcie directly|1.0
sata|stands_for|serial ata|1.0
sata|max_speed|6 Gbps for SATA III|1.0
iops|stands_for|input output operations per second|1.0
iops|measures|storage performance|1.0
sequential read|faster_than|random read|1.0
wear leveling|is_a|ssd technique|1.0
wear leveling|distributes|writes evenly across cells|1.0
trim|is_a|ssd command|1.0
trim|informs|ssd which blocks are unused|1.0
nand flash|types|slc mlc tlc qlc|1.0
slc|stores|1 bit per cell|1.0
tlc|stores|3 bits per cell|1.0
object storage|is_a|storage architecture|1.0
object storage|examples|s3 gcs azure blob|1.0
block storage|is_a|storage architecture|1.0
block storage|used_for|databases and vms|1.0
file storage|is_a|storage architecture|1.0
file storage|uses|hierarchical directories|1.0
iscsi|is_a|storage protocol|1.0
nfs|stands_for|network file system|1.0
nfs|is_a|distributed file system|1.0
ceph|is_a|distributed storage system|1.0
ceph|provides|object block and file storage|1.0
