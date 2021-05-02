#include "../include/chess.h"

std::vector<chess::move> chess::game::moves(){
    // Check for 3-fold Repetition
    /*if(this->gameHistory.size() >= 5){
		if (chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 3], this->currentPosition()) && chess::position::pieceEqual(this->gameHistory[this->gameHistory.size() - 5], this->currentPosition())){
			return {};
		}
	}*/
    // Continue with normal move generation
	return this->currentPosition().moves();
}

std::vector<chess::move> chess::position::moves(){
	return {};
}