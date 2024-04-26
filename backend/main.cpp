#include "mainwindow.h"

#include <QApplication>

#include"APISetup.hpp"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    //

    auto categoryFactory = std::make_unique<CategoryFactory>();
    auto categories = TryLoadFromFile<qint64, Category>(*categoryFactory, ":/assets/categories.json");//
    auto categoriesApi = CRUDAPI<qint64, Category>{std::move(categories), std::move(categoryFactory)};

    auto sessionFactory = std::make_unique<SessionEntryFactory>();
    auto sessions = TryLoadFromFile<qint64, SessionEntry>(*sessionFactory, ":/assets/sessions.json");//
    auto sessionsApi = SessionAPI<qint64>{std::move(sessions), std::move(sessionFactory)};

    auto httpServer = QHttpServer{};
    httpServer.route
        (
            "/", []()
            {
                return "RestAPI server";
            }
        );

    AddCRUDRoutes(httpServer, "/api/categories/", categoriesApi, sessionsApi);

    const auto port = httpServer.listen(QHostAddress::Any, PORT);
    if(!port)
    {
        //qcore translate todo
        qDebug() << "Server failed to start";
        return 0;
    }

    qDebug() << QString("Running on http://%1:").arg(HOST).append("%1/").arg(PORT);

    //
    w.show();
    return a.exec();
}
