#ifndef QXXML_H
#define QXXML_H

#include <QtXml>

namespace Qx
{

class XmlStreamReaderError
{
//-Instance Members----------------------------------------------------------------------------------------------
private:
    static inline const QHash<QXmlStreamReader::Error, QString> STD_ERR_TXT = {
        {QXmlStreamReader::NoError, "No error has occured."},
        {QXmlStreamReader::CustomError, "A custom error has been raised with raiseError()."},
        {QXmlStreamReader::NotWellFormedError, "The parser internally raised an error due to the read XML not being well-formed."},
        {QXmlStreamReader::PrematureEndOfDocumentError, "The input stream ended before a well-formed XML document was parsed."},
        {QXmlStreamReader::UnexpectedElementError, "The parser encountered an element that was different to those it expected."}
    };

    //TODO: Because of the missed errorString() method that is part of QIODevice, this hash isn't needed

//-Instance Members----------------------------------------------------------------------------------------------
private:
    QXmlStreamReader::Error mErrorType;
    QString mErrorText;

//-Constructor---------------------------------------------------------------------------------------------------
public:
    XmlStreamReaderError();
    XmlStreamReaderError(QXmlStreamReader::Error standardError);
    XmlStreamReaderError(QString customError);

//-Instance Functions--------------------------------------------------------------------------------------------
public:
    bool isValid();
    QXmlStreamReader::Error getType();
    QString getText();
};

//-Functions-------------------------------------------------------------------------------------------------------------
//Public:
QString xmlSanitized(QString string);

}

#endif // QXXML_H
