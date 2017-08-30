#include "SLScanner/RegularExpression.h"
#include <stack>
#include <queue>

using namespace Silly;

RegularExpression::RegularExpression(std::string pattern_) {
    pattern = pattern_;
    for (Alphabet c : pattern_) {
        if (c != '|' && c != '+' && c != '*' && c != '(' && c != ')')
            alphabets.insert(c);
    }
    std::string postfix = toPostfix();
    try {
        buildTree(postfix);
    } catch (std::string str) {
        std::cerr << str << std::endl;
        std::cerr << "Sample: \"(1|2|3|4)*+5+6\"" << std::endl;
    }
#if SL_GTEST_PROTECTED_OPEN_RE
    std::cout << "POSTFIX FROM TREE\n" << toPostfixFromTree(tree) << std::endl;
#endif
    buildNFA();
    reboot();
}

NondeterministicFiniteAutomaton::StateType RegularExpression::operator()(std::string str) {
    return NondeterministicFiniteAutomaton::operator()(str);
}

bool RegularExpression::operator[](std::string str) {
    return NondeterministicFiniteAutomaton::operator()(std::move(str)) == NondeterministicFiniteAutomaton::Accepted;
}

std::string RegularExpression::toPostfix() {
    std::stack<Alphabet> working;
    std::string postfix;
    for (Alphabet a : pattern) {
        if (a == '(' || a == '+' || a == '|') {
            working.push(a);
        } else if (a == ')' || a == '\0') {
            while (true) {
                Alphabet out = working.top();
                working.pop();
                if (out == '(' || working.empty()) {
                    break;
                }
                postfix += out;

            }
        } else {
            postfix += a;
        }
    }
    while (!working.empty()) {
        Alphabet out = working.top();
        working.pop();
        postfix += out;
    }
    return postfix;
}

void RegularExpression::buildTree(std::string postfix) {
#if SL_GTEST_PROTECTED_OPEN_RE
    std::cout << "\nPOSTFIX\n" << postfix << std::endl;
#endif
    std::stack<ETreeNode> working;
    for (char &it : postfix) {
        ETreeNode node(new ExpressionTree);
        node->expression = it;
        if (it == '+' || it == '|') {
            if (working.size() < 2)
                throw std::string("Wrong Expression.");
            node->right = std::move(working.top());
            working.pop();
            node->left = std::move(working.top());
            working.pop();
        } else if (it == '*') {
            node->left = std::move(working.top());
            working.pop();
        }
        working.push(std::move(node));
    }
    tree = std::move(working.top());
}

void RegularExpression::buildNFA() {
    currentState = StartState + 1;
    std::pair<State, State> finalTerm = buildNode(tree);
    nETransitions.insert(ETransition(StartState, finalTerm.first));
    State accept = currentState++;
    nETransitions.insert(ETransition(finalTerm.second, accept));
    nAcceptingStates.insert(accept);
    construct();
}

std::pair<FiniteAutomaton::State, FiniteAutomaton::State> RegularExpression::buildNode(const ETreeNode &node) {
    std::pair<State, State> leftTerm(-1, -1), rightTerm(-1, -1);
    if (!node)
        return std::pair<State, State>(-1, -1);
    if (node->left)
        leftTerm = buildNode(node->left);
    if (node->right)
        rightTerm = buildNode(node->right);
    switch (node->expression) {
        case '+': {
            if (leftTerm.first == -1 || leftTerm.second == -1) {
                throw std::string("Bad Left Term");
            }
            if (rightTerm.first == -1 || rightTerm.second == -1) {
                throw std::string("Bad Right Term");
            }
            State state0 = currentState++;
            ETransition transition0(state0, leftTerm.first);
            State state1 = currentState++;
            ETransition transition1(leftTerm.second, state1);
            State state2 = currentState++;
            ETransition transition2(state1, state2);
            ETransition transition3(state2, rightTerm.first);
            State state3 = currentState++;
            ETransition transition4(rightTerm.second, state3);
            nETransitions.insert(transition0);
            nETransitions.insert(transition1);
            nETransitions.insert(transition2);
            nETransitions.insert(transition3);
            nETransitions.insert(transition4);
            nStates.insert(state0);
            nStates.insert(state1);
            nStates.insert(state2);
            nStates.insert(state3);
            return std::pair<State, State>(state0, state3);
        }
        case '|': {
            if (leftTerm.first == -1 || leftTerm.second == -1) {
                throw std::string("Bad Left Term");
            }
            if (rightTerm.first == -1 || rightTerm.second == -1) {
                throw std::string("Bad Right Term");
            }
            State state0 = currentState++;
            ETransition transition0(state0, leftTerm.first);
            ETransition transition1(state0, rightTerm.first);
            State state1 = currentState++;
            ETransition transition2(leftTerm.second, state1);
            ETransition transition3(rightTerm.second, state1);
            nETransitions.insert(transition0);
            nETransitions.insert(transition1);
            nETransitions.insert(transition2);
            nETransitions.insert(transition3);
            nStates.insert(state0);
            nStates.insert(state1);
            return std::pair<State, State>(state0, state1);
        }
        case '*': {
            if (leftTerm.first == -1 || leftTerm.second == -1) {
                throw std::string("Bad Left Term");
            }
            State state0 = currentState++;
            ETransition transition0(state0, leftTerm.first);
            ETransition transition1(leftTerm.second, leftTerm.first);
            State state1 = currentState++;
            ETransition transition2(leftTerm.second, state1);
            ETransition transition3(state0, state1);
            nETransitions.insert(transition0);
            nETransitions.insert(transition1);
            nETransitions.insert(transition2);
            nETransitions.insert(transition3);
            nStates.insert(state0);
            nStates.insert(state1);
            return std::pair<State, State>(state0, state1);
        }
        default: {
            State state0 = currentState++;
            State state1 = currentState++;
            nTransitions[Move(state0, node->expression)] = state1;
            return std::pair<State, State>(state0, state1);
        }
    }
}

#if SL_GTEST_PROTECTED_OPEN_RE
std::string RegularExpression::toPostfixFromTree(ETreeNode &node) {
    std::string postfix_;
    if (node->left)
        postfix_ += toPostfixFromTree(node->left);
    if (node->right)
        postfix_ += toPostfixFromTree(node->right);
    postfix_ += node->expression;
    return postfix_;
}

#endif