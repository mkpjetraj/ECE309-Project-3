// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"

#include <stack>

// * and / are 2, + and - are 1, ( is 0
static int prec(char op) {
    if (op == '*' || op == '/') return 2;
    if (op == '+' || op == '-') return 1;
    return 0;
}

// pop 2 numbers and 1 op, do the math, push the answer
static bool doOp(std::stack<double>& nums, std::stack<char>& ops, std::string* error) {
    if (nums.size() < 2 || ops.empty()) {
        *error = "bad expression";
        return false;
    }
    char op = ops.top();
    ops.pop();
    double b = nums.top();   // right side comes off first
    nums.pop();
    double a = nums.top();
    nums.pop();

    if (op == '+') nums.push(a + b);
    else if (op == '-') nums.push(a - b);
    else if (op == '*') nums.push(a * b);
    else if (op == '/') {
        if (b == 0) {
            *error = "division by zero";
            return false;
        }
        nums.push(a / b);
    } else {
        *error = "bad expression";   // a ( got left over
        return false;
    }
    return true;
}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out, std::string* error) {
    std::stack<double> nums;
    std::stack<char> ops;

    for (size_t i = 0; i < tokens.size(); i++) {
        CalcToken t = tokens[i];

        if (t.kind == CalcToken::Kind::Number) {
            nums.push(t.value);
        } else if (t.kind == CalcToken::Kind::LParen) {
            ops.push('(');
        } else if (t.kind == CalcToken::Kind::RParen) {
            while (!ops.empty() && ops.top() != '(') {
                if (!doOp(nums, ops, error)) return false;
            }
            if (ops.empty()) {
                *error = "bad expression";
                return false;
            }
            ops.pop();   // get rid of the (
        } else {
            char c = '/';
            if (t.kind == CalcToken::Kind::Plus) c = '+';
            else if (t.kind == CalcToken::Kind::Minus) c = '-';
            else if (t.kind == CalcToken::Kind::Times) c = '*';

            // >= so 10-4-3 goes left to right
            while (!ops.empty() && ops.top() != '(' && prec(ops.top()) >= prec(c)) {
                if (!doOp(nums, ops, error)) return false;
            }
            ops.push(c);
        }
    }

    while (!ops.empty()) {
        if (!doOp(nums, ops, error)) return false;
    }

    if (nums.size() != 1) {
        *error = "bad expression";
        return false;
    }
    *out = nums.top();
    return true;
}
