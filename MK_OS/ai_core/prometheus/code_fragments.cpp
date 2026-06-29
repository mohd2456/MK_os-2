#ifndef MK_PROMETHEUS_CODE_FRAGMENTS_CPP
#define MK_PROMETHEUS_CODE_FRAGMENTS_CPP

#include <string>
#include <vector>

// ===================================================================================
// MK PROMETHEUS — CODE FRAGMENTS (Bootstrap)
// ===================================================================================
// Initial high-precision droplets (0.8-1.0) for the Prometheus pool.
// ~100 code fragments across Python, C++, Bash covering common patterns.
// Each fragment is a proven, reusable building block.
// ===================================================================================

struct MKCodeFragment {
    std::string code;
    std::string language;
    std::string domain;
    std::vector<std::string> triggers;
};

static std::vector<MKCodeFragment> getBootstrapFragments() {
    std::vector<MKCodeFragment> frags;

    // ===== LOOP PATTERNS (20) =====
    frags.push_back({"for i in range(n):\n    print(i)", "python", "loops",
        {"for", "loop", "range", "iterate", "count"}});
    frags.push_back({"for item in collection:\n    process(item)", "python", "loops",
        {"for", "each", "iterate", "collection", "list"}});
    frags.push_back({"while condition:\n    do_work()\n    condition = update()", "python", "loops",
        {"while", "loop", "condition", "repeat"}});
    frags.push_back({"result = [x*2 for x in numbers]", "python", "loops",
        {"list", "comprehension", "transform", "map"}});
    frags.push_back({"result = [x for x in items if x > threshold]", "python", "loops",
        {"filter", "comprehension", "list", "condition"}});
    frags.push_back({"for i, item in enumerate(items):\n    print(f'{i}: {item}')", "python", "loops",
        {"enumerate", "index", "loop", "number"}});
    frags.push_back({"for key, val in dictionary.items():\n    print(f'{key}={val}')", "python", "loops",
        {"dict", "items", "loop", "key", "value"}});
    frags.push_back({"result = {k: v for k, v in items if v > 0}", "python", "loops",
        {"dict", "comprehension", "filter"}});
    frags.push_back({"for (int i = 0; i < n; i++) {\n    arr[i] = i * 2;\n}", "cpp", "loops",
        {"for", "loop", "array", "index", "c++"}});
    frags.push_back({"for (auto& item : container) {\n    process(item);\n}", "cpp", "loops",
        {"range", "for", "each", "container", "c++"}});
    frags.push_back({"while (!queue.empty()) {\n    auto item = queue.front();\n    queue.pop();\n}", "cpp", "loops",
        {"while", "queue", "process", "drain"}});
    frags.push_back({"std::for_each(vec.begin(), vec.end(), [](auto& x){ x *= 2; });", "cpp", "loops",
        {"for_each", "transform", "lambda", "stl"}});
    frags.push_back({"for f in *.txt; do\n    echo \"Processing $f\"\ndone", "bash", "loops",
        {"for", "files", "glob", "bash", "iterate"}});
    frags.push_back({"while read -r line; do\n    echo \"$line\"\ndone < file.txt", "bash", "loops",
        {"while", "read", "file", "line", "bash"}});
    frags.push_back({"seq 1 100 | while read n; do echo $((n*n)); done", "bash", "loops",
        {"seq", "pipe", "loop", "sequence"}});
    frags.push_back({"import itertools\nfor combo in itertools.combinations(items, 2):\n    print(combo)", "python", "loops",
        {"combinations", "itertools", "pairs", "permutations"}});
    frags.push_back({"result = list(map(lambda x: x**2, numbers))", "python", "loops",
        {"map", "lambda", "transform", "square"}});
    frags.push_back({"from functools import reduce\ntotal = reduce(lambda a, b: a + b, numbers)", "python", "loops",
        {"reduce", "fold", "accumulate", "total"}});
    frags.push_back({"for i in range(0, len(items), batch_size):\n    batch = items[i:i+batch_size]", "python", "loops",
        {"batch", "chunk", "slice", "step"}});
    frags.push_back({"while True:\n    data = get_input()\n    if data == 'quit': break\n    process(data)", "python", "loops",
        {"infinite", "loop", "input", "break", "repl"}});

    // ===== FILE OPERATIONS (15) =====
    frags.push_back({"with open('file.txt', 'r') as f:\n    content = f.read()", "python", "file_ops",
        {"read", "file", "open", "text"}});
    frags.push_back({"with open('file.txt', 'w') as f:\n    f.write(data)", "python", "file_ops",
        {"write", "file", "save", "output"}});
    frags.push_back({"with open('file.txt', 'a') as f:\n    f.write(line + '\\n')", "python", "file_ops",
        {"append", "file", "add", "log"}});
    frags.push_back({"import os\nfor root, dirs, files in os.walk(path):\n    for f in files:\n        print(os.path.join(root, f))", "python", "file_ops",
        {"walk", "directory", "recursive", "find", "files"}});
    frags.push_back({"import shutil\nshutil.copy2(src, dst)", "python", "file_ops",
        {"copy", "file", "duplicate", "backup"}});
    frags.push_back({"import os\nos.remove(filepath)", "python", "file_ops",
        {"delete", "remove", "file", "cleanup"}});
    frags.push_back({"import os\nos.makedirs(path, exist_ok=True)", "python", "file_ops",
        {"mkdir", "directory", "create", "folder"}});
    frags.push_back({"import glob\nfiles = glob.glob('**/*.py', recursive=True)", "python", "file_ops",
        {"glob", "find", "pattern", "search", "files"}});
    frags.push_back({"import json\nwith open('data.json') as f:\n    data = json.load(f)", "python", "file_ops",
        {"json", "load", "parse", "read", "config"}});
    frags.push_back({"import json\nwith open('out.json', 'w') as f:\n    json.dump(data, f, indent=2)", "python", "file_ops",
        {"json", "write", "save", "dump", "export"}});
    frags.push_back({"import csv\nwith open('data.csv') as f:\n    reader = csv.DictReader(f)\n    for row in reader:\n        print(row)", "python", "file_ops",
        {"csv", "read", "parse", "table", "data"}});
    frags.push_back({"std::ifstream f(path);\nstd::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());", "cpp", "file_ops",
        {"read", "file", "c++", "stream"}});
    frags.push_back({"std::ofstream f(path);\nf << content;\nf.close();", "cpp", "file_ops",
        {"write", "file", "c++", "output"}});
    frags.push_back({"cat file.txt | grep pattern | sort | uniq -c | sort -rn | head -10", "bash", "file_ops",
        {"grep", "sort", "count", "frequency", "top"}});
    frags.push_back({"find . -name '*.log' -mtime +7 -delete", "bash", "file_ops",
        {"find", "delete", "old", "cleanup", "log"}});

    // ===== STRING OPERATIONS (15) =====
    frags.push_back({"parts = text.split(delimiter)", "python", "strings",
        {"split", "string", "delimiter", "tokenize"}});
    frags.push_back({"result = delimiter.join(parts)", "python", "strings",
        {"join", "string", "combine", "concat"}});
    frags.push_back({"result = text.replace(old, new_str)", "python", "strings",
        {"replace", "string", "substitute", "swap"}});
    frags.push_back({"result = f'{name} is {age} years old'", "python", "strings",
        {"format", "string", "template", "interpolate"}});
    frags.push_back({"import re\nmatches = re.findall(pattern, text)", "python", "strings",
        {"regex", "find", "pattern", "match", "search"}});
    frags.push_back({"import re\nresult = re.sub(pattern, replacement, text)", "python", "strings",
        {"regex", "replace", "substitute", "pattern"}});
    frags.push_back({"text.strip().lower()", "python", "strings",
        {"trim", "lowercase", "clean", "normalize"}});
    frags.push_back({"text.startswith(prefix) or text.endswith(suffix)", "python", "strings",
        {"starts", "ends", "check", "prefix", "suffix"}});
    frags.push_back({"words = text.split()\nword_count = len(words)", "python", "strings",
        {"count", "words", "split", "length"}});
    frags.push_back({"result = ''.join(c for c in text if c.isalnum())", "python", "strings",
        {"clean", "alphanumeric", "strip", "sanitize"}});
    frags.push_back({"reversed_str = text[::-1]", "python", "strings",
        {"reverse", "string", "backwards", "flip"}});
    frags.push_back({"is_palindrome = text == text[::-1]", "python", "strings",
        {"palindrome", "check", "reverse", "same"}});
    frags.push_back({"std::string result = str.substr(start, length);", "cpp", "strings",
        {"substring", "slice", "extract", "c++"}});
    frags.push_back({"echo \"$text\" | sed 's/old/new/g'", "bash", "strings",
        {"replace", "sed", "substitute", "bash"}});
    frags.push_back({"echo \"$text\" | awk '{print $1, $3}'", "bash", "strings",
        {"awk", "columns", "extract", "fields"}});

    // ===== NETWORK OPERATIONS (10) =====
    frags.push_back({"import requests\nresponse = requests.get(url)\ndata = response.json()", "python", "network",
        {"http", "get", "request", "api", "fetch"}});
    frags.push_back({"import requests\nresponse = requests.post(url, json=payload)", "python", "network",
        {"http", "post", "request", "api", "send"}});
    frags.push_back({"import urllib.request\nurllib.request.urlretrieve(url, filename)", "python", "network",
        {"download", "file", "url", "fetch", "save"}});
    frags.push_back({"import socket\ns = socket.socket(socket.AF_INET, socket.SOCK_STREAM)\ns.connect((host, port))", "python", "network",
        {"socket", "connect", "tcp", "network"}});
    frags.push_back({"from http.server import HTTPServer, SimpleHTTPRequestHandler\nHTTPServer(('', 8000), SimpleHTTPRequestHandler).serve_forever()", "python", "network",
        {"server", "http", "serve", "host", "web"}});
    frags.push_back({"import requests\nfor page in range(1, 11):\n    r = requests.get(f'{url}?page={page}')\n    items.extend(r.json()['results'])", "python", "network",
        {"paginate", "api", "pages", "fetch", "all"}});
    frags.push_back({"curl -s -X GET \"$url\" -H 'Content-Type: application/json'", "bash", "network",
        {"curl", "get", "api", "request", "bash"}});
    frags.push_back({"curl -s -X POST \"$url\" -H 'Content-Type: application/json' -d \"$json_data\"", "bash", "network",
        {"curl", "post", "api", "send", "bash"}});
    frags.push_back({"wget -q -O output.file \"$url\"", "bash", "network",
        {"wget", "download", "file", "fetch"}});
    frags.push_back({"import asyncio\nimport aiohttp\nasync with aiohttp.ClientSession() as session:\n    async with session.get(url) as resp:\n        data = await resp.json()", "python", "network",
        {"async", "http", "concurrent", "aiohttp"}});

    // ===== DATA STRUCTURE OPERATIONS (10) =====
    frags.push_back({"from collections import Counter\ncounts = Counter(items)\nmost_common = counts.most_common(10)", "python", "data_structures",
        {"count", "frequency", "most", "common", "counter"}});
    frags.push_back({"from collections import defaultdict\ngrouped = defaultdict(list)\nfor item in items:\n    grouped[item.key].append(item)", "python", "data_structures",
        {"group", "dict", "organize", "categorize"}});
    frags.push_back({"sorted_items = sorted(items, key=lambda x: x['score'], reverse=True)", "python", "data_structures",
        {"sort", "key", "order", "rank", "descending"}});
    frags.push_back({"filtered = list(filter(lambda x: x > threshold, items))", "python", "data_structures",
        {"filter", "condition", "select", "threshold"}});
    frags.push_back({"result = list(map(transform_fn, items))", "python", "data_structures",
        {"map", "transform", "apply", "convert"}});
    frags.push_back({"unique_items = list(set(items))", "python", "data_structures",
        {"unique", "deduplicate", "set", "distinct"}});
    frags.push_back({"merged = {**dict1, **dict2}", "python", "data_structures",
        {"merge", "dict", "combine", "union"}});
    frags.push_back({"import heapq\ntop_k = heapq.nlargest(k, items, key=lambda x: x.score)", "python", "data_structures",
        {"top", "heap", "largest", "priority", "best"}});
    frags.push_back({"stack = []\nstack.append(item)  # push\ntop = stack.pop()  # pop", "python", "data_structures",
        {"stack", "push", "pop", "lifo"}});
    frags.push_back({"from collections import deque\nqueue = deque()\nqueue.append(item)\nfront = queue.popleft()", "python", "data_structures",
        {"queue", "deque", "fifo", "enqueue"}});

    // ===== ERROR HANDLING (10) =====
    frags.push_back({"try:\n    result = risky_operation()\nexcept ValueError as e:\n    print(f'Invalid value: {e}')\nexcept Exception as e:\n    print(f'Error: {e}')", "python", "error_handling",
        {"try", "except", "catch", "error", "handle"}});
    frags.push_back({"assert condition, f'Expected {expected}, got {actual}'", "python", "error_handling",
        {"assert", "check", "verify", "validate"}});
    frags.push_back({"if not os.path.exists(path):\n    raise FileNotFoundError(f'{path} does not exist')", "python", "error_handling",
        {"check", "exists", "file", "validate", "path"}});
    frags.push_back({"result = value if value is not None else default_value", "python", "error_handling",
        {"none", "default", "fallback", "null", "check"}});
    frags.push_back({"import logging\nlogging.basicConfig(level=logging.INFO)\nlogger = logging.getLogger(__name__)", "python", "error_handling",
        {"logging", "log", "debug", "info", "monitor"}});
    frags.push_back({"try {\n    riskyOperation();\n} catch (const std::exception& e) {\n    std::cerr << \"Error: \" << e.what() << std::endl;\n}", "cpp", "error_handling",
        {"try", "catch", "exception", "error", "c++"}});
    frags.push_back({"if (ptr == nullptr) {\n    throw std::runtime_error(\"Null pointer\");\n}", "cpp", "error_handling",
        {"null", "check", "pointer", "validate", "c++"}});
    frags.push_back({"set -euo pipefail\ntrap 'echo Error on line $LINENO' ERR", "bash", "error_handling",
        {"strict", "error", "trap", "bash", "safe"}});
    frags.push_back({"if ! command -v \"$tool\" &>/dev/null; then\n    echo \"$tool not found\"\n    exit 1\nfi", "bash", "error_handling",
        {"check", "command", "exists", "dependency", "bash"}});
    frags.push_back({"from contextlib import contextmanager\n@contextmanager\ndef managed_resource():\n    r = acquire()\n    try:\n        yield r\n    finally:\n        release(r)", "python", "error_handling",
        {"context", "manager", "resource", "cleanup", "with"}});

    // ===== COMMON ALGORITHMS (10) =====
    frags.push_back({"def binary_search(arr, target):\n    lo, hi = 0, len(arr)-1\n    while lo <= hi:\n        mid = (lo+hi)//2\n        if arr[mid] == target: return mid\n        elif arr[mid] < target: lo = mid+1\n        else: hi = mid-1\n    return -1", "python", "algorithms",
        {"binary", "search", "find", "sorted", "efficient"}});
    frags.push_back({"def quicksort(arr):\n    if len(arr) <= 1: return arr\n    pivot = arr[len(arr)//2]\n    left = [x for x in arr if x < pivot]\n    mid = [x for x in arr if x == pivot]\n    right = [x for x in arr if x > pivot]\n    return quicksort(left) + mid + quicksort(right)", "python", "algorithms",
        {"sort", "quicksort", "order", "fast", "recursive"}});
    frags.push_back({"import hashlib\nhash_val = hashlib.sha256(data.encode()).hexdigest()", "python", "algorithms",
        {"hash", "sha256", "checksum", "digest", "crypto"}});
    frags.push_back({"seen = set()\nunique = [x for x in items if x not in seen and not seen.add(x)]", "python", "algorithms",
        {"deduplicate", "unique", "remove", "duplicates"}});
    frags.push_back({"from functools import lru_cache\n@lru_cache(maxsize=128)\ndef expensive(n):\n    return compute(n)", "python", "algorithms",
        {"cache", "memoize", "optimize", "lru", "fast"}});
    frags.push_back({"def flatten(lst):\n    return [item for sub in lst for item in (flatten(sub) if isinstance(sub, list) else [sub])]", "python", "algorithms",
        {"flatten", "nested", "list", "recursive", "deep"}});
    frags.push_back({"from itertools import groupby\nfor key, group in groupby(sorted(items, key=keyfn), key=keyfn):\n    print(key, list(group))", "python", "algorithms",
        {"group", "cluster", "categorize", "aggregate"}});
    frags.push_back({"def gcd(a, b):\n    while b:\n        a, b = b, a % b\n    return a", "python", "algorithms",
        {"gcd", "euclidean", "greatest", "common", "divisor"}});
    frags.push_back({"def is_prime(n):\n    if n < 2: return False\n    for i in range(2, int(n**0.5)+1):\n        if n % i == 0: return False\n    return True", "python", "algorithms",
        {"prime", "check", "number", "math", "factor"}});
    frags.push_back({"def bfs(graph, start):\n    visited, queue = set(), [start]\n    while queue:\n        node = queue.pop(0)\n        if node not in visited:\n            visited.add(node)\n            queue.extend(graph[node] - visited)\n    return visited", "python", "algorithms",
        {"bfs", "graph", "search", "traverse", "breadth"}});

    // ===== SYSTEM OPERATIONS (10) =====
    frags.push_back({"import subprocess\nresult = subprocess.run(['ls', '-la'], capture_output=True, text=True)\nprint(result.stdout)", "python", "system",
        {"run", "command", "subprocess", "execute", "shell"}});
    frags.push_back({"import os\nvalue = os.environ.get('VAR_NAME', 'default')", "python", "system",
        {"env", "environment", "variable", "config", "get"}});
    frags.push_back({"import os\nhome = os.path.expanduser('~')\nconfig_path = os.path.join(home, '.config', 'app')", "python", "system",
        {"path", "home", "directory", "join", "config"}});
    frags.push_back({"import psutil\nfor proc in psutil.process_iter(['pid', 'name', 'cpu_percent']):\n    print(proc.info)", "python", "system",
        {"process", "list", "ps", "running", "monitor"}});
    frags.push_back({"import sys\nargs = sys.argv[1:]\nif '--help' in args:\n    print_usage()", "python", "system",
        {"args", "argv", "command", "line", "arguments"}});
    frags.push_back({"import platform\ninfo = f'{platform.system()} {platform.release()} ({platform.machine()})'", "python", "system",
        {"platform", "os", "system", "info", "detect"}});
    frags.push_back({"import time\nstart = time.time()\ndo_work()\nprint(f'Took {time.time()-start:.2f}s')", "python", "system",
        {"time", "benchmark", "measure", "performance", "duration"}});
    frags.push_back({"ps aux | grep process_name | grep -v grep | awk '{print $2}'", "bash", "system",
        {"process", "pid", "find", "grep", "bash"}});
    frags.push_back({"df -h | awk 'NR>1 {print $5, $6}' | sort -rn | head -5", "bash", "system",
        {"disk", "usage", "space", "storage", "check"}});
    frags.push_back({"export PATH=\"$HOME/.local/bin:$PATH\"\nexport EDITOR=vim", "bash", "system",
        {"export", "path", "environment", "setup", "config"}});

    return frags;
}

// Helper function to get fragments as simple string vectors for pool bootstrap
static std::vector<std::string> getCodeFragmentStrings() {
    auto frags = getBootstrapFragments();
    std::vector<std::string> result;
    for (const auto& f : frags) {
        result.push_back(f.code);
    }
    return result;
}

#endif // MK_PROMETHEUS_CODE_FRAGMENTS_CPP
