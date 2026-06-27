# MK Coding Knowledge Base
# Format: source|relation|target|weight
# 200+ facts about programming languages, patterns, libraries, and tools.
# This enables MK to assist with code generation and programming questions.

# === PYTHON SYNTAX ===
python|is_a|programming language|1.0
python|has|dynamic typing|1.0
python|uses|indentation for blocks|1.0
python|has|list comprehension|1.0
python|has|decorators|1.0
python|has|f-strings|1.0
python|has|generators|1.0
python|has|context managers|1.0
def|is_a|python keyword|1.0
def|used_for|function definition|1.0
class|is_a|python keyword|1.0
class|used_for|object definition|1.0
import|is_a|python keyword|1.0
import|used_for|loading modules|1.0
lambda|is_a|python keyword|1.0
lambda|used_for|anonymous functions|1.0
list comprehension|syntax|[x for x in iterable]|1.0
list comprehension|is_a|python pattern|1.0
decorator|syntax|@decorator_name|1.0
decorator|used_for|modifying function behavior|1.0
f-string|syntax|f"text {variable}"|1.0
f-string|used_for|string formatting|1.0
python|has|pip package manager|1.0
python|has|virtual environments|1.0
python|file_extension|.py|1.0
self|is_a|python convention|1.0
self|refers_to|current instance|1.0
__init__|is_a|python constructor|1.0
__init__|used_for|initializing objects|1.0
yield|is_a|python keyword|1.0
yield|used_for|creating generators|1.0
with|is_a|python keyword|1.0
with|used_for|context management|1.0
try except|is_a|python pattern|1.0
try except|used_for|error handling|1.0

# === JAVASCRIPT ===
javascript|is_a|programming language|1.0
javascript|runs_in|browser|1.0
javascript|runs_in|node.js|1.0
javascript|has|event loop|1.0
javascript|has|closures|1.0
javascript|has|prototypes|1.0
const|is_a|javascript keyword|1.0
const|used_for|immutable binding|1.0
let|is_a|javascript keyword|1.0
let|used_for|block-scoped variable|1.0
arrow function|syntax|() => {}|1.0
arrow function|is_a|javascript pattern|1.0
async await|is_a|javascript pattern|1.0
async await|used_for|asynchronous code|1.0
fetch|is_a|javascript api|1.0
fetch|used_for|http requests|1.0
promise|is_a|javascript concept|1.0
promise|used_for|async operations|1.0
dom|is_a|javascript api|1.0
dom|used_for|manipulating html|1.0
document.querySelector|is_a|dom method|1.0
document.querySelector|used_for|selecting elements|1.0
addEventListener|is_a|dom method|1.0
addEventListener|used_for|handling events|1.0
json|is_a|data format|1.0
json|used_with|javascript|1.0
JSON.parse|used_for|string to object|1.0
JSON.stringify|used_for|object to string|1.0
npm|is_a|package manager|1.0
npm|used_for|javascript packages|1.0
module.exports|is_a|node pattern|1.0
require|is_a|node function|1.0
import export|is_a|es6 module pattern|1.0
spread operator|syntax|...array|1.0
destructuring|is_a|javascript pattern|1.0
template literal|syntax|`text ${var}`|1.0

# === C++ ===
cpp|is_a|programming language|1.0
cpp|has|manual memory management|1.0
cpp|has|templates|1.0
cpp|has|RAII|1.0
cpp|has|multiple inheritance|1.0
cpp|compiles_to|machine code|1.0
#include|is_a|cpp directive|1.0
#include|used_for|including headers|1.0
class|is_a|cpp keyword|1.0
template|is_a|cpp keyword|1.0
template|used_for|generic programming|1.0
vector|is_a|cpp container|1.0
vector|is_a|dynamic array|1.0
map|is_a|cpp container|1.0
map|is_a|key-value store|1.0
pointer|is_a|cpp concept|1.0
pointer|stores|memory address|1.0
reference|is_a|cpp concept|1.0
reference|is_a|alias to variable|1.0
smart pointer|is_a|cpp concept|1.0
unique_ptr|is_a|smart pointer|1.0
shared_ptr|is_a|smart pointer|1.0
new delete|used_for|heap allocation|1.0
namespace|is_a|cpp concept|1.0
namespace|used_for|avoiding name conflicts|1.0
std|is_a|cpp namespace|1.0
virtual|is_a|cpp keyword|1.0
virtual|used_for|polymorphism|1.0
override|is_a|cpp keyword|1.0
constexpr|is_a|cpp keyword|1.0
constexpr|used_for|compile-time computation|1.0
auto|is_a|cpp keyword|1.0
auto|used_for|type inference|1.0
cpp|file_extension|.cpp .h .hpp|1.0
std::string|is_a|cpp class|1.0
std::cout|used_for|output|1.0
std::cin|used_for|input|1.0

# === HTML/CSS ===
html|is_a|markup language|1.0
html|used_for|web page structure|1.0
css|is_a|style language|1.0
css|used_for|web page styling|1.0
div|is_a|html element|1.0
div|is_a|block container|1.0
span|is_a|html element|1.0
span|is_a|inline container|1.0
flexbox|is_a|css layout|1.0
flexbox|property|display: flex|1.0
grid|is_a|css layout|1.0
grid|property|display: grid|1.0
margin|is_a|css property|1.0
margin|controls|outer spacing|1.0
padding|is_a|css property|1.0
padding|controls|inner spacing|1.0
class selector|syntax|.classname|1.0
id selector|syntax|#idname|1.0
media query|is_a|css feature|1.0
media query|used_for|responsive design|1.0
position|is_a|css property|1.0
position|values|static relative absolute fixed sticky|1.0
box model|is_a|css concept|1.0
box model|includes|margin padding border content|1.0
semantic html|includes|header nav main article footer|1.0

# === DESIGN PATTERNS ===
singleton|is_a|design pattern|1.0
singleton|ensures|one instance only|1.0
singleton|category|creational|1.0
factory|is_a|design pattern|1.0
factory|used_for|object creation|1.0
factory|category|creational|1.0
observer|is_a|design pattern|1.0
observer|used_for|event notification|1.0
observer|category|behavioral|1.0
mvc|is_a|architectural pattern|1.0
mvc|stands_for|model view controller|1.0
mvc|separates|data logic presentation|1.0
strategy|is_a|design pattern|1.0
strategy|used_for|swappable algorithms|1.0
decorator pattern|is_a|design pattern|1.0
decorator pattern|used_for|extending behavior|1.0
adapter|is_a|design pattern|1.0
adapter|used_for|interface compatibility|1.0
iterator|is_a|design pattern|1.0
iterator|used_for|sequential access|1.0
builder|is_a|design pattern|1.0
builder|used_for|complex object construction|1.0
pub sub|is_a|messaging pattern|1.0
pub sub|used_for|decoupled communication|1.0
middleware|is_a|pattern|1.0
middleware|used_for|request processing pipeline|1.0

# === LIBRARIES AND FRAMEWORKS ===
flask|is_a|python framework|1.0
flask|used_for|web applications|1.0
flask|has|routing decorators|1.0
express|is_a|node.js framework|1.0
express|used_for|web servers|1.0
express|has|middleware system|1.0
react|is_a|javascript library|1.0
react|used_for|user interfaces|1.0
react|has|components|1.0
react|has|virtual dom|1.0
react|uses|jsx syntax|1.0
numpy|is_a|python library|1.0
numpy|used_for|numerical computing|1.0
numpy|has|ndarray|1.0
pandas|is_a|python library|1.0
pandas|used_for|data analysis|1.0
pandas|has|dataframe|1.0
django|is_a|python framework|1.0
django|used_for|web applications|1.0
fastapi|is_a|python framework|1.0
fastapi|used_for|api development|1.0
vue|is_a|javascript framework|1.0
angular|is_a|javascript framework|1.0
next.js|is_a|react framework|1.0
tailwind|is_a|css framework|1.0
bootstrap|is_a|css framework|1.0
sqlite|is_a|database|1.0
sqlite|is_a|embedded database|1.0
postgres|is_a|database|1.0
mongodb|is_a|database|1.0
mongodb|is_a|nosql database|1.0
redis|is_a|database|1.0
redis|used_for|caching|1.0

# === CONCEPTS ===
api|stands_for|application programming interface|1.0
api|is_a|interface|1.0
rest|is_a|api style|1.0
rest|uses|http methods|1.0
http get|used_for|retrieving data|1.0
http post|used_for|creating data|1.0
http put|used_for|updating data|1.0
http delete|used_for|removing data|1.0
json|is_a|data format|1.0
json|used_for|data exchange|1.0
database|is_a|data storage|1.0
sql|is_a|query language|1.0
sql|used_for|database operations|1.0
select|is_a|sql command|1.0
insert|is_a|sql command|1.0
update|is_a|sql command|1.0
join|is_a|sql operation|1.0
index|used_for|database performance|1.0
orm|stands_for|object relational mapping|1.0
orm|used_for|database abstraction|1.0
crud|stands_for|create read update delete|1.0
crud|is_a|basic operations|1.0
websocket|is_a|protocol|1.0
websocket|used_for|real-time communication|1.0
graphql|is_a|query language|1.0
graphql|alternative_to|rest|1.0

# === TOOLS ===
git|is_a|version control system|1.0
git|command|git init|1.0
git|command|git add|1.0
git|command|git commit|1.0
git|command|git push|1.0
git|command|git pull|1.0
git|command|git branch|1.0
git|command|git merge|1.0
npm|is_a|package manager|1.0
npm|command|npm install|1.0
npm|command|npm run|1.0
pip|is_a|package manager|1.0
pip|command|pip install|1.0
docker|is_a|container platform|1.0
docker|used_for|containerization|1.0
docker|command|docker build|1.0
docker|command|docker run|1.0
dockerfile|is_a|docker config|1.0
linux|command|ls|1.0
linux|command|cd|1.0
linux|command|mkdir|1.0
linux|command|rm|1.0
linux|command|grep|1.0
linux|command|chmod|1.0
linux|command|cat|1.0
linux|command|curl|1.0
make|is_a|build tool|1.0
cmake|is_a|build system|1.0
webpack|is_a|bundler|1.0
vite|is_a|build tool|1.0

# === DEBUGGING ===
breakpoint|is_a|debug concept|1.0
breakpoint|used_for|pausing execution|1.0
console.log|is_a|javascript debug|1.0
console.log|used_for|printing values|1.0
print|is_a|python debug|1.0
print|used_for|output values|1.0
gdb|is_a|debugger|1.0
gdb|used_for|cpp debugging|1.0
stack trace|is_a|debug info|1.0
stack trace|shows|call history|1.0
logging|is_a|debug technique|1.0
logging|used_for|tracking execution|1.0
unit test|is_a|testing method|1.0
unit test|tests|individual functions|1.0
assert|is_a|testing tool|1.0
assert|used_for|verifying conditions|1.0
try catch|is_a|error handling|1.0
exception|is_a|runtime error|1.0
segfault|is_a|memory error|1.0
null pointer|causes|crash|1.0
memory leak|is_a|bug type|1.0
race condition|is_a|concurrency bug|1.0

# === BEST PRACTICES ===
dry|stands_for|dont repeat yourself|1.0
dry|is_a|principle|1.0
solid|is_a|design principles|1.0
solid|stands_for|single responsibility open-closed liskov interface-segregation dependency-inversion|1.0
testing|is_a|best practice|1.0
testing|prevents|bugs|1.0
documentation|is_a|best practice|1.0
documentation|helps|maintenance|1.0
code review|is_a|best practice|1.0
code review|catches|bugs early|1.0
refactoring|is_a|practice|1.0
refactoring|improves|code quality|1.0
clean code|has|meaningful names|1.0
clean code|has|small functions|1.0
clean code|has|single responsibility|1.0
version control|is_a|best practice|1.0
ci cd|stands_for|continuous integration deployment|1.0
ci cd|automates|testing and deployment|1.0
type safety|prevents|type errors|1.0
immutability|prevents|unexpected mutation|1.0
composition|preferred_over|inheritance|1.0
separation of concerns|is_a|principle|1.0
kiss|stands_for|keep it simple stupid|1.0
yagni|stands_for|you aint gonna need it|1.0

# === RUST LANGUAGE ===
rust|is_a|programming language|1.0
rust|has|ownership system|1.0
rust|has|borrow checker|1.0
rust|has|zero-cost abstractions|1.0
rust|has|no garbage collector|1.0
rust|guarantees|memory safety at compile time|1.0
rust|file_extension|.rs|1.0
rust|package_manager|cargo|1.0
ownership|is_a|rust concept|1.0
ownership|rule|each value has one owner|1.0
ownership|rule|value dropped when owner goes out of scope|1.0
borrowing|is_a|rust concept|1.0
borrowing|allows|references without ownership transfer|1.0
borrowing|rule|one mutable or many immutable references|1.0
lifetime|is_a|rust concept|1.0
lifetime|ensures|references are valid|1.0
lifetime|annotation|tick a as in 'a|1.0
trait|is_a|rust concept|1.0
trait|similar_to|interface in other languages|1.0
trait|used_for|shared behavior|1.0
enum|is_a|rust type|1.0
enum|can_hold|data variants|1.0
option|is_a|rust enum|1.0
option|variants|Some and None|1.0
result|is_a|rust enum|1.0
result|variants|Ok and Err|1.0
match|is_a|rust keyword|1.0
match|used_for|pattern matching|1.0
pattern matching|is_a|rust feature|1.0
pattern matching|must_be|exhaustive|1.0
struct|is_a|rust type|1.0
impl|is_a|rust keyword|1.0
impl|used_for|implementing methods|1.0
cargo|command|cargo build|1.0
cargo|command|cargo test|1.0
cargo|command|cargo run|1.0
cargo|uses|Cargo.toml for configuration|1.0
unsafe|is_a|rust keyword|1.0
unsafe|allows|bypassing borrow checker|1.0
macro|is_a|rust metaprogramming|1.0
macro|syntax|macro_name!|1.0
async await|is_a|rust feature|1.0
tokio|is_a|rust async runtime|1.0

# === GO LANGUAGE ===
go|is_a|programming language|1.0
go|created_by|google|1.0
go|has|goroutines|1.0
go|has|channels|1.0
go|has|garbage collector|1.0
go|has|static typing|1.0
go|compiles_to|machine code|1.0
go|file_extension|.go|1.0
goroutine|is_a|go concurrency primitive|1.0
goroutine|lighter_than|os thread|1.0
goroutine|started_with|go keyword|1.0
channel|is_a|go communication primitive|1.0
channel|used_for|passing data between goroutines|1.0
channel|can_be|buffered or unbuffered|1.0
interface|is_a|go type|1.0
interface|implemented|implicitly|1.0
defer|is_a|go keyword|1.0
defer|executes|function at end of surrounding function|1.0
slice|is_a|go type|1.0
slice|is_a|dynamic view of array|1.0
map|is_a|go builtin type|1.0
go|error_handling|explicit error return values|1.0
go|has|multiple return values|1.0
go|package_manager|go modules|1.0
go|command|go build|1.0
go|command|go test|1.0
go|command|go run|1.0
go|has|built-in concurrency|1.0
select|is_a|go keyword|1.0
select|used_for|waiting on multiple channels|1.0
go fmt|used_for|code formatting|1.0
context|is_a|go package|1.0
context|used_for|cancellation and timeouts|1.0

# === TYPESCRIPT ===
typescript|is_a|programming language|1.0
typescript|superset_of|javascript|1.0
typescript|adds|static typing|1.0
typescript|compiles_to|javascript|1.0
typescript|file_extension|.ts .tsx|1.0
typescript|has|interfaces|1.0
typescript|has|generics|1.0
typescript|has|enums|1.0
typescript|has|type inference|1.0
generic|is_a|typescript feature|1.0
generic|syntax|<T>|1.0
generic|used_for|reusable typed components|1.0
union type|is_a|typescript feature|1.0
union type|syntax|type A or B|1.0
intersection type|is_a|typescript feature|1.0
intersection type|syntax|type A and B|1.0
utility type Partial|makes|all properties optional|1.0
utility type Required|makes|all properties required|1.0
utility type Readonly|makes|all properties readonly|1.0
utility type Pick|selects|subset of properties|1.0
utility type Omit|excludes|subset of properties|1.0
type guard|is_a|typescript pattern|1.0
type guard|used_for|narrowing types|1.0
discriminated union|is_a|typescript pattern|1.0
discriminated union|uses|literal type discriminant|1.0
never type|represents|values that never occur|1.0
unknown type|is_a|safe alternative to any|1.0
type assertion|syntax|value as Type|1.0
decorator|is_a|typescript feature|1.0
decorator|syntax|@decoratorName|1.0
namespace|is_a|typescript feature|1.0
mapped type|is_a|typescript feature|1.0
conditional type|is_a|typescript feature|1.0
conditional type|syntax|T extends U ? X : Y|1.0
tsc|is_a|typescript compiler|1.0
tsconfig.json|configures|typescript compilation|1.0

# === ALGORITHM COMPLEXITY ===
big o notation|describes|algorithm time complexity|1.0
O(1)|is_a|constant time|1.0
O(1)|example|array index access|1.0
O(log n)|is_a|logarithmic time|1.0
O(log n)|example|binary search|1.0
O(n)|is_a|linear time|1.0
O(n)|example|linear search|1.0
O(n log n)|is_a|linearithmic time|1.0
O(n log n)|example|merge sort quick sort|1.0
O(n squared)|is_a|quadratic time|1.0
O(n squared)|example|bubble sort insertion sort|1.0
O(2 to the n)|is_a|exponential time|1.0
O(n factorial)|is_a|factorial time|1.0
O(n factorial)|example|brute force traveling salesman|1.0
space complexity|measures|memory usage|1.0
amortized analysis|averages|cost over sequence of operations|1.0
best case|is_a|minimum operations|1.0
worst case|is_a|maximum operations|1.0
average case|is_a|expected operations|1.0
binary search|requires|sorted array|1.0
binary search|time_complexity|O(log n)|1.0
hash table lookup|time_complexity|O(1) average|1.0
hash table lookup|time_complexity|O(n) worst case|1.0

# === ADVANCED DATA STRUCTURES ===
b-tree|is_a|self-balancing tree|1.0
b-tree|used_in|databases and file systems|1.0
b-tree|property|all leaves at same depth|1.0
b-tree|has|multiple keys per node|1.0
trie|is_a|tree data structure|1.0
trie|also_called|prefix tree|1.0
trie|used_for|string search and autocomplete|1.0
trie|time_complexity|O(key length) for lookup|1.0
heap|is_a|tree-based data structure|1.0
heap|types|min-heap and max-heap|1.0
heap|used_for|priority queues|1.0
heap|insert_complexity|O(log n)|1.0
skip list|is_a|probabilistic data structure|1.0
skip list|used_for|ordered key-value store|1.0
skip list|average_complexity|O(log n)|1.0
bloom filter|is_a|probabilistic data structure|1.0
bloom filter|can_have|false positives|1.0
bloom filter|cannot_have|false negatives|1.0
bloom filter|used_for|membership testing|1.0
red-black tree|is_a|self-balancing BST|1.0
red-black tree|guarantees|O(log n) operations|1.0
avl tree|is_a|self-balancing BST|1.0
avl tree|more_balanced_than|red-black tree|1.0
segment tree|is_a|data structure|1.0
segment tree|used_for|range queries|1.0
disjoint set|is_a|data structure|1.0
disjoint set|also_called|union-find|1.0
graph|is_a|data structure|1.0
graph|has|vertices and edges|1.0
directed graph|has|one-way edges|1.0
adjacency list|is_a|graph representation|1.0
adjacency matrix|is_a|graph representation|1.0

# === SYSTEM DESIGN ===
load balancer|distributes|traffic across servers|1.0
load balancer|algorithms|round robin least connections|1.0
load balancer|types|L4 and L7|1.0
caching|is_a|performance optimization|1.0
caching|strategies|write-through write-back write-around|1.0
cache invalidation|is_a|hard problem|1.0
cache eviction|policies|LRU LFU FIFO|1.0
sharding|is_a|horizontal partitioning|1.0
sharding|distributes|data across databases|1.0
sharding|key_types|hash range directory|1.0
message queue|is_a|async communication|1.0
message queue|examples|kafka rabbitmq sqs|1.0
message queue|enables|decoupled services|1.0
microservices|is_a|architecture pattern|1.0
microservices|opposite_of|monolith|1.0
microservices|communicate_via|api or messaging|1.0
rate limiting|is_a|api protection|1.0
rate limiting|algorithms|token bucket leaky bucket|1.0
circuit breaker|is_a|fault tolerance pattern|1.0
circuit breaker|prevents|cascading failures|1.0
eventual consistency|is_a|consistency model|1.0
strong consistency|is_a|consistency model|1.0
cdn|stands_for|content delivery network|1.0
cdn|used_for|caching content at edge|1.0
horizontal scaling|adds|more machines|1.0
vertical scaling|adds|more resources to machine|1.0
database replication|types|master-slave master-master|1.0
consensus|is_a|distributed agreement|1.0

# === NETWORKING PROTOCOLS ===
grpc|is_a|rpc framework|1.0
grpc|uses|protocol buffers|1.0
grpc|uses|http2|1.0
grpc|supports|bidirectional streaming|1.0
protocol buffers|is_a|serialization format|1.0
protocol buffers|created_by|google|1.0
quic|is_a|transport protocol|1.0
quic|built_on|udp|1.0
quic|used_by|http3|1.0
quic|reduces|connection latency|1.0
webrtc|is_a|protocol|1.0
webrtc|used_for|peer-to-peer communication|1.0
webrtc|supports|audio video data|1.0
mqtt|is_a|messaging protocol|1.0
mqtt|used_for|iot devices|1.0
mqtt|pattern|publish subscribe|1.0
mqtt|is_a|lightweight protocol|1.0
http2|is_a|protocol|1.0
http2|has|multiplexing|1.0
http2|has|header compression|1.0
http2|has|server push|1.0
http3|is_a|protocol|1.0
http3|uses|quic transport|1.0
sse|stands_for|server sent events|1.0
sse|is_a|one-way streaming|1.0

# === CLOUD AND DEVOPS ===
kubernetes|is_a|container orchestrator|1.0
kubernetes|manages|containerized applications|1.0
kubernetes|has|pods|1.0
kubernetes|has|services|1.0
kubernetes|has|deployments|1.0
kubernetes|has|namespaces|1.0
pod|is_a|kubernetes unit|1.0
pod|contains|one or more containers|1.0
service|is_a|kubernetes abstraction|1.0
service|provides|stable network endpoint|1.0
helm|is_a|kubernetes package manager|1.0
docker image|is_a|container template|1.0
docker container|is_a|running instance of image|1.0
docker layer|is_a|filesystem change|1.0
docker compose|used_for|multi-container applications|1.0
terraform|is_a|infrastructure as code tool|1.0
terraform|uses|declarative configuration|1.0
terraform|manages|cloud resources|1.0
terraform|has|state file|1.0
ci cd pipeline|automates|build test deploy|1.0
github actions|is_a|ci cd platform|1.0
jenkins|is_a|ci cd server|1.0
infrastructure as code|manages|infra via code files|1.0
gitops|is_a|deployment methodology|1.0
gitops|uses|git as source of truth|1.0
blue-green deployment|is_a|release strategy|1.0
canary deployment|is_a|gradual release strategy|1.0
rolling update|is_a|deployment strategy|1.0

# === SECURITY ===
oauth2|is_a|authorization framework|1.0
oauth2|grant_types|authorization_code client_credentials implicit|1.0
oauth2|uses|access tokens|1.0
jwt|stands_for|json web token|1.0
jwt|has|header payload signature|1.0
jwt|is_a|stateless auth token|1.0
jwt|encoded_with|base64url|1.0
cors|stands_for|cross origin resource sharing|1.0
cors|controls|cross-domain requests|1.0
cors|uses|http headers|1.0
xss|stands_for|cross site scripting|1.0
xss|injects|malicious scripts|1.0
xss|prevented_by|input sanitization|1.0
xss|types|stored reflected dom-based|1.0
csrf|stands_for|cross site request forgery|1.0
csrf|tricks|user into unwanted actions|1.0
csrf|prevented_by|anti-csrf tokens|1.0
sql injection|injects|malicious sql|1.0
sql injection|prevented_by|parameterized queries|1.0
sql injection|is_a|injection attack|1.0
bcrypt|is_a|password hashing algorithm|1.0
bcrypt|has|built-in salt|1.0
argon2|is_a|password hashing algorithm|1.0
argon2|winner_of|password hashing competition|1.0
https|prevents|man in the middle attacks|1.0
certificate|is_a|digital identity|1.0
certificate authority|issues|certificates|1.0
public key cryptography|uses|key pairs|1.0
public key|used_for|encryption|1.0
private key|used_for|decryption and signing|1.0

# === DATABASE INTERNALS ===
b+ tree|is_a|index structure|1.0
b+ tree|stores|data only in leaves|1.0
b+ tree|used_by|most relational databases|1.0
b+ tree|leaves_are|linked for range scans|1.0
wal|stands_for|write ahead log|1.0
wal|ensures|durability|1.0
wal|writes|changes before applying|1.0
mvcc|stands_for|multi version concurrency control|1.0
mvcc|allows|concurrent reads and writes|1.0
mvcc|used_by|postgres and mysql innodb|1.0
acid|stands_for|atomicity consistency isolation durability|1.0
acid|is_a|transaction property|1.0
transaction isolation|levels|read uncommitted read committed repeatable read serializable|1.0
clustered index|is_a|index type|1.0
clustered index|determines|physical row order|1.0
non-clustered index|is_a|index type|1.0
non-clustered index|stores|pointer to data|1.0
query optimizer|generates|execution plan|1.0
query optimizer|uses|cost-based optimization|1.0
deadlock detection|monitors|circular waits|1.0
connection pooling|reuses|database connections|1.0
sharding|splits|data across multiple databases|1.0
replication lag|is_a|delay between master and replica|1.0
vacuum|is_a|postgres maintenance|1.0
vacuum|reclaims|dead tuple space|1.0

# === COMPILER DESIGN ===
lexer|is_a|compiler phase|1.0
lexer|also_called|tokenizer or scanner|1.0
lexer|converts|source code to tokens|1.0
token|is_a|lexical unit|1.0
token|types|keyword identifier literal operator|1.0
parser|is_a|compiler phase|1.0
parser|converts|tokens to AST|1.0
parser|types|top-down bottom-up|1.0
ast|stands_for|abstract syntax tree|1.0
ast|represents|program structure|1.0
semantic analysis|is_a|compiler phase|1.0
semantic analysis|checks|types and scoping|1.0
intermediate representation|is_a|compiler phase|1.0
intermediate representation|abstracts|machine details|1.0
ir|examples|three-address code SSA LLVM IR|1.0
optimization pass|is_a|compiler phase|1.0
optimization pass|examples|dead code elimination constant folding inlining|1.0
code generation|is_a|compiler phase|1.0
code generation|produces|machine code or bytecode|1.0
register allocation|is_a|optimization|1.0
register allocation|maps|variables to cpu registers|1.0
llvm|is_a|compiler infrastructure|1.0
llvm|provides|reusable optimization passes|1.0
llvm|used_by|clang rust swift|1.0
jit compilation|stands_for|just in time compilation|1.0
jit compilation|compiles|at runtime|1.0
jit compilation|used_by|java javascript|1.0
garbage collector|types|mark-sweep generational reference counting|1.0
linker|is_a|build tool|1.0
linker|combines|object files into executable|1.0
linker|resolves|symbol references|1.0
