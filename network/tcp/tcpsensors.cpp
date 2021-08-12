#include "tcpsensors.h"
// TCP server
TCPSensors::TCPSensors(AMUR::AmurSensors *sensors, QString *hostname): tcpSocket(new QTcpSocket(this))
{
    this->sensors = sensors;
    this->hostName = hostname;
    initNetwork();
}


TCPSensors::~TCPSensors()
{

}

void TCPSensors::stop()
{

}

void TCPSensors::initNetwork()
{

    // find out IP addresses of this machine
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    in.setDevice(tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);
    //connect(getFortuneButton, &QAbstractButton::clicked,
    //        this, &Client::requestNewFortune);
    connect(tcpSocket, &QIODevice::readyRead, this, &TCPSensors::readControl);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &TCPSensors::displayError);

    QNetworkConfigurationManager manager;

    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
        settings.beginGroup(QLatin1String("QtNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();

        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered){
            config = manager.defaultConfiguration();
        }

        networkSession = new QNetworkSession(config, this);
        connect(networkSession, &QNetworkSession::opened, this, &TCPSensors::sessionOpened);

        networkSession->open();
    }
}

void TCPSensors::requestNewControl()
{
tcpSocket->abort();
tcpSocket->connectToHost(*hostName, port);
}

void TCPSensors::readControl()
{
in.startTransaction();

QString nextSensors;
in >> nextSensors;

if (!in.commitTransaction())
    return;

if (nextSensors == currentSensors) {
    QTimer::singleShot(0, this, &TCPSensors::requestNewControl);
    return;
}

currentSensors = nextSensors;
}

void TCPSensors::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
        //    QMessageBox::information(this, tr("Fortune Client"),
        //                             tr("The host was not found. Please check the "
        //                                "host name and port settings."));
            break;
        case QAbstractSocket::ConnectionRefusedError:
        //    QMessageBox::information(this, tr("Fortune Client"),
        //                             tr("The connection was refused by the peer. "
        //                                "Make sure the fortune server is running, "
        //                                "and check that the host name and port "
        //                                "settings are correct."));
            break;
        default:
        //    QMessageBox::information(this, tr("Fortune Client"),
        //                             tr("The following error occurred: %1.")
        //                             .arg(tcpSocket->errorString()));
            break;
    }

}

void TCPSensors::sessionOpened()
{
// Save the used configuration
QNetworkConfiguration config = networkSession->configuration();
QString id;
if (config.type() == QNetworkConfiguration::UserChoice)
    id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
else
    id = config.identifier();

QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
settings.beginGroup(QLatin1String("QtNetwork"));
settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
settings.endGroup();

}
