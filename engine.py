import chess

userGame = chess.defaultGame()

while not userGame.finished():
    print(userGame.position().ascii())
    move = None
    while True:
        move = input("What is the move you would like to make? ")
        if userGame.validMove(move):
            break
    userGame.move(move)
print("Game Result:", userGame.result())