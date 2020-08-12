#ifndef QXXML_H
#define QXXML_H

#include <QtXml>

namespace Qx
{

class XmlStreamReaderError
{
//-Instance Members----------------------------------------------------------------------------------------------
private:
    QXmlStreamReader::Error mErrorType;
    QString mErrorText;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    XmlStreamReaderError();
    XmlStreamReaderError(QXmlStreamReader::Error standardError);
    XmlStreamReaderError(QString customError);

//-Class Functions-----------------------------------------------------------------------------------------------
private:
    static QString textFromStandardError(QXmlStreamReader::Error standardError);

//-Instance Functions--------------------------------------------------------------------------------------------
public:
    bool isValid();
    QXmlStreamReader::Error getType();
    QString getText();
};

}

#endif // QXXML_H
