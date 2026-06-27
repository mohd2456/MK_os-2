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
