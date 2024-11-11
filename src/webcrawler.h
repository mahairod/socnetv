/***************************************************************************
 SocNetV: Social Network Visualizer
 version: 3.0.4
 Written in Qt
 
                         webcrawler.h  -  description
                             -------------------
    copyright         : (C) 2005-2021 by Dimitris B. Kalamaras
    project site      : https://socnetv.org

 ***************************************************************************/

/*******************************************************************************
*     This program is free software: you can redistribute it and/or modify     *
*     it under the terms of the GNU General Public License as published by     *
*     the Free Software Foundation, either version 3 of the License, or        *
*     (at your option) any later version.                                      *
*                                                                              *
*     This program is distributed in the hope that it will be useful,          *
*     but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*     GNU General Public License for more details.                             *
*                                                                              *
*     You should have received a copy of the GNU General Public License        *
*     along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
********************************************************************************/

#ifndef WEBCRAWLER_H
#define WEBCRAWLER_H

#include <QNetworkReply>
#include <QThreadPool>
#include <QReadWriteLock>
#include "QCQueue.h"

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

using namespace std;

/**
 * @brief The WebCrawler class
 * Parses HTML code it receives, locates urls inside it and puts them into a url queue (passed from the parent)
 * while emitting signals to the parent to create new nodes and edges between them.
 */
class WebCrawler : public QObject  {
    Q_OBJECT
    //    QThread wc_spiderThread;
public:

    WebCrawler (
            QCQueue<QUrl> *urlQueue,
            const QUrl &startUrl,
            const QStringList &urlPatternsIncluded,
            const QStringList &urlPatternsExcluded,
            const QStringList &linkClasses,
            const int &maxNodes,
            const int &maxLinksPerPage,
            const bool &intLinks = true,
            const bool &childLinks = true,
            const bool &parentLinks = false,
            const bool &selfLinks = false,
            const bool &extLinksIncluded = false,
            const bool &extLinksCrawl = false,
            const bool &socialLinks = false,
            const int &delayBetween  = 0
            );

    ~WebCrawler();

public slots:
    void parse(QNetworkReply *reply);
    void newLink(int s, QUrl target, bool enqueue_to_frontier);

signals:
    void signalCreateNode(const int &no,
                          const QString &url,
                          const bool &signalMW=false);
    void signalCreateEdge (const int &source, const int &target);
    void signalStartSpider();
    void finished (QString);

private:
    QCQueue<QUrl> *m_urlQueue;
    QThreadPool *threadPool;

    // a map of all known urls to their node number
    QMap <QUrl, int> knownUrls;
	QReadWriteLock m_urlsLock;

    volatile int m_discoveredNodes;

    const QUrl m_initialUrl;
    const int m_maxUrls;
    const int m_maxLinksPerPage;

    const bool m_intLinks;
    const bool m_childLinks;
    const bool m_parentLinks;
    const bool m_selfLinks;
    const bool m_extLinksIncluded;
    const bool m_extLinksCrawl;
    const bool m_socialLinks;

    const int m_delayBetween;

    const QStringList m_urlPatternsIncluded;
    const QStringList m_urlPatternsExcluded;
    const QStringList m_linkClasses;
    const QStringList m_socialLinksExcluded;

    void parseImpl(QNetworkReply *reply);
    bool interrupted(const char* func);
};



#endif
