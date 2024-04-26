#ifndef APISETUP_HPP
#define APISETUP_HPP

#include"RestAPI.hpp"

#define SCHEME "htpp"
#define HOST "127.0.0.1"
#define PORT 12345

//TODO ASK FOR AUTH?? CHANGE AUTH??

template<typename K = qint64, typename T = void, typename = enable_if_t<std::conjunction_v<std::is_base_of<JSONable, T>, std::is_base_of<Updatable, T>>>>
void AddCRUDRoutes(QHttpServer &httpServer, const QString &apiPath, CRUDAPI<K, T> &api, const SessionAPI<K> &sessionApi)
{
    //GET paginated data list
    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Get,
            [&api](const QHttpServerRequest &request) {return api.GetPaginatedDataList(request);}
        );

    //GET single item
    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Get,
            [&api](K itemId) {return api.GetItem(itemId);}
        );

    //POST
    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Post,
            [&api, &sessionApi](const QHttpServerRequest &request)
            {
                if(!sessionApi.Authorize(request))
                    return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
                return api.PostItem(request);
            }
        );

    //PUT
    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Put,
            [&api, &sessionApi](K itemId, const QHttpServerRequest &request)
            {
                if(!sessionApi.Authorize(request))
                    return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
                return api.PutItem(itemId, request);
            }
        );

    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Patch,
            [&api, &sessionApi](K itemId, const QHttpServerRequest &request)
            {
                if(!sessionApi.Authorize(request))
                    return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
                return api.PutItemFields(itemId, request);
            }
        );

    //DELETE
    httpServer.route
        (
            QString("%1").arg(apiPath), //apiPath?
            QHttpServerRequest::Method::Patch,
            [&api, &sessionApi](K itemId, const QHttpServerRequest &request)
            {
                if(!sessionApi.Authorize(request))
                    return QHttpServerResponse(QHttpServerResponder::StatusCode::Unauthorized);
                return api.DeleteItem(itemId);
            }
        );
}

#endif // APISETUP_HPP
