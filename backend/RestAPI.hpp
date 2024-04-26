#ifndef RESTAPI_HPP
#define RESTAPI_HPP

#include<QFuture>
#include<QtConcurrentRun>

#include"APIUtility.hpp"

template<typename K = qint64, typename T = void, typename = enable_if_t<std::conjunction_v<std::is_base_of<JSONable, T>, std::is_base_of<Updatable, T>>>>
class CRUDAPI
{

public:

    explicit CRUDAPI(const IdMap<K, T> &data, std::unique_ptr<FactoryFromJSON<T>> factory) :
        m_data(data),
        m_factory(std::move(factory))
    {}

    //TODO PAGINATOR?
    //TODO QFUTURE
    QFuture<QHttpServerResponse> GetPaginatedDataList(const QHttpServerRequest &request) const
    {
        using PaginatedDataType = PaginatedData<IdMap<K, T>>;
        auto optionalPage = std::optional<qsizetype>{};
        auto optionalPerPage = std::optional<qsizetype>{};
        auto optionalDelay = std::optional<qint64>{};

        if(request.query().hasQueryItem("page"))
            optionalPage = request.query().queryItemValue("page").toLongLong();
        if(request.query().hasQueryItem("per_page"))
            optionalPerPage = request.query().queryItemValue("per_page").toLongLong();
        if(request.query().hasQueryItem("delay"))
            optionalDelay = request.query().queryItemValue("delay").toLongLong();

        if( (optionalPage.has_value() && optionalPage.value() < 1) || (optionalPerPage.has_value() && optionalPerPage.value() < 1))
        {
            return QtConcurrent::run
                (
                [](){ return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);}
                );
        }

        auto paginatedData = PaginatedDataType
        {
            m_data, optionalPage
                ?   optionalPage.value() : PaginatedDataType::DEFAULT_PAGE,
                    optionalPerPage
                ?   optionalPerPage.value() : PaginatedDataType::DEFAULT_PAGE_SIZE
        };

        return QtConcurrent::run
            (
            [paginatedData = std::move(paginatedData), optionalDelay]()
                {
                    if(optionalDelay.has_value())
                        QThread::sleep(optionalDelay.value());
                    return paginatedData.IsValid()
                        ? QHttpServerResponse(paginatedData.ToJSON())
                        : QHttpServerResponse(QHttpServerResponder::StatusCode::NoContent);
                }
            );
    }

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
        return QHttpServerResponse(entry.value().ToJSON(), QHttpServerResponder::StatusCode::Created);
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

template<typename K = qint64>
class SessionAPI
{
public:

    explicit SessionAPI(const IdMap<K, SessionEntry> &sessions, std::unique_ptr<FactoryFromJSON<SessionEntry>> factory) :
        m_sessions(sessions),
        m_factory(std::move(factory))
    {}

    QHttpServerResponse RegisterSession(const QHttpServerRequest &request)
    {
        const auto optionalJson = ByteArrayToJSONObject(request.body());
        if(!optionalJson.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);
        const auto optionalItem = m_factory->FromJSON(optionalJson.value());
        if(!optionalItem.has_value())
            return QHttpServerResponse(QHttpServerResponder::StatusCode::BadRequest);

        const auto session = m_sessions.insert(optionalItem.value().id, optionalItem.value());
        session.value().StartSession();
        return QHttpServerResponse(session.value().ToJSON());
    }

    bool Authorize(const QHttpServerRequest &request) const
    {
        const auto optionalToken = GetTokenFromRequest(request);
        if(!optionalToken.has_value())
            return false;

        const auto optionalSession = std::find(m_sessions.begin(), m_sessions.end(), optionalToken.value());
        return optionalSession != m_sessions.end() && optionalSession.value() == optionalToken.value();
    }

private:

    IdMap<K, SessionEntry> m_sessions;
    std::unique_ptr<FactoryFromJSON<SessionEntry>> m_factory;
};

#endif // RESTAPI_HPP
