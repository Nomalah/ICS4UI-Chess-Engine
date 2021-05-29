import requests
import json
import time
from collections import deque
import subprocess

activeChallenges = deque()
botInGame = False
botIsPlayingWhite = None
botId = None
supportedTimeControl = ["15+10"]
supportedVariant = ["Std"]
apiKey = "D6U94LoVxhC30ot2"
apiHeader = {"Authorization": "Bearer " + apiKey}

def dest(urlShort):
    return "https://lichess.org/api" + urlShort

print("Getting account info")
accountInfo = requests.get(dest("/account"), headers=apiHeader)
botId = accountInfo.json()["id"]

def fjsonPrint(unformatJson):
    print(json.dumps(unformatJson, indent=4))

def getBotMove(listOfMoves):
    print("Generating bot move")
    engineThinking = subprocess.run(["./lichessEngine"] + listOfMoves, stdout=subprocess.PIPE)
    #format engine output
    moveUCI = engineThinking.stdout.decode("utf-8")
    if len(moveUCI) == 5 and moveUCI[4] == "*":
        return moveUCI[:4]
    return moveUCI

def handleGameState(gameId, gameState):
    if gameState["status"] != "started":
        return True
    
    botMove = None

    listOfMoves = gameState["moves"].split()
    if len(listOfMoves) % 2 == 0:
        # white's move
        if botIsPlayingWhite:
            if len(listOfMoves) > 1:
                print("Opponent has moved {}".format(listOfMoves[-1]))
            botMove = getBotMove(listOfMoves)            
    else:
        # black's move
        if not botIsPlayingWhite:
            print("Opponent has moved {}".format(listOfMoves[-1]))
            botMove = getBotMove(listOfMoves)            
    
    if botMove != None:
        makeMoveResponse = None
        print("Sending move {}".format(botMove))
        for _ in range(5): 
            print("Attempting...")
            makeMoveResponse = requests.post(dest("/bot/game/{}/move/{}".format(gameId, botMove)), headers=apiHeader)
            if makeMoveResponse != None and makeMoveResponse.status_code == 200:
                print("Move successful")
                break
        else:
            print("Move failed - resigning")
            abortGame = requests.post(dest("/bot/game/{}/resign".format(gameId)), headers=apiHeader)
            return
        

    return False
    

def gameStart(jsonEvent):
    global botIsPlayingWhite
    global botId
    gameId = jsonEvent["game"]["id"]
    print("Starting game with id {}".format(gameId))
    print("Opening game stream")
    incomingGameData = None
    for _ in range(5): 
        time.sleep(1)
        print("Attempting...")
        incomingGameData = requests.get(dest("/bot/game/stream/{}".format(gameId)), headers=apiHeader, stream=True)
        if incomingGameData != None and incomingEvents.status_code == 200:
            break
    else:
        return
    print("Opening game stream successful")
    if incomingGameData.encoding is None:
        incomingGameData.encoding = "utf-8"
    gameOver = False
    timeout = None
    moveNum = 0
    startTime = time.time()
    for line in incomingGameData.iter_lines(decode_unicode=True):
        if line:
            jsonResponse = json.loads(line)
            if jsonResponse["type"] == "gameFull":
                botIsPlayingWhite = jsonResponse["white"]["id"] == botId
                gameOver = handleGameState(gameId, jsonResponse["state"])
                moveNum += 1
            elif jsonResponse["type"] == "gameState":
                gameOver = handleGameState(gameId, jsonResponse)
                moveNum += 1

        if moveNum == 2 and botIsPlayingWhite and time.time()-startTime > 30:
            print("Aborting game - timeout")
            abortGame = requests.post(dest("/bot/game/{}/abort".format(gameId)), headers=apiHeader)
            incomingGameData.close()
            return
        if moveNum == 1 and not botIsPlayingWhite and time.time()-startTime > 30:
            print("Aborting game - timeout")
            abortGame = requests.post(dest("/bot/game/{}/abort".format(gameId)), headers=apiHeader)
            incomingGameData.close()
            return

        if gameOver:
            return

def challenge(jsonEvent):
    global botInGame
    global activeChallenges
    if not botInGame:
        print("Evaluating challenge")
        if jsonEvent["challenge"]["timeControl"]["show"] in supportedTimeControl and jsonEvent["challenge"]["variant"]["short"] in supportedVariant:
            acceptChallenge = requests.post(dest("/challenge/{}/accept".format(jsonEvent["challenge"]["id"])), headers=apiHeader)
            if acceptChallenge.status_code == 200:
                print("Accepted challenge from {}".format(jsonEvent["challenge"]["challenger"]["name"]))
                botInGame = True
                return
        else:
            print("Challenge had unsupported time control or variant")
            declineChallenge = requests.post(dest("/challenge/{}/decline".format(jsonEvent["challenge"]["id"])), headers=apiHeader, data={"reason":"timeControl"})
        if len(activeChallenges) > 0:
            challenge(activeChallenges.popleft())
    else:
        print("Added challenge to challenge queue")
        activeChallenges.append(jsonEvent)

def gameFinished(jsonEvent):
    global botInGame
    global botIsPlayingWhite
    global activeChallenges
    print("Game finished, reseting")
    botInGame = False
    botIsPlayingWhite = None
    if len(activeChallenges) > 0:
        print("Accepting next challenge")
        challenge(activeChallenges.popleft())
    else:
        print("Waiting for next challenge")

def challengeCanceled(jsonEvent):
    pass

def eventHandler(jsonEvent):
    handlerDict = {
        "gameStart": gameStart,
        "gameFinish": gameFinished,
        "challenge": challenge,
        "challengeCanceled": challengeCanceled
    }
    handler = handlerDict.get(jsonEvent["type"], lambda a : None)
    handler(jsonEvent)
    return

print("Opening event stream")
incomingEvents = None
while incomingEvents == None or incomingEvents.status_code != 200:
    time.sleep(1)
    print("Attempting...")
    incomingEvents = requests.get(dest("/stream/event"), headers=apiHeader, stream=True)
print("Opening event stream successful")


if incomingEvents.encoding is None:
    incomingEvents.encoding = "utf-8"

for line in incomingEvents.iter_lines(decode_unicode=True):
    if line:
        eventHandler(json.loads(line))
