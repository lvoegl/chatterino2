#include "providers/ChattersApi.hpp"

#include "common/Channel.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchChannel.hpp"

#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>

namespace chatterino {

namespace {

    QString chatterFromObject(const QJsonValue chatter)
    {
        return chatter.toObject().value("name").toString().toLower();
    }

    QStringList userListFromJsonArray(const QJsonArray chatterArray)
    {
        QStringList chatterList;

        for (const auto chatter : chatterArray)
        {
            chatterList.append(chatterFromObject(chatter));
        }

        return chatterList;
    }

    NetworkRequest makeRequest(QString url, QUrlQuery urlQuery)
    {
        assert(!url.startsWith("/"));

        const QString baseUrl = Env::get().chattersApiBaseUrl;
        QUrl fullUrl(baseUrl + url);
        fullUrl.setQuery(urlQuery);

        return NetworkRequest(fullUrl).timeout(10 * 1000).header("Accept",
                                                                 "application/json");
    }

    void forwardErrorMessage(NetworkResult result, ErrorCallback onError)
    {
        auto root = result.parseJson();
        QString error = root.value("message").toString();
        onError(error);
    }

}  // namespace

void ChattersApi::loadChatters(TwitchChannel *channel,
                               ResultCallback<QString, QStringList,
                                   QStringList, QStringList> onLoaded,
                               ErrorCallback onError)
{
    qCDebug(chatterinoChattersApi)
        << "Loading chatters list for" << channel->getName();

    QUrlQuery urlQuery;
    urlQuery.addQueryItem("channelId", channel->roomId());
    makeRequest("chatters/all", urlQuery)
        .onSuccess([channel, onLoaded](NetworkResult result) -> Outcome {
            qCDebug(chatterinoChattersApi)
                << "Successfully loaded chatters list for"
                << channel->getName();

            auto root = result.parseJson();
            QString broadcaster = root.value("broadcaster").toString();
            QJsonArray moderators = root.value("moderators").toArray();
            QJsonArray vips = root.value("vips").toArray();
            QJsonArray viewers = root.value("viewers").toArray();

            QString broadcasterChatter;
            if (!broadcaster.isNull())
            {
                broadcasterChatter = chatterFromObject(broadcaster);
            }

            QStringList chatterList = userListFromJsonArray(viewers);
            QStringList modList = userListFromJsonArray(moderators);
            QStringList vipList = userListFromJsonArray(vips);
            onLoaded(broadcasterChatter, modList, vipList, chatterList);

            return Success;
        })
        .onError([channel, onError](NetworkResult result) {
            qCDebug(chatterinoChattersApi)
                << "Failed to load chatters list for" << channel->getName();

            auto root = result.parseJson();
            QString error = root.value("message").toString();
            onError(error);
        })
        .execute();
}

}  // namespace chatterino
