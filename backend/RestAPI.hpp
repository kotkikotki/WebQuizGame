#ifndef RESTAPI_HPP
#define RESTAPI_HPP

#include"APIUtility.hpp"

template<typename K = quint64, typename T = void, typename = enable_if_t<std::conjunction_v<std::is_base_of<JSONable, T>, std::is_base_of<Updatable, T>>>>
class CRUDAPI
{

public:

    explicit CRUDAPI(const IdMap<K, T> &data, std::unique_ptr<FactoryFromJSON<T>> factory) :
        m_data(data),
        m_factory(std::move(factory))
    {}

    //TODO PAGINATOR?
    //TODO QFUTURE

    //READ
    QHttpServerResponse GetItem(K itemId) const
    {
        const auto item = m_data.find(itemId);
        return item != m_data.end()
            ? QHttpServerResponse(item.value().ToJSON())
            : QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);

    }

    //CREATE
    QHttpServerResponse PostItem(const QHttpServerRequest &request)
    {
        const auto optionalJson = ByteArrayToJSONObject(request.body());
        if(!optionalJson.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        const auto optionalItem = m_factory->FromJSON(optionalJson.value());
        if(!optionalItem.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        if(m_data.contains(optionalItem.value().id))
            return QHttpServerResponse(QHttpServerResponder::StatusCode::AlreadyReported);

        const auto entry = m_data.insert(optionalItem.value().id, optionalItem.value());
        return QHttpServerResponse(entry.value().ToJSON, QHttpServerResponder::StatusCode::Created);
    }

    //UPDATE
    QHttpServerResponse PutItem(K itemId, const QHttpServerRequest &request)
    {
        const auto optionalJson = ByteArrayToJSONObject(request.body());
        if(!optionalJson.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        auto item = m_data.find(itemId);
        if(item == m_data.end())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
        if(!item.value().Update(optionalJson.value()))
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        return QHttpServerResponse(item.value().ToJSON());
    }

    QHttpServerResponse PutItemFields(K itemId, const QHttpServerRequest &request)
    {
        const auto optionalJson = ByteArrayToJSONObject(request.body());
        if(!optionalJson.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        auto item = m_data.find(itemId);
        if(item == m_data.end())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
        item.value().UpdateFields(optionalJson.value());

        return QHttpServerResponse(item.value().ToJSON());
    }

    //DELETE
    QHttpServerResponse DeleteItem(K itemId)
    {
        if(!m_data.remove(itemId))
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
        return QHttpServerResponse(QHttpServerResponder::StatusCode::Ok);
    }

private:

    IdMap<K, T> m_data;
    std::unique_ptr<FactoryFromJSON<T>> m_factory;
};

#endif // RESTAPI_HPP
