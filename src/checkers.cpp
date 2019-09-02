// TODO:  (1) use transposition tables for seen board positions (reduce evaluation calculation)
//        (2) perform smart move ordering on minimax tree (increases pruning frequency)
//        (3) increase operation efficiency by using efficient data structures
//        (4) clean up naming / code in general for readability and brevity (along with efficiency)
//  Most of this is in the name of increasing search depths in the minimax tree
#include <algorithm>
#include <checkers.hpp>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <termcolor.hpp>



////////////////////////////////////////////////////////////////////////////////////
// BEGIN  Checkers parameter definitions (in order of appearance in checkers.hpp) //
////////////////////////////////////////////////////////////////////////////////////
char Checkers::squareMap[5] = {
      '*' // Regular - Player 1
    , 'o' // Regular - Player 2
    , '#' // King - Player 1
    , '8' // King - Player 2
    , ' ' // Empty
};
char Checkers::columnMap[8] = {
      'A'
    , 'B'
    , 'C'
    , 'D'
    , 'E'
    , 'F'
    , 'G'
    , 'H'
};
unsigned int Checkers::timeLimitLower = 3;
unsigned int Checkers::timeLimitUpper = 60;
unsigned int Checkers::moveLimit = 50;
double Checkers::timeRemainingThreshold = 0.1; // in seconds
////////////////////////////////////
// END  Player method definitions //
////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// BEGIN  Player method definitions (in order of appearance in checkers.hpp) //
///////////////////////////////////////////////////////////////////////////////
Checkers::Player::Player() {
    this->game = nullptr;
    this->isComputer = false;
    this->previousMoveTime = Duration::zero();
    this->totalMoveTime = Duration::zero();
    this->timeLimit = 0;
    this->maxDepthReached = 0;
}


Checkers::Player::Player(Checkers::Game * game, bool isComputer, double timeLimit) {
    this->game = game;
    this->isComputer = isComputer;
    this->previousMoveTime = Duration::zero();
    this->totalMoveTime = Duration::zero();
    this->timeLimit = timeLimit;
    this->maxDepthReached = 0;
}


Checkers::Move Checkers::Player::makeMove(Checkers::Board const& board) {
    Checkers::Move move;
    std::vector<Checkers::Move> moveList;
    std::string playerAction;
    std::string savedGameFilePath;
    bool playerMoved;
    uint8_t i;

    // computer
    if(this->isComputer) {
        moveList = this->game->getMovesFromBoard_andPlayer(board, this->game->getPlayerTurn());

        if(!moveList.size()) {
            move.player = this->game->getPlayerTurn();
            move.xPath[0] = move.yPath[0] = 0xFFU;
            playerMoved = true;
            this->game->stop();
        }

        else if(moveList.size() == 1) {
            move = moveList[0];
            this->maxDepthReached = 0;
        }

        else {
            //move = moveList[std::rand() & (moveList.size() - 1)]; // pick randomly
            move = this->pickMoveFromBoard(board); // pick using alpha beta
        }
    }

    // human
    else {
        playerMoved = false;

        // get possible moves
        moveList = this->game->getMovesFromBoard_andPlayer(board, this->game->getPlayerTurn());

        // if there are no possible moves, end the game
        if(!moveList.size()) {
            move.player = this->game->getPlayerTurn();
            move.xPath[0] = move.yPath[0] = 0xFFU;
            playerMoved = true;
            this->game->stop();
        }

        while(!playerMoved) {
            for(i = 0; i < moveList.size(); i++) {
                std::cout << "  [" << (i + 1) << "]  " << this->game->parseMove(moveList[i]) << std::endl;
            }
            std::cout << "  [S]  Save game" << std::endl;
            std::cout << "  [Q]  Forfeit game" << std::endl;
            
            while((std::cout << "Please select an action: ")
                    && (!(std::cin >> playerAction)
                        || (   (playerAction.c_str()[0] != 'S' && playerAction.c_str()[0] != 's')
                            && (playerAction.c_str()[0] != 'Q' && playerAction.c_str()[0] != 'q')
                            && ((uint8_t) std::strtoll(playerAction.c_str(), nullptr, 10) < 1
                                || (uint8_t) std::strtoll(playerAction.c_str(), nullptr, 10) > moveList.size())))) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            
            if(playerAction.c_str()[0] == 'S' || playerAction.c_str()[0] == 's') {
                std::cout << "Please enter a saved game file path: ";
                std::cin >> savedGameFilePath;
                this->game->save(savedGameFilePath);
            } else if(playerAction.c_str()[0] == 'Q' || playerAction.c_str()[0] == 'q') {
                move.player = this->game->getPlayerTurn();
                move.xPath[0] = move.yPath[0] = 0xFFU;
                playerMoved = true;
                std::cout << "Quitting game ..." << std::endl;
                this->game->stop();
            } else {
                move = moveList[std::strtoll(playerAction.c_str(), nullptr, 10) - 1];
                playerMoved = true;
            }
        }
    }

    return move;
}


Checkers::Move Checkers::Player::pickMoveFromBoard(Checkers::Board const& board) {
    // the tree variables
    uint8_t depth, maxDepthReached = 1;
    struct {
        bool isMaxNode;
        int alpha;
        int beta;
        int value;
        Checkers::Board nodeBoard;
        std::vector<Checkers::Move> nodeMoves;
        int moveIterator;
        int numMoves;
    } nodeStack[50]; // i is even => max node, i is odd => min node

    // player variables
    uint8_t players[2];
    players[0] = this->game->getPlayerTurn();
    players[1] = (~players[0]) & 1;

    // move variables
    Checkers::Move theMove;

    // time variables
    Checkers::Time timeInitial = this->game->getMoveStartTime();
    Checkers::Duration timeDiff;
    double timeRemaining = this->timeLimit;

    int score, bestMoveIndex; //bestMoveIndexFinished;

    // initialize the root node
    nodeStack[0].nodeBoard = board;
    nodeStack[0].nodeMoves = Checkers::Game::getMovesFromBoard_andPlayer(board, players[0]);
    nodeStack[0].numMoves = nodeStack[0].nodeMoves.size();
    nodeStack[0].isMaxNode = true;

    do {
        depth = 0;

        // refresh the root node
        nodeStack[0].alpha = INT_MIN;
        nodeStack[0].beta = INT_MAX;
        nodeStack[0].value = INT_MIN;
        nodeStack[0].moveIterator = 0;

        // while we still haven't checked all the moves of the root node
        while(nodeStack[0].moveIterator < nodeStack[0].numMoves && timeRemaining > Checkers::timeRemainingThreshold) {
            // no need to evaluate other moves; either a cut off, or all moves were evaluated
            if(    nodeStack[depth].beta <= nodeStack[depth].alpha
                || nodeStack[depth].moveIterator >= nodeStack[depth].numMoves) {
                // stop evaluating children
                // update the parent, decrease the depth

                if(!depth--) {
                    if(nodeStack[1].value > nodeStack[0].value) {
                        nodeStack[0].value = nodeStack[1].value;
                        bestMoveIndex = nodeStack[0].moveIterator - 1;
                    }

                    if(nodeStack[0].value > nodeStack[0].alpha) {
                        nodeStack[0].alpha = nodeStack[0].value;
                    }

                    break;
                }

                if(nodeStack[depth].isMaxNode) {
                    if(nodeStack[depth + 1].value > nodeStack[depth].value) {
                        nodeStack[depth].value = nodeStack[depth + 1].value;
                        if(!depth) {
                            bestMoveIndex = nodeStack[depth].moveIterator - 1;
                        }
                    }
                    
                    if(nodeStack[depth].value > nodeStack[depth].alpha) {
                        nodeStack[depth].alpha = nodeStack[depth].value;
                    }
                }

                else {
                    if(nodeStack[depth + 1].value < nodeStack[depth].value) {
                        nodeStack[depth].value = nodeStack[depth + 1].value;
                    }
                    
                    if(nodeStack[depth].value < nodeStack[depth].beta) {
                        nodeStack[depth].beta = nodeStack[depth].value;
                    }
                }
            }

            // we still need to evaluate other moves
            else {
                // get the next node's board from our list of moves, then increment the move iterator
                nodeStack[depth + 1].nodeBoard =
                    this->game->getNextBoardFromMove_andBoard(nodeStack[depth].nodeMoves[nodeStack[depth].moveIterator]
                                                              , nodeStack[depth].nodeBoard);
                nodeStack[depth].moveIterator++;

                // next node is not @ max depth... 
                if(depth + 1 < maxDepthReached) {
                    // go one level deeper
                    depth++;

                    // make the child node the opposite of the parent node
                    nodeStack[depth].isMaxNode = !nodeStack[depth - 1].isMaxNode;

                    nodeStack[depth].value = nodeStack[depth].isMaxNode? INT_MIN :INT_MAX;
                    nodeStack[depth].beta = nodeStack[depth - 1].beta;
                    nodeStack[depth].alpha = nodeStack[depth - 1].alpha;

                    nodeStack[depth].nodeMoves = Checkers::Game::getMovesFromBoard_andPlayer(nodeStack[depth].nodeBoard, players[depth & 1]);
                    nodeStack[depth].numMoves = nodeStack[depth].nodeMoves.size();

                    nodeStack[depth].moveIterator = 0;
                }
                
                // next node is leaf node => we evaluate heuristic function and update our values
                else {
                    score = this->evaluateBoard(nodeStack[depth + 1].nodeBoard);

                    if(nodeStack[depth].isMaxNode) {
                        if(score > nodeStack[depth].value) {
                            nodeStack[depth].value = score;
                            if(!depth) {
                                bestMoveIndex = nodeStack[depth].moveIterator - 1;
                            }
                        }
                        
                        if(nodeStack[depth].value > nodeStack[depth].alpha) {
                            nodeStack[depth].alpha = nodeStack[depth].value;
                        }
                    }

                    else {
                        if(score < nodeStack[depth].value) {
                            nodeStack[depth].value = score;
                        }

                        if(nodeStack[depth].value < nodeStack[depth].beta) {
                            nodeStack[depth].beta = nodeStack[depth].value;
                        }
                    }
                }
            }

            timeDiff = Checkers::Clock::now() - timeInitial;
            timeRemaining = this->timeLimit - double(timeDiff.count()) * Checkers::Clock::period::num / Checkers::Clock::period::den;
        }
        
        // only update the move if the search finished
        // also, only increase the max depth if it played a role in picking the move
        if(timeRemaining > Checkers::timeRemainingThreshold) {
            this->maxDepthReached = maxDepthReached++;
            theMove = nodeStack[0].nodeMoves[bestMoveIndex];
        }
        timeDiff = Checkers::Clock::now() - timeInitial;
        timeRemaining = this->timeLimit - double(timeDiff.count()) * Checkers::Clock::period::num / Checkers::Clock::period::den;
    } while(timeRemaining > Checkers::timeRemainingThreshold && maxDepthReached < 50);

    return theMove;
}


int Checkers::Player::evaluateBoard(Board const& board) {
    int i, j;
    int score[2] = {0};
    int pieceCount[2] = {0};
    uint8_t me = this->game->getPlayerTurn();
    uint8_t opponent = ~me & 1;
    uint8_t square;

    Checkers::Piece tempPiece;

    // challenge empty squares attacked by the enemy pieces
    // control the sides

    for(i = 0; i < 2; i++) {
        for(j = 0; j < 12; j++) {
            tempPiece = board.pieces[i][j];

            if(tempPiece.xPos <= 7 && tempPiece.yPos <= 7) {
                // track each player's piece counts (for checking for win conditions)
                pieceCount[i]++;

                // 1 : piece counts and types
                score[i] += 15 * (100 + 80 * tempPiece.isKing);
                
                // 2 : protect regular pieces
                if(tempPiece.yPos + (2 * i - 1) >= 0 && tempPiece.yPos + (2 * i - 1) <= 7 && !tempPiece.isKing) {
                    if(tempPiece.xPos - 1 >= 0) {
                        // check squares behind
                        square = board.squares[tempPiece.yPos + (2 * i - 1)][tempPiece.xPos - 1];
                        if(square != 4 && (square & 1) == i) {
                            score[i] += 150;
                        }
                    }

                    if(tempPiece.xPos + 1 <= 7) {
                        // check squares behind
                        square = board.squares[tempPiece.yPos + (2 * i - 1)][tempPiece.xPos + 1];
                        if(square != 4 && (square & 1) == i) {
                            score[i] += 150;
                        }
                    }

                    // challenge enemy pieces
                    if(tempPiece.yPos + (2 - 4 * i) >= 0 && tempPiece.yPos + (2 - 4 * i) <= 7) {
                        square = board.squares[tempPiece.yPos + (2 - 4 * i)][tempPiece.xPos];
                        if(square != 4 && (square & 1) == (~i & 1)) {
                            score[i] += 100;
                        }
                    }
                }

                // 3 : control the center
                score[i] += 3 * (100 - ((abs(4 - tempPiece.xPos) + abs(4 - tempPiece.yPos)) * 10));

                // 4 : advance regular pieces, and
                // 5 : keep regular pieces on back rank if possible
                if(!tempPiece.isKing) {
                    score[i] += 5 * (10 * abs(7 * i - tempPiece.yPos));
                    if(tempPiece.yPos == 7 * i) {
                        score[i] += 300;
                    }
                }
                
            }
        }
    }

    // I have no more pieces => loss
    if(!pieceCount[me]) {
        return INT_MIN;
    }

    // opponent has no more pieces => win
    if(!pieceCount[opponent]) {
        return INT_MAX;
    }

    // I have no more moves => loss
    if(Checkers::Game::getMovesFromBoard_andPlayer(board, me).size() == 0) {
        return INT_MIN;
    }

    // opponent has no more moves => win
    if(Checkers::Game::getMovesFromBoard_andPlayer(board, opponent).size() == 0) {
        return INT_MAX;
    }

    // no clear win conditions met, we just return the score difference
    return score[me] - score[opponent];
}


void Checkers::Player::addMadeMove(Checkers::Move const& move) {
    this->moveList.push_back(move);
}


void Checkers::Player::addTotalMoveTime(Checkers::Duration const& moveTime) {
    this->totalMoveTime += moveTime;
}


void Checkers::Player::setPreviousMoveTime(Checkers::Duration const& moveTime) {
    this->previousMoveTime = moveTime;
}


Checkers::Duration Checkers::Player::getTotalMoveTime() {
    return this->totalMoveTime;
}


Checkers::Duration Checkers::Player::getPreviousMoveTime() {
    return this->previousMoveTime;
}


int Checkers::Player::getMaxDepthReached() {
    if(this->isComputer) {
        return this->maxDepthReached;
    } else {
        return -1;
    }
}
////////////////////////////////////
// END  Player method definitions //
////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// BEGIN  Game method definitions (in order of appearance in checkers.hpp) //
/////////////////////////////////////////////////////////////////////////////
Checkers::Game::Game() {
    this->inProgress = false;
    this->players[0] = Checkers::Player(nullptr, false, 0);
    this->players[1] = Checkers::Player(nullptr, false, 0);
    this->playerTurn = 0;
    this->previousMoveTime = Duration::zero();
    this->totalMoveTime = Duration::zero();
    this->moveCount = 0;
    this->numMovesSinceCapture = 0;
    this->reset();
}


void Checkers::Game::start(  bool playerOneComputer
                           , bool playerTwoComputer
                           , unsigned short playerFirstMove
                           , double computerTimeLimit) {
    Checkers::Board board;
    Checkers::Move move;
    Checkers::Time ti, tf;
    Checkers::Duration moveDuration;

    // initialize the players using given parameters
    this->players[0] = Checkers::Player(this, playerOneComputer, computerTimeLimit);
    this->players[1] = Checkers::Player(this, playerTwoComputer, computerTimeLimit);

    // set up game state variables
    this->inProgress = true;
    this->playerTurn = playerFirstMove;

    while(this->inProgress) {
        // print the board
        this->printCurrentBoard();
        std::cout << "Player " << (this->playerTurn + 1) << " to move ..." << std::endl;

        // get the current board
        board = this->getCurrentBoard();

        // give the board to the player to make their move
        ti = this->moveStartTime = Checkers::Clock::now();
        move = this->players[this->playerTurn].makeMove(board);
        tf = Checkers::Clock::now();

        // determine the time it took to make the move
        moveDuration = tf - ti;

        // record move time for game
        this->previousMoveTime = moveDuration;
        this->totalMoveTime += moveDuration;

        // record move time for player
        this->players[this->playerTurn].addTotalMoveTime(moveDuration);
        this->players[this->playerTurn].setPreviousMoveTime(moveDuration);

        // add move to the game's move list
        this->moveList.push_back(move);

        // add move to the player's move list
        this->players[this->playerTurn].addMadeMove(move);

        // increment the total move count and number of moves since capture
        this->moveCount++;
        this->numMovesSinceCapture++;

        // update the board
        board = Checkers::Game::getNextBoardFromMove(move);
        this->setCurrentBoard(board);
        
        // make the other player's turn
        this->playerTurn = (~this->playerTurn) & 1;

        // TODO: clean this up... it's kind of ugly
        if(this->numMovesSinceCapture > Checkers::moveLimit) {
            this->stop();
        }
    }

    // if no move was made, there were no legal moves to make
    if((move.xPath[0] > 7 || move.yPath[0] > 7) && this->numMovesSinceCapture <= Checkers::moveLimit) {
        std::cout << "Game over! Player " << (this->playerTurn + 1) << " wins!" << std::endl;
    }

    // the move limit was exceeded;  TODO: implement other official rules (a map for board states could prove useful)
    else {
        std::cout << "Game over! Player 1 and Player 2 draw!" << std::endl;
    }

}


void Checkers::Game::stop() {
    this->inProgress = false;
}


// TODO: cover all bases w.r.t. initialization
void Checkers::Game::reset() {
    Checkers::Board board;
    uint8_t i, j, m, n;
    
    // set rng seed
    std::srand(std::time(0));

    // initialize relevant game state variables
    this->moveCount = 0;
    this->numMovesSinceCapture = 0;
    this->previousMoveTime = Duration::zero();
    this->totalMoveTime = Duration::zero();

    // initializing the board squares to beginning state
    m = n = 0;
    for(i = 0; i < 8; i++) {
        
        for(j = 0; j < 8; j++) {
            if(!((i + j) & 1) && i < 3) {
                board.squares[i][j] = 0; // player one reg.

                board.pieces[0][m].isKing = false;
                board.pieces[0][m].xPos = j;
                board.pieces[0][m].yPos = i;

                m++;
            } else if(!((i + j) & 1) && i > 4) {
                board.squares[i][j] = 1; // player two reg.

                board.pieces[1][n].isKing = false;
                board.pieces[1][n].xPos = j;
                board.pieces[1][n].yPos = i;

                n++;
            } else {
                board.squares[i][j] = 4; // empty
            }
        }
    }

    this->setCurrentBoard(board);
}


void Checkers::Game::load(std::string const& filePath) {
    std::ifstream inputFile(filePath.c_str());
    int i, j;
    int k[2] = {0};
    int square;
    uint8_t player;

    if(!inputFile.is_open()) {
        std::cout << "Warning: Game could not be loaded!" << std::endl;
        std::cout << "         Failed to open '" << filePath << "' for reading." << std::endl;
    } else {
        Checkers::Board board;

        // initialize the board to "zero'd out" state
        for(i = 0; i < 8; i++) {
            for(j = 0; j < 8; j++) {
                board.squares[i][j] = 4;
            }
        }
        for(i = 0; i < 2; i++) {
            for(j = 0; j < 12; j++) {
                board.pieces[i][j].isKing = false;
                board.pieces[i][j].xPos = 0xFFU;
                board.pieces[i][j].yPos = 0xFFU;
            }
        }

        // read in board and pieces
        for(i = 7; i >= 0; i--) {
            for(j = 0; j < 8; j++) {
                inputFile >> square;
                board.squares[i][j] = square;

                if(square < 4) {
                    player = square & 1;
                    board.pieces[player][k[player]].isKing = square & 2;
                    board.pieces[player][k[player]].xPos = j;
                    board.pieces[player][k[player]].yPos = i;
                    k[player]++;
                }
            }
        }

        inputFile.close();
        std::cout << "Game successfully loaded from '" << filePath << "'!" << std::endl;
        this->setCurrentBoard(board);
    }
}


void Checkers::Game::save(std::string const& filePath) {
    int i, j;
    
    std::ofstream outputFile(filePath.c_str());
    Checkers::Board board = this->getCurrentBoard();
    
    if(!outputFile.is_open()) {
        std::cout << "Warning: Game could not be saved!" << std::endl;
        std::cout << "         Failed to open '" << filePath << "' for writing." << std::endl;
    } else {
        // write out board
        for(i = 7; i >= 0; i--) {
            for(j = 0; j < 8; j++) {
                outputFile << int(board.squares[i][j]) << (j < 7 ? " " : "");
            }
            outputFile << std::endl;
        }

        outputFile.close();
        std::cout << "Game successfully saved to '" << filePath << "'!" << std::endl;
    }
}


Checkers::Board Checkers::Game::getCurrentBoard() {
    return this->masterBoard;
}


void Checkers::Game::setCurrentBoard(Checkers::Board const& board) {
    this->masterBoard = board;
}


Checkers::Board Checkers::Game::getNextBoardFromMove(Move const& move) {
    Checkers::Board board = this->getCurrentBoard();

    return Checkers::Game::getNextBoardFromMove_andBoard(move, board);
}


Checkers::Board Checkers::Game::getNextBoardFromMove_andBoard(Move const& move, Board const& board) {
    int i;
    Checkers::Board tempBoard;
    Checkers::Move tempMove;
    signed short srcPiece;
    signed short jmpPiece;
    int xDiff, yDiff;
    uint8_t opponent;
    uint8_t xJmp, yJmp;
    

    // check for invalid move paths; return the given board if invalid
    if(move.xPath[0] > 7 || move.yPath[0] > 7 || move.xPath[1] > 7 || move.yPath[1] > 7   // src/dest out of range
        || ((move.xPath[0] + move.yPath[0]) & 1) || ((move.xPath[1] + move.yPath[1]) & 1) // src/dest is white square
        || board.squares[move.yPath[1]][move.xPath[1]] != 4) {                            // dest square occupied
        return board;
    }


    // single move (no jump) => return after move
    else if(abs(move.xPath[1] - move.xPath[0]) == 1 && abs(move.yPath[1] - move.yPath[0]) == 1) {
        srcPiece = -1;
        
        // find the src piece
        for(i = 0; i < 12; i++) {
            if(    board.pieces[move.player][i].xPos == move.xPath[0]
                && board.pieces[move.player][i].yPos == move.yPath[0]) {
                srcPiece = i;
            }
        }

        // inconsistent board check
        if(srcPiece < 0) {
            return board;
        }

        tempBoard = board;

        // handle promotion
        if(!board.pieces[move.player][srcPiece].isKing && move.yPath[1] == 7 * ((~move.player) & 1)) {
            tempBoard.pieces[move.player][srcPiece].isKing = true;
        }

        // update the board
        tempBoard.squares[move.yPath[0]][move.xPath[0]] = 4;
        tempBoard.squares[move.yPath[1]][move.xPath[1]] = (int(tempBoard.pieces[move.player][srcPiece].isKing) << 1) | move.player;

        // update the piece location
        tempBoard.pieces[move.player][srcPiece].xPos = move.xPath[1];
        tempBoard.pieces[move.player][srcPiece].yPos = move.yPath[1];

        return tempBoard;
    }


    // jump move => perform jump; shift move path if no promotion
    else if(abs(xDiff = (move.xPath[1] - move.xPath[0])) == 2 && abs(yDiff = (move.yPath[1] - move.yPath[0])) == 2) {
        opponent = (~move.player) & 1;
        srcPiece = -1;

        // find the src and jmp pieces
        for(i = 0; i < 12; i++) {
            if(    board.pieces[move.player][i].xPos == move.xPath[0]
                && board.pieces[move.player][i].yPos == move.yPath[0]) {
                srcPiece = i;
            }
        }

        // inconsistent board check
        if(srcPiece < 0) {
            return board;
        }

        tempBoard = board;
        tempMove = move;

        do {
            jmpPiece = -1;

            xJmp = tempMove.xPath[0] + (0 < xDiff) - (xDiff < 0);
            yJmp = tempMove.yPath[0] + (0 < yDiff) - (yDiff < 0);

            for(i = 0; i < 12; i++) {
                if(    tempBoard.pieces[opponent][i].xPos == xJmp
                    && tempBoard.pieces[opponent][i].yPos == yJmp) {
                    jmpPiece = i;
                }
            }
            
            // inconsistent board check
            if(jmpPiece < 0) {
                return board;
            }
                
            // handle promotion
            if(!board.pieces[move.player][srcPiece].isKing && tempMove.yPath[1] == 7 * opponent) {
                tempBoard.pieces[move.player][srcPiece].isKing = true;
            }

            // update the board
            tempBoard.squares[tempMove.yPath[0]][tempMove.xPath[0]] = 4;
            tempBoard.squares[yJmp][xJmp] = 4;
            tempBoard.squares[tempMove.yPath[1]][tempMove.xPath[1]] = (int(tempBoard.pieces[move.player][srcPiece].isKing) << 1) | move.player;

            // update the piece location
            tempBoard.pieces[move.player][srcPiece].xPos = 0xFFU;
            tempBoard.pieces[move.player][srcPiece].yPos = 0xFFU;
            tempBoard.pieces[opponent][jmpPiece].xPos = 0xFFU;
            tempBoard.pieces[opponent][jmpPiece].yPos = 0xFFU;
            tempBoard.pieces[move.player][srcPiece].xPos = tempMove.xPath[1];
            tempBoard.pieces[move.player][srcPiece].yPos = tempMove.yPath[1];

            // return if promoted
            if(!board.pieces[move.player][srcPiece].isKing && tempBoard.pieces[move.player][srcPiece].isKing) {
                this->numMovesSinceCapture = 0;
                return tempBoard;
            }

            // shift move path to the left
            for(i = 0; tempMove.xPath[i] >= 0 && tempMove.xPath[i] <= 7 && tempMove.yPath[i] >= 0 && tempMove.yPath[i] <= 7; i++) {
                tempMove.xPath[i] = tempMove.xPath[i + 1];
                tempMove.yPath[i] = tempMove.yPath[i + 1];
            }
        } while(!(tempMove.xPath[0] > 7 || tempMove.yPath[0] > 7 || tempMove.xPath[1] > 7 || tempMove.yPath[1] > 7
                || ((tempMove.xPath[0] + tempMove.yPath[0]) & 1) || ((tempMove.xPath[1] + tempMove.yPath[1]) & 1)
                || tempBoard.squares[tempMove.yPath[1]][tempMove.xPath[1]] != 4)
                  && (abs(xDiff = (tempMove.xPath[1] - tempMove.xPath[0])) == 2 && abs(yDiff = (tempMove.yPath[1] - tempMove.yPath[0])) == 2));

        this->numMovesSinceCapture = 0;
        return tempBoard;
    }


    // catch-all for all other invalid moves
    else { 
        return board;
    }
}


// TODO: make the jumps section efficient (or at least clean up the code so it's more compact)
std::vector<Checkers::Move> Checkers::Game::getMovesFromBoard_andPlayer(Checkers::Board const& board, unsigned short player) {
    int i, j, k;
    Checkers::Move tempMove;
    Checkers::Piece tempPiece;    // we make a temp piece so we don't have to do pointer addition each time we get the piece
    std::vector<Checkers::Move> moveList;
    int xDest[2], yDest[2];
    struct {
        uint8_t x, y;             // x,y position of current square
        uint8_t checked;          // variable for checking jump directions
        bool descJump;            // one of the descendants made a jump (this way, we don't record invalid jumps)
    } boardStack[13], node;
    bool pieceJumped[8][8] = {0}; // map describing whether a piece was jumped
    uint8_t depth;                // current depth of the move stack
    uint8_t opponent = (~player) & 1;

    tempMove.player = player;


    // first, check for jumps
    for(i = 0; i < 12; i++) {
        tempPiece = board.pieces[player][i];
        tempMove.xPath[0] = tempPiece.xPos;
        tempMove.yPath[0] = tempPiece.yPos;

        boardStack[0].x = tempPiece.xPos;
        boardStack[0].y = tempPiece.yPos;
        boardStack[0].checked = 0;
        depth = 0;

        if(tempPiece.isKing) {

            while(boardStack[0].checked != 0xFU || depth != 0) {
                node = boardStack[depth];

                // check top right
                if(!(boardStack[depth].checked & 0x8U)) {
                    boardStack[depth].checked |= 0x8U;

                    if(    (node.x + 2) >= 0 && (node.x + 2) <= 7 && (node.y + 2) >= 0 && (node.y + 2) <= 7 // in range
                        && board.squares[node.y + 1][node.x + 1] != 4              // jump square not empty
                        && (board.squares[node.y + 1][node.x + 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y + 1][node.x + 1]                    // piece wasn't already jumped
                        && (board.squares[node.y + 2][node.x + 2] == 4             // dest square empty OR
                            || (node.x + 2 == boardStack[0].x && node.y + 2 == boardStack[0].y))) { // dest square same as start square


                        pieceJumped[node.y + 1][node.x + 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x + 2;
                        boardStack[depth].y = node.y + 2;
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }
                
                // check top left
                else if(!(boardStack[depth].checked & 0x4U)) {
                    boardStack[depth].checked |= 0x4U;

                    if(    (node.x - 2) >= 0 && (node.x - 2) <= 7 && (node.y + 2) >= 0 && (node.y + 2) <= 7 // in range
                        && board.squares[node.y + 1][node.x - 1] != 4              // jump square not empty
                        && (board.squares[node.y + 1][node.x - 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y + 1][node.x - 1]                    // piece wasn't already jumped
                        && (board.squares[node.y + 2][node.x - 2] == 4             // dest square empty OR
                            || (node.x - 2 == boardStack[0].x && node.y + 2 == boardStack[0].y))) { // dest square same as start square

                        pieceJumped[node.y + 1][node.x - 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x - 2;
                        boardStack[depth].y = node.y + 2;
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }
                
                // check bottom left
                else if(!(boardStack[depth].checked & 0x2U)) {
                    boardStack[depth].checked |= 0x2U;

                    if(    (node.x - 2) >= 0 && (node.x - 2) <= 7 && (node.y - 2) >= 0 && (node.y - 2) <= 7 // in range
                        && board.squares[node.y - 1][node.x - 1] != 4              // jump square not empty
                        && (board.squares[node.y - 1][node.x - 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y - 1][node.x - 1]                    // piece wasn't already jumped
                        && (board.squares[node.y - 2][node.x - 2] == 4             // dest square empty OR
                            || (node.x - 2 == boardStack[0].x && node.y - 2 == boardStack[0].y))) { // dest square same as start square

                        pieceJumped[node.y - 1][node.x - 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x - 2;
                        boardStack[depth].y = node.y - 2;
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }
                
                // check bottom right
                else if(!(boardStack[depth].checked & 0x1U)) {
                    boardStack[depth].checked |= 0x1U;

                    if(    (node.x + 2) >= 0 && (node.x + 2) <= 7 && (node.y - 2) >= 0 && (node.y - 2) <= 7 // in range
                        && board.squares[node.y - 1][node.x + 1] != 4              // jump square not empty
                        && (board.squares[node.y - 1][node.x + 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y - 1][node.x + 1]                    // piece wasn't already jumped
                        && (board.squares[node.y - 2][node.x + 2] == 4             // dest square empty OR
                            || (node.x + 2 == boardStack[0].x && node.y - 2 == boardStack[0].y))) { // dest square same as start square

                        pieceJumped[node.y - 1][node.x + 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x + 2;
                        boardStack[depth].y = node.y - 2;
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }
                
                // everything has been checked already
                // as long as our desc (if any) didn't make a jump, we should count the stack as a jump move
                else {
                    if(!boardStack[depth].descJump) {
                        for(j = 1; j <= depth; j++) {
                            tempMove.xPath[j] = boardStack[j].x;
                            tempMove.yPath[j] = boardStack[j].y;
                        }

                        // only add the "move-ender" if we know that the move doesn't jump 12 pieces
                        if(j < 13) {
                            tempMove.xPath[j] = tempMove.yPath[j] = 0xFFU;
                        }
                        moveList.push_back(tempMove);
                    }

                    // if we're in here, a desc of our parent definitely made a jump
                    boardStack[depth - 1].descJump = true;
                    
                    // remove jumped piece status
                    pieceJumped[(node.y + boardStack[depth - 1].y) / 2][(node.x + boardStack[depth - 1].x) / 2] = false;

                    depth--; // go up one
                }
            }
        }

        // not a king
        else {
            while(boardStack[0].checked != 0xCU || depth != 0) {
                node = boardStack[depth];

                // check (y-direction) right
                if(!(boardStack[depth].checked & 0x8U)) {
                    boardStack[depth].checked |= 0x8U;

                    if(    (node.x + 2) >= 0 && (node.x + 2) <= 7 && (node.y + (4 * opponent - 2)) >= 0 && (node.y + (4 * opponent - 2)) <= 7 // in range
                        && board.squares[node.y + (2 * opponent - 1)][node.x + 1] != 4              // jump square not empty
                        && (board.squares[node.y + (2 * opponent - 1)][node.x + 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y + (2 * opponent - 1)][node.x + 1]                    // piece wasn't already jumped
                        && board.squares[node.y + (4 * opponent - 2)][node.x + 2] == 4) {           // dest square empty OR


                        pieceJumped[node.y + (2 * opponent - 1)][node.x + 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x + 2;
                        boardStack[depth].y = node.y + (4 * opponent - 2);
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }

                // check (y-direction) left
                else if(!(boardStack[depth].checked & 0x4U)) {
                    boardStack[depth].checked |= 0x4U;

                    if(    (node.x - 2) >= 0 && (node.x - 2) <= 7 && (node.y + (4 * opponent - 2)) >= 0 && (node.y + (4 * opponent - 2)) <= 7 // in range
                        && board.squares[node.y + (2 * opponent - 1)][node.x - 1] != 4              // jump square not empty
                        && (board.squares[node.y + (2 * opponent - 1)][node.x - 1] & 1) == opponent // jump square holds enemy piece
                        && !pieceJumped[node.y + (2 * opponent - 1)][node.x - 1]                    // piece wasn't already jumped
                        && board.squares[node.y + (4 * opponent - 2)][node.x - 2] == 4) {           // dest square empty OR


                        pieceJumped[node.y + (2 * opponent - 1)][node.x - 1] = true; // we have jumped that piece
                        depth++;
                        boardStack[depth].x = node.x - 2;
                        boardStack[depth].y = node.y + (4 * opponent - 2);
                        boardStack[depth].checked = 0;
                        boardStack[depth].descJump = false;
                    }
                }
                
                // everything has been checked already
                // as long as our desc (if any) didn't make a jump, we should count the stack as a jump move
                else {
                    if(!boardStack[depth].descJump) {
                        for(j = 1; j <= depth; j++) {
                            tempMove.xPath[j] = boardStack[j].x;
                            tempMove.yPath[j] = boardStack[j].y;
                        }

                        // only add the "move-ender" if we know that the move doesn't jump 12 pieces
                        if(j < 13) {
                            tempMove.xPath[j] = tempMove.yPath[j] = 0xFFU;
                        }
                        moveList.push_back(tempMove);
                    }

                    // if we're in here, a desc of our parent definitely made a jump
                    boardStack[depth - 1].descJump = true;
                    
                    // remove jumped piece status
                    pieceJumped[(node.y + boardStack[depth - 1].y) / 2][(node.x + boardStack[depth - 1].x) / 2] = false;

                    depth--; // go up one
                }
            }
        }
    }

    // if no jumps were found, look for simple moves
    if(!moveList.size()) {
        for(i = 0; i < 12; i++) {
            tempPiece = board.pieces[player][i];
            tempMove.xPath[0] = tempPiece.xPos;
            tempMove.yPath[0] = tempPiece.yPos;
            tempMove.xPath[2] = tempMove.yPath[2] = 0xFFU;

            xDest[0] = tempPiece.xPos - 1;
            xDest[1] = tempPiece.xPos + 1;
            yDest[0] = tempPiece.yPos - 1;
            yDest[1] = tempPiece.yPos + 1;

            if(tempPiece.isKing) {
                for(j = 0; j < 2; j++) {
                    for(k = 0; k < 2; k++) {
                        if(    xDest[j] >= 0 && xDest[j] <= 7
                            && yDest[k] >= 0 && yDest[k] <= 7
                            && board.squares[yDest[k]][xDest[j]] & 4) {
                            tempMove.xPath[1] = xDest[j];
                            tempMove.yPath[1] = yDest[k];
                            moveList.push_back(tempMove);
                        }
                    }
                }
            } else {
                for(j = 0; j < 2; j++) {
                    if(    xDest[j] >= 0 && xDest[j] <= 7
                        && yDest[opponent] >= 0 && yDest[opponent] <= 7
                        && board.squares[yDest[opponent]][xDest[j]] & 4) {
                            tempMove.xPath[1] = xDest[j];
                            tempMove.yPath[1] = yDest[opponent];
                            moveList.push_back(tempMove);
                    }
                }
            }
        }
    }
    
    return moveList;
}


std::string Checkers::Game::parseMove(Move const& move) {
    std::stringstream movePath;
    int i;

    for(i = 0; i < 13; i++) {
        // valid path check (out of range and white square)
        if(move.xPath[i] > 7 || move.yPath[i] > 7 || 1 & (move.xPath[0] + move.yPath[0])) {
            return movePath.str();
        }

        movePath << (i ? " -> " : "");
        movePath << Checkers::columnMap[move.xPath[i]] << move.yPath[i] + 1;
    }
    
    return movePath.str();
}


uint8_t Checkers::Game::getPlayerTurn() {
    return this->playerTurn;
}


Checkers::Time Checkers::Game::getMoveStartTime() {
    return this->moveStartTime;
}


// TODO (optional): Add option to reverse the printed board
void Checkers::Game::printCurrentBoard() {
    Checkers::Board board = this->getCurrentBoard();
    int i, j;
    int numReg[2] = {0};
    int numKing[2] = {0};

    for(i = 0; i < 2; i++) {
        for(j = 0; j < 12; j++) {
            if(board.pieces[i][j].xPos <= 7 && board.pieces[i][j].yPos <= 7) {
                if(board.pieces[i][j].isKing) {
                    numKing[i]++;
                } else {
                    numReg[i]++;
                }
            }
        }
    }

    std::cout << std::endl;

    for(i = 7; i >= 0; i--) {
        std::cout << "     ";

        for(j = 0; j < 8; j++) {
            if((j + i) & 1) {
                std::cout << termcolor::on_white << "       ";
            } else {
                std::cout << termcolor::on_grey << "       ";
            }
        }

        std::cout << termcolor::reset;
        if(i == 7) {
            std::cout << " Player 1 Info";
        } else if(i == 6) {
            std::cout << "  > total move time      : " << (((double) this->players[0].getTotalMoveTime().count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        } else if(i == 4) {
            std::cout << " Player 2 Info";
        } else if(i == 3) {
            std::cout << "  > total move time      : " << (((double) this->players[1].getTotalMoveTime().count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        } else if(i == 1) {
            std::cout << " Game Info";
        } else if(i == 0) {
            std::cout << "  > current move count   : " << this->moveCount;
        }
        std::cout << std::endl;
        std::cout << "  " << (i + 1) << "  ";

        // print square
        for(j = 0; j < 8; j++) {
            if((j + i) & 1) {
                std::cout << termcolor::on_white << "   ";
            } else {
                std::cout << termcolor::on_grey << "   ";
            }

            if(board.squares[i][j] & 1) {
                std::cout << termcolor::yellow << Checkers::squareMap[board.squares[i][j]];
            } else {
                std::cout << termcolor::cyan << Checkers::squareMap[board.squares[i][j]];
            }
            
            std::cout << "   ";
        }

        std::cout << termcolor::reset;
        if(i == 7) {
            std::cout << "  > reg. piece (" << termcolor::cyan << Checkers::squareMap[0] << termcolor::reset << ")       : " << numReg[0];
        } else if(i == 6) {
            std::cout << "  > previous move time   : " << (((double) this->players[0].getPreviousMoveTime().count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        } else if(i == 4) {
            std::cout << "  > reg. piece (" << termcolor::yellow << Checkers::squareMap[1] << termcolor::reset << ")       : " << numReg[1];
        } else if(i == 3) {
            std::cout << "  > previous move time   : " << (((double) this->players[1].getPreviousMoveTime().count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        } else if(i == 1) {
            std::cout << "  > total move time      : " << (((double) this->totalMoveTime.count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        } else if(i == 0) {
            //std::cout << "  > moves since capture  : " << this->numMovesSinceCapture;
        }
        std::cout << std::endl;
        std::cout << "     ";

        for(j = 0; j < 8; ++j) {
            if((j + i) & 1) {
                std::cout << termcolor::on_white << "       ";
            } else {
                std::cout << termcolor::on_grey << "       ";
            }
        }

        std::cout << termcolor::reset;
        if (i == 7) {
            std::cout << "  > king piece (" << termcolor::cyan << Checkers::squareMap[2] << termcolor::reset << ")       : " << numKing[0];
        } else if(i == 6 && this->players[0].getMaxDepthReached() >= 0) {
            std::cout << "  > alpha-beta max depth : " << this->players[0].getMaxDepthReached();
        } else if(i == 4) {
            std::cout << "  > king piece (" << termcolor::yellow << Checkers::squareMap[3] << termcolor::reset << ")       : " << numKing[1];
        } else if(i == 3 && this->players[1].getMaxDepthReached() >= 0) {
            std::cout << "  > alpha-beta max depth : " << this->players[1].getMaxDepthReached();
        } else if(i == 1) {
            std::cout << "  > previous move time   : " << (((double) this->previousMoveTime.count()) * Checkers::Clock::period::num / Checkers::Clock::period::den) << "s";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "     ";
    for(i = 0; i < 8; i++) {
        std::cout << "   " << Checkers::columnMap[i] << "   ";
    }
    std::cout << std::endl << std::endl;
}
//////////////////////////////////
// END  Game method definitions //
//////////////////////////////////
