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

    // ===== ASYNC/AWAIT PATTERNS (8) =====
    frags.push_back({"import asyncio\n\nasync def fetch_data(url):\n    await asyncio.sleep(1)\n    return f'Data from {url}'\n\nasync def main():\n    results = await asyncio.gather(\n        fetch_data('url1'),\n        fetch_data('url2'),\n        fetch_data('url3')\n    )\n    print(results)\n\nasyncio.run(main())", "python", "async",
        {"async", "await", "gather", "concurrent", "parallel"}});
    frags.push_back({"import asyncio\nfrom asyncio import Queue\n\nasync def producer(queue):\n    for i in range(10):\n        await queue.put(i)\n    await queue.put(None)\n\nasync def consumer(queue):\n    while True:\n        item = await queue.get()\n        if item is None: break\n        print(f'Got: {item}')", "python", "async",
        {"async", "queue", "producer", "consumer", "pattern"}});
    frags.push_back({"import asyncio\n\nasync def with_timeout(coro, timeout=5.0):\n    try:\n        return await asyncio.wait_for(coro, timeout=timeout)\n    except asyncio.TimeoutError:\n        print('Operation timed out')\n        return None", "python", "async",
        {"async", "timeout", "wait_for", "cancel", "deadline"}});
    frags.push_back({"import asyncio\nimport aiofiles\n\nasync def read_file_async(path):\n    async with aiofiles.open(path, 'r') as f:\n        content = await f.read()\n    return content", "python", "async",
        {"async", "file", "aiofiles", "read", "nonblocking"}});
    frags.push_back({"import asyncio\n\nasync def retry(coro_fn, max_retries=3, delay=1.0):\n    for attempt in range(max_retries):\n        try:\n            return await coro_fn()\n        except Exception as e:\n            if attempt == max_retries - 1: raise\n            await asyncio.sleep(delay * (attempt + 1))", "python", "async",
        {"async", "retry", "backoff", "resilient", "fault"}});
    frags.push_back({"import asyncio\n\nasync def semaphore_limited(urls, max_concurrent=5):\n    sem = asyncio.Semaphore(max_concurrent)\n    async def fetch(url):\n        async with sem:\n            await asyncio.sleep(0.1)\n            return url\n    return await asyncio.gather(*[fetch(u) for u in urls])", "python", "async",
        {"semaphore", "rate", "limit", "concurrent", "throttle"}});
    frags.push_back({"const fetchData = async (url) => {\n  try {\n    const response = await fetch(url);\n    const data = await response.json();\n    return data;\n  } catch (error) {\n    console.error('Fetch failed:', error);\n    throw error;\n  }\n};", "javascript", "async",
        {"async", "await", "fetch", "javascript", "api"}});
    frags.push_back({"const parallel = async (tasks) => {\n  const results = await Promise.allSettled(tasks);\n  return results.map(r => r.status === 'fulfilled' ? r.value : r.reason);\n};", "javascript", "async",
        {"promise", "allSettled", "parallel", "javascript", "batch"}});

    // ===== TESTING PATTERNS (8) =====
    frags.push_back({"import pytest\n\ndef test_addition():\n    assert 1 + 1 == 2\n\ndef test_string_upper():\n    assert 'hello'.upper() == 'HELLO'\n\n@pytest.mark.parametrize('input,expected', [(1,1), (2,4), (3,9)])\ndef test_square(input, expected):\n    assert input ** 2 == expected", "python", "testing",
        {"test", "pytest", "assert", "parametrize", "unit"}});
    frags.push_back({"import pytest\nfrom unittest.mock import Mock, patch\n\n@patch('module.external_api')\ndef test_with_mock(mock_api):\n    mock_api.return_value = {'status': 'ok'}\n    result = function_under_test()\n    assert result == 'ok'\n    mock_api.assert_called_once()", "python", "testing",
        {"mock", "patch", "unittest", "test", "stub"}});
    frags.push_back({"import pytest\n\n@pytest.fixture\ndef database():\n    db = create_test_db()\n    yield db\n    db.cleanup()\n\ndef test_insert(database):\n    database.insert({'key': 'value'})\n    assert database.count() == 1", "python", "testing",
        {"fixture", "pytest", "setup", "teardown", "test"}});
    frags.push_back({"describe('Calculator', () => {\n  let calc;\n  beforeEach(() => { calc = new Calculator(); });\n  \n  it('should add numbers', () => {\n    expect(calc.add(2, 3)).toBe(5);\n  });\n  \n  it('should handle negatives', () => {\n    expect(calc.add(-1, 1)).toBe(0);\n  });\n});", "javascript", "testing",
        {"jest", "describe", "test", "expect", "javascript"}});
    frags.push_back({"import pytest\nimport time\n\n@pytest.fixture(scope='session')\ndef slow_resource():\n    resource = expensive_setup()\n    yield resource\n    expensive_teardown(resource)\n\ndef test_performance():\n    start = time.time()\n    result = heavy_computation()\n    assert time.time() - start < 2.0", "python", "testing",
        {"performance", "benchmark", "fixture", "session", "test"}});
    frags.push_back({"from hypothesis import given, strategies as st\n\n@given(st.lists(st.integers()))\ndef test_sort_is_idempotent(lst):\n    sorted_once = sorted(lst)\n    sorted_twice = sorted(sorted_once)\n    assert sorted_once == sorted_twice", "python", "testing",
        {"property", "hypothesis", "fuzz", "generative", "test"}});
    frags.push_back({"import requests_mock\n\ndef test_api_client():\n    with requests_mock.Mocker() as m:\n        m.get('http://api.example.com/data', json={'result': 42})\n        response = my_client.get_data()\n        assert response['result'] == 42", "python", "testing",
        {"mock", "requests", "api", "integration", "test"}});
    frags.push_back({"#[cfg(test)]\nmod tests {\n    use super::*;\n    \n    #[test]\n    fn test_basic() {\n        assert_eq!(add(2, 3), 5);\n    }\n    \n    #[test]\n    #[should_panic]\n    fn test_divide_by_zero() {\n        divide(1, 0);\n    }\n}", "rust", "testing",
        {"rust", "test", "assert", "cfg", "mod"}});

    // ===== DEPLOYMENT / DOCKER / CI-CD (7) =====
    frags.push_back({"FROM python:3.11-slim\nWORKDIR /app\nCOPY requirements.txt .\nRUN pip install --no-cache-dir -r requirements.txt\nCOPY . .\nEXPOSE 8000\nCMD [\"uvicorn\", \"main:app\", \"--host\", \"0.0.0.0\", \"--port\", \"8000\"]", "docker", "deployment",
        {"docker", "dockerfile", "container", "deploy", "image"}});
    frags.push_back({"version: '3.8'\nservices:\n  web:\n    build: .\n    ports:\n      - '8000:8000'\n    environment:\n      - DATABASE_URL=postgresql://db:5432/app\n    depends_on:\n      - db\n  db:\n    image: postgres:15\n    environment:\n      - POSTGRES_PASSWORD=secret", "docker", "deployment",
        {"docker-compose", "services", "postgres", "deploy", "stack"}});
    frags.push_back({"name: CI\non: [push, pull_request]\njobs:\n  test:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v3\n      - uses: actions/setup-python@v4\n        with:\n          python-version: '3.11'\n      - run: pip install -r requirements.txt\n      - run: pytest --cov=src tests/", "yaml", "deployment",
        {"github", "actions", "ci", "pipeline", "workflow"}});
    frags.push_back({"stages:\n  - test\n  - build\n  - deploy\n\ntest:\n  stage: test\n  script:\n    - pip install -r requirements.txt\n    - pytest\n\ndeploy:\n  stage: deploy\n  script:\n    - docker build -t app .\n    - docker push registry/app:latest\n  only:\n    - main", "yaml", "deployment",
        {"gitlab", "ci", "pipeline", "deploy", "stages"}});
    frags.push_back({"#!/bin/bash\nset -euo pipefail\necho 'Building...'\ndocker build -t myapp:$(git rev-parse --short HEAD) .\necho 'Pushing...'\ndocker push myapp:$(git rev-parse --short HEAD)\necho 'Deploying...'\nkubectl set image deployment/myapp myapp=myapp:$(git rev-parse --short HEAD)", "bash", "deployment",
        {"deploy", "script", "docker", "kubernetes", "push"}});
    frags.push_back({"apiVersion: apps/v1\nkind: Deployment\nmetadata:\n  name: myapp\nspec:\n  replicas: 3\n  selector:\n    matchLabels:\n      app: myapp\n  template:\n    spec:\n      containers:\n      - name: myapp\n        image: myapp:latest\n        ports:\n        - containerPort: 8000", "yaml", "deployment",
        {"kubernetes", "k8s", "deployment", "pod", "replicas"}});
    frags.push_back({"terraform {\n  required_providers {\n    aws = { source = \"hashicorp/aws\" }\n  }\n}\n\nresource \"aws_instance\" \"web\" {\n  ami           = \"ami-0c55b159cbfafe1f0\"\n  instance_type = \"t2.micro\"\n  tags = { Name = \"WebServer\" }\n}", "hcl", "deployment",
        {"terraform", "infrastructure", "aws", "cloud", "provision"}});

    // ===== DATABASE OPERATIONS (7) =====
    frags.push_back({"import sqlite3\n\nconn = sqlite3.connect('app.db')\ncursor = conn.cursor()\ncursor.execute('''CREATE TABLE IF NOT EXISTS users\n    (id INTEGER PRIMARY KEY, name TEXT, email TEXT)''')\ncursor.execute('INSERT INTO users (name, email) VALUES (?, ?)', ('Alice', 'alice@example.com'))\nconn.commit()\nconn.close()", "python", "database",
        {"sqlite", "database", "create", "insert", "table"}});
    frags.push_back({"import sqlite3\n\ndef query_users(search):\n    conn = sqlite3.connect('app.db')\n    cursor = conn.cursor()\n    cursor.execute('SELECT * FROM users WHERE name LIKE ?', (f'%{search}%',))\n    results = cursor.fetchall()\n    conn.close()\n    return results", "python", "database",
        {"sqlite", "query", "select", "search", "database"}});
    frags.push_back({"import psycopg2\n\nconn = psycopg2.connect(\n    host='localhost', dbname='mydb',\n    user='user', password='pass'\n)\ncur = conn.cursor()\ncur.execute('SELECT * FROM users WHERE active = %s', (True,))\nrows = cur.fetchall()\nconn.close()", "python", "database",
        {"postgres", "postgresql", "query", "connect", "database"}});
    frags.push_back({"from sqlalchemy import create_engine, Column, Integer, String\nfrom sqlalchemy.orm import declarative_base, Session\n\nBase = declarative_base()\n\nclass User(Base):\n    __tablename__ = 'users'\n    id = Column(Integer, primary_key=True)\n    name = Column(String(50))\n    email = Column(String(100))\n\nengine = create_engine('sqlite:///app.db')\nBase.metadata.create_all(engine)", "python", "database",
        {"sqlalchemy", "orm", "model", "database", "schema"}});
    frags.push_back({"import redis\n\nr = redis.Redis(host='localhost', port=6379, db=0)\nr.set('key', 'value')\nr.expire('key', 3600)\nvalue = r.get('key')\nr.hset('user:1', mapping={'name': 'Alice', 'age': '30'})\nuser = r.hgetall('user:1')", "python", "database",
        {"redis", "cache", "key", "value", "store"}});
    frags.push_back({"from pymongo import MongoClient\n\nclient = MongoClient('mongodb://localhost:27017/')\ndb = client['mydb']\ncollection = db['users']\ncollection.insert_one({'name': 'Alice', 'age': 30})\nresults = collection.find({'age': {'$gt': 25}})\nfor doc in results:\n    print(doc)", "python", "database",
        {"mongodb", "nosql", "document", "collection", "query"}});
    frags.push_back({"-- Common SQL patterns\nSELECT u.name, COUNT(o.id) as order_count\nFROM users u\nLEFT JOIN orders o ON u.id = o.user_id\nWHERE u.active = true\nGROUP BY u.name\nHAVING COUNT(o.id) > 5\nORDER BY order_count DESC\nLIMIT 10;", "sql", "database",
        {"sql", "join", "group", "aggregate", "query"}});

    // ===== WEB FRAMEWORKS (7) =====
    frags.push_back({"from flask import Flask, request, jsonify\n\napp = Flask(__name__)\n\n@app.route('/api/users', methods=['GET'])\ndef get_users():\n    return jsonify({'users': []})\n\n@app.route('/api/users', methods=['POST'])\ndef create_user():\n    data = request.json\n    return jsonify(data), 201\n\nif __name__ == '__main__':\n    app.run(debug=True)", "python", "web",
        {"flask", "api", "route", "web", "server"}});
    frags.push_back({"from fastapi import FastAPI, HTTPException\nfrom pydantic import BaseModel\n\napp = FastAPI()\n\nclass User(BaseModel):\n    name: str\n    email: str\n\n@app.get('/users/{user_id}')\nasync def get_user(user_id: int):\n    return {'user_id': user_id}\n\n@app.post('/users')\nasync def create_user(user: User):\n    return user", "python", "web",
        {"fastapi", "api", "pydantic", "async", "rest"}});
    frags.push_back({"const express = require('express');\nconst app = express();\napp.use(express.json());\n\napp.get('/api/items', (req, res) => {\n  res.json({ items: [] });\n});\n\napp.post('/api/items', (req, res) => {\n  const item = req.body;\n  res.status(201).json(item);\n});\n\napp.listen(3000, () => console.log('Server on port 3000'));", "javascript", "web",
        {"express", "nodejs", "api", "rest", "server"}});
    frags.push_back({"from flask import Flask, render_template, redirect, url_for, flash\nfrom flask_login import LoginManager, login_user, login_required\n\napp = Flask(__name__)\nlogin_manager = LoginManager(app)\n\n@app.route('/login', methods=['GET', 'POST'])\ndef login():\n    if request.method == 'POST':\n        user = User.query.filter_by(email=request.form['email']).first()\n        if user and user.check_password(request.form['password']):\n            login_user(user)\n            return redirect(url_for('dashboard'))\n    return render_template('login.html')", "python", "web",
        {"flask", "login", "auth", "session", "security"}});
    frags.push_back({"from fastapi import Depends, HTTPException, status\nfrom fastapi.security import OAuth2PasswordBearer\nimport jwt\n\noauth2_scheme = OAuth2PasswordBearer(tokenUrl='token')\n\nasync def get_current_user(token: str = Depends(oauth2_scheme)):\n    try:\n        payload = jwt.decode(token, SECRET_KEY, algorithms=['HS256'])\n        return payload['sub']\n    except jwt.InvalidTokenError:\n        raise HTTPException(status_code=401, detail='Invalid token')", "python", "web",
        {"jwt", "auth", "token", "oauth", "security"}});
    frags.push_back({"from fastapi import WebSocket\n\n@app.websocket('/ws')\nasync def websocket_endpoint(websocket: WebSocket):\n    await websocket.accept()\n    while True:\n        data = await websocket.receive_text()\n        await websocket.send_text(f'Echo: {data}')", "python", "web",
        {"websocket", "realtime", "socket", "stream", "bidirectional"}});
    frags.push_back({"import React, { useState, useEffect } from 'react';\n\nfunction App() {\n  const [data, setData] = useState([]);\n  const [loading, setLoading] = useState(true);\n\n  useEffect(() => {\n    fetch('/api/items')\n      .then(res => res.json())\n      .then(json => { setData(json.items); setLoading(false); });\n  }, []);\n\n  if (loading) return <div>Loading...</div>;\n  return <ul>{data.map(item => <li key={item.id}>{item.name}</li>)}</ul>;\n}", "javascript", "web",
        {"react", "hooks", "useEffect", "useState", "frontend"}});

    // ===== DESIGN PATTERNS (8) =====
    frags.push_back({"class Singleton:\n    _instance = None\n    \n    def __new__(cls):\n        if cls._instance is None:\n            cls._instance = super().__new__(cls)\n        return cls._instance\n    \n    def __init__(self):\n        if not hasattr(self, '_initialized'):\n            self._initialized = True\n            self.data = {}", "python", "patterns",
        {"singleton", "pattern", "instance", "global", "one"}});
    frags.push_back({"from abc import ABC, abstractmethod\n\nclass Animal(ABC):\n    @abstractmethod\n    def speak(self): pass\n\nclass Dog(Animal):\n    def speak(self): return 'Woof!'\n\nclass Cat(Animal):\n    def speak(self): return 'Meow!'\n\ndef animal_factory(animal_type):\n    factories = {'dog': Dog, 'cat': Cat}\n    return factories[animal_type]()", "python", "patterns",
        {"factory", "pattern", "abstract", "create", "polymorphism"}});
    frags.push_back({"class EventEmitter:\n    def __init__(self):\n        self._listeners = {}\n    \n    def on(self, event, callback):\n        self._listeners.setdefault(event, []).append(callback)\n    \n    def emit(self, event, *args):\n        for callback in self._listeners.get(event, []):\n            callback(*args)\n    \n    def off(self, event, callback):\n        if event in self._listeners:\n            self._listeners[event].remove(callback)", "python", "patterns",
        {"observer", "event", "listener", "publish", "subscribe"}});
    frags.push_back({"class Command:\n    def execute(self): pass\n    def undo(self): pass\n\nclass UndoStack:\n    def __init__(self):\n        self._history = []\n    \n    def execute(self, command):\n        command.execute()\n        self._history.append(command)\n    \n    def undo(self):\n        if self._history:\n            cmd = self._history.pop()\n            cmd.undo()", "python", "patterns",
        {"command", "undo", "history", "action", "pattern"}});
    frags.push_back({"from functools import wraps\n\ndef decorator(func):\n    @wraps(func)\n    def wrapper(*args, **kwargs):\n        print(f'Before {func.__name__}')\n        result = func(*args, **kwargs)\n        print(f'After {func.__name__}')\n        return result\n    return wrapper\n\n@decorator\ndef greet(name):\n    return f'Hello, {name}!'", "python", "patterns",
        {"decorator", "wrapper", "pattern", "aspect", "middleware"}});
    frags.push_back({"class Builder:\n    def __init__(self):\n        self._config = {}\n    \n    def set_host(self, host):\n        self._config['host'] = host\n        return self\n    \n    def set_port(self, port):\n        self._config['port'] = port\n        return self\n    \n    def set_debug(self, debug):\n        self._config['debug'] = debug\n        return self\n    \n    def build(self):\n        return Server(**self._config)\n\nserver = Builder().set_host('0.0.0.0').set_port(8080).set_debug(True).build()", "python", "patterns",
        {"builder", "pattern", "fluent", "chain", "config"}});
    frags.push_back({"class Strategy:\n    def execute(self, data): pass\n\nclass QuickSort(Strategy):\n    def execute(self, data): return sorted(data)\n\nclass BubbleSort(Strategy):\n    def execute(self, data):\n        arr = list(data)\n        for i in range(len(arr)):\n            for j in range(len(arr)-1-i):\n                if arr[j] > arr[j+1]: arr[j], arr[j+1] = arr[j+1], arr[j]\n        return arr\n\nclass Sorter:\n    def __init__(self, strategy): self._strategy = strategy\n    def sort(self, data): return self._strategy.execute(data)", "python", "patterns",
        {"strategy", "pattern", "algorithm", "interchangeable", "policy"}});
    frags.push_back({"from typing import TypeVar, Generic, List\n\nT = TypeVar('T')\n\nclass Repository(Generic[T]):\n    def __init__(self):\n        self._items: List[T] = []\n    \n    def add(self, item: T) -> None:\n        self._items.append(item)\n    \n    def get(self, index: int) -> T:\n        return self._items[index]\n    \n    def find(self, predicate) -> List[T]:\n        return [item for item in self._items if predicate(item)]", "python", "patterns",
        {"generic", "repository", "type", "hint", "typed"}});

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
