// FIXME: Move this to tp-qt4 itself
#ifndef TYPES_H
#define TYPES_H

#include <QByteArray>
#include <QList>
#include <QMetaType>

typedef QList<QByteArray> CertificateDataList;
Q_DECLARE_METATYPE(CertificateDataList)

void registerTypes();

#endif
