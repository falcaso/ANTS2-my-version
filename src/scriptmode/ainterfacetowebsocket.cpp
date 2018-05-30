#include "ainterfacetowebsocket.h"
#include "awebsocketstandalonemessanger.h"
#include "awebsocketsession.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>

AInterfaceToWebSocket::AInterfaceToWebSocket()
{
    standaloneMessenger = new AWebSocketStandaloneMessanger();
    sessionMessenger    = new AWebSocketSession();
}

AInterfaceToWebSocket::~AInterfaceToWebSocket()
{
    standaloneMessenger->deleteLater();
    sessionMessenger->deleteLater();
}

void AInterfaceToWebSocket::SetTimeout(int milliseconds)
{
    standaloneMessenger->setTimeout(milliseconds);
    sessionMessenger->setTimeout(milliseconds);
}

const QString AInterfaceToWebSocket::SendTextMessage(const QString &Url, const QVariant& message, bool WaitForAnswer)
{
   bool bOK = standaloneMessenger->sendTextMessage(Url, message, WaitForAnswer);

   if (!bOK)
   {
       abort(standaloneMessenger->getError());
       return "";
   }
   return standaloneMessenger->getReceivedMessage();
}

int AInterfaceToWebSocket::Ping(const QString &Url)
{
    int ping = standaloneMessenger->ping(Url);

    if (ping < 0)
    {
        abort(standaloneMessenger->getError());
        return -1;
    }
    return ping;
}

void AInterfaceToWebSocket::Connect(const QString &Url)
{
    bool bOK = sessionMessenger->connect(Url);
    if (!bOK)
        abort(sessionMessenger->getError());
}

void AInterfaceToWebSocket::Disconnect()
{
    sessionMessenger->disconnect();
}

const QString AInterfaceToWebSocket::SendText(const QString &message)
{
    bool bOK = sessionMessenger->sendText(message);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendObject(const QVariant &object)
{
    if (object.type() != QMetaType::QVariantMap)
    {
        abort("Argument type of SendObject() method should be object!");
        return false;
    }
    QVariantMap vm = object.toMap();
    QJsonObject js = QJsonObject::fromVariantMap(vm);

    bool bOK = sessionMessenger->sendJson(js);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QString AInterfaceToWebSocket::SendFile(const QString &fileName)
{
    bool bOK = sessionMessenger->sendFile(fileName);
    if (bOK)
        return sessionMessenger->getTextReply();
    else
    {
        abort(sessionMessenger->getError());
        return 0;
    }
}

const QVariant AInterfaceToWebSocket::GetBinaryReplyAsObject() const
{
    const QByteArray& bs = sessionMessenger->getBinaryReply();
    QJsonDocument doc( QJsonDocument::fromJson(bs) );
    QJsonObject json = doc.object();

    QVariantMap vm = json.toVariantMap();
    return vm;
}

bool AInterfaceToWebSocket::SaveBinaryReplyToFile(const QString &fileName)
{
    const QByteArray& bs = sessionMessenger->getBinaryReply();
    QJsonDocument doc( QJsonDocument::fromJson(bs) );

    QFile saveFile(fileName);
    if ( !saveFile.open(QIODevice::WriteOnly) )
    {
        abort( QString("Cannot save binary reply to file: ") + fileName );
        return false;
    }
    saveFile.write(doc.toJson());
    saveFile.close();
    return true;
}
