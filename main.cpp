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
            return {child, true};
        }
    }

    return {state, false};
}

std::pair<state_t, int> minimax(state_t state, int depth, Players players, int turn)
{
    int score;
    ++generated;
    pair<state_t, int> result = {state, -turn * numeric_limits<int>::max()};

    if (depth == 15)
    {
        TTable.reset();
    }

    if (state.CheckDraw())
    {
        return {state, 0};
    }

    pair<state_t, bool> checked_children = check_children(state, players.turn(state.moves));

    if (checked_children.second)
    {
        return {checked_children.first, -turn * (state.width * state.height + 1 - state.moves) / 2};
    }

    int max = -turn * (state.width * state.height - 1 - state.moves) / 2;
    if (int info = TTable.get(state.key()))
    {
        max = info + state.min_score - 1;
    }

    // Si no es estado terminal, expande
    ++expanded;
    score = -turn * numeric_limits<int>::max();
    vector<state_t> child_states = child_vector(state, players.turn(state.moves));

    for (state_t child : child_states)
    {
        score = -minimax(child, depth - 1, players, -turn).second;
        if (score >= max)
            return {child, score};
        if (score > result.second)
        {
            result = {child, score};
        }
    }

    TTable.put(state.key(), -turn * (result.second - state.min_score + 1));

    return result;
}

int get_best_move(state_t state, Players players)
{
    int best_move = -1;
    int best_score = std::numeric_limits<int>::min();

    for (int col : columnOrder)
    {
        if (state.CheckDown(col))
        {
            state_t new_state = state.MakeMove(players.turn(state.moves), col);
            std::pair<state_t, int> result = minimax(new_state, 15, players, -1);

            int score = result.second; // Obtener el puntaje del resultado

            if (score > best_score)
            {
                best_score = score;
                best_move = col;
            }
        }
    }

    return best_move;
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
        NT r = selection(root, players, turn, factor);
        float reward = simulation(r.n->state, players, r.t);
        backpropagation(r.n, reward, players, r.t);
    }
    return *best_child(root, 0);
}


int main(int argc, const char **argv)
{
    int option = 0;
    string seq = "";
    if (argc > 1)
    {
        option = atoi(argv[1]);
        if (argc > 2)
        {
            seq = argv[2];
        }
    }
    else
    {
        cout << "No arguments\nCorrect syntax: ./connect4 <option> <initial state>" << endl
             << "Options: 1 - MCTS VS MCTS" << endl
             << "         2 - MCTS VS PLAYER " << endl
             << "         3 - MCTS VS MINIMAX " << endl
             << "         4 - MINIMAX VS PLAYER" << endl
             << "         5 - MINIMAX VS MINIMAX" << endl;
        exit(1);
    }

    PlayerData playerOne, playerTwo;
    state_t b, move;
    pair<state_t, int> mini;

    int columnChoice;
    int turn = 1;
    int choice;

    // Initialize player data and other variables based on the chosen option
    switch (option)
    {
    case 1:
	strcpy(playerOne.playerName, "MCTS 1");
        strcpy(playerTwo.playerName, "MCTS 2");
        break;
    case 2:
    	strcpy(playerOne.playerName, "MCTS");
        strcpy(playerTwo.playerName, "PLAYER");
        break;
    case 3:
   	strcpy(playerOne.playerName, "MCTS");
        strcpy(playerTwo.playerName, "MINIMAX");
        break;
    case 4:
        strcpy(playerOne.playerName, "MINIMAX");
        strcpy(playerTwo.playerName, "PLAYER");
        break;
    case 5:
        strcpy(playerOne.playerName, "MINIMAX 1");
        strcpy(playerTwo.playerName, "MINIMAX 2");
        break;
    }

    playerOne.playerPiece = 'X';
    playerTwo.playerPiece = 'O';

    Players players(playerOne, playerTwo);

    FillBoard(b, seq, players, turn);

    cout << "Initial state: " << endl;
    b.BoardPrint();

    int iters = 5000;
    float factor = 1;
    Node root(b);
    root.is_root = true;
    pair<state_t, Node> next_move = {b, root};

    time_point<high_resolution_clock> start_time = high_resolution_clock::now();

    if (option == 1) // MCTS VS MCTS
{
    Node result = MCTS(iters, &root, factor, players, turn);
    while (true)
    {
        if (result.state.CheckDraw() || result.state.GetWinner(players) != 0)
            break;

        result = MCTS(iters, &result, factor, players, turn);
    }

    next_move.first = result.state;
}
else if (option == 2) // MCTS VS PLAYER
{
    while (true)
    {
        b.BoardPrint();
        if (b.CheckDraw() || b.GetWinner(players) != 0)
            break;

        if (turn == 1)
        {
            Node result = MCTS(iters, &(next_move.second), factor, players, turn);
            next_move = {result.state, result};
            turn = -1;
        }
        else
        {
            int choice = b.PlayerTurn(playerTwo);
            b = next_move.first.MakeMove(playerTwo, choice);
            Node root(b);
            root.is_root = true;
            next_move = {b, root};
            turn = 1;
        }
    }
}
else if (option == 3) // MCTS VS MINIMAX 
{
    while (true)
    {
        b.BoardPrint();
        if (b.CheckDraw() || b.GetWinner(players) != 0)
            break;

        if (turn == 1)
        {
            Node result = MCTS(iters, &(next_move.second), factor, players, turn);
            next_move = {result.state, result};
            turn = -1;
        }
        else
        {
            int best_move = get_best_move(b, players);
            b = b.MakeMove(playerTwo, best_move);
            Node root(b);
            root.is_root = true;
            next_move = {b, root};
            turn = 1;
        }
    }
}
else if (option == 4) // MINIMAX VS PLAYER
{
    while (true)
    {
        b.BoardPrint();
        if (b.CheckDraw() || b.GetWinner(players) != 0)
            break;

        if (turn == 1)
        {
            int best_move = get_best_move(b, players);
            b = b.MakeMove(playerOne, best_move);
            turn = -1;
        }
        else
        {
            int choice = b.PlayerTurn(playerTwo);
            b = b.MakeMove(playerTwo, choice);
            turn = 1;
        }
    }
    next_move.first = b;
}


else if (option == 5) // MINIMAX VS MINIMAX
{
    while (true)
    {
        b.BoardPrint();
        if (b.CheckDraw() || b.GetWinner(players) != 0)
            break;

        if (turn == 1)
        {
            int best_move = get_best_move(b, players);
            b = b.MakeMove(playerOne, best_move);
            turn = -1;
        }
        else
        {
            int best_move = get_best_move(b, players);
            b = b.MakeMove(playerTwo, best_move);
            turn = 1;
        }
    }
    next_move.first = b;
}

    duration<double> elapsed_time = high_resolution_clock::now() - start_time;

    cout << "Last state: " << endl;
    next_move.first.BoardPrint();

    cout << "\nInitial state: " << seq << endl;
    int winner = next_move.first.GetWinner(players);
    if (winner == 1)
    {
        cout << "WINNER: " << playerOne.playerName << " " << playerOne.playerPiece << endl;
    }
    else if (winner == -1)
    {
        cout << "WINNER: " << playerTwo.playerName << " " << playerTwo.playerPiece << endl;
    }
    else if (next_move.first.CheckDraw())
    {
        cout << "DRAW" << endl;
    }

    cout << "Time: " << elapsed_time.count() << " seconds" << endl;

    return 0;
}
