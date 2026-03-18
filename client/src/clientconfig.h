/** clientconfig.h
 * Pull from settings.ini and provide configuration values to client code.
 * It will also produce a default settings.ini if one does not exist, with default profiles.
 */

#include "task.h"

#include <QString>

class ClientConfig
{
public:
    ClientConfig();
    
    // Get the file path to the settings.ini file
    static QString settingsFilePath();

    // Client settings
    static bool syncEnabled();
    static QString serverAddress();
    static int serverPort();
    static QString clientId();
    static QString clientCertPath();
    static QString clientKeyPath();
    static QString serverCaPath();
};