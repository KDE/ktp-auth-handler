#include "sasl-channel.h"

SaslChannel::SaslChannel(const Tp::ChannelPtr &channel)
{
    m_channel = channel;
}

SaslChannel::~SaslChannel()
{

}

void SaslChannel::startMechanismWithData(const QString &mechanism, const QByteArray &data)
{
    iface()->StartMechanismWithData(mechanism, data);
}


void SaslChannel::acceptSasl()
{
    iface()->AcceptSASL();
}


void SaslChannel::close()
{
    m_channel->requestClose();
}


Tp::Client::ChannelInterfaceSASLAuthenticationInterface * SaslChannel::iface()
{
    return m_channel->interface<Tp::Client::ChannelInterfaceSASLAuthenticationInterface>();
}
