#ifndef STRUCTS_HPP
#define STRUCTS_HPP

#include<QJsonObject>
#include<QJsonArray>
#include<optional>

#include"Utility.hpp"

struct JSONable
{
    virtual QJsonObject ToJSON() const = 0;
    virtual ~JSONable() = default;
};

struct Updatable
{
    virtual bool Update(const QJsonObject &json) = 0;
    virtual void UpdateFields(const QJsonObject &json) = 0;
    virtual ~Updatable() = default;
};

template<typename T>
struct FactoryFromJSON
{
    virtual std::optional<T> FromJSON(const QJsonObject &json) const = 0;
    virtual ~FactoryFromJSON() = default;
};

template<typename T, typename = enable_if_t<std::is_base_of_v<JSONable, T>>>
QJsonArray ToJSONArray(const QList<T> &list)
{
    auto jsonArray = QJsonArray{};
    for(const auto &e: list)
    {
        jsonArray.append(e.ToJSON());
    }

    return jsonArray;
}

template<typename T, typename Factory, typename = enable_if_t<std::is_base_of_v<FactoryFromJSON<T>, Factory>>>
QList<T> FromJSONArray(const QJsonArray &jsonArray)
{
    auto list =  QList<T>{};
    for(const auto &e: jsonArray)
    {
        auto factory = Factory{};
        auto optional = factory.FromJSON(e.toObject());
        if(optional.has_value())
            list.append(optional.value());
    }

    return list;
}

struct Answer : public JSONable, public Updatable
{
    qint64 id; //qint8
    QString answerText;
    bool isTrue;

    explicit Answer(const QString &answerText,
                    const bool isTrue) :
        id(NextId()),
        answerText(answerText),
        isTrue(isTrue)
    {}

    QJsonObject ToJSON() const override
    {
        return QJsonObject
            {
                {"id", id},
                {"answerText", answerText},
                {"isTrue", isTrue}
            };
    }

    bool Update(const QJsonObject &json) override
    {
        if(!json.contains("answerText")
            || !json.contains("isTrue"))
            return false;

        answerText = json.value("answerText").toString();
        isTrue = json.value("isTrue").toBool();
        return true;
    }

    void UpdateFields(const QJsonObject &json) override
    {
        if(json.contains("answerText"))
            answerText = json.value("answerText").toString();
        if(json.contains("isTrue"))
            isTrue = json.value("isTrue").toBool();
    }

private:
    static qint64 NextId()
    {
        static auto lastId = qint64{1};
        return lastId++;
    }
};

struct AnswerFactory : public FactoryFromJSON<Answer>
{
    std::optional<Answer> FromJSON(const QJsonObject &json) const override
    {
        if(!json.contains("answerText")
            || !json.contains("isTrue"))
            return std::nullopt;
        return Answer
            (
                json.value("answerText").toString(),
                json.value("isTrue").toBool()
            );
    }
};

struct Category : public JSONable, public Updatable
{
    qint64 id; //qint8
    QString categoryText;
    QUrl iconUrl;

    explicit Category(const QString &categoryText,
                      const QUrl &iconUrl) :
        id(NextId()),
        categoryText(categoryText),
        iconUrl(iconUrl)
    {}

    QJsonObject ToJSON() const override
    {
        return QJsonObject
        {
            {"id", id},
            {"categoryText", categoryText},
            {"iconUrl", iconUrl.toString()}
        };
    }

    bool Update(const QJsonObject &json) override
    {
        if(!json.contains("categoryText")
            || !json.contains("iconUrl"))
            return false;

        categoryText = json.value("categoryText").toString();
        iconUrl.setPath(json.value("iconUrl").toString());
        return true;
    }

    void UpdateFields(const QJsonObject &json) override
    {
        if(json.contains("categoryText"))
            categoryText = json.value("categoryText").toString();
        if(json.contains("iconUrl"))
            iconUrl.setPath(json.value("iconUrl").toString());
    }

private:
    static qint64 NextId()
    {
        static auto lastId = qint64{1};
        return lastId++;
    }
};

struct CategoryFactory : public FactoryFromJSON<Category>
{
    std::optional<Category> FromJSON(const QJsonObject &json) const override
    {
        if(!json.contains("categoryText")
            || !json.contains("iconUrl"))
            return std::nullopt;

        auto iconUrl = QUrl{json.value("iconUrl").toString()};
        return Category
            (
                json.value("categoryText").toString(),
                iconUrl
            );
    }
};

struct Question : public JSONable, public Updatable
{
    qint64 id;
    QString questionText;

    Category category;
    QList<Answer> answers;

    explicit Question(const QString &questionText,
                      const Category &category,
                      const QList<Answer> &answers) :
        id(NextId()),
        questionText(questionText),
        category(category),
        answers(answers)
    {}

    QJsonObject ToJSON() const override
    {
        return QJsonObject
            {
                {"id", id},
                {"questionText", questionText},
                {"category", category.ToJSON()},
                {"answers", ToJSONArray(answers)}
            };
    }

    bool Update(const QJsonObject &json) override
    {
        if(!json.contains("questionText")
            || !json.contains("category")
            || !json.contains("answers"))
            return false;

        questionText = json.value("questionText").toString();
        const auto categoryFactory = CategoryFactory{};
        const auto categoryOptional = categoryFactory.FromJSON(json.value("category").toObject());
        if(!categoryOptional.has_value())
            return false;
        category = categoryOptional.value();
        answers = FromJSONArray<Answer, AnswerFactory>(json.value("answers").toArray());

        return true;
    }

    void UpdateFields(const QJsonObject &json) override
    {
        if(json.contains("questionText"))
            questionText = json.value("questionText").toString();
        if(json.contains("category"))
        {
            const auto categoryFactory = CategoryFactory{};
            const auto categoryOptional = categoryFactory.FromJSON(json.value("category").toObject());
            if(categoryOptional.has_value())
                category = categoryOptional.value();
        }
        if(json.contains("answers"))
            answers = FromJSONArray<Answer, AnswerFactory>(json.value("answers").toArray());
    }

private:
    static qint64 NextId()
    {
        static auto lastId = qint64{1};
        return lastId++;
    }
};

struct QuestionFactory : public FactoryFromJSON<Question>
{
    std::optional<Question> FromJSON(const QJsonObject &json) const override
    {
        if(!json.contains("questionText")
            || !json.contains("category")
            || !json.contains("answers"))
            return std::nullopt;

        const auto categoryFactory = CategoryFactory{};
        const auto categoryOptional = categoryFactory.FromJSON(json.value("category").toObject());
        if(!categoryOptional.has_value())
            return std::nullopt;

        const auto category = categoryOptional.value();
        return Question
            (
                json.value("questionText").toString(),
                category,
                FromJSONArray<Answer, AnswerFactory>(json.value("answers").toArray())
                );
    }
};

//
//
struct SessionEntry : public JSONable
{
    qint64 id;
    std::optional<QUuid> token;

    explicit SessionEntry() :
        id(NextId())
    {}

    void StartSession()
    {
        token = GenerateToken();
    }

    void EndSession()
    {
        token = std::nullopt;
    }

    QJsonObject ToJSON() const override
    {
        return token    ? QJsonObject
        {
            {"id", id},
            {"token", token->toString(QUuid::StringFormat::WithoutBraces)}
        }
                        : QJsonObject{};
    }

    bool operator==(const QString &other) const
    {
        return token.has_value() && (token.value() == QUuid::fromString(other)); //TODOREAD
    }

private:

    QUuid GenerateToken()
    {
        return QUuid::createUuid();
    }

    static qint64 NextId()
    {
        static auto lastId = qint64{1};
        return lastId++;
    }
};

struct SessionEntryFactory : public FactoryFromJSON<SessionEntry>
{
    std::optional<SessionEntry> FromJSON(const QJsonObject &json) const override
    {
        return SessionEntry(); //TODO AUTH?
    }
};

template<typename K = qint64, typename T = void>
class IdMap : public QMap<K, T>
{
public:
    IdMap() = default;
    explicit IdMap(const FactoryFromJSON<T> &factory, const QJsonArray &array) : QMap<K, T>()
    {
        for(const auto &json: array)
        {
            if(json.isObject())
            {
                const auto optional = factory.FromJSON(json.toObject());
                if(optional.has_value())
                    QMap<K, T>::insert(optional.value().id, optional.value());
            }
        }
    }
};

template<typename T>
class PaginatedData : public JSONable
{
public:

    static constexpr qsizetype DEFAULT_PAGE = 1;
    static constexpr qsizetype DEFAULT_PAGE_SIZE = 8;

    explicit PaginatedData(const T &container, qsizetype page = DEFAULT_PAGE, qsizetype size = DEFAULT_PAGE_SIZE)
    {
        const auto containerSize = container.size();
        const auto pageIndex = page - 1;
        const auto pageSize = qMin(size, containerSize);
        const auto totalPages = (containerSize % pageSize) == 0
                                    ? (containerSize / pageSize)
                                    : (containerSize / pageSize) + 1;

        m_valid = containerSize > (pageIndex * pageSize);

        if(!m_valid)
        {
            m_json = QJsonObject{};
            return;
        }

        auto data = QJsonArray{};
        auto iterator = container.begin();
        std::advance(iterator, std::min(pageIndex * pageSize, containerSize)); //TODO
        for(qsizetype i = 0; i < pageSize && iterator != container.end(); ++i, ++iterator)
            data.push_back(iterator->ToJSON());

        m_json = QJsonObject
        {
            {"page", pageIndex + 1},
            {"per_page", pageSize},
            {"total", containerSize},
            {"total_pages", totalPages},
            {"data", data}
        };
    }

    QJsonObject ToJSON() const
    {
        return m_json;
    }

    constexpr bool IsValid() const
    {
        return m_valid;
    }

private:

    QJsonObject m_json;
    bool m_valid;
};

#endif // STRUCTS_HPP
