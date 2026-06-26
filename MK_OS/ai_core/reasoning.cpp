#include <iostream>
#include <string>
#include <stack>
#include <cctype>
#include <cmath>
#include <map>

class MKReasoning {
private:
    std::map<std::string, double> variables;

    double applyOp(double a, double b, char op) {
        switch(op) {
            case '+': return a + b;
            case '-': return a - b;
            case '*': return a * b;
            case '/': return b != 0 ? a / b : NAN;
        }
        return NAN;
    }

    int precedence(char op) {
        if(op == '+' || op == '-') return 1;
        if(op == '*' || op == '/') return 2;
        return 0;
    }

public:
    double evaluateExpression(const std::string& expr) {
        std::stack<double> values;
        std::stack<char> ops;

        for(size_t i = 0; i < expr.length(); i++) {
            if(expr[i] == ' ') continue;

            // Upgrade: Handles integers AND decimals (e.g. 3.14) safely
            if(isdigit(expr[i]) || expr[i] == '.') {
                std::string numStr;
                while(i < expr.length() && (isdigit(expr[i]) || expr[i] == '.')) {
                    numStr.push_back(expr[i]);
                    i++;
                }
                try {
                    values.push(std::stod(numStr));
                } catch(...) {
                    values.push(0.0);
                }
                i--;
            }
            else if(isalpha(expr[i])) {
                std::string var;
                while(i < expr.length() && isalpha(expr[i])) {
                    var.push_back(expr[i]);
                    i++;
                }
                
                if(variables.find(var) != variables.end()) {
                    values.push(variables[var]);
                    i--; 
                } else if(var == "sqrt") {
                    while(i < expr.length() && expr[i] == ' ') i++;
                    if(i < expr.length() && expr[i] == '(') {
                        i++;
                        std::string inner;
                        int openBrackets = 1;
                        while(i < expr.length() && openBrackets > 0) {
                            if(expr[i] == '(') openBrackets++;
                            if(expr[i] == ')') openBrackets--;
                            if(openBrackets > 0) inner.push_back(expr[i]);
                            i++;
                        }
                        double innerVal = evaluateExpression(inner);
                        values.push(sqrt(innerVal));
                        i--;
                    }
                } else {
                    values.push(0.0); // Default value for uninitialized variable safely
                    i--;
                }
            }
            else if(expr[i] == '(') {
                ops.push(expr[i]);
            }
            else if(expr[i] == ')') {
                while(!ops.empty() && ops.top() != '(') {
                    if(values.size() < 2) break; // Underflow safety check
                    double val2 = values.top(); values.pop();
                    double val1 = values.top(); values.pop();
                    char op = ops.top(); ops.pop();
                    values.push(applyOp(val1, val2, op));
                }
                if(!ops.empty()) ops.pop();
            }
            else {
                while(!ops.empty() && precedence(ops.top()) >= precedence(expr[i])) {
                    if(values.size() < 2) break; // Underflow safety check
                    double val2 = values.top(); values.pop();
                    double val1 = values.top(); values.pop();
                    char op = ops.top(); ops.pop();
                    values.push(applyOp(val1, val2, op));
                }
                ops.push(expr[i]);
            }
        }

        while(!ops.empty()) {
            if(values.size() < 2) break; // Underflow safety check
            double val2 = values.top(); values.pop();
            double val1 = values.top(); values.pop();
            char op = ops.top(); ops.pop();
            values.push(applyOp(val1, val2, op));
        }

        return !values.empty() ? values.top() : 0.0;
    }

    void assign(const std::string& var, double value) {
        variables[var] = value;
    }
};