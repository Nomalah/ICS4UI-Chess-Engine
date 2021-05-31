#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "include/chess.h"

struct timing {
	size_t totalRuns;
	std::time_t totalTime;
};
timing movesTime = { 0, 0 };
timing moveTime  = { 0, 0 };
timing undoTime  = { 0, 0 };

#define TIME(line, time)                                                                                                                             \
	auto time##__startTime = std::chrono::high_resolution_clock::now();                                                                              \
	line;                                                                                                                                            \
	time.totalTime += (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - time##__startTime).count()); \
	time.totalRuns++;

struct perftResult {
	size_t total;
	std::vector<std::pair<chess::moveData, size_t>> moves;
	size_t captures;
	size_t enPassant;
	size_t castles;
	size_t promotions;
};

perftResult perft(size_t testDepth, const std::string& fen) {
	chess::game gameToTest(fen);
	perftResult result = { 0, {}, 0 };
	auto perftTest     = [&gameToTest, &result](const auto perftTest, const std::size_t depth) -> std::size_t {
        if (depth == 0) {
            return 1;
        }
        if (depth == 1) {
            TIME(auto legalMoves = gameToTest.moves(), movesTime);
            for (chess::moveData legalMove : legalMoves) {
                switch (legalMove.flags & 0xFF00) {
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
        TIME(chess::staticVector validMoves(gameToTest.moves()), movesTime);
        for (chess::moveData validMove : validMoves) {
            TIME(gameToTest.move(validMove), moveTime);
            nodes += perftTest(perftTest, depth - 1);
            TIME(gameToTest.undo(), undoTime);
        }
        return nodes;
	};

	for (auto& validMove : gameToTest.moves()) {
		gameToTest.move(validMove);
		result.moves.push_back({ validMove, perftTest(perftTest, testDepth - 1) });
		gameToTest.undo();
		result.total += result.moves.back().second;
	}
	return result;
}

struct perftTestResult {
	std::size_t depth;
	std::size_t nodes;
};

struct perftTest {
	std::string testFen;
	std::vector<perftTestResult> testList;
};

int main(int argc, const char* argv[]) {
	// Tests come from https://www.chessprogramming.org/Perft_Results
	std::vector<perftTest> tests { { { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", { { 1, 20 }, { 2, 400 }, { 3, 8902 }, { 4, 197281 }, { 5, 4865609 } } },
		                             { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", { { 1, 48 }, { 2, 2039 }, { 3, 97862 }, { 4, 4085603 }, { 5, 193690690 } } },
		                             { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", { { 1, 14 }, { 2, 191 }, { 3, 2812 }, { 4, 43238 }, { 5, 674624 } } },
		                             { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", { { 1, 6 }, { 2, 264 }, { 3, 9467 }, { 4, 422333 }, { 5, 15833292 } } },
		                             { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", { { 1, 44 }, { 2, 1486 }, { 3, 62379 }, { 4, 2103487 }, { 5, 89941194 } } },
		                             { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", { { 1, 46 }, { 2, 2079 }, { 3, 89890 }, { 4, 3894594 }, { 5, 164075551 } } },
		                             { "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2", { { 1, 8 } } },
		                             { "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 5 3", { { 1, 8 } } },
		                             { "r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2", { { 1, 19 } } },
		                             { "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2", { { 1, 5 } } },
		                             { "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2", { { 1, 44 } } },
		                             { "rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9", { { 1, 39 } } },
		                             { "2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4", { { 1, 9 } } },
		                             { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", { { 3, 62379 } } },
		                             { "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", { { 6, 1134888 } } },
		                             { "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", { { 6, 1015133 } } },
		                             { "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", { { 6, 1440467 } } },
		                             { "5k2/8/8/8/8/8/8/4K2R w K - 0 1", { { 6, 661072 } } },
		                             { "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", { { 6, 803711 } } },
		                             { "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", { { 4, 1274206 } } },
		                             { "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", { { 4, 1720476 } } },
		                             { "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", { { 6, 3821001 } } },
		                             { "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", { { 5, 1004658 } } },
		                             { "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", { { 6, 217342 } } },
		                             { "8/P1k5/K7/8/8/8/8/8 w - - 0 1", { { 6, 92683 } } },
		                             { "K1k5/8/P7/8/8/8/8/8 w - - 0 1", { { 6, 2217 } } },
		                             { "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", { { 7, 567584 } } },
		                             { "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", { { 4, 23527 } } } } };

	if (argc == 3) {
		do {
			std::string fen = argv[1];
			if (fen.length() <= 2) {
				std::cout << "\u001b[31mImproper fen string was given - running default tests\n";
				break;
			}
			chess::position testPosition = chess::position::fromFen(fen);
			std::cout << "\u001b[34m[Test]@Position=" << fen << std::endl;
			std::cout << "\u001b[33m" << testPosition.ascii() << "\u001b[34m" << std::endl;
			perftResult testResult = perft(std::stoull(argv[2]), fen);
			for (auto& [move, total] : testResult.moves) {
				std::cout << "\t[" << move.toString() << "]:[" << total << "]\n";
			}
			std::cout << "[Total Nodes Visited]:[" << testResult.total << "]\u001b[0m\n";
			return 0;
		} while (false);
	} else {
		std::cout << "\u001b[31mImproper or no arguments were given - running default tests" << std::endl;
	}

	size_t totalTests  = 0;
	size_t passedTests = 0;

	for (const perftTest& test : tests) {
		chess::position testPosition = chess::position::fromFen(test.testFen);
		std::cout << "\u001b[34m[Test]@Position=" << test.testFen << std::endl;
		for (const perftTestResult& knownTestResult : test.testList) {
			auto startTime         = std::chrono::high_resolution_clock::now();
			perftResult testResult = perft(knownTestResult.depth, test.testFen);
			auto endTime           = std::chrono::high_resolution_clock::now();
			auto duration          = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			if (testResult.total != knownTestResult.nodes) {
				std::cout << "\t\u001b[31m[Failed Test]@Depth=" << knownTestResult.depth << "\u001b[0m -> [Perft Result]:[" << testResult.total << "] - [Known Result]:[" << knownTestResult.nodes << "] - \u001b[34m[kn/s]:[" << testResult.total * 1000 / duration << "]" << std::endl;
				std::cout << "\t\t\u001b[33m[Captures]:[" << testResult.captures << "]\n";
				std::cout << "\t\t[En Passant]:[" << testResult.enPassant << "]\n";
				std::cout << "\t\t[Castles]:[" << testResult.castles << "]\n";
				std::cout << "\t\t[Promotions]:[" << testResult.promotions << "]\n\u001b[34m";
				for (auto& [move, total] : testResult.moves) {
					std::cout << "\t\t[" << move.toString() << "]:[" << total << "]\n";
				}
			} else {
				std::cout << "\t\u001b[32m[Passed Test]@Depth=" << knownTestResult.depth << "\u001b[0m -> [Perft Result]:[" << testResult.total << "] - [Known Result]:[" << knownTestResult.nodes << "] - \u001b[34m[kn/s]:[" << testResult.total * 1000 / duration << "]" << std::endl;
				passedTests++;
			}
			totalTests++;
		}
	}
	std::cout << "\u001b[34m[Passed/Total]:[" << passedTests << "/" << totalTests << "]\u001b[0m\n";
	std::cout << "\u001b[34m[.moves()]:[" << (movesTime.totalTime / movesTime.totalRuns) << "ns/iter]:[" << movesTime.totalRuns << "iters]\u001b[0m\n";
	std::cout << "\u001b[34m[.move()]:[" << (moveTime.totalTime / moveTime.totalRuns) << "ns/iter]:[" << moveTime.totalRuns << "iters]\u001b[0m\n";
	std::cout << "\u001b[34m[.undo()]:[" << (undoTime.totalTime / undoTime.totalRuns) << "ns/iter]:[" << undoTime.totalRuns << "iters]\u001b[0m\n";
}