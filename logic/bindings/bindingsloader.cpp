#include "bindingsloader.h"

BindingsLoader::BindingsLoader(QString configPath): configPath(configPath)
{
    loadBindings();
}

void BindingsLoader::loadBindings() {
    // Open the config file for reading
    QFile file(configPath);
    if (file.open(QIODevice::ReadOnly)) {
        // Read all the JSON data from the file
        QByteArray jsonData = file.readAll();

        // Create a JSON document from the JSON data
        QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));
        // Extract the JSON object from the JSON document
        QJsonObject jsonObj = jsonDoc.object();

        // Parse the JSON object to populate the bindTable
        parseJson(jsonObj);

        file.close(); // Close the file
    }
}

void BindingsLoader::saveBindings() {
    // Create a JSON object to store the key-value pairs
    QJsonObject jsonObj;

    // Save the key-value pairs from bindTable to the JSON object
    saveToJson(jsonObj);

    // Create a JSON document from the JSON object
    QJsonDocument jsonDoc(jsonObj);

    // Open the config file for writing
    QFile file(configPath);
    if (file.open(QIODevice::WriteOnly)) {
        // Write the JSON document to the file as formatted JSON
        file.write(jsonDoc.toJson());
        file.close();
    }
}

QHash<QString, QVariant> BindingsLoader::getBindTable() const
{
    return bindTable;
}

void BindingsLoader::parseJson(const QJsonObject& obj, const QString& parentPath) {
    QStringList keys = obj.keys(); // Get all keys in the JSON object
    foreach (const QString& key, keys) { // Iterate through each key
        QJsonValue value = obj.value(key); // Get the value corresponding to the key

        // Construct the full path by appending the current key to the parent path
        QString path = parentPath.isEmpty() ? key : parentPath + "." + key;

        if (value.isObject()) {
            // If the value is an object, recursively call parseJson with the nested object and updated path
            parseJson(value.toObject(), path);
        } else {

            bindTable.insert(path, value.toVariant());
        }
    }
}

void BindingsLoader::saveToJson(QJsonObject& obj, const QString& parentPath) {
    // Iterate through the bindTable using a QHashIterator
    QHashIterator<QString, QVariant> iter(bindTable);
    while (iter.hasNext()) {
        iter.next();
        QString path = iter.key();

        // Check if the current path starts with the parentPath
        if (path.startsWith(parentPath + ".")) {
            // Extract the key from the path by selecting the last section after the "."
            QString key = path.section(".", -1);

            // Insert the key-value pair into the JSON object
            obj.insert(key, QJsonValue::fromVariant(iter.value()));
        }
    }
}
