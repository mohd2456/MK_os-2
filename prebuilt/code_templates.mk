# MK Code Template Library
# Format: TEMPLATE|name|language|description
# Followed by CODE lines, ended by ENDTEMPLATE
# Placeholders use {name} syntax for variable parts.
# Used by the Code Intelligence engine to assemble working programs.

# === PYTHON TEMPLATES ===

TEMPLATE|python_function|python|Basic function definition
CODE
def {function_name}({parameters}):
    """{docstring}"""
    {body}
    return {return_value}
ENDTEMPLATE

TEMPLATE|python_class|python|Class with constructor
CODE
class {class_name}:
    """{docstring}"""
    
    def __init__(self, {init_params}):
        {init_body}
    
    def {method_name}(self, {method_params}):
        """{method_doc}"""
        {method_body}
ENDTEMPLATE

TEMPLATE|python_main|python|Main entry point with argparse
CODE
import argparse
import sys

def main():
    parser = argparse.ArgumentParser(description="{description}")
    parser.add_argument("{arg_name}", help="{arg_help}")
    args = parser.parse_args()
    
    {main_body}

if __name__ == "__main__":
    main()
ENDTEMPLATE

TEMPLATE|python_flask_app|python|Basic Flask web application
CODE
from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route("/{route}", methods=["GET"])
def {handler_name}():
    {handler_body}
    return jsonify({response})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port={port})
ENDTEMPLATE

TEMPLATE|python_file_io|python|Read and write files
CODE
def read_file(filepath):
    with open(filepath, "r") as f:
        return f.read()

def write_file(filepath, content):
    with open(filepath, "w") as f:
        f.write(content)
ENDTEMPLATE

TEMPLATE|python_list_comprehension|python|List comprehension pattern
CODE
{result_name} = [{expression} for {item} in {iterable} if {condition}]
ENDTEMPLATE

TEMPLATE|python_try_except|python|Error handling pattern
CODE
try:
    {try_body}
except {exception_type} as e:
    print(f"Error: {e}")
    {except_body}
finally:
    {finally_body}
ENDTEMPLATE

TEMPLATE|python_dict_ops|python|Dictionary operations
CODE
{dict_name} = {}
{dict_name}["{key}"] = {value}
for key, value in {dict_name}.items():
    {loop_body}
ENDTEMPLATE

TEMPLATE|python_unittest|python|Unit test class
CODE
import unittest

class Test{class_name}(unittest.TestCase):
    def setUp(self):
        {setup_body}
    
    def test_{test_name}(self):
        {test_body}
        self.assertEqual({actual}, {expected})

if __name__ == "__main__":
    unittest.main()
ENDTEMPLATE

TEMPLATE|python_dataclass|python|Dataclass definition
CODE
from dataclasses import dataclass
from typing import List, Optional

@dataclass
class {class_name}:
    {field_name}: {field_type}
    {optional_field}: Optional[{optional_type}] = None
ENDTEMPLATE

# === C++ TEMPLATES ===

TEMPLATE|cpp_class|cpp|Class with header guard pattern
CODE
#ifndef {GUARD_NAME}_H
#define {GUARD_NAME}_H

#include <string>
#include <vector>

class {class_name} {
private:
    {private_members}

public:
    {class_name}({constructor_params});
    ~{class_name}();
    
    {public_methods}
};

#endif // {GUARD_NAME}_H
ENDTEMPLATE

TEMPLATE|cpp_main|cpp|Main program entry point
CODE
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    {main_body}
    return 0;
}
ENDTEMPLATE

TEMPLATE|cpp_function|cpp|Function definition
CODE
{return_type} {function_name}({parameters}) {
    {body}
    return {return_value};
}
ENDTEMPLATE

TEMPLATE|cpp_struct|cpp|Data structure definition
CODE
struct {struct_name} {
    {field_type} {field_name};
    {field_type2} {field_name2};
    
    {struct_name}() : {field_name}({default1}), {field_name2}({default2}) {}
};
ENDTEMPLATE

TEMPLATE|cpp_file_io|cpp|File reading and writing
CODE
#include <fstream>
#include <sstream>
#include <string>

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
    out.close();
}
ENDTEMPLATE

TEMPLATE|cpp_vector_ops|cpp|Vector operations pattern
CODE
#include <vector>
#include <algorithm>

std::vector<{type}> {vec_name};
{vec_name}.push_back({value});
std::sort({vec_name}.begin(), {vec_name}.end());
auto it = std::find({vec_name}.begin(), {vec_name}.end(), {search});
ENDTEMPLATE

TEMPLATE|cpp_map_usage|cpp|Map/hashmap usage
CODE
#include <unordered_map>
#include <string>

std::unordered_map<std::string, {value_type}> {map_name};
{map_name}["{key}"] = {value};
auto it = {map_name}.find("{key}");
if (it != {map_name}.end()) {
    {found_body}
}
ENDTEMPLATE

TEMPLATE|cpp_singleton|cpp|Singleton pattern
CODE
class {class_name} {
private:
    static {class_name}* instance;
    {class_name}() {}
    
public:
    static {class_name}* getInstance() {
        if (!instance) instance = new {class_name}();
        return instance;
    }
    
    {public_methods}
};
{class_name}* {class_name}::instance = nullptr;
ENDTEMPLATE

# === JAVASCRIPT TEMPLATES ===

TEMPLATE|js_function|javascript|Function declaration
CODE
function {function_name}({parameters}) {
    {body}
    return {return_value};
}
ENDTEMPLATE

TEMPLATE|js_arrow|javascript|Arrow function
CODE
const {function_name} = ({parameters}) => {
    {body}
};
ENDTEMPLATE

TEMPLATE|js_class|javascript|ES6 class
CODE
class {class_name} {
    constructor({constructor_params}) {
        {constructor_body}
    }
    
    {method_name}({method_params}) {
        {method_body}
    }
}
ENDTEMPLATE

TEMPLATE|js_fetch|javascript|Fetch API call
CODE
async function {function_name}({parameters}) {
    try {
        const response = await fetch("{url}", {
            method: "{method}",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({body})
        });
        const data = await response.json();
        return data;
    } catch (error) {
        console.error("Error:", error);
    }
}
ENDTEMPLATE

TEMPLATE|js_express_server|javascript|Express.js server
CODE
const express = require("express");
const app = express();
app.use(express.json());

app.get("/{route}", (req, res) => {
    {handler_body}
    res.json({response});
});

app.listen({port}, () => {
    console.log("Server running on port {port}");
});
ENDTEMPLATE

TEMPLATE|js_dom_manipulation|javascript|DOM manipulation
CODE
const {element} = document.getElementById("{element_id}");
{element}.addEventListener("{event}", (e) => {
    {handler_body}
});
{element}.innerHTML = {content};
ENDTEMPLATE

TEMPLATE|js_promise|javascript|Promise pattern
CODE
function {function_name}({parameters}) {
    return new Promise((resolve, reject) => {
        {async_body}
        if ({success_condition}) {
            resolve({result});
        } else {
            reject(new Error("{error_message}"));
        }
    });
}
ENDTEMPLATE

TEMPLATE|js_array_methods|javascript|Array manipulation
CODE
const {result} = {array}
    .filter({filter_item} => {filter_condition})
    .map({map_item} => {map_expression})
    .reduce((acc, {reduce_item}) => {reduce_expression}, {initial});
ENDTEMPLATE

# === HTML/CSS TEMPLATES ===

TEMPLATE|html_page|html|Basic HTML page structure
CODE
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <link rel="stylesheet" href="{stylesheet}">
</head>
<body>
    <header>{header_content}</header>
    <main>{main_content}</main>
    <footer>{footer_content}</footer>
    <script src="{script}"></script>
</body>
</html>
ENDTEMPLATE

TEMPLATE|css_flexbox|css|Flexbox layout
CODE
.{container_class} {
    display: flex;
    flex-direction: {direction};
    justify-content: {justify};
    align-items: {align};
    gap: {gap};
}

.{item_class} {
    flex: {flex_value};
    {item_styles}
}
ENDTEMPLATE

TEMPLATE|css_grid|css|CSS Grid layout
CODE
.{container_class} {
    display: grid;
    grid-template-columns: {columns};
    grid-template-rows: {rows};
    gap: {gap};
}

.{item_class} {
    grid-column: {col_span};
    grid-row: {row_span};
}
ENDTEMPLATE

TEMPLATE|html_form|html|Form with inputs
CODE
<form action="{action}" method="{method}">
    <label for="{field_id}">{label_text}</label>
    <input type="{input_type}" id="{field_id}" name="{field_name}" required>
    
    <button type="submit">{button_text}</button>
</form>
ENDTEMPLATE

# === PROJECT STRUCTURE TEMPLATES ===

TEMPLATE|project_python|structure|Python project layout
CODE
{project_name}/
    {project_name}/
        __init__.py
        main.py
        {module_name}.py
    tests/
        __init__.py
        test_{module_name}.py
    requirements.txt
    setup.py
    README.md
ENDTEMPLATE

TEMPLATE|project_node|structure|Node.js project layout
CODE
{project_name}/
    src/
        index.js
        {module_name}.js
        routes/
            {route_name}.js
    tests/
        {module_name}.test.js
    package.json
    README.md
    .gitignore
ENDTEMPLATE

TEMPLATE|project_cpp|structure|C++ project layout
CODE
{project_name}/
    src/
        main.cpp
        {module_name}.cpp
        {module_name}.h
    include/
        {project_name}/
            {header_name}.h
    tests/
        test_{module_name}.cpp
    CMakeLists.txt
    README.md
ENDTEMPLATE
