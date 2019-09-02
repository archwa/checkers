#include <checkers.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <termcolor.hpp>


int main(int argc, char ** argv) {

    using namespace Checkers;

    Game checkers = Game();
    std::string loadGame;          // will a game be loaded?
    std::string savedGameFilePath; // the path of the saved game file

    std::string playerOneComputer; // is Player 1 a computer?
    std::string playerTwoComputer; // is Player 2 a computer?
    std::string computerTimeLimit; // what is the computer time limit?
    std::string playerFirstMove;   // which player will move first?
    std::string confirmParams;     // confirm the parameters with the user


    while(confirmParams != "y") {

        while((std::cout << "Would you like to load a saved game? (y / n): ")
                && (!(std::cin >> loadGame) || (loadGame != "y" && loadGame != "n"))) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        if(loadGame == "y") {
            std::cout << " > Please enter a saved game file path: ";
            std::cin >> savedGameFilePath;
        }
            

        while((std::cout << "Will Player 1 be a computer? (y / n): ")
                && (!(std::cin >> playerOneComputer) || (playerOneComputer != "y" && playerOneComputer != "n"))) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        while((std::cout << "Will Player 2 be a computer? (y / n): ")
                && (!(std::cin >> playerTwoComputer) || (playerTwoComputer != "y" && playerTwoComputer != "n"))) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        if(playerOneComputer == "y" || playerTwoComputer == "y") {
            while((std::cout << " > Please enter a time limit for computer movement (in seconds [" << timeLimitLower << ", " << timeLimitUpper << "]): ")
                    && (!(std::cin >> computerTimeLimit) || std::strtoll(computerTimeLimit.c_str(), nullptr, 10) < timeLimitLower || std::strtoll(computerTimeLimit.c_str(), nullptr, 0) > timeLimitUpper)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }


        while((std::cout << "Which Player will make the first move? (1 / 2): ")
                && (!(std::cin >> playerFirstMove) || (playerFirstMove != "1" && playerFirstMove != "2"))) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }


        // confirm the specified parameters
        std::cout << std::endl << "The following parameters have been specified:" << std::endl;

        if(loadGame == "y") {
            std::cout << " > Load game from '" << savedGameFilePath << "'" << std::endl;
        }
        std::cout << " > Player 1 " << (playerOneComputer == "y" ? "will" : "will not") << " be a computer" << std::endl;
        std::cout << " > Player 2 " << (playerTwoComputer == "y" ? "will" : "will not") << " be a computer" << std::endl;
        if(computerTimeLimit != "") {
            std::cout << " > Time limit for computer movement will be " << std::strtoll(computerTimeLimit.c_str(), nullptr, 10) << " seconds" << std::endl;
        }
        std::cout << " > Player " << playerFirstMove << " will move first" << std::endl;

        while((std::cout << "Is this correct? (y / n): ")
                && (!(std::cin >> confirmParams) || (confirmParams != "y" && confirmParams != "n"))) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        std::cout << std::endl;
    }


    if(loadGame == "y") {
        checkers.load(savedGameFilePath);
    }

    checkers.start(  playerOneComputer == "y" ? true : false
                   , playerTwoComputer == "y" ? true : false
                   , playerFirstMove == "1" ? 0 : 1
                   , static_cast<double>(std::strtoll(computerTimeLimit.c_str(), nullptr, 10)));

    return 0;
}
