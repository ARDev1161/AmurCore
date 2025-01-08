#ifndef ROBOTENTRY_H
#define ROBOTENTRY_H

#include <QObject>
#include <QtNetwork>

class RobotEntry
{
    QString m_name;
    QString m_machineID;

    QHostAddress m_address; ///< IP address
    quint16 m_port;
    quint16 m_portForAnswer = 0;
    QString m_macAddress; ///< MAC address

    QUdpSocket *m_socket = nullptr;
public:
    RobotEntry() = default;
    RobotEntry(QUdpSocket *socket,
               const QHostAddress &address,
               quint16 port,
               QString machineID = QString(),
               QString name = QString(),
               QString macAddress = QString())
        : m_socket(socket)
        , m_address(address)
        , m_port(port)
        , m_machineID(machineID)
        , m_name(name)
        , m_macAddress(macAddress)
    {}

    void sendData(const QByteArray &data)
    {
        if (m_socket == nullptr) return;
        if(m_portForAnswer > 0)
            m_socket->writeDatagram(data, m_address, m_portForAnswer);
        else
            m_socket->writeDatagram(data, m_address, m_port);
    }

    QString name() const { return m_name; }
    void setName(const QString &val) { m_name = val; }
    QString machineID() const { return m_machineID; }
    void setMachineID(const QString &val) { m_machineID = val; }


    QHostAddress address() const { return m_address; }
    void setAddress(const QHostAddress &val) { m_address = val; }
    quint16 port() const { return m_port; }
    void setPort(quint16 val) { m_port = val; }
    quint16 getPortForAnswer() const { return m_portForAnswer; }
    void setPortForAnswer(quint16 val) { m_portForAnswer = val; }

    // Новое поле: MAC-адрес
    QString macAddress() const { return m_macAddress; }
    void setMacAddress(const QString &val) { m_macAddress = val; }
};

using RobotEntryPtr = std::shared_ptr<RobotEntry>;
using RobotList = QList<RobotEntryPtr>;

#endif // ROBOTENTRY_H
