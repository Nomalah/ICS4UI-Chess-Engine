#include <iostream>
#include <vector>
#include <chrono>

#include "include/chess.h"

size_t perft(size_t testDepth, const chess::position& startingPosition){
	chess::game gameToTest = {{startingPosition}};
	auto perftTest = [&gameToTest](const auto perftTest, const std::size_t depth) -> std::size_t {
        if(depth == 1)
			return gameToTest.moves().size();

		std::size_t nodes = 0;
		std::vector<chess::move> validMoves = gameToTest.moves();
		for(chess::move validMove : validMoves){
            gameToTest.move(validMove);
			nodes += perftTest(perftTest, depth - 1);
			gameToTest.undo();
		}
		return nodes;
	};
	return perftTest(perftTest, testDepth);
}

struct perftTest{
	std::string testFen;
	std::vector<std::size_t> perftKnownResults;
};

int main(){
	// Tests come from https://www.chessprogramming.org/Perft_Results
	perftTest tests[] = {
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {1, 20, 400, 8902, 197281, 4865609}},
		{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", {1, 48, 2039, 97862, 4085603, 193690690}},
		{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1, 14, 191, 2812, 43238, 674624}},
		{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1, 6, 264, 9467, 422333, 15833292}},
		{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1, 44, 1486, 62379, 2103487, 89941194}},
		{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", {1, 46, 2079, 89890, 3894594, 164075551}}
    };

	size_t totalTests = 0;
	size_t passedTests = 0;

	for(const perftTest& test : tests){
		chess::position testPosition = chess::position::fromFen(test.testFen);
		std::cout << "\u001b[34m[Test]@Position=" << test.testFen << std::endl;
		for (std::size_t testDepth = 1; testDepth < test.perftKnownResults.size(); testDepth++) {
			auto startTime = std::chrono::high_resolution_clock::now();
			std::size_t perftResult = perft(testDepth, testPosition);
			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
			if (perftResult != test.perftKnownResults[testDepth]) {
				std::cout << "\t\u001b[31m[Failed Test]@Depth=" << testDepth << "\u001b[0m -> [Perft Result]:[" << perftResult << "] - [Known Result]:[" << test.perftKnownResults[testDepth] << "] - \u001b[34m[kn/s]:[" << perftResult / (duration ? duration : 1) << "]" << std::endl;
			} else {
				std::cout << "\t\u001b[32m[Passed Test]@Depth=" << testDepth << "\u001b[0m -> [Perft Result]:[" << perftResult << "] - [Known Result]:[" << test.perftKnownResults[testDepth] << "] - \u001b[34m[kn/s]:[" << perftResult / (duration ? duration : 1) << "]" << std::endl;
				passedTests++;
			}
			totalTests++;
		}
	}
	std::cout << "\u001b[34m[Passed/Total]:[" << passedTests << "/" << totalTests << "]\u001b[0m\n";
}