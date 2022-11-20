#include "jsonworker.h"

JSONWorker::JSONWorker(QObject *parent)
    : QObject{parent}
{

}

int JSONWorker::readJSON(QString filename, const QJsonObject &json)
{
    QFile loadFile(filename);

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));

    readJSON(loadDoc.object());


    return 0;
}

int JSONWorker::writeJSON(QString filename, QJsonObject &json) const
{
    QFile saveFile(filename);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject gameObject;

    writeJSON(gameObject);

    saveFile.write(QJsonDocument(gameObject).toJson());


    return 0;
}


//! [1]
void Game::read(const QJsonObject &json)
{
    if (json.contains("player") && json["player"].isObject())
        mPlayer.read(json["player"].toObject());

    if (json.contains("levels") && json["levels"].isArray()) {
        QJsonArray levelArray = json["levels"].toArray();
        mLevels.clear();
        mLevels.reserve(levelArray.size());
        for (int levelIndex = 0; levelIndex < levelArray.size(); ++levelIndex) {
            QJsonObject levelObject = levelArray[levelIndex].toObject();
            Level level;
            level.read(levelObject);
            mLevels.append(level);
        }
    }
}
//! [1]

//! [2]
void Game::write(QJsonObject &json) const
{
    QJsonObject playerObject;
    mPlayer.write(playerObject);
    json["player"] = playerObject;

    QJsonArray levelArray;
    for (const Level &level : mLevels) {
        QJsonObject levelObject;
        level.write(levelObject);
        levelArray.append(levelObject);
    }
    json["levels"] = levelArray;
}
//! [2]
