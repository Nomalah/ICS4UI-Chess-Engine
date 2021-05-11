#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "include/chess.h"

struct perftResult {
	size_t total;
	std::vector<std::pair<chess::move, size_t>> moves;
	size_t captures;
	size_t enPassant;
	size_t castles;
	size_t promotions;
};

perftResult perft(size_t testDepth, const chess::position& startingPosition) {
	chess::game gameToTest = {{startingPosition}};
	perftResult result = {0, {}, 0};
	auto perftTest = [&gameToTest, &result](const auto perftTest, const std::size_t depth) -> std::size_t {
		if (depth == 0){
			return 1;
        }
		if (depth == 1) {
			std::vector<chess::move> legalMoves = gameToTest.moves();
			for (chess::move legalMove : legalMoves) {
				switch(legalMove.flags & 0xFF00){
					case 0x6000:
						[[fallthrough]];
					case 0x7000:
						result.enPassant++;
						[[fallthrough]];
					case 0x1000:
						result.captures++;
						break;
					case 0x2000 ... 0x5000:
						result.castles++;
						break;
					case 0x1100 ... 0x1F00:
						result.captures++;
						[[fallthrough]];
					case 0x0100 ... 0x0F00:
						result.promotions++;
						break;
				}
			}
			return legalMoves.size();
		}

		std::size_t nodes = 0;
		std::vector<chess::move> validMoves = gameToTest.moves();
		for (chess::move validMove : validMoves) {
			gameToTest.move(validMove);
			nodes += perftTest(perftTest, depth - 1);
			gameToTest.undo();
		}
		return nodes;
	};

	for (auto& validMove : gameToTest.moves()) {
		gameToTest.move(validMove);
		result.moves.push_back({validMove, perftTest(perftTest, testDepth - 1)});
		gameToTest.undo();
		result.total += result.moves.back().second;
	}
	return result;
}

struct perftTest {
	std::string testFen;
	std::vector<std::size_t> perftKnownResults;
};

int main(int argc, const char* argv[]) {
	// Tests come from https://www.chessprogramming.org/Perft_Results
	std::vector<perftTest> tests = {
		{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", {1, 20, 400, 8902, 197281, 4865609}},
		{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", {1, 48, 2039, 97862, 4085603, 193690690}},
		{"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", {1, 14, 191, 2812, 43238, 674624}},
		{"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", {1, 6, 264, 9467, 422333, 15833292}},
		{"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", {1, 44, 1486, 62379, 2103487, 89941194}},
		{"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", {1, 46, 2079, 89890, 3894594, 164075551}}};

	if (argc == 2) {
		do {
			std::string fen = argv[1];
			;
			if (fen.length() <= 2) {
				std::cout << "\u001b[31mImproper fen string was given - running default tests\n";
				break;
			}
			chess::position testPosition = chess::position::fromFen(fen);
			std::cout << "\u001b[34m[Test]@Position=" << fen << std::endl;
			auto moves = testPosition.moves();
			for (int moveNum = 0; moveNum < moves.size(); moveNum++) {
				std::cout << "\t\u001b[0m[" << moveNum << "]:[" << moves[moveNum].toString() << "]" << std::endl;
			}
			return 0;
		} while (false);
	} else {
		std::cout << "\u001b[31mImproper or no arguments were given - running default tests" << std::endl;
	}

	size_t totalTests = 0;
	size_t passedTests = 0;

	for (const perftTest& test : tests) {
		chess::position testPosition = chess::position::fromFen(test.testFen);
		std::cout << "\u001b[34m[Test]@Position=" << test.testFen << std::endl;
		for (std::size_t testDepth = 1; testDepth < test.perftKnownResults.size(); testDepth++) {
			auto startTime = std::chrono::high_resolution_clock::now();
			perftResult testResult = perft(testDepth, testPosition);
			auto endTime = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			if (testResult.total != test.perftKnownResults[testDepth]) {
				std::cout << "\t\u001b[31m[Failed Test]@Depth=" << testDepth << "\u001b[0m -> [Perft Result]:[" << testResult.total << "] - [Known Result]:[" << test.perftKnownResults[testDepth] << "] - \u001b[34m[kn/s]:[" << testResult.total * 1000 / duration << "]" << std::endl;
				std::cout << "\t\t\u001b[33m[Captures]:[" << testResult.captures << "]\n";
				std::cout << "\t\t[En Passant]:[" << testResult.enPassant << "]\n";
				std::cout << "\t\t[Castles]:[" << testResult.castles << "]\n";
				std::cout << "\t\t[Promotions]:[" << testResult.promotions << "]\n\u001b[34m";
				for (auto& [move, total] : testResult.moves) {
					std::cout << "\t\t[" << move.toString() << "]:[" << total << "]\n";
				}
			} else {
				std::cout << "\t\u001b[32m[Passed Test]@Depth=" << testDepth << "\u001b[0m -> [Perft Result]:[" << testResult.total << "] - [Known Result]:[" << test.perftKnownResults[testDepth] << "] - \u001b[34m[kn/s]:[" << testResult.total * 1000 / duration << "]" << std::endl;
				passedTests++;
			}
			totalTests++;
		}
	}
	std::cout << "\u001b[34m[Passed/Total]:[" << passedTests << "/" << totalTests << "]\u001b[0m\n";
}