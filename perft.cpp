#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "include/chess.hpp"

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
        TIME(chess::staticVector validMoves { gameToTest.moves() }, movesTime);
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
	std::vector<perftTest> tests {
		{ { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", { { 1, 20 }, { 2, 400 }, { 3, 8902 }, { 4, 197281 }, { 5, 4865609 }, { 6, 119060324 } } },
		  { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", { { 1, 48 }, { 2, 2039 }, { 3, 97862 }, { 4, 4085603 }, { 5, 193690690 } } },
		  { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", { { 1, 14 }, { 2, 191 }, { 3, 2812 }, { 4, 43238 }, { 5, 674624 }, { 6, 11030083 }, { 7, 178633661 } } },
		  { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", { { 1, 6 }, { 2, 264 }, { 3, 9467 }, { 4, 422333 }, { 5, 15833292 }, { 6, 706045033 } } },
		  { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", { { 1, 44 }, { 2, 1486 }, { 3, 62379 }, { 4, 2103487 }, { 5, 89941194 } } },
		  { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", { { 1, 46 }, { 2, 2079 }, { 3, 89890 }, { 4, 3894594 }, { 5, 164075551 } } },
		  { "r6r/1b2k1bq/8/8/7B/8/8/R3K2R b KQ - 3 2", { { 1, 8 }, { 2, 192 }, { 3, 8355 }, { 4, 206081 }, { 5, 9296387 }, { 6, 233123274 } } },
		  { "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 5 3", { { 1, 8 }, { 2, 72 }, { 3, 492 }, { 4, 5380 }, { 5, 36744 }, { 6, 444954 }, { 7, 3039234 }, { 8, 39213236 } } },
		  { "r1bqkbnr/pppppppp/n7/8/8/P7/1PPPPPPP/RNBQKBNR w KQkq - 2 2", { { 1, 19 }, { 2, 380 }, { 3, 8163 }, { 4, 182327 }, { 5, 4364503 }, { 6, 107844586 } } },
		  { "r3k2r/p1pp1pb1/bn2Qnp1/2qPN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQkq - 3 2", { { 1, 5 }, { 2, 259 }, { 3, 11766 }, { 4, 563603 }, { 5, 24890051 } } },
		  { "2kr3r/p1ppqpb1/bn2Qnp1/3PN3/1p2P3/2N5/PPPBBPPP/R3K2R b KQ - 3 2", { { 1, 44 }, { 2, 2385 }, { 3, 99756 }, { 4, 5144430 }, { 5, 213199102 } } },
		  { "rnb2k1r/pp1Pbppp/2p5/q7/2B5/8/PPPQNnPP/RNB1K2R w KQ - 3 9", { { 1, 39 }, { 2, 1577 }, { 3, 63647 }, { 4, 2433142 }, { 5, 102218344 } } },
		  { "2r5/3pk3/8/2P5/8/2K5/8/8 w - - 5 4", { { 1, 9 }, { 2, 163 }, { 3, 1349 }, { 4, 23718 }, { 5, 177964 }, { 6, 3245821 }, { 7, 24053172 }, { 8, 449857603 } } },
		  { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", { { 1, 44 }, { 2, 1486 }, { 3, 62379 }, { 4, 2103487 }, { 5, 89941194 } } },
		  { "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", { { 1, 18 }, { 2, 92 }, { 3, 1670 }, { 4, 10138 }, { 5, 185429 }, { 6, 1134888 }, { 7, 20757544 }, { 8, 130459988 } } },
		  { "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", { { 1, 13 }, { 2, 102 }, { 3, 1266 }, { 4, 10276 }, { 5, 135655 }, { 6, 1015133 }, { 7, 14047573 }, { 8, 102499941 } } },
		  { "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", { { 1, 15 }, { 2, 126 }, { 3, 1928 }, { 4, 13931 }, { 5, 206379 }, { 6, 1440467 }, { 7, 21190412 }, { 8, 144302151 } } },
		  { "5k2/8/8/8/8/8/8/4K2R w K - 0 1", { { 1, 15 }, { 2, 66 }, { 3, 1198 }, { 4, 6399 }, { 5, 120330 }, { 6, 661072 }, { 7, 12762196 }, { 8, 73450134 } } },
		  { "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", { { 1, 16 }, { 2, 71 }, { 3, 1286 }, { 4, 7418 }, { 5, 141077 }, { 6, 803711 }, { 7, 15594314 }, { 8, 91628014 } } },
		  { "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", { { 1, 26 }, { 2, 1141 }, { 3, 27826 }, { 4, 1274206 }, { 5, 31912360 } } },
		  { "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", { { 1, 44 }, { 2, 1494 }, { 3, 50509 }, { 4, 1720476 }, { 5, 58773923 } } },
		  { "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", { { 1, 11 }, { 2, 133 }, { 3, 1442 }, { 4, 19174 }, { 5, 266199 }, { 6, 3821001 }, { 7, 60651209 } } },
		  { "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", { { 1, 29 }, { 2, 165 }, { 3, 5160 }, { 4, 31961 }, { 5, 1004658 }, { 6, 6333969 }, { 7, 197004390 } } },
		  { "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", { { 1, 9 }, { 2, 40 }, { 3, 472 }, { 4, 2661 }, { 5, 38983 }, { 6, 217342 }, { 7, 3742283 }, { 8, 20625698 }, { 9, 397481663 } } },
		  { "8/P1k5/K7/8/8/8/8/8 w - - 0 1", { { 1, 6 }, { 2, 27 }, { 3, 273 }, { 4, 1329 }, { 5, 18135 }, { 6, 92683 }, { 7, 1555980 }, { 8, 8110830 }, { 9, 153850274 } } },
		  { "K1k5/8/P7/8/8/8/8/8 w - - 0 1", { { 1, 2 }, { 2, 6 }, { 3, 13 }, { 4, 63 }, { 5, 382 }, { 6, 2217 }, { 7, 15453 }, { 8, 93446 }, { 9, 998319 }, { 10, 5966690 }, { 11, 85822924 } } },
		  { "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", { { 1, 10 }, { 2, 25 }, { 3, 268 }, { 4, 926 }, { 5, 10857 }, { 6, 43261 }, { 7, 567584 }, { 8, 2518905 }, { 9, 37109897 } } },
		  { "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", { { 1, 37 }, { 2, 183 }, { 3, 6559 }, { 4, 23527 }, { 5, 811573 }, { 6, 3114998 }, { 7, 104644508 } } } }
	};

	if (argc == 3) {
		std::string fen              = argv[1];
		chess::position testPosition = chess::position::fromFen(fen);
		std::cout << "\u001b[34m[Test]@Position=" << fen << std::endl;
		std::cout << "\u001b[33m" << testPosition.ascii() << "\u001b[34m" << std::endl;
		auto startTime         = std::chrono::high_resolution_clock::now();
		perftResult testResult = perft(std::stoull(argv[2]), fen);
		auto endTime           = std::chrono::high_resolution_clock::now();
		auto duration          = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		for (auto& [move, total] : testResult.moves) {
			std::cout << "\t[" << move.toString() << "]:[" << total << "]\n";
		}
		std::cout << "[Total Nodes Visited]:[" << testResult.total << "] [Test Duration]:[" << duration << "ms]\u001b[0m\n";
		return 0;
	}

	std::cout << "\u001b[31mImproper or no arguments were given - running default tests" << std::endl;

	size_t perftTotalTests  = 0;
	size_t perftPassedTests = 0;
	size_t fenTotalTests    = 0;
	size_t fenPassedTests   = 0;

	for (const perftTest& test : tests) {
		std::cout << "\u001b[34m[Test]@Position=" << test.testFen << std::endl;
		fenTotalTests++;
		std::string testFenResult { chess::game { test.testFen }.currentPosition().toFen() };
		if (testFenResult == test.testFen) {
			fenPassedTests++;
			std::cout << "\t[Fen]\u001b[32m[Passed Test]\u001b[0m -> [Result]:[" << testFenResult << "]" << std::endl;
		} else {
			std::cout << "\t[Fen]\u001b[31m[Failed Test]\u001b[0m -> [Result]:[" << testFenResult << "] - [Known Result]:[" << test.testFen << "]" << std::endl;
		}

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
				std::cout << "\t\u001b[32m[Passed Test]@Depth=" << knownTestResult.depth << "\u001b[0m -> [Perft Result]:[" << testResult.total << "] - \u001b[34m[kn/s]:[" << testResult.total * 1000 / duration << "]" << std::endl;
				perftPassedTests++;
			}
			perftTotalTests++;
		}
	}
	std::cout << "\u001b[34m[Perft:Passed/Total]:[" << perftPassedTests << "/" << perftTotalTests << "]\u001b[0m\n";
	std::cout << "\u001b[34m[Fen:Passed/Total]:[" << fenPassedTests << "/" << fenTotalTests << "]\u001b[0m\n";
	std::cout << "\u001b[34m[.moves()]:[" << (movesTime.totalTime / movesTime.totalRuns) << "ns/iter]:[" << movesTime.totalRuns << "iters]\u001b[0m\n";
	std::cout << "\u001b[34m[.move()]:[" << (moveTime.totalTime / moveTime.totalRuns) << "ns/iter]:[" << moveTime.totalRuns << "iters]\u001b[0m\n";
	std::cout << "\u001b[34m[.undo()]:[" << (undoTime.totalTime / undoTime.totalRuns) << "ns/iter]:[" << undoTime.totalRuns << "iters]\u001b[0m\n";
}