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

        result.error = "Could not parse math expression. Try:\n"
                      "  - solve x^2+3x-4=0\n"
                      "  - convert 5 miles to km\n"
                      "  - what is 15% of 200\n"
                      "  - sin 45";
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
               lower.find("tan(") != std::string::npos;
    }
};

#endif // MK_MATH_SOLVER_CPP
