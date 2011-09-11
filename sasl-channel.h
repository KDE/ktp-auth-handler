//this is all utterly bollucks. Rewrite this.

#include <TelepathyQt4/Channel>
#include <TelepathyQt4/PendingStringList>

//: public Tp::RefCounted

class SaslChannel
{
public:
    SaslChannel(const Tp::ChannelPtr &channel);
    virtual ~SaslChannel();

    void startMechanismWithData(const QString &mechanism, const QByteArray &data);
//    void respond(const QByteArray &data);
    void acceptSasl();
//    void abortSasl(int reason, const QString &message=QString()); //FIXME enum type
    void close();
    
    //properties
    Tp::PendingStringList availableMechanims() const;
//    bool canTryAgain() const;
    
signals:
//    void saslStatusChanged(SASLStatus status, const QString &reason, const QVariantMap &details);
//  void newChallenge()
    
private:
    Tp::ChannelPtr m_channel;  

    Tp::Client::ChannelInterfaceSASLAuthenticationInterface* iface();
};
