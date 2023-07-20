#ifndef CLASS_HPP
#define CLASS_HPP

#include <iostream>
#include <cstring>
#include <vector>
#include <experimental/random>

using namespace std;

// Estructura que almacena la data de los jugadores
struct PlayerData
{
	char playerName[25];
	char playerPiece;
};

struct Players
{
	PlayerData player1;
	PlayerData player2;

	Players(PlayerData p1, PlayerData p2);

	PlayerData turn(int t);
};

Players::Players(PlayerData p1, PlayerData p2)
{
	player1 = p1;
	player2 = p2;
}

PlayerData Players::turn(int t)
{
	if (t % 2 == 0)
	{
		return player1;
	}
	return player2;
}

static int columnOrder[] = {4, 3, 5, 2, 6, 1, 7};

class state_t
{
public:
	static const int width = 7;
	static const int height = 6;
	static const int min_score = -(width * height) / 2 + 3;
	static const int max_score = (width * height + 1) / 2 - 3;
	string board[10] = {"         ",
						"         ",
						"         ",
						"         ",
						"         ",
						"         ",
						"         ",
						"         ",
						"         "};
	vector<int> free_slots{6, 6, 6, 6, 6, 6, 6};
	unsigned int moves = 0;
	uint64_t current_position = 0;
	uint64_t mask = 0;

	int PlayerTurn(PlayerData activePlayer);
	state_t MakeMove(PlayerData activePlayer, int columnChoice);
	state_t RandMove(PlayerData activePlayer);
	bool CheckDown(int columnChoice);
	void BoardPrint(void);
	bool CheckWinner(PlayerData activePlayer);
	int GetWinner(Players players);
	bool CheckDraw(void);
	vector<int> GetPossibleMoves(void);
	uint64_t bottom_mask(int col);
	void bit_play(int col);
	uint64_t key();
};

int state_t::PlayerTurn(PlayerData activePlayer)
{
	int columnChoice;
	do
	{
		cout << "\n";
		cout << activePlayer.playerName << "'s Turn \n";
		cout << "Please enter the column number: ";
		cin >> columnChoice;
		cout << "\n";

		// Si la primera fila en la columna seleccionada tiene una ficha, esta llena.
		while (board[1][columnChoice] == 'O' || board[1][columnChoice] == 'X')
		{
			cout << "Full column, please enter a different number: ";
			cin >> columnChoice;
		}

	} while (columnChoice < 1 || columnChoice > 7);

	return columnChoice;
}

state_t state_t::MakeMove(PlayerData activePlayer, int columnChoice)
{
	state_t s(*this);
	
	s.board[s.free_slots[columnChoice - 1]][columnChoice] = activePlayer.playerPiece;
	s.free_slots[columnChoice - 1]--;
	s.bit_play(columnChoice - 1);
	s.moves++;
	return s;
}

// Random Move for Default Policy in MCTS
state_t state_t::RandMove(PlayerData activePlayer)
{
	state_t s(*this);
	vector<int> pm = s.GetPossibleMoves();
	int size = static_cast<int>(pm.size());

	if (size > 0)
	{
		int col = experimental::randint(0, size - 1);
		s = s.MakeMove(activePlayer, pm[col]);
	}
	return s;
}

bool state_t::CheckDown(int columnChoice)
{
	// Chequea si existe una ficha debajo para apilar la siguiente.
	return free_slots[columnChoice - 1] > 0;
}

void state_t::BoardPrint(void)
{
	// Realiza la impresion del estado del tablero.
	int rows = 6, columns = 7, i, ix;

	cout << " _______ " << endl;

	for (i = 1; i <= rows; i++)
	{
		cout << "|";
		for (ix = 1; ix <= columns; ix++)
		{
			if (board[i][ix] != 'X' && board[i][ix] != 'O')
				board[i][ix] = '*';

			cout << board[i][ix];
		}

		cout << "|" << endl;
	}
	cout << " ^^^^^^^ " << endl;
	cout << " 1234567 " << endl;
}

bool state_t::CheckWinner(PlayerData activePlayer)
{
	char Piece;

	Piece = activePlayer.playerPiece;

	// recorre toda la matriz en busca de las
	// 4 posibles combinaciones para activar
	// el centinela de victoria y terminar
	// la partica.
	for (int i = 8; i >= 1; --i)
	{

		for (int ix = 9; ix >= 1; --ix)
		{

			if (board[i][ix] == Piece &&
				board[i][ix - 1] == Piece &&
				board[i][ix - 2] == Piece &&
				board[i][ix - 3] == Piece)
			{
				return true;
			}

			if (board[i][ix] == Piece &&
				board[i - 1][ix - 1] == Piece &&
				board[i - 2][ix - 2] == Piece &&
				board[i - 3][ix - 3] == Piece)
			{
				return true;
			}

			if (board[i][ix] == Piece &&
				board[i - 1][ix] == Piece &&
				board[i - 2][ix] == Piece &&
				board[i - 3][ix] == Piece)
			{
				return true;
			}

			if (board[i][ix] == Piece &&
				board[i][ix + 1] == Piece &&
				board[i][ix + 2] == Piece &&
				board[i][ix + 3] == Piece)
			{
				return true;
			}

			if (board[i][ix] == Piece &&
				board[i - 1][ix + 1] == Piece &&
				board[i - 2][ix + 2] == Piece &&
				board[i - 3][ix + 3] == Piece)
			{
				return true;
			}
		}
	}

	return false;
}

int state_t::GetWinner(Players players)
{
	if (CheckWinner(players.player1))
	{
		return 1;
	}
	else if (CheckWinner(players.player2))
	{
		return -1;
	}
	return 0;
}

bool state_t::CheckDraw(void)
{
	return moves == 42;
}

vector<int> state_t::GetPossibleMoves(void)
{
	vector<int> possible_moves;
	for (int i = 0; i < 7; i++)
	{
		if (free_slots[i] != 0)
		{
			possible_moves.push_back(i + 1);
		}
	}
	return possible_moves;
}

uint64_t state_t::bottom_mask(int col)
{
	return UINT64_C(1) << col * (height + 1);
}

void state_t::bit_play(int col)
{
	current_position ^= mask;
	mask |= mask + bottom_mask(col);
}

uint64_t state_t::key()
{
	return current_position + mask;
}

void WinnerMessage(PlayerData activePlayer)
{
	// solo imprime un mensaje al ganador.
	cout << endl
		 << activePlayer.playerName << " Wins!" << endl;
}

class Node
{
public:
	int visits = 1;
	float reward = 0.0;
	state_t state;
	Node *parent;
	vector<Node*> children;
	vector<int> children_move;
	bool is_root = false;

	Node(Node *n);
	Node(state_t s);
	Node(state_t s, Node *p);

	void AddChild(state_t child, int col);
	void Update(float r);
	bool FullyExplored(void);
};

Node::Node(Node *n)
{
	visits = n->visits;
	reward = n->reward;
	state = n->state;
	parent = n->parent;
	children = n->children;
	children_move = n->children_move;
}

Node::Node(state_t s)
{
	state = s;
}

Node::Node(state_t s, Node *p)
{
	state = s;
	parent = p;
}

void Node::AddChild(state_t child, int col)
{
	Node *new_child = new Node(child, this);
	children.push_back(new_child);
	children_move.push_back(col);
}

void Node::Update(float r)
{
	reward += r;
	visits++;
}

bool Node::FullyExplored(void)
{
	return children.size() == state.GetPossibleMoves().size();
}

void FillBoard(state_t &state, string seq, Players p, int &turn)
{
	PlayerData player;
	for (unsigned int i = 0; i < seq.size(); i++)
	{
		int col = seq[i] - '1';
		col++;

		player = p.turn(state.moves);
		turn = -turn;
		/*
	if (state.moves % 2 == 0) {
		player = player1;
	} else {
		player = player2;
	}
	*/

		state = state.MakeMove(player, col);
	}
}

#endif