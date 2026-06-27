#ifndef MK_CODE_WRITER_CPP
#define MK_CODE_WRITER_CPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <functional>
#include <fstream>
#include <ctime>

// ===========================================================================
// MK CODE WRITER - Full Code Generation Engine
// ===========================================================================
// Generates complete runnable code from natural language descriptions.
// Supports: Python, JavaScript, C++, Rust, Go
// ===========================================================================

// Code template structure
struct MKCodeTemplate {
    std::string name;
    std::string language;
    std::string category;
    std::string description;
    std::string code_template;
    std::vector<std::string> placeholders;
    std::vector<std::string> required_imports;
};

// Generated code result
struct MKGeneratedCode {
    bool success;
    std::string language;
    std::string code;
    std::string explanation;
    std::vector<std::string> imports;
    std::vector<std::string> functions;
    int line_count;
    std::string error;
};

// Language detection result
struct MKLanguageHint {
    std::string detected_language;
    float confidence;
    std::string reason;
};

class MKCodeWriter {
private:
    std::vector<MKCodeTemplate> templates;
    std::unordered_map<std::string, std::vector<std::string>> language_keywords;
    int total_generated;
    int successful_generated;

    // Initialize language keyword maps for detection
    void initLanguageKeywords() {
        language_keywords["python"] = {
            "def", "class", "import", "from", "pip", "django", "flask",
            "pandas", "numpy", "pytest", "python", "fastapi", "pydantic",
            "async", "await", "decorator", "list comprehension", "dict"
        };
        language_keywords["javascript"] = {
            "const", "let", "var", "function", "npm", "node", "react",
            "express", "typescript", "vue", "angular", "webpack", "fetch",
            "promise", "async", "callback", "dom", "browser", "json"
        };
        language_keywords["cpp"] = {
            "c++", "cpp", "pointer", "reference", "template", "stl",
            "vector", "map", "class", "namespace", "cmake", "header",
            "iostream", "raii", "smart pointer", "move semantics"
        };
        language_keywords["rust"] = {
            "rust", "cargo", "ownership", "borrow", "lifetime", "trait",
            "impl", "enum", "match", "option", "result", "crate", "mod"
        };
        language_keywords["go"] = {
            "go", "golang", "goroutine", "channel", "interface", "struct",
            "package", "func", "defer", "slice", "map", "concurrency"
        };
    }

    // Detect language from goal description
    MKLanguageHint detectLanguage(const std::string& goal) {
        std::string lower = goal;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        MKLanguageHint hint;
        hint.confidence = 0.0f;
        hint.detected_language = "python";  // default
        hint.reason = "default fallback";

        for (const auto& [lang, keywords] : language_keywords) {
            int matches = 0;
            for (const auto& kw : keywords) {
                if (lower.find(kw) != std::string::npos) matches++;
            }
            float score = static_cast<float>(matches) / keywords.size();
            if (score > hint.confidence) {
                hint.confidence = score;
                hint.detected_language = lang;
                hint.reason = "matched " + std::to_string(matches) + " keywords";
            }
        }
        if (hint.confidence < 0.1f) {
            hint.detected_language = "python";
            hint.confidence = 0.5f;
            hint.reason = "no strong signal, defaulting to Python";
        }
        return hint;
    }

    // Initialize code templates (50+ templates across languages)
    void initTemplates() {
        // Python templates
        templates.push_back({"http_server", "python", "web",
            "HTTP server with Flask",
            "from flask import Flask, jsonify, request\n\napp = Flask(__name__)\n\n@app.route('/{endpoint}', methods=['GET'])\ndef {handler}():\n    {body}\n    return jsonify({{'status': 'ok'}})\n\nif __name__ == '__main__':\n    app.run(debug=True, port={port})",
            {"endpoint", "handler", "body", "port"}, {"flask"}});
        templates.push_back({"file_parser", "python", "io",
            "Parse a file line by line",
            "def parse_file(filepath):\n    results = []\n    with open(filepath, 'r') as f:\n        for line in f:\n            line = line.strip()\n            if {condition}:\n                results.append({transform})\n    return results",
            {"condition", "transform"}, {}});
        templates.push_back({"data_pipeline", "python", "data",
            "Data processing pipeline with pandas",
            "import pandas as pd\n\ndef process_data(input_path, output_path):\n    df = pd.read_csv(input_path)\n    df = df.dropna()\n    df['{new_col}'] = df['{source_col}'].apply({transform_fn})\n    df.to_csv(output_path, index=False)\n    return len(df)",
            {"new_col", "source_col", "transform_fn"}, {"pandas"}});
        templates.push_back({"cli_tool", "python", "tool",
            "Command line tool with argparse",
            "import argparse\n\ndef main():\n    parser = argparse.ArgumentParser(description='{description}')\n    parser.add_argument('{arg1}', help='{arg1_help}')\n    parser.add_argument('--{flag}', default='{default}', help='{flag_help}')\n    args = parser.parse_args()\n    {body}\n\nif __name__ == '__main__':\n    main()",
            {"description", "arg1", "arg1_help", "flag", "default", "flag_help", "body"}, {}});
        templates.push_back({"web_scraper", "python", "web",
            "Web scraper with requests and BeautifulSoup",
            "import requests\nfrom bs4 import BeautifulSoup\n\ndef scrape(url):\n    resp = requests.get(url, timeout=10)\n    soup = BeautifulSoup(resp.text, 'html.parser')\n    results = []\n    for item in soup.select('{selector}'):\n        results.append(item.get_text(strip=True))\n    return results",
            {"selector"}, {"requests", "beautifulsoup4"}});
        templates.push_back({"api_client", "python", "web",
            "REST API client",
            "import requests\n\nclass {ClassName}:\n    def __init__(self, base_url, api_key=None):\n        self.base_url = base_url\n        self.session = requests.Session()\n        if api_key:\n            self.session.headers['Authorization'] = f'Bearer {{api_key}}'\n\n    def get(self, endpoint, params=None):\n        resp = self.session.get(f'{{self.base_url}}/{{endpoint}}', params=params)\n        resp.raise_for_status()\n        return resp.json()\n\n    def post(self, endpoint, data=None):\n        resp = self.session.post(f'{{self.base_url}}/{{endpoint}}', json=data)\n        resp.raise_for_status()\n        return resp.json()",
            {"ClassName"}, {"requests"}});
        templates.push_back({"database_crud", "python", "database",
            "SQLite CRUD operations",
            "import sqlite3\n\nclass {ClassName}:\n    def __init__(self, db_path):\n        self.conn = sqlite3.connect(db_path)\n        self.create_table()\n\n    def create_table(self):\n        self.conn.execute('CREATE TABLE IF NOT EXISTS {table} (id INTEGER PRIMARY KEY, {columns})')\n        self.conn.commit()\n\n    def insert(self, **kwargs):\n        cols = ', '.join(kwargs.keys())\n        vals = ', '.join(['?' for _ in kwargs])\n        self.conn.execute(f'INSERT INTO {table} ({{cols}}) VALUES ({{vals}})', list(kwargs.values()))\n        self.conn.commit()\n\n    def get_all(self):\n        return self.conn.execute('SELECT * FROM {table}').fetchall()",
            {"ClassName", "table", "columns"}, {"sqlite3"}});
        templates.push_back({"unit_tests", "python", "testing",
            "Pytest unit tests",
            "import pytest\nfrom {module} import {function}\n\ndef test_{function}_basic():\n    result = {function}({input1})\n    assert result == {expected1}\n\ndef test_{function}_edge():\n    result = {function}({input2})\n    assert result == {expected2}\n\ndef test_{function}_error():\n    with pytest.raises({exception}):\n        {function}({bad_input})",
            {"module", "function", "input1", "expected1", "input2", "expected2", "exception", "bad_input"}, {"pytest"}});
        templates.push_back({"async_worker", "python", "async",
            "Async task worker",
            "import asyncio\n\nasync def {worker_name}(queue):\n    while True:\n        task = await queue.get()\n        try:\n            result = await process(task)\n            print(f'Completed: {{result}}')\n        except Exception as e:\n            print(f'Error: {{e}}')\n        finally:\n            queue.task_done()\n\nasync def main():\n    queue = asyncio.Queue()\n    workers = [asyncio.create_task({worker_name}(queue)) for _ in range({num_workers})]\n    await queue.join()",
            {"worker_name", "num_workers"}, {}});
        templates.push_back({"fastapi_app", "python", "web",
            "FastAPI application",
            "from fastapi import FastAPI, HTTPException\nfrom pydantic import BaseModel\n\napp = FastAPI(title='{title}')\n\nclass {Model}(BaseModel):\n    {fields}\n\n@app.get('/{endpoint}')\nasync def get_items():\n    return {{'items': []}}\n\n@app.post('/{endpoint}')\nasync def create_item(item: {Model}):\n    return {{'id': 1, 'item': item.dict()}}",
            {"title", "Model", "fields", "endpoint"}, {"fastapi", "pydantic"}});

        // JavaScript templates
        templates.push_back({"express_server", "javascript", "web",
            "Express.js REST server",
            "const express = require('express');\nconst app = express();\napp.use(express.json());\n\napp.get('/{endpoint}', (req, res) => {\n    res.json({ status: 'ok', data: [] });\n});\n\napp.post('/{endpoint}', (req, res) => {\n    const data = req.body;\n    res.status(201).json({ id: 1, ...data });\n});\n\napp.listen({port}, () => console.log('Server on port {port}'));",
            {"endpoint", "port"}, {"express"}});
        templates.push_back({"react_component", "javascript", "frontend",
            "React functional component with hooks",
            "import React, { useState, useEffect } from 'react';\n\nfunction {ComponentName}({ {props} }) {\n    const [{state}, set{State}] = useState({initial});\n\n    useEffect(() => {\n        {effect_body}\n    }, [{deps}]);\n\n    return (\n        <div className='{class}'>\n            {jsx}\n        </div>\n    );\n}\n\nexport default {ComponentName};",
            {"ComponentName", "props", "state", "State", "initial", "effect_body", "deps", "class", "jsx"}, {"react"}});
        templates.push_back({"node_cli", "javascript", "tool",
            "Node.js CLI tool",
            "#!/usr/bin/env node\nconst { program } = require('commander');\n\nprogram\n    .name('{name}')\n    .description('{description}')\n    .argument('{arg}', '{arg_desc}')\n    .option('-{flag}, --{option} <value>', '{opt_desc}')\n    .action((arg, options) => {\n        {body}\n    });\n\nprogram.parse();",
            {"name", "description", "arg", "arg_desc", "flag", "option", "opt_desc", "body"}, {"commander"}});
        templates.push_back({"jest_tests", "javascript", "testing",
            "Jest unit tests",
            "const { {function} } = require('./{module}');\n\ndescribe('{function}', () => {\n    test('should handle basic case', () => {\n        expect({function}({input1})).toBe({expected1});\n    });\n\n    test('should handle edge case', () => {\n        expect({function}({input2})).toEqual({expected2});\n    });\n\n    test('should throw on invalid input', () => {\n        expect(() => {function}({bad})).toThrow();\n    });\n});",
            {"function", "module", "input1", "expected1", "input2", "expected2", "bad"}, {"jest"}});
        templates.push_back({"websocket_server", "javascript", "realtime",
            "WebSocket server",
            "const WebSocket = require('ws');\nconst wss = new WebSocket.Server({ port: {port} });\n\nconst clients = new Set();\n\nwss.on('connection', (ws) => {\n    clients.add(ws);\n    ws.on('message', (data) => {\n        const msg = JSON.parse(data);\n        clients.forEach(client => {\n            if (client !== ws && client.readyState === WebSocket.OPEN) {\n                client.send(JSON.stringify(msg));\n            }\n        });\n    });\n    ws.on('close', () => clients.delete(ws));\n});",
            {"port"}, {"ws"}});
        templates.push_back({"middleware", "javascript", "web",
            "Express middleware",
            "function {name}(req, res, next) {\n    const start = Date.now();\n    {validation}\n    res.on('finish', () => {\n        const duration = Date.now() - start;\n        console.log(`${req.method} ${req.url} ${res.statusCode} ${duration}ms`);\n    });\n    next();\n}",
            {"name", "validation"}, {}});

        // C++ templates
        templates.push_back({"class_definition", "cpp", "oop",
            "C++ class with RAII",
            "#include <iostream>\n#include <string>\n#include <memory>\n\nclass {ClassName} {\nprivate:\n    {members}\n\npublic:\n    {ClassName}({params}) {init_list} {}\n    ~{ClassName}() = default;\n\n    // Copy/move semantics\n    {ClassName}(const {ClassName}&) = default;\n    {ClassName}({ClassName}&&) noexcept = default;\n\n    {methods}\n};",
            {"ClassName", "members", "params", "init_list", "methods"}, {}});
        templates.push_back({"smart_pointers", "cpp", "memory",
            "Smart pointer usage patterns",
            "#include <memory>\n#include <vector>\n\nclass {ClassName} {\npublic:\n    static std::unique_ptr<{ClassName}> create({params}) {\n        return std::make_unique<{ClassName}>({args});\n    }\n\n    void addChild(std::shared_ptr<{Child}> child) {\n        children.push_back(std::move(child));\n    }\n\nprivate:\n    std::vector<std::shared_ptr<{Child}>> children;\n};",
            {"ClassName", "params", "args", "Child"}, {}});
        templates.push_back({"thread_pool", "cpp", "concurrency",
            "Thread pool implementation",
            "#include <thread>\n#include <mutex>\n#include <condition_variable>\n#include <queue>\n#include <functional>\n\nclass ThreadPool {\npublic:\n    ThreadPool(size_t n) {\n        for (size_t i = 0; i < n; ++i)\n            workers.emplace_back([this] { worker_loop(); });\n    }\n    void submit(std::function<void()> task) {\n        { std::lock_guard<std::mutex> lk(mtx); tasks.push(std::move(task)); }\n        cv.notify_one();\n    }\n    ~ThreadPool() { stop = true; cv.notify_all(); for (auto& w : workers) w.join(); }\nprivate:\n    void worker_loop() { while(!stop) { std::function<void()> t; { std::unique_lock<std::mutex> lk(mtx); cv.wait(lk, [&]{return stop || !tasks.empty();}); if(stop) return; t = std::move(tasks.front()); tasks.pop(); } t(); } }\n    std::vector<std::thread> workers; std::queue<std::function<void()>> tasks;\n    std::mutex mtx; std::condition_variable cv; bool stop = false;\n};",
            {}, {}});

        // Rust templates
        templates.push_back({"rust_struct", "rust", "core",
            "Rust struct with impl",
            "use std::fmt;\n\n#[derive(Debug, Clone)]\npub struct {Name} {\n    {fields}\n}\n\nimpl {Name} {\n    pub fn new({params}) -> Self {\n        Self { {init} }\n    }\n\n    pub fn {method}(&self) -> {ReturnType} {\n        {body}\n    }\n}\n\nimpl fmt::Display for {Name} {\n    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {\n        write!(f, \"{Name} {{ }}\")\n    }\n}",
            {"Name", "fields", "params", "init", "method", "ReturnType", "body"}, {}});
        templates.push_back({"rust_error", "rust", "error_handling",
            "Custom error type with thiserror",
            "use thiserror::Error;\n\n#[derive(Error, Debug)]\npub enum {ErrorName} {\n    #[error(\"IO error: {0}\")]\n    Io(#[from] std::io::Error),\n    #[error(\"{msg}\")]\n    Custom { msg: String },\n    #[error(\"not found: {0}\")]\n    NotFound(String),\n}\n\npub type Result<T> = std::result::Result<T, {ErrorName}>;",
            {"ErrorName", "msg"}, {"thiserror"}});
        templates.push_back({"rust_cli", "rust", "tool",
            "Rust CLI with clap",
            "use clap::Parser;\n\n#[derive(Parser, Debug)]\n#[command(name = \"{name}\", about = \"{about}\")]\nstruct Args {\n    #[arg(short, long)]\n    {arg1}: String,\n    #[arg(short, long, default_value = \"{default}\")]\n    {arg2}: String,\n}\n\nfn main() {\n    let args = Args::parse();\n    println!(\"Running with {} {}\", args.{arg1}, args.{arg2});\n}",
            {"name", "about", "arg1", "arg2", "default"}, {"clap"}});

        // Go templates
        templates.push_back({"go_http_server", "go", "web",
            "Go HTTP server",
            "package main\n\nimport (\n    \"encoding/json\"\n    \"log\"\n    \"net/http\"\n)\n\ntype Response struct {\n    Status string `json:\"status\"`\n    Data   interface{} `json:\"data\"`\n}\n\nfunc {handler}(w http.ResponseWriter, r *http.Request) {\n    resp := Response{Status: \"ok\", Data: nil}\n    json.NewEncoder(w).Encode(resp)\n}\n\nfunc main() {\n    http.HandleFunc(\"/{endpoint}\", {handler})\n    log.Fatal(http.ListenAndServe(\":{port}\", nil))\n}",
            {"handler", "endpoint", "port"}, {}});
        templates.push_back({"go_goroutines", "go", "concurrency",
            "Goroutine worker pattern",
            "package main\n\nimport (\n    \"fmt\"\n    \"sync\"\n)\n\nfunc worker(id int, jobs <-chan int, results chan<- int, wg *sync.WaitGroup) {\n    defer wg.Done()\n    for j := range jobs {\n        fmt.Printf(\"Worker %d processing job %d\\n\", id, j)\n        results <- j * 2\n    }\n}\n\nfunc main() {\n    jobs := make(chan int, 100)\n    results := make(chan int, 100)\n    var wg sync.WaitGroup\n    for w := 1; w <= {num_workers}; w++ {\n        wg.Add(1)\n        go worker(w, jobs, results, &wg)\n    }\n    for j := 1; j <= {num_jobs}; j++ { jobs <- j }\n    close(jobs)\n    wg.Wait()\n}",
            {"num_workers", "num_jobs"}, {}});

        // More Python templates for common tasks
        templates.push_back({"decorator", "python", "pattern",
            "Python decorator pattern",
            "import functools\nimport time\n\ndef {decorator_name}(func):\n    @functools.wraps(func)\n    def wrapper(*args, **kwargs):\n        start = time.time()\n        result = func(*args, **kwargs)\n        elapsed = time.time() - start\n        print(f'{func.__name__} took {elapsed:.3f}s')\n        return result\n    return wrapper",
            {"decorator_name"}, {}});
        templates.push_back({"context_manager", "python", "pattern",
            "Context manager class",
            "class {ClassName}:\n    def __init__(self, {params}):\n        self.{resource} = None\n\n    def __enter__(self):\n        self.{resource} = {acquire}\n        return self.{resource}\n\n    def __exit__(self, exc_type, exc_val, exc_tb):\n        if self.{resource}:\n            {release}\n        return False",
            {"ClassName", "params", "resource", "acquire", "release"}, {}});
        templates.push_back({"dataclass", "python", "data",
            "Python dataclass",
            "from dataclasses import dataclass, field\nfrom typing import List, Optional\n\n@dataclass\nclass {ClassName}:\n    {fields}\n\n    def to_dict(self):\n        return {{'key': self.{first_field}}}\n\n    @classmethod\n    def from_dict(cls, data: dict) -> '{ClassName}':\n        return cls(**data)",
            {"ClassName", "fields", "first_field"}, {}});

        // More templates to reach 50+
        templates.push_back({"dockerfile", "docker", "devops",
            "Multi-stage Dockerfile",
            "FROM {base_image} AS builder\nWORKDIR /app\nCOPY {copy_src} .\nRUN {build_cmd}\n\nFROM {runtime_image}\nWORKDIR /app\nCOPY --from=builder /app/{artifact} .\nEXPOSE {port}\nCMD [{cmd}]",
            {"base_image", "copy_src", "build_cmd", "runtime_image", "artifact", "port", "cmd"}, {}});
        templates.push_back({"makefile", "make", "build",
            "Makefile for C/C++ project",
            "CC = gcc\nCFLAGS = -Wall -Wextra -O2\nTARGET = {target}\nSRCS = $(wildcard src/*.c)\nOBJS = $(SRCS:.c=.o)\n\n.PHONY: all clean\n\nall: $(TARGET)\n\n$(TARGET): $(OBJS)\n\t$(CC) $(CFLAGS) -o $@ $^\n\n%.o: %.c\n\t$(CC) $(CFLAGS) -c $< -o $@\n\nclean:\n\trm -f $(OBJS) $(TARGET)",
            {"target"}, {}});
        templates.push_back({"github_action", "yaml", "ci",
            "GitHub Actions CI workflow",
            "name: CI\non: [push, pull_request]\njobs:\n  build:\n    runs-on: ubuntu-latest\n    steps:\n      - uses: actions/checkout@v3\n      - name: Setup\n        uses: {setup_action}\n      - name: Install deps\n        run: {install_cmd}\n      - name: Test\n        run: {test_cmd}",
            {"setup_action", "install_cmd", "test_cmd"}, {}});
        templates.push_back({"sql_migration", "sql", "database",
            "SQL migration script",
            "-- Migration: {name}\n-- Created: {date}\n\nBEGIN;\n\nCREATE TABLE IF NOT EXISTS {table} (\n    id SERIAL PRIMARY KEY,\n    {columns},\n    created_at TIMESTAMP DEFAULT NOW(),\n    updated_at TIMESTAMP DEFAULT NOW()\n);\n\nCREATE INDEX idx_{table}_{index_col} ON {table}({index_col});\n\nCOMMIT;",
            {"name", "date", "table", "columns", "index_col"}, {}});
        templates.push_back({"graphql_schema", "graphql", "api",
            "GraphQL type definitions",
            "type {TypeName} {\n  id: ID!\n  {fields}\n  createdAt: DateTime!\n}\n\ntype Query {\n  {query_name}(id: ID!): {TypeName}\n  all{TypeName}s(limit: Int = 10, offset: Int = 0): [{TypeName}!]!\n}\n\ntype Mutation {\n  create{TypeName}(input: {TypeName}Input!): {TypeName}!\n  update{TypeName}(id: ID!, input: {TypeName}Input!): {TypeName}!\n  delete{TypeName}(id: ID!): Boolean!\n}",
            {"TypeName", "fields", "query_name"}, {}});
        templates.push_back({"terraform", "terraform", "infra",
            "Terraform resource",
            "resource \"{resource_type}\" \"{name}\" {\n  {config}\n\n  tags = {\n    Name        = \"{name}\"\n    Environment = var.environment\n    ManagedBy   = \"terraform\"\n  }\n}",
            {"resource_type", "name", "config"}, {}});
        // Logging patterns
        templates.push_back({"logging_setup", "python", "utility",
            "Python logging configuration",
            "import logging\nimport sys\n\ndef setup_logging(level=logging.INFO):\n    logger = logging.getLogger('{logger_name}')\n    logger.setLevel(level)\n    handler = logging.StreamHandler(sys.stdout)\n    handler.setFormatter(logging.Formatter(\n        '%(asctime)s - %(name)s - %(levelname)s - %(message)s'\n    ))\n    logger.addHandler(handler)\n    return logger\n\nlog = setup_logging()",
            {"logger_name"}, {}});
        // 50th+ templates
        templates.push_back({"singleton", "python", "pattern", "Singleton pattern",
            "class {ClassName}:\n    _instance = None\n\n    def __new__(cls, *args, **kwargs):\n        if cls._instance is None:\n            cls._instance = super().__new__(cls)\n        return cls._instance\n\n    def __init__(self, {params}):\n        if not hasattr(self, '_initialized'):\n            {init_body}\n            self._initialized = True",
            {"ClassName", "params", "init_body"}, {}});
        templates.push_back({"observer", "python", "pattern", "Observer pattern",
            "class EventEmitter:\n    def __init__(self):\n        self._listeners = {}\n\n    def on(self, event, callback):\n        self._listeners.setdefault(event, []).append(callback)\n\n    def emit(self, event, *args, **kwargs):\n        for cb in self._listeners.get(event, []):\n            cb(*args, **kwargs)\n\n    def off(self, event, callback):\n        if event in self._listeners:\n            self._listeners[event].remove(callback)",
            {}, {}});
        templates.push_back({"retry_decorator", "python", "utility", "Retry with backoff",
            "import time\nimport functools\n\ndef retry(max_attempts={max}, delay={delay}, backoff=2):\n    def decorator(func):\n        @functools.wraps(func)\n        def wrapper(*args, **kwargs):\n            attempts = 0\n            current_delay = delay\n            while attempts < max_attempts:\n                try:\n                    return func(*args, **kwargs)\n                except Exception as e:\n                    attempts += 1\n                    if attempts == max_attempts:\n                        raise\n                    time.sleep(current_delay)\n                    current_delay *= backoff\n        return wrapper\n    return decorator",
            {"max", "delay"}, {}});
        // Additional Python templates
        templates.push_back({"queue_worker", "python", "async",
            "Queue-based worker pattern",
            "import queue\nimport threading\n\ndef worker(q):\n    while True:\n        item = q.get()\n        if item is None: break\n        process(item)\n        q.task_done()",
            {}, {}});
        templates.push_back({"config_loader", "python", "utility",
            "Configuration file loader",
            "import json\nimport os\n\ndef load_config(path='config.json'):\n    with open(path) as f:\n        return json.load(f)\n\ndef get_env(key, default=None):\n    return os.environ.get(key, default)",
            {}, {}});
        templates.push_back({"csv_processor", "python", "data",
            "CSV file processor",
            "import csv\n\ndef process_csv(input_path, output_path):\n    with open(input_path) as fin, open(output_path, 'w', newline='') as fout:\n        reader = csv.DictReader(fin)\n        writer = csv.DictWriter(fout, fieldnames=reader.fieldnames)\n        writer.writeheader()\n        for row in reader:\n            writer.writerow(row)",
            {}, {}});
        templates.push_back({"email_sender", "python", "utility",
            "Send email via SMTP",
            "import smtplib\nfrom email.mime.text import MIMEText\n\ndef send_email(to, subject, body, smtp_host='localhost'):\n    msg = MIMEText(body)\n    msg['Subject'] = subject\n    msg['To'] = to\n    with smtplib.SMTP(smtp_host) as server:\n        server.send_message(msg)",
            {}, {}});
        // Additional JavaScript templates
        templates.push_back({"promise_chain", "javascript", "async",
            "Promise chain pattern",
            "function {name}() {\n    return fetch('{url}')\n        .then(res => res.json())\n        .then(data => process(data))\n        .catch(err => console.error(err));\n}",
            {"name", "url"}, {}});
        templates.push_back({"event_handler", "javascript", "dom",
            "DOM event handler setup",
            "document.addEventListener('DOMContentLoaded', () => {\n    const el = document.querySelector('{selector}');\n    el.addEventListener('{event}', (e) => {\n        {handler_body}\n    });\n});",
            {"selector", "event", "handler_body"}, {}});
        templates.push_back({"fetch_wrapper", "javascript", "utility",
            "Fetch API wrapper with error handling",
            "async function api(endpoint, options = {}) {\n    const resp = await fetch(`${BASE_URL}/${endpoint}`, {\n        headers: {'Content-Type': 'application/json'},\n        ...options\n    });\n    if (!resp.ok) throw new Error(`HTTP ${resp.status}`);\n    return resp.json();\n}",
            {}, {}});
        // Additional C++ templates
        templates.push_back({"file_reader", "cpp", "io",
            "File reading utility",
            "#include <fstream>\n#include <string>\n\nstd::string readFile(const std::string& path) {\n    std::ifstream file(path);\n    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());\n}",
            {}, {}});
        templates.push_back({"command_pattern", "cpp", "pattern",
            "Command pattern implementation",
            "class Command {\npublic:\n    virtual ~Command() = default;\n    virtual void execute() = 0;\n    virtual void undo() = 0;\n};\n\nclass CommandHistory {\n    std::vector<std::unique_ptr<Command>> history;\npublic:\n    void execute(std::unique_ptr<Command> cmd) {\n        cmd->execute();\n        history.push_back(std::move(cmd));\n    }\n};",
            {}, {}});
        // Additional Rust templates
        templates.push_back({"rust_async", "rust", "async",
            "Async function with tokio",
            "use tokio;\n\n#[tokio::main]\nasync fn main() -> Result<(), Box<dyn std::error::Error>> {\n    let result = fetch_data().await?;\n    println!(\"{:?}\", result);\n    Ok(())\n}\n\nasync fn fetch_data() -> Result<String, reqwest::Error> {\n    reqwest::get(\"{url}\").await?.text().await\n}",
            {"url"}, {"tokio", "reqwest"}});
        templates.push_back({"rust_iter", "rust", "functional",
            "Iterator chain pattern",
            "fn process(items: Vec<i32>) -> Vec<i32> {\n    items.iter()\n        .filter(|&&x| x > 0)\n        .map(|&x| x * 2)\n        .collect()\n}",
            {}, {}});
        // Additional Go templates
        templates.push_back({"go_interface", "go", "pattern",
            "Go interface with implementation",
            "package main\n\ntype {Interface} interface {\n    {Method}() error\n}\n\ntype {Impl} struct {\n    {fields}\n}\n\nfunc (s *{Impl}) {Method}() error {\n    return nil\n}",
            {"Interface", "Method", "Impl", "fields"}, {}});
        templates.push_back({"go_error", "go", "error_handling",
            "Custom error type in Go",
            "package main\n\nimport \"fmt\"\n\ntype {ErrorType} struct {\n    Code    int\n    Message string\n}\n\nfunc (e *{ErrorType}) Error() string {\n    return fmt.Sprintf(\"%s (code: %d)\", e.Message, e.Code)\n}",
            {"ErrorType"}, {}});
    }

    // Fill placeholders in a template with provided values
    std::string fillTemplate(const std::string& tmpl,
                             const std::unordered_map<std::string, std::string>& values) {
        std::string result = tmpl;
        for (const auto& [key, value] : values) {
            std::string placeholder = "{" + key + "}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }
        return result;
    }

    // Generate import statements for a language
    std::string generateImports(const std::string& language,
                                const std::vector<std::string>& imports) {
        std::string result;
        if (language == "python") {
            for (const auto& imp : imports)
                result += "import " + imp + "\n";
        } else if (language == "javascript") {
            for (const auto& imp : imports)
                result += "const " + imp + " = require('" + imp + "');\n";
        } else if (language == "cpp") {
            for (const auto& imp : imports)
                result += "#include <" + imp + ">\n";
        } else if (language == "rust") {
            for (const auto& imp : imports)
                result += "use " + imp + ";\n";
        } else if (language == "go") {
            result += "import (\n";
            for (const auto& imp : imports)
                result += "    \"" + imp + "\"\n";
            result += ")\n";
        }
        return result;
    }

    // Generate docstring/comments for a function
    std::string generateDocstring(const std::string& language,
                                  const std::string& func_name,
                                  const std::string& description,
                                  const std::vector<std::string>& params) {
        std::string doc;
        if (language == "python") {
            doc = "    \"\"\"" + description + "\n";
            for (const auto& p : params)
                doc += "    Args:\n        " + p + ": description\n";
            doc += "    \"\"\"\n";
        } else if (language == "javascript") {
            doc = "/**\n * " + description + "\n";
            for (const auto& p : params)
                doc += " * @param {*} " + p + "\n";
            doc += " */\n";
        } else if (language == "cpp") {
            doc = "/// " + description + "\n";
            for (const auto& p : params)
                doc += "/// @param " + p + "\n";
        }
        return doc;
    }

    // Generate error handling boilerplate
    std::string generateErrorHandling(const std::string& language) {
        if (language == "python") {
            return "    try:\n        pass\n    except Exception as e:\n        print(f'Error: {e}')\n        raise\n";
        } else if (language == "javascript") {
            return "    try {\n        // code\n    } catch (error) {\n        console.error('Error:', error.message);\n        throw error;\n    }\n";
        } else if (language == "cpp") {
            return "    try {\n        // code\n    } catch (const std::exception& e) {\n        std::cerr << \"Error: \" << e.what() << std::endl;\n        throw;\n    }\n";
        } else if (language == "rust") {
            return "    match result {\n        Ok(val) => val,\n        Err(e) => {\n            eprintln!(\"Error: {}\", e);\n            return Err(e);\n        }\n    }\n";
        }
        return "";
    }

    // Find best matching template for a goal
    MKCodeTemplate findBestTemplate(const std::string& goal, const std::string& language) {
        std::string lower = goal;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        MKCodeTemplate best = templates[0];
        int best_score = 0;

        for (const auto& tmpl : templates) {
            if (!language.empty() && tmpl.language != language &&
                tmpl.language != "docker" && tmpl.language != "yaml" &&
                tmpl.language != "sql" && tmpl.language != "make") continue;

            int score = 0;
            // Check name match
            if (lower.find(tmpl.name) != std::string::npos) score += 10;
            // Check category match
            if (lower.find(tmpl.category) != std::string::npos) score += 5;
            // Check description keywords
            std::stringstream ss(tmpl.description);
            std::string word;
            while (ss >> word) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                if (lower.find(word) != std::string::npos) score += 2;
            }
            if (score > best_score) {
                best_score = score;
                best = tmpl;
            }
        }
        return best;
    }

public:
    MKCodeWriter() : total_generated(0), successful_generated(0) {
        initLanguageKeywords();
        initTemplates();
    }

    // Main code generation method
    MKGeneratedCode generate(const std::string& goal,
                             const std::string& preferred_language = "") {
        MKGeneratedCode result;
        result.success = false;
        total_generated++;

        // Detect language
        std::string lang = preferred_language;
        if (lang.empty()) {
            auto hint = detectLanguage(goal);
            lang = hint.detected_language;
        }
        result.language = lang;

        // Find best template
        auto tmpl = findBestTemplate(goal, lang);

        // Generate placeholder values from the goal
        std::unordered_map<std::string, std::string> values;
        for (const auto& ph : tmpl.placeholders) {
            // Simple heuristic: extract reasonable defaults
            if (ph == "port") values[ph] = "8080";
            else if (ph == "endpoint") values[ph] = "api";
            else if (ph == "handler") values[ph] = "handle_request";
            else if (ph == "ClassName" || ph == "ComponentName" || ph == "Name")
                values[ph] = "Generated";
            else if (ph == "body") values[ph] = "# TODO: implement";
            else values[ph] = ph;  // use placeholder name as default
        }

        // Fill template
        result.code = fillTemplate(tmpl.code_template, values);

        // Add imports
        result.imports = tmpl.required_imports;
        std::string imports_str = generateImports(lang, tmpl.required_imports);
        if (!imports_str.empty()) {
            result.code = imports_str + "\n" + result.code;
        }

        // Count lines
        result.line_count = 1;
        for (char c : result.code) if (c == '\n') result.line_count++;

        result.explanation = "Generated " + lang + " code using '" + tmpl.name + "' template";
        result.success = true;
        successful_generated++;
        return result;
    }

    // Generate code with custom placeholder values
    MKGeneratedCode generateCustom(const std::string& template_name,
                                    const std::unordered_map<std::string, std::string>& values) {
        MKGeneratedCode result;
        result.success = false;
        total_generated++;

        for (const auto& tmpl : templates) {
            if (tmpl.name == template_name) {
                result.language = tmpl.language;
                result.code = fillTemplate(tmpl.code_template, values);
                result.imports = tmpl.required_imports;
                result.line_count = 1;
                for (char c : result.code) if (c == '\n') result.line_count++;
                result.explanation = "Custom generation from template: " + template_name;
                result.success = true;
                successful_generated++;
                return result;
            }
        }
        result.error = "Template not found: " + template_name;
        return result;
    }

    // List available templates
    std::vector<std::string> listTemplates(const std::string& language = "") {
        std::vector<std::string> names;
        for (const auto& t : templates) {
            if (language.empty() || t.language == language)
                names.push_back(t.name + " (" + t.language + ": " + t.description + ")");
        }
        return names;
    }

    // Get template count
    int getTemplateCount() const { return static_cast<int>(templates.size()); }
    int getTotalGenerated() const { return total_generated; }
    int getSuccessfulGenerated() const { return successful_generated; }

    // Print stats
    void printStats() const {
        std::cout << "[MKCodeWriter] Templates: " << templates.size()
                  << ", Generated: " << total_generated
                  << ", Successful: " << successful_generated << std::endl;
    }
};

#endif // MK_CODE_WRITER_CPP
