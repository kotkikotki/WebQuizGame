#ifndef APIUTILITY_HPP
#define APIUTILITY_HPP

#include<QFile>
#include<QJsonParseError>
#include<QHttpServer>

#include"Structs.hpp"

static std::optional<QByteArray> ReadFileToByteArray(const QString &path)
{
    auto file = QFile{path};

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return std::nullopt;

    return file.readAll();
}

static std::optional<QJsonArray> ByteArrayToJSONArray(const QByteArray &array)
{
    auto error = QJsonParseError{};
    const auto json = QJsonDocument::fromJson(array, &error);
    if(error.error || !json.isArray())
        return std::nullopt;

    return json.array();
}

static std::optional<QJsonObject> ByteArrayToJSONObject(const QByteArray &array)
{
    auto error = QJsonParseError{};
    const auto json = QJsonDocument::fromJson(array, &error);
    if(error.error || !json.isObject())
        return std::nullopt;

    return json.object();
}

template<typename K = quint64, typename T = void>
static IdMap<K, T> TryLoadFromFile(const FactoryFromJSON<T> &factory, const QString &path)
{
    const auto optionalByteArray = ReadFileToByteArray(path);

    if(!optionalByteArray.has_value())
    {
        qDebug() << "Reading file " << path << "failed";
        return IdMap<K, T>();
    }

    const auto optionalArray = ByteArrayToJSONArray(optionalByteArray.value());
    if(!optionalArray.has_value())
    {
        qDebug() << "Content of file " << path << "is not a json array";
        return IdMap<K, T>();
    }

    return IdMap<K, T>(factory, optionalArray.value());

}

static QByteArray GetValueFromHeader(const QList<QPair<QByteArray, QByteArray>> &headers, QByteArrayView headerName)
{
    for(const auto &[key, value] : headers)
    {
        if(key.compare(headerName, Qt::CaseInsensitive) == 0)
            return value;
    }
    return QByteArray();
}

static std::optional<QString> GetTokenFromRequest(const QHttpServerRequest &request)
{
    auto token = std::optional<QString>{};
    auto bytes = GetValueFromHeader(request.headers(), "token");
    if(!bytes.isEmpty())
        token = bytes;

    return token;
}

#endif // APIUTILITY_HPP
