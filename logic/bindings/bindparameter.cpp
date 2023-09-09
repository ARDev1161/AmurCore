#include "bindparameter.h"

QList<QStringList> BindParamJoy::splitCommand(const QString &input)
{
    QList<QStringList> result;

    QStringList substrings = input.split("|");
    for (const QString& substring : substrings)
    {
        QStringList elements = substring.split("+");
        result.append(elements);
    }

    return result;
}
//void BindParamJoy::getSignalByString(const QString &signalName) const
//{
//    return signalIndexMap.value(signalName, -1);
//}
