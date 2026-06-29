#ifndef MK_MATH_SOLVER_CPP
#define MK_MATH_SOLVER_CPP

#include <string>
#include <cmath>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>
#include <iostream>

// ============================================================
// MK Math/Science Solver
// Solves quadratic equations, unit conversions, physics formulas,
// trigonometry, percentages, and natural language math queries.
// ============================================================

struct MKMathResult {
    std::string answer;
    bool success;
    std::string error;
};

class MKMathSolver {
private:
    // Unit conversion factors (to SI base units)
    std::map<std::string, double> toMeters;
    std::map<std::string, double> toKg;
    std::map<std::string, double> toLiters;

    std::string toLower(const std::string& s) const {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::tolower);
        return r;
    }

    std::string trimStr(const std::string& s) const {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    std::string formatDouble(double val) const {
        std::ostringstream oss;
        if (val == (int)val && std::abs(val) < 1e9) {
            oss << (long long)val;
        } else {
            oss.precision(6);
            oss << val;
        }
        return oss.str();
    }


public:
    MKMathSolver() {
        // Distance conversions (all to meters)
        toMeters["km"] = 1000.0;
        toMeters["m"] = 1.0;
        toMeters["cm"] = 0.01;
        toMeters["mm"] = 0.001;
        toMeters["mi"] = 1609.34;
        toMeters["miles"] = 1609.34;
        toMeters["ft"] = 0.3048;
        toMeters["feet"] = 0.3048;
        toMeters["in"] = 0.0254;
        toMeters["inches"] = 0.0254;
        toMeters["yd"] = 0.9144;
        toMeters["yards"] = 0.9144;

        // Mass conversions (all to kg)
        toKg["kg"] = 1.0;
        toKg["g"] = 0.001;
        toKg["mg"] = 0.000001;
        toKg["lbs"] = 0.453592;
        toKg["lb"] = 0.453592;
        toKg["pounds"] = 0.453592;
        toKg["oz"] = 0.0283495;
        toKg["ounces"] = 0.0283495;
        toKg["ton"] = 1000.0;
        toKg["tons"] = 1000.0;

        // Volume conversions (all to liters)
        toLiters["l"] = 1.0;
        toLiters["ml"] = 0.001;
        toLiters["gal"] = 3.78541;
        toLiters["gallons"] = 3.78541;
        toLiters["qt"] = 0.946353;
        toLiters["cup"] = 0.236588;
        toLiters["cups"] = 0.236588;
    }

    // Solve quadratic equation ax^2 + bx + c = 0
    MKMathResult solveQuadratic(double a, double b, double c) const {
        MKMathResult result;
        result.success = false;

        if (a == 0) {
            if (b == 0) {
                result.error = "Not a valid equation (a=0, b=0)";
                return result;
            }
            // Linear equation
            double x = -c / b;
            result.answer = "x = " + formatDouble(x) + " (linear equation)";
            result.success = true;
            return result;
        }

        double discriminant = b * b - 4 * a * c;

        if (discriminant > 0) {
            double x1 = (-b + std::sqrt(discriminant)) / (2 * a);
            double x2 = (-b - std::sqrt(discriminant)) / (2 * a);
            result.answer = "x1 = " + formatDouble(x1) + ", x2 = " + formatDouble(x2);
            result.success = true;
        } else if (discriminant == 0) {
            double x = -b / (2 * a);
            result.answer = "x = " + formatDouble(x) + " (double root)";
            result.success = true;
        } else {
            double realPart = -b / (2 * a);
            double imagPart = std::sqrt(-discriminant) / (2 * a);
            result.answer = "x1 = " + formatDouble(realPart) + " + " + formatDouble(imagPart) + "i, "
                          + "x2 = " + formatDouble(realPart) + " - " + formatDouble(imagPart) + "i";
            result.success = true;
        }
        return result;
    }

    // Unit conversion
    MKMathResult convert(double value, const std::string& fromUnit, const std::string& toUnit) const {
        MKMathResult result;
        result.success = false;
        std::string from = toLower(fromUnit);
        std::string to = toLower(toUnit);

        // Temperature (special case)
        if ((from == "c" || from == "celsius") && (to == "f" || to == "fahrenheit")) {
            double f = value * 9.0 / 5.0 + 32.0;
            result.answer = formatDouble(value) + " C = " + formatDouble(f) + " F";
            result.success = true;
            return result;
        }
        if ((from == "f" || from == "fahrenheit") && (to == "c" || to == "celsius")) {
            double c = (value - 32.0) * 5.0 / 9.0;
            result.answer = formatDouble(value) + " F = " + formatDouble(c) + " C";
            result.success = true;
            return result;
        }
        if ((from == "c" || from == "celsius") && (to == "k" || to == "kelvin")) {
            double k = value + 273.15;
            result.answer = formatDouble(value) + " C = " + formatDouble(k) + " K";
            result.success = true;
            return result;
        }

        // Distance
        if (toMeters.count(from) && toMeters.count(to)) {
            double meters = value * toMeters.at(from);
            double converted = meters / toMeters.at(to);
            result.answer = formatDouble(value) + " " + fromUnit + " = " +
                           formatDouble(converted) + " " + toUnit;
            result.success = true;
            return result;
        }

        // Mass
        if (toKg.count(from) && toKg.count(to)) {
            double kg = value * toKg.at(from);
            double converted = kg / toKg.at(to);
            result.answer = formatDouble(value) + " " + fromUnit + " = " +
                           formatDouble(converted) + " " + toUnit;
            result.success = true;
            return result;
        }

        // Volume
        if (toLiters.count(from) && toLiters.count(to)) {
            double liters = value * toLiters.at(from);
            double converted = liters / toLiters.at(to);
            result.answer = formatDouble(value) + " " + fromUnit + " = " +
                           formatDouble(converted) + " " + toUnit;
            result.success = true;
            return result;
        }

        result.error = "Unknown conversion: " + fromUnit + " to " + toUnit;
        return result;
    }


    // Physics formulas
    MKMathResult physics(const std::string& formula, const std::map<std::string, double>& vars) const {
        MKMathResult result;
        result.success = false;
        std::string f = toLower(formula);

        if (f == "f=ma" || f == "force") {
            if (vars.count("m") && vars.count("a")) {
                double force = vars.at("m") * vars.at("a");
                result.answer = "F = " + formatDouble(force) + " N (Newtons)";
                result.success = true;
            } else { result.error = "Need m (mass in kg) and a (acceleration in m/s^2)"; }
        } else if (f == "e=mc2" || f == "energy") {
            if (vars.count("m")) {
                double c = 299792458.0;
                double energy = vars.at("m") * c * c;
                result.answer = "E = " + formatDouble(energy) + " J (Joules)";
                result.success = true;
            } else { result.error = "Need m (mass in kg)"; }
        } else if (f == "v=d/t" || f == "speed" || f == "velocity") {
            if (vars.count("d") && vars.count("t")) {
                double v = vars.at("d") / vars.at("t");
                result.answer = "v = " + formatDouble(v) + " m/s";
                result.success = true;
            } else { result.error = "Need d (distance in m) and t (time in s)"; }
        } else if (f == "ke=0.5mv2" || f == "kinetic") {
            if (vars.count("m") && vars.count("v")) {
                double ke = 0.5 * vars.at("m") * vars.at("v") * vars.at("v");
                result.answer = "KE = " + formatDouble(ke) + " J (Joules)";
                result.success = true;
            } else { result.error = "Need m (mass in kg) and v (velocity in m/s)"; }
        } else if (f == "ohm" || f == "v=ir") {
            if (vars.count("i") && vars.count("r")) {
                double v = vars.at("i") * vars.at("r");
                result.answer = "V = " + formatDouble(v) + " V (Volts)";
                result.success = true;
            } else { result.error = "Need i (current in A) and r (resistance in Ohms)"; }
        } else {
            result.error = "Unknown formula: " + formula +
                          ". Supported: F=ma, E=mc2, v=d/t, KE=0.5mv2, V=IR";
        }
        return result;
    }

    // Trigonometry (input in degrees)
    MKMathResult trig(const std::string& func, double angleDegrees) const {
        MKMathResult result;
        result.success = true;
        double radians = angleDegrees * M_PI / 180.0;

        std::string f = toLower(func);
        if (f == "sin") {
            result.answer = "sin(" + formatDouble(angleDegrees) + "°) = " + formatDouble(std::sin(radians));
        } else if (f == "cos") {
            result.answer = "cos(" + formatDouble(angleDegrees) + "°) = " + formatDouble(std::cos(radians));
        } else if (f == "tan") {
            if (std::fmod(angleDegrees, 180.0) == 90.0) {
                result.answer = "tan(" + formatDouble(angleDegrees) + "°) = undefined";
            } else {
                result.answer = "tan(" + formatDouble(angleDegrees) + "°) = " + formatDouble(std::tan(radians));
            }
        } else {
            result.error = "Unknown trig function: " + func + ". Use: sin, cos, tan";
            result.success = false;
        }
        return result;
    }

    // Percentage calculation
    MKMathResult percentage(double value, double percent) const {
        MKMathResult result;
        double answer = value * percent / 100.0;
        result.answer = formatDouble(percent) + "% of " + formatDouble(value) +
                       " = " + formatDouble(answer);
        result.success = true;
        return result;
    }

    // Parse and solve natural language math
    MKMathResult solve(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        // Pattern: "solve x^2+3x-4=0" or quadratic
        if (lower.find("solve") != std::string::npos || lower.find("x^2") != std::string::npos ||
            lower.find("quadratic") != std::string::npos) {
            // Try to extract a, b, c from simple patterns
            // Format: ax^2 + bx + c = 0
            double a = 1, b = 0, c = 0;
            // Simple parser for "x^2+3x-4"
            std::string expr = lower;
            // Remove "solve", "=0", spaces
            for (const auto& rem : std::vector<std::string>{"solve", "=0", "= 0"}) {
                size_t pos = expr.find(rem);
                if (pos != std::string::npos) expr.erase(pos, rem.size());
            }
            expr.erase(std::remove(expr.begin(), expr.end(), ' '), expr.end());

            // Parse terms
            if (expr.find("x^2") != std::string::npos || expr.find("x2") != std::string::npos) {
                // Extract coefficient of x^2
                size_t xpos = expr.find("x^2");
                if (xpos == std::string::npos) xpos = expr.find("x2");
                if (xpos > 0) {
                    std::string coef = expr.substr(0, xpos);
                    if (coef.empty() || coef == "+") a = 1;
                    else if (coef == "-") a = -1;
                    else { try { a = std::stod(coef); } catch (...) { a = 1; } }
                }
                // Look for bx term after x^2
                std::string rest = expr.substr(xpos + 3);
                size_t xpos2 = rest.find("x");
                if (xpos2 != std::string::npos) {
                    std::string bcoef = rest.substr(0, xpos2);
                    if (bcoef.empty() || bcoef == "+") b = 1;
                    else if (bcoef == "-") b = -1;
                    else { try { b = std::stod(bcoef); } catch (...) { b = 0; } }
                    // Remaining is c
                    std::string cstr = rest.substr(xpos2 + 1);
                    if (!cstr.empty()) {
                        try { c = std::stod(cstr); } catch (...) { c = 0; }
                    }
                } else if (!rest.empty()) {
                    try { c = std::stod(rest); } catch (...) { c = 0; }
                }
                return solveQuadratic(a, b, c);
            }
        }

        // Pattern: "convert X unit to unit"
        if (lower.find("convert") != std::string::npos) {
            std::istringstream iss(lower);
            std::string word;
            double value = 0;
            std::string fromUnit, toUnit;
            bool foundValue = false;

            while (iss >> word) {
                if (word == "convert" || word == "to") continue;
                if (!foundValue) {
                    try { value = std::stod(word); foundValue = true; continue; } catch (...) {}
                }
                if (foundValue && fromUnit.empty()) { fromUnit = word; continue; }
                if (!fromUnit.empty()) { toUnit = word; break; }
            }
            if (foundValue && !fromUnit.empty() && !toUnit.empty()) {
                return convert(value, fromUnit, toUnit);
            }
        }

        // Pattern: "what is X% of Y"
        if (lower.find("%") != std::string::npos && lower.find("of") != std::string::npos) {
            size_t pctPos = lower.find("%");
            size_t ofPos = lower.find("of");
            // Extract number before %
            std::string beforePct = lower.substr(0, pctPos);
            double pct = 0;
            // Find last number
            std::istringstream iss(beforePct);
            std::string word;
            while (iss >> word) {
                try { pct = std::stod(word); } catch (...) {}
            }
            // Extract number after "of"
            std::string afterOf = lower.substr(ofPos + 2);
            double value = 0;
            std::istringstream iss2(afterOf);
            while (iss2 >> word) {
                try { value = std::stod(word); break; } catch (...) {}
            }
            if (pct > 0 && value > 0) {
                return percentage(value, pct);
            }
        }

        // Pattern: "sin/cos/tan X"
        for (const auto& func : std::vector<std::string>{"sin", "cos", "tan"}) {
            if (lower.find(func) != std::string::npos) {
                // Extract angle
                size_t pos = lower.find(func) + func.size();
                std::string rest = trimStr(lower.substr(pos));
                // Remove parentheses
                rest.erase(std::remove(rest.begin(), rest.end(), '('), rest.end());
                rest.erase(std::remove(rest.begin(), rest.end(), ')'), rest.end());
                rest.erase(std::remove(rest.begin(), rest.end(), ' '), rest.end());
                // Remove degree symbol
                size_t degPos = rest.find("deg");
                if (degPos != std::string::npos) rest = rest.substr(0, degPos);
                try {
                    double angle = std::stod(rest);
                    return trig(func, angle);
                } catch (...) {}
            }
        }

        // Pattern: derivatives
        if (lower.find("derivative") != std::string::npos || lower.find("diff") != std::string::npos) {
            return derivative(input);
        }

        // Pattern: integrals
        if (lower.find("integral") != std::string::npos || lower.find("integrate") != std::string::npos) {
            return integral(input);
        }

        // Pattern: statistics
        if (lower.find("mean") != std::string::npos || lower.find("average") != std::string::npos ||
            lower.find("median") != std::string::npos || lower.find("stddev") != std::string::npos ||
            lower.find("standard deviation") != std::string::npos || lower.find("variance") != std::string::npos ||
            lower.find("stats") != std::string::npos) {
            return statistics(input);
        }

        // Pattern: correlation
        if (lower.find("correlation") != std::string::npos || lower.find("correlate") != std::string::npos) {
            return correlation(input);
        }

        // Pattern: base conversion
        if (lower.find("binary") != std::string::npos || lower.find("octal") != std::string::npos ||
            lower.find("hex") != std::string::npos || lower.find("to bin") != std::string::npos ||
            lower.find("to oct") != std::string::npos) {
            return baseConvert(input);
        }

        // Pattern: matrix operations
        if (lower.find("determinant") != std::string::npos || lower.find("det ") != std::string::npos ||
            lower.find("inverse") != std::string::npos || lower.find("matrix") != std::string::npos) {
            return matrixOp(input);
        }

        // Pattern: number theory
        if (lower.find("factorial") != std::string::npos || lower.find("fibonacci") != std::string::npos ||
            lower.find("fib ") != std::string::npos || lower.find("gcd") != std::string::npos ||
            lower.find("lcm") != std::string::npos) {
            return numberTheory(input);
        }

        result.error = "Could not parse math expression. Try:\n"
                      "  - solve x^2+3x-4=0\n"
                      "  - convert 5 miles to km\n"
                      "  - what is 15% of 200\n"
                      "  - sin 45\n"
                      "  - derivative x^2 at 3\n"
                      "  - integral x^2 from 0 to 5\n"
                      "  - mean 1 2 3 4 5\n"
                      "  - median 3 1 4 1 5\n"
                      "  - factorial 10\n"
                      "  - fibonacci 12\n"
                      "  - gcd 48 36\n"
                      "  - to binary 255\n"
                      "  - to hex 255\n"
                      "  - determinant 1 2 3 4";
        return result;
    }

    // ─────────────────────────────────────────
    //  CALCULUS: Numerical derivative approximation
    // ─────────────────────────────────────────
    MKMathResult derivative(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        // Parse "derivative x^N at X" or "derivative of x^N at X"
        double exponent = 2.0;
        double coeff = 1.0;
        double atPoint = 1.0;

        // Try to extract x^N
        size_t xpos = lower.find("x^");
        if (xpos != std::string::npos) {
            try { exponent = std::stod(lower.substr(xpos + 2)); } catch (...) {}
            // Check for coefficient before x
            if (xpos > 0) {
                std::string before = lower.substr(0, xpos);
                // Find the last number in before
                std::istringstream iss(before);
                std::string word;
                while (iss >> word) {
                    try { coeff = std::stod(word); } catch (...) {}
                }
                if (coeff == 0) coeff = 1.0;
            }
        } else if (lower.find("x2") != std::string::npos) {
            exponent = 2.0;
        } else if (lower.find("x3") != std::string::npos) {
            exponent = 3.0;
        }

        // Extract "at X"
        size_t atPos = lower.find("at ");
        if (atPos != std::string::npos) {
            try { atPoint = std::stod(lower.substr(atPos + 3)); } catch (...) {}
        }

        // Numerical derivative using central difference: f'(x) = (f(x+h) - f(x-h)) / 2h
        double h = 1e-7;
        auto f = [coeff, exponent](double x) { return coeff * std::pow(x, exponent); };
        double deriv = (f(atPoint + h) - f(atPoint - h)) / (2.0 * h);

        // Also compute symbolic result: d/dx (coeff * x^exp) = coeff*exp * x^(exp-1)
        double symbolic = coeff * exponent * std::pow(atPoint, exponent - 1);

        result.answer = "d/dx(" + formatDouble(coeff) + "*x^" + formatDouble(exponent) + ") at x=" +
                       formatDouble(atPoint) + " = " + formatDouble(symbolic) +
                       "\nSymbolic: " + formatDouble(coeff * exponent) + "*x^" +
                       formatDouble(exponent - 1) +
                       "\nNumerical check: " + formatDouble(deriv);
        result.success = true;
        return result;
    }

    // ─────────────────────────────────────────
    //  CALCULUS: Numerical integration (Simpson's rule)
    // ─────────────────────────────────────────
    MKMathResult integral(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        double exponent = 2.0;
        double coeff = 1.0;
        double a = 0.0, b = 1.0;

        // Parse x^N
        size_t xpos = lower.find("x^");
        if (xpos != std::string::npos) {
            try { exponent = std::stod(lower.substr(xpos + 2)); } catch (...) {}
            if (xpos > 0) {
                std::string before = lower.substr(0, xpos);
                std::istringstream iss(before);
                std::string word;
                while (iss >> word) {
                    try { coeff = std::stod(word); } catch (...) {}
                }
                if (coeff == 0) coeff = 1.0;
            }
        }

        // Parse bounds "from A to B"
        size_t fromPos = lower.find("from ");
        size_t toPos = lower.find(" to ");
        if (fromPos != std::string::npos && toPos != std::string::npos) {
            std::string fromStr = lower.substr(fromPos + 5);
            try { a = std::stod(fromStr); } catch (...) {}
            std::string toStr = lower.substr(toPos + 4);
            try { b = std::stod(toStr); } catch (...) {}
        }

        // Simpson's rule with 1000 intervals
        int n = 1000;
        double h_val = (b - a) / n;
        auto f = [coeff, exponent](double x) { return coeff * std::pow(x, exponent); };

        double sum = f(a) + f(b);
        for (int i = 1; i < n; i++) {
            double x = a + i * h_val;
            sum += (i % 2 == 0) ? 2.0 * f(x) : 4.0 * f(x);
        }
        double numerical = (h_val / 3.0) * sum;

        // Symbolic: integral of coeff*x^exp = coeff * x^(exp+1)/(exp+1) + C
        double symbolic = coeff * (std::pow(b, exponent + 1) - std::pow(a, exponent + 1)) / (exponent + 1);

        result.answer = "integral(" + formatDouble(coeff) + "*x^" + formatDouble(exponent) +
                       ") from " + formatDouble(a) + " to " + formatDouble(b) + " = " +
                       formatDouble(symbolic) +
                       "\nSymbolic antiderivative: " + formatDouble(coeff / (exponent + 1)) +
                       "*x^" + formatDouble(exponent + 1) + " + C" +
                       "\nSimpson's rule check: " + formatDouble(numerical);
        result.success = true;
        return result;
    }

    // ─────────────────────────────────────────
    //  STATISTICS
    // ─────────────────────────────────────────
    MKMathResult statistics(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        // Extract numbers from input
        std::vector<double> numbers;
        std::istringstream iss(input);
        std::string word;
        while (iss >> word) {
            try {
                double v = std::stod(word);
                numbers.push_back(v);
            } catch (...) {}
        }

        if (numbers.empty()) {
            result.error = "No numbers found. Example: mean 1 2 3 4 5";
            return result;
        }

        // Compute all stats
        double sum = 0;
        for (double v : numbers) sum += v;
        double mean = sum / numbers.size();

        // Median
        std::vector<double> sorted = numbers;
        std::sort(sorted.begin(), sorted.end());
        double median;
        if (sorted.size() % 2 == 0)
            median = (sorted[sorted.size()/2 - 1] + sorted[sorted.size()/2]) / 2.0;
        else
            median = sorted[sorted.size()/2];

        // Variance and stddev
        double variance = 0;
        for (double v : numbers) variance += (v - mean) * (v - mean);
        variance /= numbers.size();
        double stddev = std::sqrt(variance);

        if (lower.find("mean") != std::string::npos || lower.find("average") != std::string::npos) {
            result.answer = "Mean = " + formatDouble(mean);
        } else if (lower.find("median") != std::string::npos) {
            result.answer = "Median = " + formatDouble(median);
        } else if (lower.find("stddev") != std::string::npos || lower.find("standard deviation") != std::string::npos) {
            result.answer = "Standard deviation = " + formatDouble(stddev);
        } else if (lower.find("variance") != std::string::npos) {
            result.answer = "Variance = " + formatDouble(variance);
        } else {
            // Show all stats
            result.answer = "Statistics for " + std::to_string(numbers.size()) + " values:\n"
                          "  Mean: " + formatDouble(mean) + "\n"
                          "  Median: " + formatDouble(median) + "\n"
                          "  Std Dev: " + formatDouble(stddev) + "\n"
                          "  Variance: " + formatDouble(variance) + "\n"
                          "  Min: " + formatDouble(sorted.front()) + "\n"
                          "  Max: " + formatDouble(sorted.back()) + "\n"
                          "  Sum: " + formatDouble(sum);
        }
        result.success = true;
        return result;
    }

    // Correlation between two sets (first half, second half of numbers)
    MKMathResult correlation(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::vector<double> numbers;
        std::istringstream iss(input);
        std::string word;
        while (iss >> word) {
            try { numbers.push_back(std::stod(word)); } catch (...) {}
        }
        if (numbers.size() < 4 || numbers.size() % 2 != 0) {
            result.error = "Need even number of values (X1 Y1 X2 Y2 ...). Example: correlation 1 2 2 4 3 6";
            return result;
        }
        size_t n = numbers.size() / 2;
        std::vector<double> x(numbers.begin(), numbers.begin() + n);
        std::vector<double> y(numbers.begin() + n, numbers.end());

        double xMean = 0, yMean = 0;
        for (size_t i = 0; i < n; i++) { xMean += x[i]; yMean += y[i]; }
        xMean /= n; yMean /= n;

        double num = 0, denX = 0, denY = 0;
        for (size_t i = 0; i < n; i++) {
            num += (x[i] - xMean) * (y[i] - yMean);
            denX += (x[i] - xMean) * (x[i] - xMean);
            denY += (y[i] - yMean) * (y[i] - yMean);
        }
        double r = (denX > 0 && denY > 0) ? num / std::sqrt(denX * denY) : 0.0;
        result.answer = "Pearson correlation r = " + formatDouble(r);
        result.success = true;
        return result;
    }

    // ─────────────────────────────────────────
    //  BASE CONVERSION
    // ─────────────────────────────────────────
    MKMathResult baseConvert(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        // Extract number
        long long num = 0;
        std::istringstream iss(input);
        std::string word;
        while (iss >> word) {
            try { num = std::stoll(word); break; } catch (...) {}
        }

        if (lower.find("binary") != std::string::npos || lower.find("bin") != std::string::npos) {
            // Convert to binary
            if (num == 0) { result.answer = formatDouble(num) + " in binary = 0"; result.success = true; return result; }
            std::string bin;
            long long n = std::abs(num);
            while (n > 0) { bin = (char)('0' + n % 2) + bin; n /= 2; }
            if (num < 0) bin = "-" + bin;
            result.answer = formatDouble(num) + " in binary = " + bin;
            result.success = true;
        } else if (lower.find("octal") != std::string::npos || lower.find("oct") != std::string::npos) {
            std::ostringstream oss;
            oss << std::oct << num;
            result.answer = formatDouble(num) + " in octal = " + oss.str();
            result.success = true;
        } else if (lower.find("hex") != std::string::npos) {
            std::ostringstream oss;
            oss << std::hex << std::uppercase << num;
            result.answer = formatDouble(num) + " in hexadecimal = 0x" + oss.str();
            result.success = true;
        } else {
            // Show all bases
            std::string bin;
            long long n = std::abs(num);
            if (n == 0) bin = "0";
            while (n > 0) { bin = (char)('0' + n % 2) + bin; n /= 2; }
            if (num < 0) bin = "-" + bin;
            std::ostringstream oss_oct, oss_hex;
            oss_oct << std::oct << num;
            oss_hex << std::hex << std::uppercase << num;
            result.answer = formatDouble(num) + ":\n"
                          "  Binary: " + bin + "\n"
                          "  Octal: " + oss_oct.str() + "\n"
                          "  Hex: 0x" + oss_hex.str();
            result.success = true;
        }
        return result;
    }

    // ─────────────────────────────────────────
    //  MATRIX OPERATIONS (2x2)
    // ─────────────────────────────────────────
    MKMathResult matrixOp(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        // Extract 4 numbers for 2x2 matrix [[a,b],[c,d]]
        std::vector<double> nums;
        std::istringstream iss(input);
        std::string word;
        while (iss >> word) {
            try { nums.push_back(std::stod(word)); } catch (...) {}
        }

        if (nums.size() < 4) {
            result.error = "Need 4 numbers for 2x2 matrix. Example: determinant 1 2 3 4";
            return result;
        }

        double a = nums[0], b = nums[1], c = nums[2], d = nums[3];

        if (lower.find("determinant") != std::string::npos || lower.find("det") != std::string::npos) {
            double det = a * d - b * c;
            result.answer = "Matrix [[" + formatDouble(a) + "," + formatDouble(b) + "],[" +
                           formatDouble(c) + "," + formatDouble(d) + "]]\n"
                           "Determinant = " + formatDouble(det);
            result.success = true;
        } else if (lower.find("inverse") != std::string::npos) {
            double det = a * d - b * c;
            if (std::abs(det) < 1e-10) {
                result.error = "Matrix is singular (determinant = 0), no inverse exists.";
                return result;
            }
            double ia = d / det, ib = -b / det, ic = -c / det, id = a / det;
            result.answer = "Inverse of [[" + formatDouble(a) + "," + formatDouble(b) + "],[" +
                           formatDouble(c) + "," + formatDouble(d) + "]]:\n"
                           "  [[" + formatDouble(ia) + ", " + formatDouble(ib) + "],\n"
                           "   [" + formatDouble(ic) + ", " + formatDouble(id) + "]]";
            result.success = true;
        } else {
            // Show both
            double det = a * d - b * c;
            result.answer = "Matrix [[" + formatDouble(a) + "," + formatDouble(b) + "],[" +
                           formatDouble(c) + "," + formatDouble(d) + "]]\n"
                           "  Determinant: " + formatDouble(det) + "\n"
                           "  Trace: " + formatDouble(a + d);
            if (std::abs(det) > 1e-10) {
                result.answer += "\n  Invertible: yes";
            } else {
                result.answer += "\n  Invertible: no (singular)";
            }
            result.success = true;
        }
        return result;
    }

    // ─────────────────────────────────────────
    //  NUMBER THEORY
    // ─────────────────────────────────────────
    MKMathResult numberTheory(const std::string& input) const {
        MKMathResult result;
        result.success = false;
        std::string lower = toLower(input);

        if (lower.find("factorial") != std::string::npos) {
            long long n = 0;
            std::istringstream iss(input);
            std::string word;
            while (iss >> word) {
                try { n = std::stoll(word); if (n > 0) break; } catch (...) {}
            }
            if (n < 0 || n > 20) {
                result.error = "Factorial supports 0-20 (to avoid overflow)";
                return result;
            }
            long long fact = 1;
            for (long long i = 2; i <= n; i++) fact *= i;
            result.answer = formatDouble(n) + "! = " + std::to_string(fact);
            result.success = true;
        } else if (lower.find("fibonacci") != std::string::npos || lower.find("fib") != std::string::npos) {
            int n = 0;
            std::istringstream iss(input);
            std::string word;
            while (iss >> word) {
                try { n = std::stoi(word); if (n > 0) break; } catch (...) {}
            }
            if (n < 1 || n > 90) {
                result.error = "Fibonacci supports positions 1-90";
                return result;
            }
            long long a = 0, b_val = 1;
            for (int i = 2; i <= n; i++) {
                long long tmp = a + b_val;
                a = b_val;
                b_val = tmp;
            }
            long long fib = (n == 1) ? 0 : b_val;
            result.answer = "Fibonacci(" + std::to_string(n) + ") = " + std::to_string(fib);
            // Also show sequence up to n (max 15 terms)
            if (n <= 15) {
                result.answer += "\nSequence: ";
                long long fa = 0, fb = 1;
                for (int i = 1; i <= n; i++) {
                    if (i == 1) { result.answer += "0"; }
                    else {
                        result.answer += " " + std::to_string(fb);
                        long long tmp = fa + fb; fa = fb; fb = tmp;
                    }
                }
            }
            result.success = true;
        } else if (lower.find("gcd") != std::string::npos) {
            std::vector<long long> nums;
            std::istringstream iss(input);
            std::string word;
            while (iss >> word) {
                try { nums.push_back(std::stoll(word)); } catch (...) {}
            }
            if (nums.size() < 2) {
                result.error = "Need at least 2 numbers. Example: gcd 48 36";
                return result;
            }
            auto gcdFn = [](long long x, long long y) -> long long {
                x = std::abs(x); y = std::abs(y);
                while (y != 0) { long long t = y; y = x % y; x = t; }
                return x;
            };
            long long g = nums[0];
            for (size_t i = 1; i < nums.size(); i++) g = gcdFn(g, nums[i]);
            result.answer = "GCD = " + std::to_string(g);
            if (nums.size() == 2) {
                long long lcm = std::abs(nums[0] * nums[1]) / g;
                result.answer += "\nLCM = " + std::to_string(lcm);
            }
            result.success = true;
        } else if (lower.find("lcm") != std::string::npos) {
            std::vector<long long> nums;
            std::istringstream iss(input);
            std::string word;
            while (iss >> word) {
                try { nums.push_back(std::stoll(word)); } catch (...) {}
            }
            if (nums.size() < 2) {
                result.error = "Need at least 2 numbers. Example: lcm 12 18";
                return result;
            }
            auto gcdFn = [](long long x, long long y) -> long long {
                x = std::abs(x); y = std::abs(y);
                while (y != 0) { long long t = y; y = x % y; x = t; }
                return x;
            };
            long long l = nums[0];
            for (size_t i = 1; i < nums.size(); i++) {
                l = std::abs(l * nums[i]) / gcdFn(l, nums[i]);
            }
            result.answer = "LCM = " + std::to_string(l);
            result.success = true;
        } else {
            result.error = "Unknown number theory operation. Supported: factorial, fibonacci, gcd, lcm";
        }
        return result;
    }

    // Detect if input is a math query
    bool isMathQuery(const std::string& input) const {
        std::string lower = toLower(input);
        return lower.find("solve") != std::string::npos ||
               lower.find("convert") != std::string::npos ||
               lower.find("calculate") != std::string::npos ||
               lower.find("x^2") != std::string::npos ||
               (lower.find("%") != std::string::npos && lower.find("of") != std::string::npos) ||
               lower.find("sin(") != std::string::npos ||
               lower.find("cos(") != std::string::npos ||
               lower.find("tan(") != std::string::npos ||
               lower.find("derivative") != std::string::npos ||
               lower.find("integral") != std::string::npos ||
               lower.find("integrate") != std::string::npos ||
               lower.find("mean") != std::string::npos ||
               lower.find("median") != std::string::npos ||
               lower.find("stddev") != std::string::npos ||
               lower.find("variance") != std::string::npos ||
               lower.find("correlation") != std::string::npos ||
               lower.find("binary") != std::string::npos ||
               lower.find("octal") != std::string::npos ||
               lower.find("hex") != std::string::npos ||
               lower.find("matrix") != std::string::npos ||
               lower.find("determinant") != std::string::npos ||
               lower.find("factorial") != std::string::npos ||
               lower.find("fibonacci") != std::string::npos ||
               lower.find("gcd") != std::string::npos ||
               lower.find("lcm") != std::string::npos;
    }
};

#endif // MK_MATH_SOLVER_CPP
