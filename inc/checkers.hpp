#ifndef __CHECKERS_HPP__
#define __CHECKERS_HPP__

#include <chrono>
#include <string>
#include <vector>

namespace Checkers {

    // checkers parameters
    extern char squareMap[5];
    extern char columnMap[8];
    extern unsigned int timeLimitLower;
    extern unsigned int timeLimitUpper;
    extern unsigned int moveLimit;
    extern double timeRemainingThreshold;


    // timing type definitions
    typedef std::chrono::steady_clock Clock;
    typedef std::chrono::steady_clock::duration Duration;
    typedef std::chrono::steady_clock::time_point Time;


    // Piece type definition
    typedef struct {
        bool isKing;
        uint8_t xPos;
        uint8_t yPos;
    } Piece;


    // Board type definition
    typedef struct {
        uint8_t squares[8][8];
        Piece pieces[2][12];
    } Board;


    // Move type definition
    typedef struct {
        uint8_t player;
        uint8_t xPath[13];
        uint8_t yPath[13];
    } Move;


    // Game forward declaration required by Player
    class Game;


    // Player class definition
	class Player {
		public:
            Player();
			Player(Game *, bool, double);

            // determine a move for a given board
			Move makeMove(Board const&);
            
            // computer board operations
            Move pickMoveFromBoard(Board const&);
            int evaluateBoard(Board const&);

            // add made moves to a move list
            void addMadeMove(Move const&);

            // time functions
            void addTotalMoveTime(Duration const&);
            void setPreviousMoveTime(Duration const&);
            Duration getTotalMoveTime();
            Duration getPreviousMoveTime();

            // alpha-beta functions
            int getMaxDepthReached();
		private:
            // for access to the game instance
            Game * game;

			std::vector<Move> moveList;

            double timeLimit;
			Duration totalMoveTime;
            Duration previousMoveTime;

			bool isComputer;
            int maxDepthReached;
	};


    class Game {
        public:
            Game();

            // general game control operations
            void start(bool, bool, unsigned short, double);
            void stop();
            void reset();

            // game state load/save functionality
            void load(std::string const&);
            void save(std::string const&);

            // board state operations for game instance
            Board getCurrentBoard();
            void setCurrentBoard(Board const&);

            // functional board and move utility functions
            Board getNextBoardFromMove(Move const&); // uses the current board
            Board getNextBoardFromMove_andBoard(Move const&, Board const&);
            static std::vector<Move> getMovesFromBoard_andPlayer(Board const&, unsigned short);
            static std::string parseMove(Move const&);

            // misc. game state getters
            uint8_t getPlayerTurn();
            Time getMoveStartTime();

            // printing functions
            void printCurrentBoard();
        private:
            // game state variables
            bool inProgress;
            Board masterBoard;

            Player players[2];
            uint8_t playerTurn;

            unsigned int moveCount;
            unsigned int numMovesSinceCapture;
            std::vector<Move> moveList;

            Time moveStartTime;
            Duration previousMoveTime;
            Duration totalMoveTime;
    };

}

#endif
