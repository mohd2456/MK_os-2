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
