#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <limits>
#include <cmath>
#include <utility>
#include <experimental/random>
#include "class.hpp"
#include "ttable.hpp"

using namespace std;
using namespace std::chrono;

unsigned expanded = 0;
unsigned generated = 0;
TranspositionTable TTable(8388593);

vector<state_t> child_vector(state_t state, PlayerData player)
{
    vector<state_t> movement;
    state_t new_state;

    for (int i = 0; i < 7; i++)
    {
        if (state.CheckDown(columnOrder[i]))
        {
            new_state = state.MakeMove(player, columnOrder[i]);
            movement.push_back(new_state);
        }
    }

    return movement;
}

pair<state_t, bool> check_children(state_t state, PlayerData player)
{
    vector<state_t> children = child_vector(state, player);

    for (state_t child : children)
    {
        if (child.CheckWinner(player))
        {
            return {child,true};
        }
    }

    return {state,false};
}

// Monte Carlo Tree Search

Node *expand(Node *node, PlayerData player)
{
    vector<int> tried_moves = node->children_move;
    vector<int> possible_moves = node->state.GetPossibleMoves();
    state_t s(node->state);
    int col;
    for (int i = 0; i < possible_moves.size(); i++)
    {
        col = possible_moves[i];
        if (find(tried_moves.begin(), tried_moves.end(), possible_moves[i]) == tried_moves.end())
        {
            s = s.MakeMove(player, possible_moves[i]);
            break;
        }
    }
    node->AddChild(s, col);

    return node->children[node->children.size() - 1];
}

Node *best_child(Node *node, float factor)
{
    float bestScore = numeric_limits<float>::lowest();
    //float bestScore = -1000000;
    vector<Node *> bestChildren;
    for (int i = 0; i < node->children.size(); i++)
    {
        float f = node->children[i]->reward / node->children[i]->visits;
        float g = sqrt(log(2 * node->visits) / node->children[i]->visits);
        float score = f + factor * g;

        if (score == bestScore)
        {
            bestChildren.push_back(node->children[i]);
        }
        else if (score > bestScore)
        {
            bestChildren.clear();
            bestChildren.push_back(node->children[i]);
            bestScore = score;
        }
    }
    int size = static_cast<int>(bestChildren.size());
    int choice = experimental::randint(0, size - 1);
    Node *c = bestChildren[choice];
    return bestChildren[choice];
}

struct NT
{
    Node *n;
    int t;
};

NT selection(Node *node, Players players, int &turn, float factor)
{
    Node *tmp(node);
    while (!tmp->state.CheckDraw() && tmp->state.GetWinner(players) == 0)
    {
        //tmp->state.BoardPrint();
        if (!tmp->FullyExplored())
        {
            tmp = expand(tmp, players.turn(tmp->state.moves));
            NT r = {tmp, -turn};
            return r;
        }
        else
        {
            tmp = best_child(tmp, factor);
            turn = -turn;
        }
    }
    NT r = {tmp, turn};
    return r;
}

float simulation(state_t state, Players players, int &turn)
{
    while (!state.CheckDraw() && state.GetWinner(players) == 0)
    {
        state = state.RandMove(players.turn(state.moves));
        turn = -turn;
    }
    return state.GetWinner(players);
}

void backpropagation(Node *node, float reward, Players players, int &turn)
{
    while (true)
    {
        node->visits++;
        node->reward -= turn * reward;
        turn = -turn;
        if (node->is_root)
        {
            break;
        }

        node = node->parent;
    }
}

Node MCTS(int max_iter, Node *root, float factor, Players players, int &turn)
{
    for (int i = 0; i < max_iter; i++)
    {
        //cout << "iter " << i << endl;
        NT r = selection(root, players, turn, factor);
        //cout << "tp" << endl;
        float reward = simulation(r.n->state, players, r.t);
        //cout << "dp" << endl;
        backpropagation(r.n, reward, players, r.t);
        //cout << "bu" << endl;
    }
    return *best_child(root, 0);
}