// FIXME: Move this to tp-qt4 itself
#include "types.h"

#include <QDBusMetaType>

void registerTypes()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;

    qDBusRegisterMetaType<CertificateDataList>();
}
